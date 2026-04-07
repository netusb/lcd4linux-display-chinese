/* $Id: widget_ring.c $
 * Ring Progress widget for LCD4Linux
 * Displays circular progress ring like AIDA64
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
#include "widget_ring.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


void widget_ring_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_RING *Ring = W->data;
    int update_ms;

    /* evaluate update property first */
    property_eval(&Ring->update);
    update_ms = (int)P2N(&Ring->update);

    /* then evaluate expression */
    property_eval(&Ring->expression);
    Ring->value = P2N(&Ring->expression);

    if (property_valid(&Ring->expr_min)) {
	property_eval(&Ring->expr_min);
	Ring->min = P2N(&Ring->expr_min);
    }

    if (property_valid(&Ring->expr_max)) {
	property_eval(&Ring->expr_max);
	Ring->max = P2N(&Ring->expr_max);
    }

    if (W->class->draw)
	W->class->draw(W);

    if (update_ms > 0) {
	timer_add_widget(widget_ring_update, Self, update_ms, 1);
    }
}


int widget_ring_init(WIDGET * Self)
{
    char *section;
    WIDGET_RING *Ring;

    if (Self->parent != NULL)
	return 0;

    section = malloc(strlen(Self->name) + 8);
    strcpy(section, "Widget:");
    strcat(section, Self->name);

    Ring = malloc(sizeof(WIDGET_RING));
    memset(Ring, 0, sizeof(WIDGET_RING));

    /* Default values */
    Ring->diameter = 100;
    Ring->thickness = 10;
    Ring->start_angle = 270;    /* Start from top (270° in math coords = 12 o'clock) */
    Ring->clockwise = 1;        /* Default: clockwise direction */
    Ring->show_value = 1;
    Ring->value_text_size = 16;
    Ring->value_precision = 0;
    Ring->value_unit = NULL;
    Ring->show_background = 1;
    Ring->min = 0.0;
    Ring->max = 100.0;
    Ring->value = 0.0;

    /* Load properties */
    property_load(section, "expression", NULL, &Ring->expression);
    property_load(section, "min", NULL, &Ring->expr_min);
    property_load(section, "max", NULL, &Ring->expr_max);
    property_load(section, "update", "1000", &Ring->update);

    /* Basic appearance settings */
    cfg_number(section, "diameter", 100, 20, 500, &Ring->diameter);
    cfg_number(section, "thickness", 10, 2, 50, &Ring->thickness);
    cfg_number(section, "start_angle", 270, 0, 360, &Ring->start_angle);
    cfg_number(section, "clockwise", 1, 0, 1, &Ring->clockwise);
    
    /* Value display settings */
    cfg_number(section, "show_value", 1, 0, 1, &Ring->show_value);
    cfg_number(section, "value_text_size", 16, 6, 48, &Ring->value_text_size);
    cfg_number(section, "value_precision", 0, 0, 2, &Ring->value_precision);
    Ring->value_unit = cfg_get(section, "value_unit", "");
    
    /* Background */
    cfg_number(section, "show_background", 1, 0, 1, &Ring->show_background);

    info("RING init: name=%s diameter=%d thickness=%d start_angle=%d clockwise=%d show_value=%d", 
         Self->name, Ring->diameter, Ring->thickness, Ring->start_angle, Ring->clockwise, Ring->show_value);

    /* Set widget size based on diameter */
    Self->x2 = Self->col + Ring->diameter;
    Self->y2 = Self->row + Ring->diameter;

    if (!property_valid(&Ring->expression)) {
	error("Warning: widget %s has no expression", section);
    }

    /* Default colors */
    Ring->ring_color.R = 0;   Ring->ring_color.G = 255; Ring->ring_color.B = 0;   Ring->ring_color.A = 255;
    Ring->background_color.R = 40; Ring->background_color.G = 40; Ring->background_color.B = 40; Ring->background_color.A = 255;
    Ring->value_text_color.R = 255; Ring->value_text_color.G = 255; Ring->value_text_color.B = 255; Ring->value_text_color.A = 255;

    /* Try to override with config colors */
    {
        char *color;
        RGBA c;
        color = cfg_get(section, "ring_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) {
            Ring->ring_color = c;
        }
        if (color) free(color);
        
        color = cfg_get(section, "background_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) {
            Ring->background_color = c;
        }
        if (color) free(color);
        
        color = cfg_get(section, "value_text_color", NULL);
        if (color && *color && color2RGBA(color, &c) >= 0) {
            Ring->value_text_color = c;
        }
        if (color) free(color);
    }

    free(section);
    Self->data = Ring;

    widget_ring_update(Self);

    return 0;
}


int widget_ring_quit(WIDGET *Self)
{
    if (Self) {
	if (Self->parent == NULL) {
	    if (Self->data) {
		WIDGET_RING *Ring = Self->data;
		property_free(&Ring->expression);
		property_free(&Ring->expr_min);
		property_free(&Ring->expr_max);
		property_free(&Ring->update);
		if (Ring->value_unit)
		    free(Ring->value_unit);
		free(Self->data);
		Self->data = NULL;
	    }
	}
    }
    return 0;
}


WIDGET_CLASS Widget_Ring = {
    .name = "ring",
    .type = WIDGET_TYPE_RC,
    .init = widget_ring_init,
    .draw = NULL,
    .quit = widget_ring_quit,
};
