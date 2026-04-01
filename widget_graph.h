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
    RGBA text_color;
    RGBA value_bg_color;    /* 数值背景颜色 */
    int style;
    int grid_lines;
    int show_value;
    int value_size;        /* 数值文字大小（像素） */
    int value_precision;   /* 小数位数：0, 1, 或 2 */
    char *value_unit;      /* 单位后缀 */
    int direction;         /* 折线方向：0=从左到右, 1=从右到左 */
} WIDGET_GRAPH;


extern WIDGET_CLASS Widget_Graph;

#endif
