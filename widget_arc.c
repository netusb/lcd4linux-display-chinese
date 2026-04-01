/* $Id: widget_arc.c $
 * Arc Gauge widget for LCD4Linux
 * Displays arc/gauge like AIDA64
 *
 * Copyright (C) 2024 LCD4Linux Team
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "debug.h"
#include "cfg.h"
#include "property.h"
#include "timer_group.h"
#include "widget.h"
#include "widget_arc.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


void widget_arc_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_ARC *Arc = W->data;
    int update_ms;

    /* evaluate update property first */
    property_eval(&Arc->update);
    update_ms = (int)P2N(&Arc->update);

    /* then evaluate expression */
    property_eval(&Arc->expression);
    Arc->value = P2N(&Arc->expression);

    if (property_valid(&Arc->expr_min)) {
	property_eval(&Arc->expr_min);
	Arc->min = P2N(&Arc->expr_min);
    }

    if (property_valid(&Arc->expr_max)) {
	property_eval(&Arc->expr_max);
	Arc->max = P2N(&Arc->expr_max);
    }

    if (W->class->draw)
	W->class->draw(W);

    if (update_ms > 0) {
	timer_add_widget(widget_arc_update, Self, update_ms, 1);
    }
}


int widget_arc_init(WIDGET * Self)
{
    char *section;
    WIDGET_ARC *Arc;

    if (Self->parent != NULL)
	return 0;

    section = malloc(strlen(Self->name) + 8);
    strcpy(section, "Widget:");
    strcat(section, Self->name);

    Arc = malloc(sizeof(WIDGET_ARC));
    memset(Arc, 0, sizeof(WIDGET_ARC));

    /* Default values - AIDA64 style */
    Arc->diameter = 100;        /* Default diameter */
    Arc->thickness = 10;        /* Default thickness */
    Arc->show_value = 1;        /* Show value by default */
    Arc->value_text_size = 12;  /* Default text size */
    Arc->value_bold = 0;
    Arc->value_italic = 0;
    Arc->show_needle = 1;       /* Show needle by default */
    Arc->needle_length = 0;     /* 0 = auto (radius - thickness) */
    Arc->needle_width = 3;      /* Default needle width */
    Arc->show_background = 0;   /* Background off by default */
    Arc->text_below = 1;        /* Text below needle by default */
    Arc->reverse = 0;           /* Normal direction (right to left) by default */
    
    Arc->min = 0.0;
    Arc->max = 100.0;
    Arc->value = 0.0;
    
    /* Alert thresholds */
    Arc->limit_min = 0.0;
    Arc->limit_1 = 33.33;
    Arc->limit_2 = 66.66;
    Arc->limit_3 = 90.0;
    Arc->limit_max = 100.0;

    /* Load properties */
    property_load(section, "expression", NULL, &Arc->expression);
    property_load(section, "min", NULL, &Arc->expr_min);
    property_load(section, "max", NULL, &Arc->expr_max);
    property_load(section, "update", "1000", &Arc->update);

    /* Basic appearance settings */
    cfg_number(section, "diameter", 100, 20, 500, &Arc->diameter);
    cfg_number(section, "thickness", 10, 2, 50, &Arc->thickness);
    
    /* Value display settings */
    cfg_number(section, "show_value", 1, 0, 1, &Arc->show_value);
    cfg_number(section, "value_text_size", 12, 6, 48, &Arc->value_text_size);
    cfg_number(section, "value_bold", 0, 0, 1, &Arc->value_bold);
    cfg_number(section, "value_italic", 0, 0, 1, &Arc->value_italic);
    
    /* Needle settings */
    cfg_number(section, "show_needle", 1, 0, 1, &Arc->show_needle);
    cfg_number(section, "needle_length", 0, 0, 100, &Arc->needle_length);
    cfg_number(section, "needle_width", 3, 1, 10, &Arc->needle_width);
    cfg_number(section, "text_below", 0, 0, 1, &Arc->text_below);
    cfg_number(section, "reverse", 0, 0, 1, &Arc->reverse);
    
    /* Background */
    cfg_number(section, "show_background", 0, 0, 1, &Arc->show_background);
    
    /* Alert thresholds */
    cfg_number(section, "limit_min", 0, 0, 100000, &Arc->limit_min);
    cfg_number(section, "limit_1", 33.33, 0, 100000, &Arc->limit_1);
    cfg_number(section, "limit_2", 66.66, 0, 100000, &Arc->limit_2);
    cfg_number(section, "limit_3", 90.0, 0, 100000, &Arc->limit_3);
    cfg_number(section, "limit_max", 100.0, 0, 100000, &Arc->limit_max);

    info("ARC init: name=%s diameter=%d thickness=%d show_value=%d reverse=%d", 
         Self->name, Arc->diameter, Arc->thickness, Arc->show_value, Arc->reverse);

    /* Set widget size based on diameter */
    Self->x2 = Self->col + Arc->diameter;
    Self->y2 = Self->row + Arc->diameter;

    if (!property_valid(&Arc->expression)) {
	error("Warning: widget %s has no expression", section);
    }

    /* Default colors - AIDA64 style */
    /* Arc colors: Green -> Yellow -> Orange -> Red */
    Arc->arc_color_min.R = 0;   Arc->arc_color_min.G = 200; Arc->arc_color_min.B = 0;   Arc->arc_color_min.A = 255;
    Arc->arc_color_1.R = 255;   Arc->arc_color_1.G = 255;   Arc->arc_color_1.B = 0;    Arc->arc_color_1.A = 255;
    Arc->arc_color_2.R = 255;   Arc->arc_color_2.G = 165;   Arc->arc_color_2.B = 0;    Arc->arc_color_2.A = 255;
    Arc->arc_color_3.R = 255;   Arc->arc_color_3.G = 0;     Arc->arc_color_3.B = 0;    Arc->arc_color_3.A = 255;
    
    /* Background colors - transparent by default */
    Arc->back_color_min.R = 0; Arc->back_color_min.G = 0; Arc->back_color_min.B = 0; Arc->back_color_min.A = 0;
    Arc->back_color_1.R = 0;   Arc->back_color_1.G = 0;   Arc->back_color_1.B = 0;   Arc->back_color_1.A = 0;
    Arc->back_color_2.R = 0;   Arc->back_color_2.G = 0;   Arc->back_color_2.B = 0;   Arc->back_color_2.A = 0;
    Arc->back_color_3.R = 0;   Arc->back_color_3.G = 0;   Arc->back_color_3.B = 0;   Arc->back_color_3.A = 0;
    
    /* Text color - white */
    Arc->value_text_color.R = 255;
    Arc->value_text_color.G = 255;
    Arc->value_text_color.B = 255;
    Arc->value_text_color.A = 255;
    
    /* Needle color - red */
    Arc->needle_color.R = 255;
    Arc->needle_color.G = 0;
    Arc->needle_color.B = 0;
    Arc->needle_color.A = 255;
    
    /* Center color - dark gray */
    Arc->center_color.R = 64;
    Arc->center_color.G = 64;
    Arc->center_color.B = 64;
    Arc->center_color.A = 255;
    
    /* Background color - black */
    Arc->background_color.R = 0;
    Arc->background_color.G = 0;
    Arc->background_color.B = 0;
    Arc->background_color.A = 255;

    /* Load colors from config */
    {
        char *color;
        RGBA c;
        
        /* Arc segment colors */
        color = cfg_get(section, "arc_color_min", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->arc_color_min = c;
        if (color) free(color);
        
        color = cfg_get(section, "arc_color_1", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->arc_color_1 = c;
        if (color) free(color);
        
        color = cfg_get(section, "arc_color_2", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->arc_color_2 = c;
        if (color) free(color);
        
        color = cfg_get(section, "arc_color_3", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->arc_color_3 = c;
        if (color) free(color);
        
        /* Background segment colors */
        color = cfg_get(section, "back_color_min", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->back_color_min = c;
        if (color) free(color);
        
        color = cfg_get(section, "back_color_1", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->back_color_1 = c;
        if (color) free(color);
        
        color = cfg_get(section, "back_color_2", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->back_color_2 = c;
        if (color) free(color);
        
        color = cfg_get(section, "back_color_3", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->back_color_3 = c;
        if (color) free(color);
        
        /* Text color */
        color = cfg_get(section, "value_text_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->value_text_color = c;
        if (color) free(color);
        
        /* Needle color */
        color = cfg_get(section, "needle_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->needle_color = c;
        if (color) free(color);
        
        /* Center color */
        color = cfg_get(section, "center_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->center_color = c;
        if (color) free(color);
        
        /* Background color */
        color = cfg_get(section, "background_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) Arc->background_color = c;
        if (color) free(color);
        
        /* Value font */
        Arc->value_font = cfg_get(section, "value_font", NULL);
    }

    free(section);
    Self->data = Arc;

    widget_arc_update(Self);

    return 0;
}


int widget_arc_quit(WIDGET * Self)
{
    if (Self) {
	if (Self->parent == NULL) {
	    if (Self->data) {
		WIDGET_ARC *Arc = Self->data;
		property_free(&Arc->expression);
		property_free(&Arc->expr_min);
		property_free(&Arc->expr_max);
		property_free(&Arc->update);
		if (Arc->value_font) free(Arc->value_font);
		free(Self->data);
		Self->data = NULL;
	    }
	}
    }
    return 0;
}



WIDGET_CLASS Widget_Arc = {
    .name = "arc",
    .type = WIDGET_TYPE_RC,
    .init = widget_arc_init,
    .draw = NULL,
    .quit = widget_arc_quit,
};
