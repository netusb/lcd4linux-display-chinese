/* TrueType font support for LCD4Linux using FreeType2 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "drv_generic_graphic.h"

static FT_Library ft_library = NULL;
static FT_Face ft_face = NULL;
static int ft_font_size = 16;
static int ft_font_width = 8;
static int ft_font_height = 16;
static int ft_initialized = 0;

static char *saved_font = NULL;
static int saved_size = 0;

int font_ttf_init(const char *fontfile, int size)
{
    int error;
    
    if (ft_initialized) {
        if (ft_face) {
            FT_Done_Face(ft_face);
            ft_face = NULL;
        }
        if (ft_library) {
            FT_Done_FreeType(ft_library);
            ft_library = NULL;
        }
    }
    
    error = FT_Init_FreeType(&ft_library);
    if (error) {
        fprintf(stderr, "FreeType: Failed to initialize library\n");
        return -1;
    }
    
    error = FT_New_Face(ft_library, fontfile, 0, &ft_face);
    if (error) {
        fprintf(stderr, "FreeType: Failed to load font '%s'\n", fontfile);
        FT_Done_FreeType(ft_library);
        return -1;
    }
    
    ft_font_size = size;
    FT_Set_Pixel_Sizes(ft_face, 0, size);
    
    /* For CJK fonts, use font size as character width (CJK chars are square) */
    /* max_advance_width can be incorrect for CJK fonts in .ttc collections */
    ft_font_width = size;
    ft_font_height = (ft_face->size->metrics.height + 63) / 64;
    
    if (ft_font_height == 0) ft_font_height = size;
    
    /* Enable UTF-8 mode */
    FT_Select_Charmap(ft_face, ft_encoding_unicode);
    
    ft_initialized = 1;
    return 0;
}

void font_ttf_quit(void)
{
    if (saved_font) {
        free(saved_font);
        saved_font = NULL;
    }
    if (ft_face) {
        FT_Done_Face(ft_face);
        ft_face = NULL;
    }
    if (ft_library) {
        FT_Done_FreeType(ft_library);
        ft_library = NULL;
    }
    ft_initialized = 0;
}

int font_ttf_get_width(void)
{
    return ft_font_width;
}

int font_ttf_get_height(void)
{
    return ft_font_height;
}

int font_ttf_switch(const char *fontfile, int size)
{
    int error;
    FT_Face new_face;
    
    if (!ft_library) return -1;
    
    error = FT_New_Face(ft_library, fontfile, 0, &new_face);
    if (error) {
        return -1;
    }
    
    FT_Set_Pixel_Sizes(new_face, 0, size);
    
    if (ft_face) {
        FT_Done_Face(ft_face);
    }
    ft_face = new_face;
    ft_font_size = size;
    /* For CJK fonts, use font size as character width (CJK chars are square) */
    ft_font_width = size;
    ft_font_height = (ft_face->size->metrics.height + 63) / 64;
    
    if (ft_font_height == 0) ft_font_height = size;
    
    return 0;
}

void font_ttf_restore(void)
{
    if (!saved_font || !ft_library) return;
    
    int error = FT_New_Face(ft_library, saved_font, 0, &ft_face);
    if (error) return;
    
    FT_Set_Pixel_Sizes(ft_face, 0, saved_size);
    ft_font_size = saved_size;
    /* For CJK fonts, use font size as character width (CJK chars are square) */
    ft_font_width = saved_size;
    ft_font_height = (ft_face->size->metrics.height + 63) / 64;
}

int font_ttf_is_available(void)
{
    return ft_initialized;
}

int font_ttf_setsize(int size)
{
    if (!ft_face) return -1;
    
    FT_Set_Pixel_Sizes(ft_face, 0, size);
    ft_font_size = size;
    /* For CJK fonts, use font size as character width (CJK chars are square) */
    ft_font_width = size;
    ft_font_height = (ft_face->size->metrics.height + 63) / 64;
    
    if (ft_font_height == 0) ft_font_height = size;
    
    return 0;
}

FT_Face font_ttf_get_face(void)
{
    return ft_face;
}

