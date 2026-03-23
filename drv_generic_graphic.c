/* $Id: drv_generic_graphic.c 1170 2012-01-16 02:50:03Z michael $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/drv_generic_graphic.c $
 *
 * generic driver helper for graphic displays
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * This file is part of LCD4Linux.
 *
 * LCD4Linux is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * LCD4Linux is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* 
 *
 * exported functions:
 *
 * int drv_generic_graphic_init (char *section, char *driver);
 *   initializes the generic graphic driver
 *
 * int drv_generic_graphic_draw (WIDGET *W);
 *   renders Text widget into framebuffer
 *   calls drv_generic_graphic_real_blit()
 *
 * int drv_generic_graphic_icon_draw (WIDGET *W);
 *   renders Icon widget into framebuffer
 *   calls drv_generic_graphic_real_blit()
 *
 * int drv_generic_graphic_bar_draw (WIDGET *W);
 *   renders Bar widget into framebuffer
 *   calls drv_generic_graphic_real_blit()
 *
 * int drv_generic_graphic_quit (void);
 *   closes generic graphic driver
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include "debug.h"
#include "cfg.h"
#include "plugin.h"
#include "layout.h"
#include "widget.h"
#include "property.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "widget_image.h"
#include "widget_graph.h"
#include "widget_arc.h"
#include "rgb.h"
#include "drv.h"
#include "drv_generic.h"
#include "drv_generic_graphic.h"
#include "font_6x8.h"
#include "font_6x8_bold.h"
#include "font_ttf.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/* pixel colors */
RGBA FG_COL = {.R = 0x00,.G = 0x00,.B = 0x00,.A = 0xff };
RGBA BG_COL = {.R = 0xff,.G = 0xff,.B = 0xff,.A = 0xff };
RGBA BL_COL = {.R = 0xff,.G = 0xff,.B = 0xff,.A = 0x00 };
RGBA NO_COL = {.R = 0x00,.G = 0x00,.B = 0x00,.A = 0x00 };

static char *Section = NULL;
static char *Driver = NULL;

/* framebuffer */
static RGBA *drv_generic_graphic_FB[LAYERS] = { NULL, };

/* exported framebuffer for TrueType fonts */
RGBA *drv_generic_graphic_FB_ptr = NULL;

/* inverted colors */
static int INVERTED = 0;

/* must be implemented by the real driver */
void (*drv_generic_graphic_real_blit) () = NULL;


/****************************************/
/*** generic Framebuffer stuff        ***/
/****************************************/

static void drv_generic_graphic_resizeFB(int rows, int cols)
{
    RGBA *newFB;
    int i, l, row, col;

    /* Layout FB is large enough */
    if (rows <= LROWS && cols <= LCOLS)
	return;

    /* get maximum values */
    if (rows < LROWS)
	rows = LROWS;
    if (cols < LCOLS)
	cols = LCOLS;

    for (l = 0; l < LAYERS; l++) {

	/* allocate and initialize new Layout FB */
	newFB = malloc(cols * rows * sizeof(*newFB));
	for (i = 0; i < rows * cols; i++)
	    newFB[i] = NO_COL;

	/* transfer contents */
	if (drv_generic_graphic_FB[l] != NULL) {
	    for (row = 0; row < LROWS; row++) {
		for (col = 0; col < LCOLS; col++) {
		    newFB[row * cols + col] = drv_generic_graphic_FB[l][row * LCOLS + col];
		}
	    }
	    free(drv_generic_graphic_FB[l]);
	}
	drv_generic_graphic_FB[l] = newFB;
    }
    
    drv_generic_graphic_FB_ptr = drv_generic_graphic_FB[0];

    LCOLS = cols;
    LROWS = rows;

}

static void drv_generic_graphic_window(int pos, int size, int max, int *wpos, int *wsize)
{
    int p1 = pos;
    int p2 = pos + size;

    *wpos = 0;
    *wsize = 0;

    if (p1 > max || p2 < 0 || size < 1)
	return;

    if (p1 < 0)
	p1 = 0;

    if (p2 > max)
	p2 = max;

    *wpos = p1;
    *wsize = p2 - p1;
}

static void drv_generic_graphic_blit(const int row, const int col, const int height, const int width)
{
    if (drv_generic_graphic_real_blit) {
	int r, c, h, w;
	drv_generic_graphic_window(row, height, DROWS, &r, &h);
	drv_generic_graphic_window(col, width, DCOLS, &c, &w);
	if (h > 0 && w > 0) {
	    drv_generic_graphic_real_blit(r, c, h, w);
	}
    }
}

