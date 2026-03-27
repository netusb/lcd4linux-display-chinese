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

typedef enum { 
    ARC_STYLE_SEMI = 0,
    ARC_STYLE_QUARTER = 1,
    ARC_STYLE_FULL = 2
} ARC_STYLE;

typedef struct WIDGET_ARC {
    PROPERTY expression;
    PROPERTY expr_min;
    PROPERTY expr_max;
    PROPERTY update;
    int width;
    int height;
    double min;
    double max;
    double value;
    double start_angle;
    double end_angle;
    int style;
    int num_major;
    int num_minor;
    RGBA arc_color;
    RGBA needle_color;
    RGBA tick_color;
    RGBA center_color;
    RGBA text_color;
    RGBA bg_color;
    int thickness;
} WIDGET_ARC;


extern WIDGET_CLASS Widget_Arc;

#endif
