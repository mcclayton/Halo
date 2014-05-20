#include "pebble.h"

Window *window;
TextLayer *text_hour_layer;
TextLayer *text_minute_layer;
TextLayer *text_date_layer;
Layer *arcs_circle_layer;
GFont dosis_font_56;
GFont dosis_font_20;
GFont dosis_font_16;

#define BACKGROUND GColorBlack
#define FOREGROUND GColorWhite


void circle_layer_update_callback(Layer *layer, GContext* ctx) {

	const GRect bounds = layer_get_bounds(layer);
	const GPoint center = grect_center_point(&bounds);

	// Define constants for hald the width and height of the current layer
	const int32_t width_center = (bounds.size.w / 2);
	const int32_t height_center = (bounds.size.h / 2);

	graphics_context_set_fill_color(ctx, FOREGROUND);

	// Get the local time
	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	// Draw the hour tick marks
	const int16_t hourHandLength = width_center-10;
	GPoint hourHand;
	int32_t hour_angle;
	for (int16_t i=0; i<61; i+=5) {
		hour_angle = TRIG_MAX_ANGLE * (i) / 60;
		hourHand.y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)hourHandLength / TRIG_MAX_RATIO) + center.y;
		hourHand.x = (int16_t)(sin_lookup(hour_angle) * (int32_t)hourHandLength / TRIG_MAX_RATIO) + center.x;

		// Draw tick mark
		graphics_context_set_fill_color(ctx, FOREGROUND);
		graphics_fill_circle(ctx, hourHand, 3);
		graphics_context_set_fill_color(ctx, BACKGROUND);
		graphics_fill_circle(ctx, hourHand, 2);
		graphics_context_set_fill_color(ctx, FOREGROUND);
	}

	// Highlight the current hour marker
	hour_angle = TRIG_MAX_ANGLE * (((t->tm_hour)%12)*5) / 60;
	hourHand.y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)hourHandLength / TRIG_MAX_RATIO) + center.y;
	hourHand.x = (int16_t)(sin_lookup(hour_angle) * (int32_t)hourHandLength / TRIG_MAX_RATIO) + center.x;
	graphics_fill_circle(ctx, hourHand, 5);

	// Draw the second arc
	const int16_t secondHandLength = (bounds.size.w / 3 - 1);
	GPoint secondHand;
	for (int16_t i=0; i<(t->tm_sec)+1; i++) {
		int32_t second_angle = TRIG_MAX_ANGLE * (i) / 60;
		secondHand.y = (int16_t)(-cos_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.y;
		secondHand.x = (int16_t)(sin_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.x;
		graphics_fill_circle(ctx, secondHand, 3);
	}

	// Draw divider between hour and minutes text
	const GPoint dividerStartOne = ((GPoint) {width_center-22, height_center+16});
	const GPoint dividerEndOne = ((GPoint) {width_center+22, height_center+16});
	const GPoint dividerStartTwo = ((GPoint) {width_center-22, height_center+17});
	const GPoint dividerEndTwo = ((GPoint) {width_center+22, height_center+17});

	graphics_context_set_stroke_color(ctx, FOREGROUND);
	graphics_draw_line(ctx, dividerStartOne, dividerEndOne);
	graphics_draw_line(ctx, dividerStartTwo, dividerEndTwo);
}

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
	// Need to be static because they're used by the system later.
	static char time_text[] = "00";
	static char minute_text[] = "00";
	static char date_text[32];

	char *time_format;

	if (!tick_time) {
		time_t now = time(NULL);
		tick_time = localtime(&now);
	}


	if (clock_is_24h_style()) {
		time_format = "%k";
	} else {
		time_format = "%I";
	}

	strftime(time_text, sizeof(time_text), time_format, tick_time);

	// Hack to handle lack of non-padded hour format string
	if (!clock_is_24h_style() && (time_text[0] == '0')) {
		memmove(time_text, &time_text[1], sizeof(time_text) - 1);
	}

	// Update the hours text
	text_layer_set_text(text_hour_layer, time_text);

	// Update the minutes text
	time_format = "%M";
	strftime(minute_text, sizeof(minute_text), time_format, tick_time);
	text_layer_set_text(text_minute_layer, minute_text);

	// Update the date text
	time_format = "%b %d";
	strftime(date_text, sizeof(date_text), time_format, tick_time);
	text_layer_set_text(text_date_layer, date_text);
}

void handle_deinit(void) {
	tick_timer_service_unsubscribe();
	text_layer_destroy(text_hour_layer);
	text_layer_destroy(text_minute_layer);
	text_layer_destroy(text_date_layer);
	layer_destroy(arcs_circle_layer);
	fonts_unload_custom_font(dosis_font_56);
	fonts_unload_custom_font(dosis_font_20);
	fonts_unload_custom_font(dosis_font_16);
	window_destroy(window);
}


void handle_init(void) {
	window = window_create();
	window_stack_push(window, true /* Animated */);
	window_set_background_color(window, BACKGROUND);

	Layer *window_layer = window_get_root_layer(window);

	GRect window_bounds = layer_get_bounds(window_layer);

	// Load in the fonts
	dosis_font_56 = fonts_load_custom_font( resource_get_handle(RESOURCE_ID_FONT_DOSIS_56) );
	dosis_font_20 = fonts_load_custom_font( resource_get_handle(RESOURCE_ID_FONT_DOSIS_20) );
	dosis_font_16 = fonts_load_custom_font( resource_get_handle(RESOURCE_ID_FONT_DOSIS_16) );

	GRect full_window_frame = ((GRect) { .origin = { 0, 0 }, .size = { window_bounds.size.w, window_bounds.size.h } });
	arcs_circle_layer = layer_create(full_window_frame);
	layer_set_update_proc(arcs_circle_layer, circle_layer_update_callback);
	layer_add_child(window_layer, arcs_circle_layer);


	// Create a text layer in the center of the screen for the hour
	text_hour_layer = text_layer_create((GRect) { .origin = { 0, 36 }, .size = { window_bounds.size.w, window_bounds.size.h/2 } });

	// Create a text layer in the center of the screen for minutes (shift origin to be below hourly text)
	text_minute_layer = text_layer_create((GRect) { .origin = { 0, 102 }, .size = { window_bounds.size.w, window_bounds.size.h/2 } });

	// Create a text layer in the center of the screen for the date (shift origin to be at the bottome of screen text)
	text_date_layer = text_layer_create((GRect) { .origin = { 0, 149 }, .size = { window_bounds.size.w, window_bounds.size.h/2 } });

	// Set the color/font/alignment of hourly text
	text_layer_set_background_color(text_hour_layer, GColorClear);
	text_layer_set_font(text_hour_layer, dosis_font_56);
	text_layer_set_text_color(text_hour_layer, FOREGROUND);
	text_layer_set_text_alignment(text_hour_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(text_hour_layer));

	// Set the color/font/alignment of minutes text
	text_layer_set_background_color(text_minute_layer, GColorClear);
	text_layer_set_font(text_minute_layer, dosis_font_20);
	text_layer_set_text_color(text_minute_layer, FOREGROUND);
	text_layer_set_text_alignment(text_minute_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(text_minute_layer));

	// Set the color/font/alignment of date text
	text_layer_set_background_color(text_date_layer, GColorClear);
	text_layer_set_font(text_date_layer, dosis_font_16);
	text_layer_set_text_color(text_date_layer, FOREGROUND);
	text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

	// Subscribe to the timer service
	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
	handle_second_tick(NULL, SECOND_UNIT);
}

int main(void) {
	handle_init();

	app_event_loop();

	handle_deinit();
}
