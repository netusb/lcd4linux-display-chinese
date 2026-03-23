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

    if (P2N(&Arc->update) > 0) {
	timer_add_widget(widget_arc_update, Self, P2N(&Arc->update), 1);
    }
}


int widget_arc_init(WIDGET * Self)
{
    char *section;
    WIDGET_ARC *Arc;
    char *style_str;

    if (Self->parent != NULL)
	return 0;

    section = malloc(strlen(Self->name) + 8);
    strcpy(section, "Widget:");
    strcat(section, Self->name);

    Arc = malloc(sizeof(WIDGET_ARC));
    memset(Arc, 0, sizeof(WIDGET_ARC));

    Arc->width = 60;
    Arc->height = 40;
    Arc->min = 0.0;
    Arc->max = 100.0;
    Arc->value = 0.0;
    Arc->start_angle = 135.0;
    Arc->end_angle = 45.0;
    Arc->style = ARC_STYLE_SEMI;
    Arc->num_major = 5;
    Arc->num_minor = 5;
    Arc->thickness = 8;

    property_load(section, "expression", NULL, &Arc->expression);
    property_load(section, "min", NULL, &Arc->expr_min);
    property_load(section, "max", NULL, &Arc->expr_max);
    property_load(section, "update", "1000", &Arc->update);

    cfg_number(section, "width", 60, 1, 500, &Arc->width);
    cfg_number(section, "height", 40, 1, 300, &Arc->height);
    cfg_number(section, "ticks", 5, 1, 20, &Arc->num_major);
    cfg_number(section, "minor", 5, 0, 10, &Arc->num_minor);
    cfg_number(section, "thickness", 8, 2, 20, &Arc->thickness);

    Self->x2 = Self->col + Arc->width;
    Self->y2 = Self->row + Arc->height;

    style_str = cfg_get(section, "style", "semi");
    if (strcasecmp(style_str, "quarter") == 0) {
	Arc->style = ARC_STYLE_QUARTER;
	Arc->start_angle = 90.0;
	Arc->end_angle = 0.0;
    } else if (strcasecmp(style_str, "full") == 0) {
	Arc->style = ARC_STYLE_FULL;
	Arc->start_angle = 0.0;
	Arc->end_angle = 360.0;
    } else {
	Arc->style = ARC_STYLE_SEMI;
	Arc->start_angle = 135.0;
	Arc->end_angle = 45.0;
    }
    free(style_str);

    if (!property_valid(&Arc->expression)) {
	error("Warning: widget %s has no expression", section);
    }

    Arc->arc_color.R = 64;
    Arc->arc_color.G = 64;
    Arc->arc_color.B = 64;
    Arc->arc_color.A = 255;

    Arc->needle_color.R = 255;
    Arc->needle_color.G = 0;
    Arc->needle_color.B = 0;
    Arc->needle_color.A = 255;

    Arc->center_color.R = 128;
    Arc->center_color.G = 128;
    Arc->center_color.B = 128;
    Arc->center_color.A = 255;

    Arc->text_color.R = 255;
    Arc->text_color.G = 255;
    Arc->text_color.B = 255;
    Arc->text_color.A = 255;

    Arc->bg_color.R = 0;
    Arc->bg_color.G = 0;
    Arc->bg_color.B = 0;
    Arc->bg_color.A = 255;

    widget_color(section, Self->name, "arc", &Arc->arc_color);
    widget_color(section, Self->name, "needle", &Arc->needle_color);
    widget_color(section, Self->name, "center", &Arc->center_color);
    widget_color(section, Self->name, "bg", &Arc->bg_color);

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
