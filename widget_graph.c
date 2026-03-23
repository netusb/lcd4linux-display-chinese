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

    property_eval(&Graph->expression);
    value = P2N(&Graph->expression);

    if (property_valid(&Graph->expr_min)) {
	property_eval(&Graph->expr_min);
	Graph->min = P2N(&Graph->expr_min);
    }

    if (property_valid(&Graph->expr_max)) {
	property_eval(&Graph->expr_max);
	Graph->max = P2N(&Graph->expr_max);
    }

    Graph->data[Graph->data_head] = value;
    Graph->data_head = (Graph->data_head + 1) % Graph->num_points;
    if (Graph->data_count < Graph->num_points)
	Graph->data_count++;

    if (W->class->draw)
	W->class->draw(W);

    if (P2N(&Graph->update) > 0) {
	timer_add_widget(widget_graph_update, Self, P2N(&Graph->update), 1);
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

    Graph->width = 0;
    Graph->height = 0;
    Graph->num_points = GRAPH_DEFAULT_POINTS;
    Graph->min = 0.0;
    Graph->max = 100.0;
    Graph->style = 0;
    Graph->data = NULL;
    Graph->data_head = 0;
    Graph->data_count = 0;

    property_load(section, "expression", NULL, &Graph->expression);
    property_load(section, "min", NULL, &Graph->expr_min);
    property_load(section, "max", NULL, &Graph->expr_max);
    property_load(section, "update", "1000", &Graph->update);

    cfg_number(section, "points", GRAPH_DEFAULT_POINTS, 1, GRAPH_MAX_POINTS, &Graph->num_points);
    cfg_number(section, "style", 0, 0, 2, &Graph->style);

    if (!property_valid(&Graph->expression)) {
	error("Warning: widget %s has no expression", section);
    }

    Graph->data = malloc(Graph->num_points * sizeof(double));
    memset(Graph->data, 0, Graph->num_points * sizeof(double));

    Graph->line_color.R = 0;
    Graph->line_color.G = 255;
    Graph->line_color.B = 0;
    Graph->line_color.A = 255;

    Graph->fill_color.R = 0;
    Graph->fill_color.G = 128;
    Graph->fill_color.B = 0;
    Graph->fill_color.A = 128;

    Graph->bg_color.R = 0;
    Graph->bg_color.G = 0;
    Graph->bg_color.B = 0;
    Graph->bg_color.A = 255;

    Graph->grid_color.R = 64;
    Graph->grid_color.G = 64;
    Graph->grid_color.B = 64;
    Graph->grid_color.A = 255;

    widget_color(section, Self->name, "color", &Graph->line_color);
    widget_color(section, Self->name, "fill", &Graph->fill_color);
    widget_color(section, Self->name, "bg", &Graph->bg_color);
    widget_color(section, Self->name, "grid", &Graph->grid_color);

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
    .type = WIDGET_TYPE_XY,
    .init = widget_graph_init,
    .draw = NULL,
    .quit = widget_graph_quit,
};
