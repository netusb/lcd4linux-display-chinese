/* $Id: widget_arc.h $
 * Arc Gauge widget for LCD4Linux
 * Displays arc/gauge like AIDA64
 *
 * Copyright (C) 2024 LCD4Linux Team
 *
 */

#ifndef _WIDGET_ARC_H_
#define _WIDGET_ARC_H_

#include "property.h"
#include "widget.h"
#include "rgb.h"

/* AIDA64 style Arc Gauge widget */
typedef struct WIDGET_ARC {
    PROPERTY expression;
    PROPERTY expr_min;
    PROPERTY expr_max;
    PROPERTY update;
    
    /* Basic appearance */
    int diameter;           /* Diameter in pixels */
    int thickness;          /* Arc thickness in pixels */
    double start_angle;     /* Start angle (0=right, 90=down, 180=left, 270=up) */
    
    /* Value display */
    int show_value;         /* 1=show value text, 0=hide */
    int value_text_size;    /* Text size in pixels */
    char *value_font;       /* Font name for value */
    int value_bold;         /* 1=bold, 0=normal */
    int value_italic;       /* 1=italic, 0=normal */
    RGBA value_text_color;  /* Value text color */
    
    /* Needle display */
    int show_needle;        /* 1=show needle, 0=hide */
    int needle_length;      /* Needle length (pixels from center) */
    int needle_width;       /* Needle width (pixels) */
    RGBA needle_color;      /* Needle color */
    RGBA center_color;      /* Center circle color (origin) */
    
    /* Text position */
    int text_below;         /* 1=text below arc, 0=text in center */
    
    /* Direction */
    int reverse;            /* 1=left to right (via top), 0=right to left (via bottom) */
    
    /* Background */
    int show_background;    /* 1=show background, 0=transparent */
    RGBA background_color;  /* Background color */
    
    /* Value range */
    double min;
    double max;
    double value;
    
    /* Alert thresholds (5 limits define 4 segments) */
    double limit_min;       /* Min value (0%) */
    double limit_1;         /* Limit 1 threshold */
    double limit_2;         /* Limit 2 threshold */
    double limit_3;         /* Limit 3 threshold */
    double limit_max;       /* Max value (100%) */
    
    /* Arc colors for each segment */
    RGBA arc_color_min;     /* Min ~ Limit 1: Green (normal) */
    RGBA arc_color_1;       /* Limit 1 ~ Limit 2: Yellow (warning) */
    RGBA arc_color_2;       /* Limit 2 ~ Limit 3: Orange (medium alert) */
    RGBA arc_color_3;       /* Limit 3 ~ Max: Red (high alert) */
    
    /* Background colors for each segment (usually transparent/black) */
    RGBA back_color_min;
    RGBA back_color_1;
    RGBA back_color_2;
    RGBA back_color_3;
} WIDGET_ARC;


extern WIDGET_CLASS Widget_Arc;

#endif