static RGBA drv_generic_graphic_blend(const int row, const int col)
{
    int l, o;
    RGBA p;
    RGBA ret;

    ret.R = BL_COL.R;
    ret.G = BL_COL.G;
    ret.B = BL_COL.B;
    ret.A = 0x00;

    /* find first opaque layer */
    /* layers below are fully covered */
    o = LAYERS - 1;
    for (l = 0; l < LAYERS; l++) {
	p = drv_generic_graphic_FB[l][row * LCOLS + col];
	if (p.A == 255) {
	    o = l;
	    break;
	}
    }

    for (l = o; l >= 0; l--) {
	p = drv_generic_graphic_FB[l][row * LCOLS + col];
	switch (p.A) {
	case 0:
	    break;
	case 255:
	    ret.R = p.R;
	    ret.G = p.G;
	    ret.B = p.B;
	    ret.A = 0xff;
	    break;
	default:
	    ret.R = (p.R * p.A + ret.R * (255 - p.A)) / 255;
	    ret.G = (p.G * p.A + ret.G * (255 - p.A)) / 255;
	    ret.B = (p.B * p.A + ret.B * (255 - p.A)) / 255;
	    ret.A = 0xff;
	}
    }
    if (INVERTED) {
	ret.R = 255 - ret.R;
	ret.G = 255 - ret.G;
	ret.B = 255 - ret.B;
    }

    return ret;
}


/****************************************/
/*** generic text handling            ***/
/****************************************/

static void drv_generic_graphic_render(const int layer, const int row, const int col, const RGBA fg, const RGBA bg,
				       const char *style, const char *txt)
{
    int c, r, x, y, len;
    int bold;

    /* sanity checks */
    if (layer < 0 || layer >= LAYERS) {
	error("%s: layer %d out of bounds (0..%d)", Driver, layer, LAYERS - 1);
	return;
    }

    /* Use TrueType font if available */
    if (font_ttf_is_available()) {
        int tw = font_ttf_get_width();
        int th = font_ttf_get_height();
        len = strlen(txt);
        
        /* maybe grow layout framebuffer */
        drv_generic_graphic_resizeFB(row + th, col + tw * len);
        
        /* render text using TrueType */
        font_ttf_render(layer, col, row, fg, bg, txt, tw * len);
        
        /* flush area */
        drv_generic_graphic_blit(row, col, th, tw * len);
        return;
    }

    len = strlen(txt);

    /* maybe grow layout framebuffer */
    drv_generic_graphic_resizeFB(row + YRES, col + XRES * len);

    r = row;
    c = col;

    /* render text into layout FB */
    bold = 0;
    while (*txt != '\0') {
	unsigned char *chr;

	/* magic char to toggle bold */
	if (*txt == '\a') {
	    bold ^= 1;
	    txt++;
	    continue;
	}
	if (bold || strstr(style, "bold") != NULL) {
	    chr = Font_6x8_bold[(int) *(unsigned char *) txt];
	} else {
	    chr = Font_6x8[(int) *(unsigned char *) txt];
	}

	for (y = 0; y < YRES; y++) {
	    for (x = 0; x < XRES; x++) {
		int mask = 1 << 6;
		mask >>= ((x * 6) / (XRES)) + 1;
		if (chr[(y * 8) / (YRES)] & mask)
		    drv_generic_graphic_FB[layer][(r + y) * LCOLS + c + x] = fg;
		else
		    drv_generic_graphic_FB[layer][(r + y) * LCOLS + c + x] = bg;
	    }
	}
	c += XRES;
	txt++;
    }

    /* flush area */
    drv_generic_graphic_blit(row, col, YRES, XRES * len);

}


/* say hello to the user */
int drv_generic_graphic_greet(const char *msg1, const char *msg2)
{
    char *line1[] = { "* LCD4Linux " VERSION " *",
	"LCD4Linux " VERSION,
	"* LCD4Linux *",
	"LCD4Linux",
	"L4Linux",
	NULL
    };

    char *line2[] = { "http://lcd4linux.bulix.org",
	"lcd4linux.bulix.org",
	NULL
    };

    int i;
    int flag = 0;

    unsigned int cols = DCOLS / XRES;
    unsigned int rows = DROWS / YRES;

    for (i = 0; line1[i]; i++) {
	if (strlen(line1[i]) <= cols) {
	    drv_generic_graphic_render(0, YRES * 0, XRES * ((cols - strlen(line1[i])) / 2), FG_COL, BG_COL, "norm",
				       line1[i]);
	    flag = 1;
	    break;
	}
    }

    if (rows >= 2) {
	for (i = 0; line2[i]; i++) {
	    if (strlen(line2[i]) <= cols) {
		drv_generic_graphic_render(0, YRES * 1, XRES * ((cols - strlen(line2[i])) / 2), FG_COL, BG_COL, "norm",
					   line2[i]);
		flag = 1;
		break;
	    }
	}
    }

    if (msg1 && rows >= 3) {
	unsigned int len = strlen(msg1);
	if (len <= cols) {
	    drv_generic_graphic_render(0, YRES * 2, XRES * ((cols - len) / 2), FG_COL, BG_COL, "norm", msg1);
	    flag = 1;
	}
    }

    if (msg2 && rows >= 4) {
	unsigned int len = strlen(msg2);
	if (len <= cols) {
	    drv_generic_graphic_render(0, YRES * 3, XRES * ((cols - len) / 2), FG_COL, BG_COL, "norm", msg2);
	    flag = 1;
	}
    }

    return flag;
}


