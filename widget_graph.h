/* $Id: widget_graph.h $
 * Graph widget for LCD4Linux
 * Displays line graph history like AIDA64
 *
 * Copyright (C) 2024 LCD4Linux Team
 *
 */

#ifndef _WIDGET_GRAPH_H_
#define _WIDGET_GRAPH_H_

#include "property.h"
#include "widget.h"
#include "rgb.h"

#define GRAPH_MAX_POINTS 200
#define GRAPH_DEFAULT_POINTS 50

typedef struct WIDGET_GRAPH {
    PROPERTY expression;
    PROPERTY expr_min;
    PROPERTY expr_max;
    PROPERTY update;
    int width;
    int height;
    int num_points;
    double min;
    double max;
    double *data;
    int data_head;
    int data_count;
    RGBA line_color;
    RGBA fill_color;
    RGBA bg_color;
    RGBA grid_color;
    int style;
} WIDGET_GRAPH;


extern WIDGET_CLASS Widget_Graph;

#endif
