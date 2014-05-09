#include "pebble.h"

Window *window;
TextLayer *text_hour_layer;
Layer *minute_circle_layer;

#define BACKGROUND GColorBlack
#define FOREGROUND GColorWhite


void circle_layer_update_callback(Layer *layer, GContext* ctx) {
	
	GRect bounds = layer_get_bounds(layer);
	const GPoint center = grect_center_point(&bounds);
	
	graphics_context_set_fill_color(ctx, FOREGROUND);
	
	const int16_t secondHandLength = (bounds.size.w / 2)-10;
	GPoint secondHand;
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	
	// Draw the minute arc
	for (int i=0; i<(t->tm_min)+1; i++) {
		int32_t second_angle = TRIG_MAX_ANGLE * (i) / 60;
		secondHand.y = (int16_t)(-cos_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.y;
		secondHand.x = (int16_t)(sin_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.x;
		graphics_fill_circle(ctx, secondHand, 5);
	}
	
	
	const int16_t minuteHandLength = (bounds.size.w / 3);
	GPoint minuteHand;
	
	// Draw the second arc
	for (int i=0; i<(t->tm_sec)+1; i++) {
		int32_t minute_angle = TRIG_MAX_ANGLE * (i) / 60;
		minuteHand.y = (int16_t)(-cos_lookup(minute_angle) * (int32_t)minuteHandLength / TRIG_MAX_RATIO) + center.y;
		minuteHand.x = (int16_t)(sin_lookup(minute_angle) * (int32_t)minuteHandLength / TRIG_MAX_RATIO) + center.x;
		graphics_fill_circle(ctx, minuteHand, 3);
	}
}

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
	// Need to be static because they're used by the system later.
	static char time_text[] = "00";
	
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
	
	text_layer_set_text(text_hour_layer, time_text);
}

void handle_deinit(void) {
	tick_timer_service_unsubscribe();
}


void handle_init(void) {
	window = window_create();
	window_stack_push(window, true /* Animated */);
	window_set_background_color(window, BACKGROUND);
	
	Layer *window_layer = window_get_root_layer(window);	
	
	
	GRect window_bounds = layer_get_bounds(window_layer);
	
	
	GRect full_window_frame = ((GRect) { .origin = { 0, 0 }, .size = { window_bounds.size.w, window_bounds.size.h } });
	minute_circle_layer = layer_create(full_window_frame);
	layer_set_update_proc(minute_circle_layer, circle_layer_update_callback);
	layer_add_child(window_layer, minute_circle_layer);
	
	
	// Create a text layer in the center of the screen for the hour
	text_hour_layer = text_layer_create((GRect) { .origin = { 0, 55 }, .size = { window_bounds.size.w, window_bounds.size.h/2 } });
	
	text_layer_set_text_color(text_hour_layer, FOREGROUND);
	text_layer_set_background_color(text_hour_layer, GColorClear);
	text_layer_set_font(text_hour_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	text_layer_set_text_alignment(text_hour_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(text_hour_layer));
	
	
	
	// Subscribe to the timer service
	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
	handle_second_tick(NULL, SECOND_UNIT);
}

int main(void) {
	handle_init();
	
	app_event_loop();
	
	handle_deinit();
}