int drv_generic_graphic_draw(WIDGET * W)
{
    WIDGET_TEXT *Text = W->data;
    RGBA fg, bg;

    fg = W->fg_valid ? W->fg_color : FG_COL;
    bg = W->bg_valid ? W->bg_color : BG_COL;

    drv_generic_graphic_render(W->layer, YRES * W->row, XRES * W->col, fg, bg, P2S(&Text->style), Text->buffer);

    return 0;
}


/****************************************/
/*** generic icon handling            ***/
/****************************************/

int drv_generic_graphic_icon_draw(WIDGET * W)
{
    WIDGET_ICON *Icon = W->data;
    RGBA fg, bg;
    unsigned char *bitmap = Icon->bitmap + YRES * Icon->curmap;
    int layer, row, col;
    int x, y;
    int visible;

    layer = W->layer;
    row = YRES * W->row;
    col = XRES * W->col;

    fg = W->fg_valid ? W->fg_color : FG_COL;
    bg = W->bg_valid ? W->bg_color : BG_COL;

    /* sanity check */
    if (layer < 0 || layer >= LAYERS) {
	error("%s: layer %d out of bounds (0..%d)", Driver, layer, LAYERS - 1);
	return -1;
    }

    /* maybe grow layout framebuffer */
    drv_generic_graphic_resizeFB(row + YRES, col + XRES);

    /* Icon visible? */
    visible = P2N(&Icon->visible) > 0;

    /* render icon */
    for (y = 0; y < YRES; y++) {
	int mask = 1 << XRES;
	for (x = 0; x < XRES; x++) {
	    int i = (row + y) * LCOLS + col + x;
	    mask >>= 1;
	    if (visible) {
		if (bitmap[y] & mask)
		    drv_generic_graphic_FB[layer][i] = fg;
		else
		    drv_generic_graphic_FB[layer][i] = bg;
	    } else {
		drv_generic_graphic_FB[layer][i] = BG_COL;
	    }
	}
    }

    /* flush area */
    drv_generic_graphic_blit(row, col, YRES, XRES);

    return 0;

}


/****************************************/
/*** generic bar handling             ***/
/****************************************/

