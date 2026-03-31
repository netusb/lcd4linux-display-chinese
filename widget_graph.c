/* $Id: widget_graph.c $
 * Graph widget for LCD4Linux
 * Displays line graph history like AIDA64
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
#include "widget_graph.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


void widget_graph_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_GRAPH *Graph = W->data;
    double value;
    int update_ms;

    /* evaluate update property first */
    property_eval(&Graph->update);
    update_ms = (int)P2N(&Graph->update);
    
    /* then evaluate expression */
    property_eval(&Graph->expression);
    value = P2N(&Graph->expression);

    info("GRAPH update: value=%.2f count=%d update=%d", value, Graph->data_count, update_ms);

    /* store data in circular buffer */
    Graph->data[Graph->data_head] = value;
    Graph->data_head = (Graph->data_head + 1) % Graph->num_points;
    if (Graph->data_count < Graph->num_points)
	Graph->data_count++;

    /* trigger redraw */
    if (W->class->draw)
	W->class->draw(W);

    /* reschedule timer */
    info("GRAPH update: reschedule timer for %d ms", update_ms);
    if (update_ms > 0) {
	timer_add_widget(widget_graph_update, Self, update_ms, 1);
    }
}


int widget_graph_init(WIDGET * Self)
{
    char *section;
    WIDGET_GRAPH *Graph;

    if (Self->parent != NULL)
	return 0;

    section = malloc(strlen(Self->name) + 8);
    strcpy(section, "Widget:");
    strcat(section, Self->name);

    Graph = malloc(sizeof(WIDGET_GRAPH));
    memset(Graph, 0, sizeof(WIDGET_GRAPH));

    Graph->width = 40;
    Graph->height = 20;
    Graph->num_points = GRAPH_DEFAULT_POINTS;
    Graph->min = 0.0;
    Graph->max = 100.0;
    Graph->style = 0;
    Graph->grid_lines = 3;
    Graph->show_value = 1;
    Graph->data = NULL;
    Graph->data_head = 0;
    Graph->data_count = 0;

    property_load(section, "expression", NULL, &Graph->expression);
    property_load(section, "min", NULL, &Graph->expr_min);
    property_load(section, "max", NULL, &Graph->expr_max);
    property_load(section, "update", "1000", &Graph->update);

    cfg_number(section, "width", 40, 1, 500, &Graph->width);
    cfg_number(section, "height", 20, 1, 300, &Graph->height);
    cfg_number(section, "points", GRAPH_DEFAULT_POINTS, 1, GRAPH_MAX_POINTS, &Graph->num_points);
    cfg_number(section, "style", 0, 0, 2, &Graph->style);
    cfg_number(section, "grid", 3, 0, 10, &Graph->grid_lines);
    cfg_number(section, "value", 1, 0, 1, &Graph->show_value);
    cfg_number(section, "value_size", 10, 6, 24, &Graph->value_size);
    cfg_number(section, "direction", 0, 0, 1, &Graph->direction);

    Self->x2 = Self->col + Graph->width;
    Self->y2 = Self->row + Graph->height;

    if (!property_valid(&Graph->expression)) {
	error("Warning: widget %s has no expression", section);
    }

    Graph->data = malloc(Graph->num_points * sizeof(double));
    memset(Graph->data, 0, Graph->num_points * sizeof(double));

    /* default colors - bright green on dark background */
    Graph->line_color.R = 0;
    Graph->line_color.G = 255;
    Graph->line_color.B = 0;
    Graph->line_color.A = 255;

    Graph->fill_color.R = 255;
    Graph->fill_color.G = 200;
    Graph->fill_color.B = 0;
    Graph->fill_color.A = 200;

    Graph->bg_color.R = 10;
    Graph->bg_color.G = 10;
    Graph->bg_color.B = 10;
    Graph->bg_color.A = 255;

    Graph->grid_color.R = 40;
    Graph->grid_color.G = 40;
    Graph->grid_color.B = 40;
    Graph->grid_color.A = 255;

    Graph->text_color.R = 255;
    Graph->text_color.G = 255;
    Graph->text_color.B = 255;
    Graph->text_color.A = 255;

    Graph->value_bg_color.R = 0;
    Graph->value_bg_color.G = 0;
    Graph->value_bg_color.B = 0;
    Graph->value_bg_color.A = 0;  /* default: transparent */

    /* try to override with config colors, but keep defaults if not set */
    {
        char *color;
        RGBA c;
        color = cfg_get(section, "line_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) {
            Graph->line_color = c;
        }
        if (color) free(color);
        
        color = cfg_get(section, "fill_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) {
            Graph->fill_color = c;
        }
        if (color) free(color);
        
        color = cfg_get(section, "bg_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) {
            Graph->bg_color = c;
        }
        if (color) free(color);
        
        color = cfg_get(section, "grid_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) {
            Graph->grid_color = c;
        }
        if (color) free(color);
        
        color = cfg_get(section, "text_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) {
            Graph->text_color = c;
        }
        if (color) free(color);
        
        color = cfg_get(section, "value_bg_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) {
            Graph->value_bg_color = c;
        }
        if (color) free(color);
    }

    /* respect config alpha values */

    free(section);
    Self->data = Graph;

    widget_graph_update(Self);

    return 0;
}


int widget_graph_quit(WIDGET * Self)
{
    if (Self) {
	if (Self->parent == NULL) {
	    if (Self->data) {
		WIDGET_GRAPH *Graph = Self->data;
		property_free(&Graph->expression);
		property_free(&Graph->expr_min);
		property_free(&Graph->expr_max);
		property_free(&Graph->update);
		if (Graph->data)
		    free(Graph->data);
		free(Self->data);
		Self->data = NULL;
	    }
	}
    }
    return 0;
}



WIDGET_CLASS Widget_Graph = {
    .name = "graph",
    .type = WIDGET_TYPE_RC,
    .init = widget_graph_init,
    .draw = NULL,
    .quit = widget_graph_quit,
};