static void render_text_horizontal(int layer, int x, int y, RGBA fg, RGBA bg, const char *text)
{
    int pen_x = x;
    int error;
    unsigned char *utf8 = (unsigned char *)text;
    unsigned int codepoint;
    int bytes;
    int total_width = 0;
    RGBA *fb;
    int max_descent = 0;
    
    if (!ft_initialized || !ft_face || !text) return;
    fb = drv_generic_graphic_get_FB(layer);
    if (!fb) return;
    
    /* First pass: calculate total width and max descent */
    while (*utf8) {
        if ((*utf8 & 0xC0) == 0x80) {
            utf8++;
            continue;
        }
        
        if ((*utf8 & 0x80) == 0) {
            codepoint = *utf8;
            bytes = 1;
        } else if ((*utf8 & 0xE0) == 0xC0) {
            if (utf8[1] == 0) break;
            codepoint = ((utf8[0] & 0x1F) << 6) | (utf8[1] & 0x3F);
            bytes = 2;
        } else if ((*utf8 & 0xF0) == 0xE0) {
            if (utf8[1] == 0 || utf8[2] == 0) break;
            codepoint = ((utf8[0] & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F);
            bytes = 3;
        } else if ((*utf8 & 0xF8) == 0xF0) {
            if (utf8[1] == 0 || utf8[2] == 0 || utf8[3] == 0) break;
            codepoint = ((utf8[0] & 0x07) << 18) | ((utf8[1] & 0x3F) << 12) | 
                        ((utf8[2] & 0x3F) << 6) | (utf8[3] & 0x3F);
            bytes = 4;
        } else {
            utf8++;
            continue;
        }
        
        error = FT_Load_Char(ft_face, codepoint, FT_LOAD_DEFAULT);
        if (!error) {
            total_width += ft_face->glyph->advance.x / 64;
            int descent = ft_face->glyph->bitmap_top + ft_face->glyph->bitmap.rows - ft_font_height;
            if (descent > max_descent) max_descent = descent;
        }
        
        utf8 += bytes;
    }
    
    if (max_descent < 0) max_descent = 0;
    
    /* Clear the background area using configured bg color (only if not transparent) */
    if (bg.A != 0) {
        for (int cy = 0; cy < ft_font_height; cy++) {
            for (int cx = 0; cx < total_width + 10; cx++) {
                int draw_x = x + cx;
                int draw_y = y + cy;
                if (draw_x >= 0 && draw_x < LCOLS && 
                    draw_y >= 0 && draw_y < LROWS) {
                    fb[draw_y * LCOLS + draw_x] = bg;
                }
            }
        }
    }
    
    /* Second pass: render text */
    utf8 = (unsigned char *)text;
    pen_x = x;
    
    while (*utf8) {
        if ((*utf8 & 0xC0) == 0x80) {
            utf8++;
            continue;
        }
        
        if ((*utf8 & 0x80) == 0) {
            codepoint = *utf8;
            bytes = 1;
        } else if ((*utf8 & 0xE0) == 0xC0) {
            if (utf8[1] == 0) break;
            codepoint = ((utf8[0] & 0x1F) << 6) | (utf8[1] & 0x3F);
            bytes = 2;
        } else if ((*utf8 & 0xF0) == 0xE0) {
            if (utf8[1] == 0 || utf8[2] == 0) break;
            codepoint = ((utf8[0] & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F);
            bytes = 3;
        } else if ((*utf8 & 0xF8) == 0xF0) {
            if (utf8[1] == 0 || utf8[2] == 0 || utf8[3] == 0) break;
            codepoint = ((utf8[0] & 0x07) << 18) | ((utf8[1] & 0x3F) << 12) | 
                        ((utf8[2] & 0x3F) << 6) | (utf8[3] & 0x3F);
            bytes = 4;
        } else {
            utf8++;
            continue;
        }
        
        error = FT_Load_Char(ft_face, codepoint, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT);
        if (!error) {
            FT_GlyphSlot g = ft_face->glyph;
            
            /* Calculate vertical center offset within widget height (YRES) */
            int glyph_height = g->bitmap.rows;
            int v_offset = (YRES - glyph_height) / 2;
            if (v_offset < 0) v_offset = 0;
            
            for (int by = 0; by < (int)g->bitmap.rows; by++) {
                for (int bx = 0; bx < (int)g->bitmap.width; bx++) {
                    unsigned char pixel = g->bitmap.buffer[by * g->bitmap.width + bx];
                    int draw_x = pen_x + g->bitmap_left + bx;
                    int draw_y = y + v_offset + by;
                    
                    if (draw_x >= 0 && draw_x < LCOLS && 
                        draw_y >= 0 && draw_y < LROWS && pixel > 0) {
                        unsigned char alpha = pixel;
                        RGBA blended;
                        if (bg.A == 0) {
                            /* Transparent background - draw foreground only */
                            blended.R = fg.R;
                            blended.G = fg.G;
                            blended.B = fg.B;
                            blended.A = (alpha * fg.A) / 255;
                        } else {
                            /* Opaque background - blend */
                            blended.R = (fg.R * alpha + bg.R * (255 - alpha)) / 255;
                            blended.G = (fg.G * alpha + bg.G * (255 - alpha)) / 255;
                            blended.B = (fg.B * alpha + bg.B * (255 - alpha)) / 255;
                            blended.A = fg.A;
                        }
                        fb[draw_y * LCOLS + draw_x] = blended;
                    }
                }
            }
            
            pen_x += g->advance.x / 64;
        }
        
        utf8 += bytes;
    }
}

static void render_text_horizontal_right(int layer, int x, int y, RGBA fg, RGBA bg, const char *text, int max_width)
{
    int pen_x = x + max_width;
    int error;
    unsigned char *utf8 = (unsigned char *)text;
    unsigned int codepoint;
    int bytes;
    RGBA *fb;
    
    if (!ft_initialized || !ft_face || !text) return;
    fb = drv_generic_graphic_get_FB(layer);
    if (!fb) return;
    
    while (*utf8) {
        if ((*utf8 & 0xC0) == 0x80) {
            utf8++;
            continue;
        }
        
        if ((*utf8 & 0x80) == 0) {
            codepoint = *utf8;
            bytes = 1;
        } else if ((*utf8 & 0xE0) == 0xC0) {
            if (utf8[1] == 0) break;
            codepoint = ((utf8[0] & 0x1F) << 6) | (utf8[1] & 0x3F);
            bytes = 2;
        } else if ((*utf8 & 0xF0) == 0xE0) {
            if (utf8[1] == 0 || utf8[2] == 0) break;
            codepoint = ((utf8[0] & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F);
            bytes = 3;
        } else if ((*utf8 & 0xF8) == 0xF0) {
            if (utf8[1] == 0 || utf8[2] == 0 || utf8[3] == 0) break;
            codepoint = ((utf8[0] & 0x07) << 18) | ((utf8[1] & 0x3F) << 12) | 
                        ((utf8[2] & 0x3F) << 6) | (utf8[3] & 0x3F);
            bytes = 4;
        } else {
            utf8++;
            continue;
        }
        
        error = FT_Load_Char(ft_face, codepoint, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT);
        if (!error) {
            FT_GlyphSlot g = ft_face->glyph;
            
            pen_x -= g->advance.x / 64;
            
            /* Calculate vertical center offset within widget height (YRES) */
            int glyph_height = g->bitmap.rows;
            int v_offset = (YRES - glyph_height) / 2;
            if (v_offset < 0) v_offset = 0;
            
            for (int by = 0; by < (int)g->bitmap.rows; by++) {
                for (int bx = 0; bx < (int)g->bitmap.width; bx++) {
                    unsigned char pixel = g->bitmap.buffer[by * g->bitmap.width + bx];
                    int draw_x = pen_x + g->bitmap_left + bx;
                    int draw_y = y + v_offset + by;
                    
                    if (draw_x >= 0 && draw_x < LCOLS && 
                        draw_y >= 0 && draw_y < LROWS && pixel > 0) {
                        unsigned char alpha = pixel;
                        RGBA blended;
                        blended.R = (fg.R * alpha + bg.R * (255 - alpha)) / 255;
                        blended.G = (fg.G * alpha + bg.G * (255 - alpha)) / 255;
                        blended.B = (fg.B * alpha + bg.B * (255 - alpha)) / 255;
                        blended.A = fg.A;
                        fb[draw_y * LCOLS + draw_x] = blended;
                    }
                }
            }
        }
        
        utf8 += bytes;
    }
}

static void render_text_vertical_up(int layer, int x, int y, RGBA fg, RGBA bg, const char *text, int max_width, int max_height)
{
    int pen_y = y + max_height;
    int pen_x = x;
    int error;
    unsigned char *utf8 = (unsigned char *)text;
    unsigned int codepoint;
    int bytes;
    int col_spacing = 8;
    RGBA *fb;
    
    if (!ft_initialized || !ft_face || !text) return;
    fb = drv_generic_graphic_get_FB(layer);
    if (!fb) return;
    
    /* Calculate usable width based on font size, bounded by screen */
    int usable_width = ft_font_width * 2;
    if (usable_width > LCOLS - x)
        usable_width = LCOLS - x;
    if (usable_width < ft_font_width + 4)
        usable_width = ft_font_width + 4;
    int col_width = usable_width;
    
    while (*utf8) {
        if ((*utf8 & 0xC0) == 0x80) {
            utf8++;
            continue;
        }
        
        if ((*utf8 & 0x80) == 0) {
            codepoint = *utf8;
            bytes = 1;
        } else if ((*utf8 & 0xE0) == 0xC0) {
            if (utf8[1] == 0) break;
            codepoint = ((utf8[0] & 0x1F) << 6) | (utf8[1] & 0x3F);
            bytes = 2;
        } else if ((*utf8 & 0xF0) == 0xE0) {
            if (utf8[1] == 0 || utf8[2] == 0) break;
            codepoint = ((utf8[0] & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F);
            bytes = 3;
        } else if ((*utf8 & 0xF8) == 0xF0) {
            if (utf8[1] == 0 || utf8[2] == 0 || utf8[3] == 0) break;
            codepoint = ((utf8[0] & 0x07) << 18) | ((utf8[1] & 0x3F) << 12) | 
                        ((utf8[2] & 0x3F) << 6) | (utf8[3] & 0x3F);
            bytes = 4;
        } else {
            utf8++;
            continue;
        }
        
        error = FT_Load_Char(ft_face, codepoint, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT);
        if (!error) {
            FT_GlyphSlot g = ft_face->glyph;
            
            pen_y -= g->bitmap_top;
            
            int char_x = pen_x + (col_width - g->bitmap.width) / 2;
            
            for (int by = 0; by < (int)g->bitmap.rows; by++) {
                for (int bx = 0; bx < (int)g->bitmap.width; bx++) {
                    unsigned char pixel = g->bitmap.buffer[by * g->bitmap.width + bx];
                    int draw_x = char_x + bx;
                    int draw_y = pen_y + by;
                    
                    if (draw_x >= 0 && draw_x < LCOLS && 
                        draw_y >= 0 && draw_y < LROWS && pixel > 0) {
                        unsigned char alpha = pixel;
                        RGBA blended;
                        blended.R = (fg.R * alpha + bg.R * (255 - alpha)) / 255;
                        blended.G = (fg.G * alpha + bg.G * (255 - alpha)) / 255;
                        blended.B = (fg.B * alpha + bg.B * (255 - alpha)) / 255;
                        blended.A = fg.A;
                        fb[draw_y * LCOLS + draw_x] = blended;
                    }
                }
            }
            
            pen_y -= (g->bitmap.rows - g->bitmap_top);
            
            if (pen_y < y && pen_x + col_width + col_spacing < LCOLS) {
                pen_y = y + max_height;
                pen_x += col_width + col_spacing;
            }
        }
        
        utf8 += bytes;
    }
}

static void render_text_vertical_down(int layer, int x, int y, RGBA fg, RGBA bg, const char *text, int max_width, int max_height)
{
    int pen_y = y;
    int pen_x = x;
    int error;
    unsigned char *utf8 = (unsigned char *)text;
    unsigned int codepoint;
    int bytes;
    RGBA *fb;
    
    if (!ft_initialized || !ft_face || !text) return;
    fb = drv_generic_graphic_get_FB(layer);
    if (!fb) return;
    
    /* Calculate usable width based on font size, bounded by screen */
    int char_width = ft_font_width + 4;
    if (char_width > LCOLS - x)
        char_width = LCOLS - x;
    if (char_width < ft_font_width)
        char_width = ft_font_width;
    
    /* Use full display height for vertical text, not just widget height */
    int available_height = LROWS - y;
    if (available_height > max_height * 8)  /* Limit to 8 rows max */
        available_height = max_height * 8;
    
    /* Clear the background area using configured bg color */
    for (int cy = 0; cy < available_height; cy++) {
        for (int cx = 0; cx < char_width + 8; cx++) {
            int draw_x = x + cx;
            int draw_y = y + cy;
            if (draw_x >= 0 && draw_x < LCOLS && 
                draw_y >= 0 && draw_y < LROWS) {
                fb[draw_y * LCOLS + draw_x] = bg;
            }
        }
    }
    
    /* Render text vertically - each character stacked below the previous */
    while (*utf8) {
        if ((*utf8 & 0xC0) == 0x80) {
            utf8++;
            continue;
        }
        
        if ((*utf8 & 0x80) == 0) {
            codepoint = *utf8;
            bytes = 1;
        } else if ((*utf8 & 0xE0) == 0xC0) {
            if (utf8[1] == 0) break;
            codepoint = ((utf8[0] & 0x1F) << 6) | (utf8[1] & 0x3F);
            bytes = 2;
        } else if ((*utf8 & 0xF0) == 0xE0) {
            if (utf8[1] == 0 || utf8[2] == 0) break;
            codepoint = ((utf8[0] & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F);
            bytes = 3;
        } else if ((*utf8 & 0xF8) == 0xF0) {
            if (utf8[1] == 0 || utf8[2] == 0 || utf8[3] == 0) break;
            codepoint = ((utf8[0] & 0x07) << 18) | ((utf8[1] & 0x3F) << 12) | 
                        ((utf8[2] & 0x3F) << 6) | (utf8[3] & 0x3F);
            bytes = 4;
        } else {
            utf8++;
            continue;
        }
        
        error = FT_Load_Char(ft_face, codepoint, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT);
        if (!error) {
            FT_GlyphSlot g = ft_face->glyph;
            
            /* Center the glyph horizontally in the column */
            int char_x = pen_x + (char_width - g->bitmap.width) / 2;
            /* Position glyph vertically, centered in font height */
            int char_y = pen_y;
            
            /* Draw the glyph */
            for (int by = 0; by < (int)g->bitmap.rows; by++) {
                for (int bx = 0; bx < (int)g->bitmap.width; bx++) {
                    unsigned char pixel = g->bitmap.buffer[by * g->bitmap.width + bx];
                    if (pixel > 0) {
                        int draw_x = char_x + bx;
                        int draw_y = char_y + by;
                        
                    if (draw_x >= 0 && draw_x < LCOLS && 
                        draw_y >= 0 && draw_y < LROWS && pixel > 0) {
                        unsigned char alpha = pixel;
                        RGBA blended;
                        if (bg.A == 0) {
                            /* Transparent background - just draw foreground with alpha */
                            blended.R = fg.R;
                            blended.G = fg.G;
                            blended.B = fg.B;
                            blended.A = (alpha * fg.A) / 255;
                        } else {
                            /* Opaque background - blend */
                            blended.R = (fg.R * alpha + bg.R * (255 - alpha)) / 255;
                            blended.G = (fg.G * alpha + bg.G * (255 - alpha)) / 255;
                            blended.B = (fg.B * alpha + bg.B * (255 - alpha)) / 255;
                            blended.A = fg.A;
                        }
                        fb[draw_y * LCOLS + draw_x] = blended;
                    }
                    }
                }
            }
            
            /* Advance pen_y by the glyph height + 1 pixel spacing */
            pen_y += g->bitmap.rows + 1;
            
            /* Wrap to next column if we exceed available height */
            if (pen_y >= y + available_height && pen_x + char_width + 8 < LCOLS) {
                pen_y = y;
                pen_x += char_width + 8;
            }
        }
        
        utf8 += bytes;
    }
}

void font_ttf_render(int layer, int x, int y, RGBA fg, RGBA bg, const char *text, int max_width, int direction, int max_height)
{
    switch (direction) {
        case 0: /* TEXT_DIR_EAST: horizontal, left to right */
            render_text_horizontal(layer, x, y, fg, bg, text);
            break;
        case 1: /* TEXT_DIR_WEST: horizontal, right to left */
            render_text_horizontal_right(layer, x, y, fg, bg, text, max_width);
            break;
        case 2: /* TEXT_DIR_NORTH: vertical, bottom to top */
            render_text_vertical_up(layer, x, y, fg, bg, text, max_width, max_height);
            break;
        case 3: /* TEXT_DIR_SOUTH: vertical, top to bottom */
            render_text_vertical_down(layer, x, y, fg, bg, text, max_width, max_height);
            break;
        default:
            render_text_horizontal(layer, x, y, fg, bg, text);
            break;
    }
}