int drv_generic_graphic_bar_draw(WIDGET * W)
{
    WIDGET_BAR *Bar = W->data;
    RGBA fg, bg, bar[2];
    int layer, row, col, len, res, rev, max, val1, val2;
    int x, y;
    DIRECTION dir;
    STYLE style;

    layer = W->layer;
    row = YRES * W->row;
    col = XRES * W->col;
    dir = Bar->direction;
    style = Bar->style;
    len = Bar->length;

    fg = W->fg_valid ? W->fg_color : FG_COL;
    bg = W->bg_valid ? W->bg_color : BG_COL;

    bar[0] = Bar->color_valid[0] ? Bar->color[0] : fg;
    bar[1] = Bar->color_valid[1] ? Bar->color[1] : fg;

    /* sanity check */
    if (layer < 0 || layer >= LAYERS) {
	error("%s: layer %d out of bounds (0..%d)", Driver, layer, LAYERS - 1);
	return -1;
    }

    /* maybe grow layout framebuffer */
    if (dir & (DIR_EAST | DIR_WEST)) {
	drv_generic_graphic_resizeFB(row + YRES, col + XRES * len);
    } else {
	drv_generic_graphic_resizeFB(row + YRES * len, col + XRES);
    }

    res = dir & (DIR_EAST | DIR_WEST) ? XRES : YRES;
    max = len * res;
    val1 = Bar->val1 * (double) (max);
    val2 = Bar->val2 * (double) (max);

    if (val1 < 1)
	val1 = 1;
    else if (val1 > max)
	val1 = max;

    if (val2 < 1)
	val2 = 1;
    else if (val2 > max)
	val2 = max;

    rev = 0;

    switch (dir) {
    case DIR_WEST:
	val1 = max - val1;
	val2 = max - val2;
	rev = 1;

    case DIR_EAST:
	for (y = 0; y < YRES; y++) {
	    int val = y < YRES / 2 ? val1 : val2;
	    RGBA bc = y < YRES / 2 ? bar[0] : bar[1];

	    for (x = 0; x < max; x++) {
		if (x < val)
		    drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + x] = rev ? bg : bc;
		else
		    drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + x] = rev ? bc : bg;

		if (style) {
		    drv_generic_graphic_FB[layer][(row + 0) * LCOLS + col + x] = fg;
		    drv_generic_graphic_FB[layer][(row + YRES - 1) * LCOLS + col + x] = fg;
		}
	    }
	    if (style) {
		drv_generic_graphic_FB[layer][(row + y) * LCOLS + col] = fg;
		drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + max - 1] = fg;
	    }
	}
	break;

    case DIR_NORTH:
	val1 = max - val1;
	val2 = max - val2;
	rev = 1;

    case DIR_SOUTH:
	for (x = 0; x < XRES; x++) {
	    int val = x < XRES / 2 ? val1 : val2;
	    RGBA bc = x < XRES / 2 ? bar[0] : bar[1];
	    for (y = 0; y < max; y++) {
		if (y < val)
		    drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + x] = rev ? bg : bc;
		else
		    drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + x] = rev ? bc : bg;
		if (style) {
		    drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + 0] = fg;
		    drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + XRES - 1] = fg;
		}
	    }
	    if (style) {
		drv_generic_graphic_FB[layer][(row + 0) * LCOLS + col + x] = fg;
		drv_generic_graphic_FB[layer][(row + max - 1) * LCOLS + col + x] = fg;
	    }
	}
	break;
    }

    /* flush area */
    if (dir & (DIR_EAST | DIR_WEST)) {
	drv_generic_graphic_blit(row, col, YRES, XRES * len);
    } else {
	drv_generic_graphic_blit(row, col, YRES * len, XRES);
    }

    return 0;
}


/****************************************/
/*** generic image handling           ***/
/****************************************/

int drv_generic_graphic_image_draw(WIDGET * W)
{
    WIDGET_IMAGE *Image = W->data;
    int layer, row, col, width, height;
    int x, y;
    int visible;

    layer = W->layer;
    row = W->row;
    col = W->col;
    width = Image->width;
    height = Image->height;

    /* sanity check */
    if (layer < 0 || layer >= LAYERS) {
	error("%s: layer %d out of bounds (0..%d)", Driver, layer, LAYERS - 1);
	return -1;
    }

    /* if no size or no image at all, do nothing */
    if (width <= 0 || height <= 0 || Image->bitmap == NULL) {
	return 0;
    }

    /* maybe grow layout framebuffer */
    drv_generic_graphic_resizeFB(row + height, col + width);

    /* render image */
    visible = P2N(&Image->visible);
    for (y = 0; y < height; y++) {
	for (x = 0; x < width; x++) {
	    int i = (row + y) * LCOLS + col + x;
	    if (visible) {
		drv_generic_graphic_FB[layer][i] = Image->bitmap[y * width + x];
	    } else {
		drv_generic_graphic_FB[layer][i] = BG_COL;
	    }
	}
    }

    /* flush area */
    drv_generic_graphic_blit(row, col, height, width);

    return 0;

}


/****************************************/
/*** generic graph handling            ***/
/****************************************/

