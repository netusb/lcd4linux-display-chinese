#ifndef _FONT_TTF_H_
#define _FONT_TTF_H_

#include "rgb.h"

int font_ttf_init(const char *fontfile, int size);
void font_ttf_quit(void);
int font_ttf_get_width(void);
int font_ttf_get_height(void);
void font_ttf_render(int layer, int x, int y, RGBA fg, RGBA bg, const char *text, int max_width);
int font_ttf_is_available(void);

#endif
