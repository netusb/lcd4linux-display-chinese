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
    
    ft_font_width = (ft_face->max_advance_width + 63) / 64;
    ft_font_height = (ft_face->size->metrics.height + 63) / 64;
    
    if (ft_font_width == 0) ft_font_width = size;
    if (ft_font_height == 0) ft_font_height = size;
    
    /* Enable UTF-8 mode */
    FT_Select_Charmap(ft_face, ft_encoding_unicode);
    
    ft_initialized = 1;
    return 0;
}

void font_ttf_quit(void)
{
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

void font_ttf_render(int layer, int x, int y, RGBA fg, RGBA bg, const char *text, int max_width)
{
    int pen_x = x;
    int pen_y = y;
    int error;
    unsigned char *utf8 = (unsigned char *)text;
    unsigned int codepoint;
    int bytes;
    int total_width = 0;
    int max_height = 0;
    
    (void)layer;
    
    if (!ft_initialized || !ft_face || !text) return;
    if (!drv_generic_graphic_FB_ptr) return;
    
    /* First pass: calculate total width */
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
            if (ft_face->glyph->bitmap.rows > max_height)
                max_height = ft_face->glyph->bitmap.rows;
        }
        
        utf8 += bytes;
    }
    
    /* Clear the background area */
    for (int cy = 0; cy < ft_font_height; cy++) {
        for (int cx = 0; cx < total_width + 10; cx++) {
            int draw_x = x + cx;
            int draw_y = y + cy;
            if (draw_x >= 0 && draw_x < LCOLS && 
                draw_y >= 0 && draw_y < LROWS) {
                drv_generic_graphic_FB_ptr[draw_y * LCOLS + draw_x] = bg;
            }
        }
    }
    
    /* Second pass: render text */
    utf8 = (unsigned char *)text;
    pen_x = x;
    
    while (*utf8) {
        /* Skip invalid continuation bytes */
        if ((*utf8 & 0xC0) == 0x80) {
            utf8++;
            continue;
        }
        
        /* Decode UTF-8 character */
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
        
        /* Load and render the glyph */
        error = FT_Load_Char(ft_face, codepoint, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT);
        if (!error) {
            FT_GlyphSlot g = ft_face->glyph;
            
            /* Draw the glyph bitmap with anti-aliasing */
            for (int by = 0; by < (int)g->bitmap.rows; by++) {
                for (int bx = 0; bx < (int)g->bitmap.width; bx++) {
                    unsigned char pixel = g->bitmap.buffer[by * g->bitmap.width + bx];
                    int draw_x = pen_x + g->bitmap_left + bx;
                    int draw_y = pen_y + ft_font_height - g->bitmap_top + by;
                    
                    if (draw_x >= 0 && draw_x < LCOLS && 
                        draw_y >= 0 && draw_y < LROWS && pixel > 0) {
                        /* Blend with background based on pixel value */
                        unsigned char alpha = pixel;
                        RGBA blended;
                        blended.R = (fg.R * alpha + bg.R * (255 - alpha)) / 255;
                        blended.G = (fg.G * alpha + bg.G * (255 - alpha)) / 255;
                        blended.B = (fg.B * alpha + bg.B * (255 - alpha)) / 255;
                        blended.A = fg.A;
                        drv_generic_graphic_FB_ptr[draw_y * LCOLS + draw_x] = blended;
                    }
                }
            }
            
            /* Advance to next character position */
            pen_x += g->advance.x / 64;
        }
        
        utf8 += bytes;
    }
}

int font_ttf_is_available(void)
{
    return ft_initialized;
}