int drv_generic_graphic_graph_draw(WIDGET * W)
{
    WIDGET_GRAPH *Graph = W->data;
    RGBA line_color, fill_color, bg_color, grid_color;
    int layer, row, col, width, height;
    int x, y, i;
    double value, normalized;
    int points_to_draw, step;
    int px, py, last_py, last_px;

    layer = W->layer;
    row = W->row;
    col = W->col;
    width = Graph->width;
    height = Graph->height;

    info("GRAPH draw called: row=%d col=%d w=%d h=%d data_count=%d", row, col, width, height, Graph->data_count);

    if (W->class->type == WIDGET_TYPE_RC) {
	row = row * YRES;
	col = col * XRES;
	width = width * XRES;
	height = height * YRES;
    }

    info("GRAPH after RC: row=%d col=%d w=%d h=%d", row, col, width, height);

    /* sanity check */
    if (layer < 0 || layer >= LAYERS) {
	error("%s: layer %d out of bounds (0..%d)", Driver, layer, LAYERS - 1);
	return -1;
    }

    if (width <= 0 || height <= 0) {
	info("GRAPH: width or height is 0");
	return 0;
    }

    /* grow framebuffer if needed */
    drv_generic_graphic_resizeFB(row + height, col + width);

    /* get colors */
    line_color = Graph->line_color;
    fill_color = Graph->fill_color;
    bg_color = Graph->bg_color;
    grid_color = Graph->grid_color;

    info("GRAPH colors: R=%d G=%d B=%d", line_color.R, line_color.G, line_color.B);

    /* clear background - fill with bg_color */
    for (y = 0; y < height; y++) {
	for (x = 0; x < width; x++) {
	    drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + x] = bg_color;
	}
    }

    info("GRAPH: background drawn");

    /* draw grid - configurable number of horizontal lines */
    if (Graph->grid_lines > 0) {
        for (i = 1; i <= Graph->grid_lines; i++) {
            int gy = (height * i) / (Graph->grid_lines + 1);
            for (x = 0; x < width; x++) {
                drv_generic_graphic_FB[layer][(row + gy) * LCOLS + col + x] = grid_color;
            }
        }
    }

    /* draw graph line - smooth connected line */
    if (Graph->data_count > 1) {
	points_to_draw = (Graph->data_count < Graph->num_points) ? 
			Graph->data_count : Graph->num_points;
	
	if (points_to_draw > 1) {
	    step = width / points_to_draw;
	    if (step < 1) step = 1;

	    last_py = -1;
	    last_px = -1;

	    /* draw smooth line connecting all points */
	    for (i = 0; i < points_to_draw; i++) {
		int data_idx = (Graph->data_head - points_to_draw + i + Graph->num_points) % Graph->num_points;
		double val = Graph->data[data_idx];
		
		/* normalize value to 0-1 range */
		if (Graph->max != Graph->min)
		    normalized = (val - Graph->min) / (Graph->max - Graph->min);
		else
		    normalized = 0;
		
		if (normalized < 0) normalized = 0;
		if (normalized > 1) normalized = 1;

		px = col + (i * step);
		py = row + height - 1 - (int)(normalized * (height - 1));

		/* clamp to valid range */
		if (px < col) px = col;
		if (px >= col + width) px = col + width - 1;
		if (py < row) py = row;
		if (py >= row + height) py = row + height - 1;

		/* draw line from last point to this point */
		if (last_py >= 0 && last_px >= 0) {
		    int dx = abs(px - last_px);
		    int dy = abs(py - last_py);
		    int sx = last_px < px ? 1 : -1;
		    int sy = last_py < py ? 1 : -1;
		    int err = dx - dy;
		    int cx = last_px;
		    int cy = last_py;
		    
		    while (1) {
			if (cx >= col && cx < col + width && cy >= row && cy < row + height) {
			    drv_generic_graphic_FB[layer][cy * LCOLS + cx] = line_color;
			}
			if (cx == px && cy == py) break;
			int e2 = 2 * err;
			if (e2 > -dy) { err -= dy; cx += sx; }
			if (e2 < dx) { err += dx; cy += sy; }
		    }
		}

		last_px = px;
		last_py = py;
	    }
	}
    }

    /* show current value as text */
    if (Graph->show_value && Graph->data_count > 0) {
        int last_idx = (Graph->data_head - 1 + Graph->num_points) % Graph->num_points;
        double display_val = Graph->data[last_idx];
        double display_max = Graph->max;
        
        /* calculate percentage if max > 0 */
        if (display_max > 0) {
            int pct = (int)(100 * display_val / display_max);
            char text[8];
            snprintf(text, sizeof(text), "%d%%", pct);
            
            /* draw simple text representation */
            int text_x = col + width - 25;
            int text_y = row + 2;
            int char_w = 4;
            int char_h = 6;
            
            /* draw text background */
            for (y = text_y; y < text_y + char_h && y < row + height; y++) {
                for (x = text_x; x < text_x + char_w * 3 && x < col + width; x++) {
                    if (y >= row && y < row + height && x >= col && x < col + width) {
                        drv_generic_graphic_FB[layer][y * LCOLS + x] = bg_color;
                    }
                }
            }
            
            /* draw simple digits */
            int digit_x = text_x + 2;
            const char *p = text;
            while (*p && digit_x < col + width - 2) {
                int digit = *p - '0';
                if (*p == '%') digit = 10;
                if (*p >= '0' && *p <= '9') {
                    for (int dy = 0; dy < 5 && text_y + dy < row + height; dy++) {
                        for (int dx = 0; dx < 3 && digit_x + dx < col + width; dx++) {
                            if (digit_x + dx >= col && digit_x + dx < col + width && 
                                text_y + dy >= row && text_y + dy < row + height) {
                                drv_generic_graphic_FB[layer][(text_y + dy) * LCOLS + (digit_x + dx)] = line_color;
                            }
                        }
                    }
                    digit_x += 4;
                }
                p++;
            }
        }
    }

    /* flush area to display */
    drv_generic_graphic_blit(row, col, height, width);
    info("GRAPH draw complete: blit called for row=%d col=%d h=%d w=%d", row, col, height, width);
    return 0;
}


