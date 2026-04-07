/* $Id: widget_ring.h $
 * Ring Progress widget for LCD4Linux
 * Displays circular progress ring like AIDA64
 *
 * Copyright (C) 2024 LCD4Linux Team
 *
 */

#ifndef _WIDGET_RING_H_
#define _WIDGET_RING_H_

#include "property.h"
#include "widget.h"
#include "rgb.h"

/* Circular Ring Progress widget */
typedef struct WIDGET_RING {
    PROPERTY expression;
    PROPERTY expr_min;
    PROPERTY expr_max;
    PROPERTY update;
    
    /* Basic appearance */
    int diameter;           /* Diameter in pixels */
    int thickness;          /* Ring thickness in pixels */
    int start_angle;        /* Start angle in degrees (0=right, 90=top, 180=left, 270=bottom) */
    
    /* Value display */
    int show_value;         /* 1=show value text, 0=hide */
    int value_text_size;    /* Text size in pixels */
    int value_precision;    /* Decimal places: 0, 1, or 2 */
    char *value_unit;       /* Unit suffix (e.g., "%", "°C") */
    RGBA value_text_color;  /* Value text color */
    
    /* Background ring */
    int show_background;    /* 1=show background ring, 0=hide */
    RGBA background_color;  /* Background ring color */
    
    /* Ring color */
    RGBA ring_color;        /* Ring foreground color */
    
    /* Value range */
    double min;
    double max;
    double value;
} WIDGET_RING;


extern WIDGET_CLASS Widget_Ring;

#endif