/****************************************/
/*** generic arc handling               ***/
/****************************************/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void draw_arc_pixel(int layer, int cx, int cy, int x, int y, RGBA * color)
{
    int coords[8][2] = {
	{x, y}, {-x, y}, {x, -y}, {-x, -y},
	{y, x}, {-y, x}, {y, -x}, {-y, -x}
    };
    int i;
    for (i = 0; i < 8; i++) {
	int px = cx + coords[i][0];
	int py = cy + coords[i][1];
	if (px >= 0 && px < LCOLS && py >= 0 && py < LROWS) {
	    int idx = py * LCOLS + px;
	    drv_generic_graphic_FB[layer][idx] = *color;
	}
    }
}

static void draw_arc_segment(int layer, int cx, int cy, int radius, double start_deg, double end_deg, RGBA * color, int thickness)
{
    double angle;
    double range = end_deg - start_deg;
    if (range < 0) range += 360;
    
    for (angle = start_deg; angle < start_deg + range; angle += 1.0) {
	double rad = angle * M_PI / 180.0;
	int r;
	for (r = radius - thickness + 1; r <= radius; r++) {
	    int x = (int)(r * cos(rad));
	    int y = -(int)(r * sin(rad));  /* invert y for screen */
	    draw_arc_pixel(layer, cx, cy, x, y, color);
	}
    }
}

static void draw_needle_line(int layer, int cx, int cy, int length, double angle, RGBA * color, int thickness)
{
    /* screen Y is down, so use -sin for mathematical angles */
    double rad = angle * M_PI / 180.0;
    int x, y;
    int i;

    for (i = 0; i <= length; i++) {
	x = cx + (int)(i * cos(rad));
	y = cy - (int)(i * sin(rad));  /* -sin for screen coordinates */
	
	if (thickness > 1) {
	    int j;
	    for (j = 0; j < thickness; j++) {
		int px = x + j - thickness/2;
		int py = y;
		if (px >= 0 && px < LCOLS && py >= 0 && py < LROWS) {
		    int idx = py * LCOLS + px;
		    drv_generic_graphic_FB[layer][idx] = *color;
		}
	    }
	} else {
	    if (x >= 0 && x < LCOLS && y >= 0 && y < LROWS) {
		int idx = y * LCOLS + x;
		drv_generic_graphic_FB[layer][idx] = *color;
	    }
	}
    }
}

static void draw_center_circle(int layer, int cx, int cy, int radius, RGBA * color)
{
    int x, y;
    int r2 = radius * radius;
    
    for (y = -radius; y <= radius; y++) {
	for (x = -radius; x <= radius; x++) {
	    if (x*x + y*y <= r2) {
		int px = cx + x;
		int py = cy + y;
		if (px >= 0 && px < LCOLS && py >= 0 && py < LROWS) {
		    int idx = py * LCOLS + px;
		    drv_generic_graphic_FB[layer][idx] = *color;
		}
	    }
	}
    }
}

int drv_generic_graphic_arc_draw(WIDGET * W)
{
    WIDGET_ARC *Arc = W->data;
    RGBA arc_color, needle_color, center_color, bg_color;
    int layer, row, col, width, height;
    int cx, cy, radius;
    double value_angle, range;
    int i;
    double tick_angle;
    RGBA tick_color;

    layer = W->layer;
    row = W->row;
    col = W->col;
    width = Arc->width;
    height = Arc->height;

    if (W->class->type == WIDGET_TYPE_RC) {
	row = row * YRES;
	col = col * XRES;
	width = width * XRES;
	height = height * YRES;
    }

    /* sanity check */
    if (layer < 0 || layer >= LAYERS) {
	error("%s: layer %d out of bounds (0..%d)", Driver, layer, LAYERS - 1);
	return -1;
    }

    /* if no size, do nothing */
    if (width <= 0 || height <= 0) {
	return 0;
    }

    /* maybe grow layout framebuffer */
    drv_generic_graphic_resizeFB(row + height, col + width);

    /* get colors */
    arc_color = Arc->arc_color;
    needle_color = Arc->needle_color;
    center_color = Arc->center_color;
    bg_color = Arc->bg_color;

    /* clear background */
    for (i = 0; i < height; i++) {
	int j;
	for (j = 0; j < width; j++) {
	    int idx = (row + i) * LCOLS + col + j;
	    drv_generic_graphic_FB[layer][idx] = bg_color;
	}
    }

    /* calculate center and radius - center arc in widget */
    cx = col + width / 2;
    cy = row + height - 15;
    
    /* calculate radius to fit in widget */
    int max_radius_h = width / 2 - 5;
    int max_radius_v = height - 15;
    radius = (max_radius_h < max_radius_v) ? max_radius_h : max_radius_v;
    if (radius < 10) radius = 10;

    /* calculate value angle - counter-clockwise from start */
    range = Arc->start_angle - Arc->end_angle;
    if (range < 0) range += 360;
    
    if (Arc->max != Arc->min) {
	double normalized = (Arc->value - Arc->min) / (Arc->max - Arc->min);
	if (normalized < 0) normalized = 0;
	if (normalized > 1) normalized = 1;
	value_angle = Arc->start_angle - normalized * range;
    } else {
	value_angle = Arc->start_angle;
    }

    /* draw arc background */
    draw_arc_segment(layer, cx, cy, radius, Arc->start_angle, Arc->end_angle, &arc_color, Arc->thickness);

    /* draw tick marks */
    tick_color = Arc->text_color;
    
    /* major ticks */
    for (i = 0; i <= Arc->num_major; i++) {
	tick_angle = Arc->start_angle - (range * i) / Arc->num_major;
	if (tick_angle < 0) tick_angle += 360;
	double rad = tick_angle * M_PI / 180.0;
	int inner = radius - Arc->thickness - 3;
	int outer = radius + 2;
	
	int x1 = cx + (int)(inner * cos(rad));
	int y1 = cy - (int)(inner * sin(rad));  /* -sin for screen */
	int x2 = cx + (int)(outer * cos(rad));
	int y2 = cy - (int)(outer * sin(rad));  /* -sin for screen */
	
	/* draw tick line using simple line */
	int dx = x2 - x1;
	int dy = y2 - y1;
	int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
	if (steps < 1) steps = 1;
	int j;
	for (j = 0; j <= steps; j++) {
	    int px = x1 + (dx * j) / steps;
	    int py = y1 + (dy * j) / steps;
	    if (px >= col && px < col + width && py >= row && py < row + height) {
		int idx = py * LCOLS + px;
		drv_generic_graphic_FB[layer][idx] = tick_color;
	    }
	}
    }

    /* minor ticks - use dimmer color */
    if (Arc->num_minor > 0) {
        int intervals = Arc->num_major;
        int ticks_per_interval = Arc->num_minor;
        int total_minor = intervals * ticks_per_interval;
        
        /* minor tick color - use needle color for visibility */
        RGBA minor_tick_color = needle_color;
        
        for (i = 1; i < total_minor; i++) {  /* skip i=0 (major tick position) */
            /* skip positions that coincide with major ticks (multiples of ticks_per_interval) */
            if (i % ticks_per_interval == 0) continue;
            
            tick_angle = Arc->start_angle - (range * i) / ticks_per_interval;
            
            if (tick_angle < 0) tick_angle += 360;
            double rad = tick_angle * M_PI / 180.0;
            int inner = radius - Arc->thickness - 1;
            int outer = radius + 3;  /* slightly longer for visibility */
            
            int x1 = cx + (int)(inner * cos(rad));
            int y1 = cy - (int)(inner * sin(rad));
            int x2 = cx + (int)(outer * cos(rad));
            int y2 = cy - (int)(outer * sin(rad));
            
            /* draw small tick */
            int dx = x2 - x1;
            int dy = y2 - y1;
            int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
            if (steps < 1) steps = 1;
            int k;
            for (k = 0; k <= steps; k++) {
                int px = x1 + (dx * k) / steps;
                int py = y1 + (dy * k) / steps;
                if (px >= col && px < col + width && py >= row && py < row + height) {
                    int idx = py * LCOLS + px;
                    drv_generic_graphic_FB[layer][idx] = minor_tick_color;
                }
            }
        }
    }

    /* draw value arc (filled portion) */
    {
	RGBA value_color;
	value_color.R = 0;
	value_color.G = 255;
	value_color.B = 0;
	value_color.A = 255;
	
	if (Arc->value > Arc->max * 0.8) {
	    value_color.R = 255;
	    value_color.G = 128;
	    value_color.B = 0;
	}
	if (Arc->value > Arc->max * 0.95) {
	    value_color.R = 255;
	    value_color.G = 0;
	    value_color.B = 0;
	}
	
	draw_arc_segment(layer, cx, cy, radius - 2, Arc->start_angle, value_angle, &value_color, Arc->thickness - 2);
    }

    /* draw needle - from center outward */
    int needle_length = radius - 10;
    if (needle_length < 5) needle_length = 5;
    draw_needle_line(layer, cx, cy, needle_length, value_angle, &needle_color, 3);

    /* draw center circle */
    draw_center_circle(layer, cx, cy, 5, &center_color);

    /* flush area */
    drv_generic_graphic_blit(row, col, height, width);

    return 0;
}


/****************************************/
/*** generic init/quit                ***/
/****************************************/

int drv_generic_graphic_init(const char *section, const char *driver)
{
    int i, l;
    char *color;
    WIDGET_CLASS wc;

    Section = (char *) section;
    Driver = (char *) driver;

    /* init layout framebuffer */
    LROWS = 0;
    LCOLS = 0;

    for (l = 0; l < LAYERS; l++)
	drv_generic_graphic_FB[l] = NULL;

    drv_generic_graphic_resizeFB(DROWS, DCOLS);

    /* sanity check */
    for (l = 0; l < LAYERS; l++) {
	if (drv_generic_graphic_FB[l] == NULL) {
	    error("%s: framebuffer could not be allocated: malloc() failed", Driver);
	    return -1;
	}
    }

    /* init generic driver & register plugins */
    drv_generic_init();

    /* set default colors */
    color = cfg_get(Section, "foreground", "000000ff");
    if (color2RGBA(color, &FG_COL) < 0) {
	error("%s: ignoring illegal color '%s'", Driver, color);
    }
    if (color)
	free(color);

    color = cfg_get(Section, "background", "ffffff00");
    if (color2RGBA(color, &BG_COL) < 0) {
	error("%s: ignoring illegal color '%s'", Driver, color);
    }
    if (color)
	free(color);

    color = cfg_get(Section, "basecolor", "ffffff");
    if (color2RGBA(color, &BL_COL) < 0) {
	error("%s: ignoring illegal color '%s'", Driver, color);
    }
    if (color)
	free(color);

    /* inverted display? */
    cfg_number(section, "inverted", 0, 0, 1, &INVERTED);

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_graphic_draw;
    widget_register(&wc);

    /* register icon widget */
    wc = Widget_Icon;
    wc.draw = drv_generic_graphic_icon_draw;
    widget_register(&wc);

    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_graphic_bar_draw;
    widget_register(&wc);

    /* register image widget */
#ifdef WITH_IMAGE
    wc = Widget_Image;
    wc.draw = drv_generic_graphic_image_draw;
    widget_register(&wc);
#endif

    /* register graph widget */
    wc = Widget_Graph;
    wc.draw = drv_generic_graphic_graph_draw;
    widget_register(&wc);

    /* register arc widget */
    wc = Widget_Arc;
    wc.draw = drv_generic_graphic_arc_draw;
    widget_register(&wc);

    /* clear framebuffer but do not blit to display */
    for (l = 0; l < LAYERS; l++)
	for (i = 0; i < LCOLS * LROWS; i++)
	    drv_generic_graphic_FB[l][i] = NO_COL;

    return 0;
}


int drv_generic_graphic_clear(void)
{
    int i, l;

    for (l = 0; l < LAYERS; l++)
	for (i = 0; i < LCOLS * LROWS; i++)
	    drv_generic_graphic_FB[l][i] = NO_COL;

    drv_generic_graphic_blit(0, 0, LROWS, LCOLS);

    return 0;
}


RGBA drv_generic_graphic_rgb(const int row, const int col)
{
    return drv_generic_graphic_blend(row, col);
}


unsigned char drv_generic_graphic_gray(const int row, const int col)
{
    RGBA p = drv_generic_graphic_blend(row, col);
    return (77 * p.R + 150 * p.G + 28 * p.B) / 255;
}


unsigned char drv_generic_graphic_black(const int row, const int col)
{
    return drv_generic_graphic_gray(row, col) < 127;
}


int drv_generic_graphic_quit(void)
{
    int l;

    for (l = 0; l < LAYERS; l++) {
	if (drv_generic_graphic_FB[l]) {
	    free(drv_generic_graphic_FB[l]);
	    drv_generic_graphic_FB[l] = NULL;
	}
    }
    widget_unregister();
    return (0);
}
