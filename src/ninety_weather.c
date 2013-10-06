#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "stdlib.h"
#include "string.h"
#include "config.h"
#include "my_math.h"
#include "suncalc.h"
#include "http.h"
#include "util.h"
#include "link_monitor.h"

// This is the Default APP_ID to work with old versions of httpebble
//#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0x04, 0x9F, 0x49, 0xC0, 0x99, 0xAD }

//#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0x29, 0x08, 0xF1, 0x7C, 0x3F, 0xAC}
// Vic's
#define MY_UUID { 0x7D, 0xE9, 0x1C, 0x28, 0x50, 0xFF, 0x45, 0xF6, 0xB2, 0x6A, 0x1D, 0x9E, 0xB7, 0x87, 0x46, 0x01 }

#define DATE_Y 83
#define TIME_Y 107
	
#define TIMEZONE_ONE_Y 11
#define TIMEZONE_TWO_Y 45
	
#define HOUR_VIBRATION_START 8
#define HOUR_VIBRATION_END 20
	
#define TIMEZONE_LOCAL_OFFSET (-7)

#define TIMEZONE_ONE_NAME "CHN"
#define TIMEZONE_ONE_OFFSET (+8)
	
#define TIMEZONE_TWO_NAME "GER" 
#define TIMEZONE_TWO_OFFSET (+2)

	
PBL_APP_INFO(MY_UUID,
	     "Vic 91", "rfrcarvalho & Vic",
	     1, 0, /* App major/minor version */
	     RESOURCE_ID_IMAGE_MENU_ICON,
	     APP_INFO_WATCH_FACE);

Window window;
TextLayer text_sunrise_layer;
TextLayer text_sunset_layer;
TextLayer DayOfWeekLayer;
BmpContainer background_image;
BmpContainer time_format_image;
TextLayer calls_layer;   			/* layer for Phone Calls info */
TextLayer sms_layer;   				/* layer for SMS info */
TextLayer debug_layer;   			/* layer for DEBUG info */

TextLayer timezone_one;
TextLayer timezone_two;
TextLayer tzOneLayer;
TextLayer tzTwoLayer;
Layer secondDotLayer;

static int our_latitude, our_longitude, our_timezone = 99;
static bool located = false;
static bool calculated_sunset_sunrise = false;

const int DATENUM_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DATENUM_0,
  RESOURCE_ID_IMAGE_DATENUM_1,
  RESOURCE_ID_IMAGE_DATENUM_2,
  RESOURCE_ID_IMAGE_DATENUM_3,
  RESOURCE_ID_IMAGE_DATENUM_4,
  RESOURCE_ID_IMAGE_DATENUM_5,
  RESOURCE_ID_IMAGE_DATENUM_6,
  RESOURCE_ID_IMAGE_DATENUM_7,
  RESOURCE_ID_IMAGE_DATENUM_8,
  RESOURCE_ID_IMAGE_DATENUM_9
};

#define TOTAL_DATE_DIGITS 8
BmpContainer date_digits_images[TOTAL_DATE_DIGITS];

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

#define TOTAL_TIME_DIGITS 4
BmpContainer time_digits_images[TOTAL_TIME_DIGITS];

// Vic's
BmpContainer tzOne_digits_images[TOTAL_TIME_DIGITS];
BmpContainer tzTwo_digits_images[TOTAL_TIME_DIGITS];
BmpContainer tzMore[2];

void set_container_image(BmpContainer *bmp_container, const int resource_id, GPoint origin) {
  layer_remove_from_parent(&bmp_container->layer.layer);
  bmp_deinit_container(bmp_container);

  bmp_init_container(resource_id, bmp_container);

  GRect frame = layer_get_frame(&bmp_container->layer.layer);
  frame.origin.x = origin.x;
  frame.origin.y = origin.y;
  layer_set_frame(&bmp_container->layer.layer, frame);

  layer_add_child(&window.layer, &bmp_container->layer.layer);
}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
	
  if (hour == 12) 
	  return hour;

  unsigned short display_hour = hour % 12;
  return display_hour;

}

void adjustTimezone(float* time) 
{
  *time += our_timezone;
  if (*time > 24) *time -= 24;
  if (*time < 0) *time += 24;
}

void updateSunsetSunrise()
{
	// Calculating Sunrise/sunset with courtesy of Michael Ehrmann
	// https://github.com/mehrmann/pebble-sunclock
	static char sunrise_text[] = "00:00";
	static char sunset_text[]  = "00:00";
	
	PblTm pblTime;
	get_time(&pblTime);

	char *time_format;

	if (clock_is_24h_style()) 
	{
	  time_format = "%R";
	} 
	else 
	{
	  time_format = "%I:%M";
	}

	float sunriseTime = calcSunRise(pblTime.tm_year, pblTime.tm_mon+1, pblTime.tm_mday, our_latitude / 10000, our_longitude / 10000, 91.0f);
	float sunsetTime = calcSunSet(pblTime.tm_year, pblTime.tm_mon+1, pblTime.tm_mday, our_latitude / 10000, our_longitude / 10000, 91.0f);
	adjustTimezone(&sunriseTime);
	adjustTimezone(&sunsetTime);
	
	if (!pblTime.tm_isdst) 
	{
	  sunriseTime+=1;
	  sunsetTime+=1;
	} 

	pblTime.tm_min = (int)(60*(sunriseTime-((int)(sunriseTime))));
	pblTime.tm_hour = (int)sunriseTime;
	string_format_time(sunrise_text, sizeof(sunrise_text), time_format, &pblTime);
	text_layer_set_text(&text_sunrise_layer, sunrise_text);

	pblTime.tm_min = (int)(60*(sunsetTime-((int)(sunsetTime))));
	pblTime.tm_hour = (int)sunsetTime;
	string_format_time(sunset_text, sizeof(sunset_text), time_format, &pblTime);
	text_layer_set_text(&text_sunset_layer, sunset_text);
}

unsigned short the_last_hour = 25;

void display_counters(TextLayer *dataLayer, struct Data d, int infoType) {
	
	static char temp_text[5];
	
	if(d.link_status != LinkStatusOK){
		memcpy(temp_text, "?", 1);
	}
	else {	
		if (infoType == 1) {
			if(d.missed) {
				memcpy(temp_text, itoa(d.missed), 4);
			}
			else {
				memcpy(temp_text, itoa(0), 4);
			}
		}
		else if(infoType == 2) {
			if(d.unread) {
				memcpy(temp_text, itoa(d.unread), 4);
			}
			else {
				memcpy(temp_text, itoa(0), 4);
			}
		}
	}
	
	text_layer_set_text(dataLayer, temp_text);
}

void failed(int32_t cookie, int http_status, void* context) {

}

void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context) {
	
	link_monitor_handle_success(&data);
}

void location(float latitude, float longitude, float altitude, float accuracy, void* context) {
	// Fix the floats
	our_latitude = latitude * 10000;
	our_longitude = longitude * 10000;
	located = true;
}

void reconnect(void* context) {
	located = false;
}

bool read_state_data(DictionaryIterator* received, struct Data* d){
	(void)d;
	bool has_data = false;
	Tuple* tuple = dict_read_first(received);
	if(!tuple) return false;
	do {
		switch(tuple->key) {
	  		case TUPLE_MISSED_CALLS:
				d->missed = tuple->value->uint8;
				
				static char temp_calls[5];
				memcpy(temp_calls, itoa(tuple->value->uint8), 4);
				text_layer_set_text(&calls_layer, temp_calls);
				
				has_data = true;
				break;
			case TUPLE_UNREAD_SMS:
				d->unread = tuple->value->uint8;
			
				static char temp_sms[5];
				memcpy(temp_sms, itoa(tuple->value->uint8), 4);
				text_layer_set_text(&sms_layer, temp_sms);
			
				has_data = true;
				break;
		}
	}
	while((tuple = dict_read_next(received)));
	return has_data;
}

void app_received_msg(DictionaryIterator* received, void* context) {	
	link_monitor_handle_success(&data);
	
	read_state_data(received, &data);

}
static void app_send_failed(DictionaryIterator* failed, AppMessageResult reason, void* context) {
	link_monitor_handle_failure(reason, &data);
}

bool register_callbacks() {
	if (callbacks_registered) {
		if (app_message_deregister_callbacks(&app_callbacks) == APP_MSG_OK)
			callbacks_registered = false;
	}
	if (!callbacks_registered) {
		app_callbacks = (AppMessageCallbacksNode){
			.callbacks = { .in_received = app_received_msg, .out_failed = app_send_failed} };
		if (app_message_register_callbacks(&app_callbacks) == APP_MSG_OK) {
      callbacks_registered = true;
      }
	}
	return callbacks_registered;
}


void receivedtime(int32_t utc_offset_seconds, bool is_dst, uint32_t unixtime, const char* tz_name, void* context)
{	
	our_timezone = (utc_offset_seconds / 3600);
	if (is_dst)
	{
		our_timezone--;
	}
	
	if (located && our_timezone != 99 && !calculated_sunset_sunrise)
    {
        updateSunsetSunrise();
	    calculated_sunset_sunrise = true;
    }
}

void update_display(PblTm *current_time) {
  
  unsigned short display_hour = get_display_hour(current_time->tm_hour);
  short tzOne_hour = current_time->tm_hour + (TIMEZONE_ONE_OFFSET - TIMEZONE_LOCAL_OFFSET);
  short tzTwo_hour = current_time->tm_hour + (TIMEZONE_TWO_OFFSET - TIMEZONE_LOCAL_OFFSET);
	
	if (tzOne_hour >= 24)	tzOne_hour -= 24;
	if (tzOne_hour <   0)	tzOne_hour += 24;
	if (tzTwo_hour >= 24)	tzTwo_hour -= 24;
	if (tzTwo_hour <   0)	tzTwo_hour += 24;	
  
  //Hour
  if (display_hour/10 != 0) {
  	set_container_image(&time_digits_images[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(4, TIME_Y));
  }
  set_container_image(&time_digits_images[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(37, TIME_Y));
	
  if(current_time->tm_min == 0
      && current_time->tm_hour >= HOUR_VIBRATION_START
      && current_time->tm_hour <= HOUR_VIBRATION_END)
  {
      vibes_double_pulse();
  }
	
  if(current_time->tm_min == 0 && current_time->tm_hour == 0)
  {
      vibes_double_pulse();
  }
	
  
  //Minute
  set_container_image(&time_digits_images[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min/10], GPoint(80, TIME_Y));
  set_container_image(&time_digits_images[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min%10], GPoint(111, TIME_Y));
	
	
	// Vic's
	if (tzOne_hour/10 != 0) {
		set_container_image(&tzOne_digits_images[0], DATENUM_IMAGE_RESOURCE_IDS[tzOne_hour/10], GPoint(71, TIMEZONE_ONE_Y));
	} else {
		layer_remove_from_parent(&tzOne_digits_images[0].layer.layer);
		bmp_deinit_container(&tzOne_digits_images[0]);
	}
	set_container_image(&tzOne_digits_images[1], DATENUM_IMAGE_RESOURCE_IDS[tzOne_hour%10], GPoint(83, TIMEZONE_ONE_Y));
	
	set_container_image(&tzOne_digits_images[2], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_min/10], GPoint(103, TIMEZONE_ONE_Y));
	set_container_image(&tzOne_digits_images[3], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_min%10], GPoint(115, TIMEZONE_ONE_Y));
		
	
	if (tzTwo_hour/10 != 0) {
		//set_container_image(&tzTwo_digits_images[0], DATENUM_IMAGE_RESOURCE_IDS[1], GPoint(71, TIMEZONE_TWO_Y));
		set_container_image(&tzTwo_digits_images[2], DATENUM_IMAGE_RESOURCE_IDS[tzTwo_hour/10], GPoint(71, TIMEZONE_TWO_Y));
	} else {
		layer_remove_from_parent(&tzTwo_digits_images[2].layer.layer);
		bmp_deinit_container(&tzTwo_digits_images[2]);
	}
	//set_container_image(&tzTwo_digits_images[1], DATENUM_IMAGE_RESOURCE_IDS[1], GPoint(83, TIMEZONE_TWO_Y));
	set_container_image(&tzTwo_digits_images[3], DATENUM_IMAGE_RESOURCE_IDS[tzTwo_hour%10], GPoint(83, TIMEZONE_TWO_Y));
	
	//text_layer_set_text(&tzTwoLayer, VIC_HOURS[tzTwo_hour]);
	
	set_container_image(&tzMore[0], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_min/10], GPoint(103, TIMEZONE_TWO_Y));
	set_container_image(&tzMore[1], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_min%10], GPoint(115, TIMEZONE_TWO_Y));
	
   
  if (the_last_hour != display_hour){
	  
	  // Day of week
	  text_layer_set_text(&DayOfWeekLayer, DAY_NAME_LANGUAGE[current_time->tm_wday]); 
	  
	  // Day
	  if (current_time->tm_mday/10 != 0) {
	  	set_container_image(&date_digits_images[0], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday/10], GPoint(day_month_x[0], DATE_Y));
	  }
	  set_container_image(&date_digits_images[1], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday%10], GPoint(day_month_x[0] + 13, DATE_Y));
	  
	  // Month
	  if ((current_time->tm_mon+1)/10 != 0) {
	    set_container_image(&date_digits_images[2], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon+1)/10], GPoint(day_month_x[1], DATE_Y));
	  }
	  set_container_image(&date_digits_images[3], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon+1)%10], GPoint(day_month_x[1] + 13, DATE_Y));
	  
	  // Year
	  set_container_image(&date_digits_images[4], DATENUM_IMAGE_RESOURCE_IDS[((1900+current_time->tm_year)%1000)/10], GPoint(day_month_x[2], DATE_Y));
	  set_container_image(&date_digits_images[5], DATENUM_IMAGE_RESOURCE_IDS[((1900+current_time->tm_year)%1000)%10], GPoint(day_month_x[2] + 13, DATE_Y));
		
	  
	  if (!clock_is_24h_style()) {
		if (current_time->tm_hour >= 12) {
		  set_container_image(&time_format_image, RESOURCE_ID_IMAGE_PM_MODE, GPoint(1, 139));
		} else {
		  layer_remove_from_parent(&time_format_image.layer.layer);
		  bmp_deinit_container(&time_format_image);
		}

		if (display_hour/10 == 0) {
		  layer_remove_from_parent(&time_digits_images[0].layer.layer);
		  bmp_deinit_container(&time_digits_images[0]);
		}
	  }
	  
	  the_last_hour = display_hour;
  }
	
}

void second_dot_layer_update_callback(Layer *me, GContext* ctx) {

	(void) me;
	
	PblTm t;
	get_time(&t);
	
	// inner dot
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_circle(ctx, GPoint((11 + t.tm_sec * 2), 74), 3);
	
	// outer circle
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_draw_circle(ctx, GPoint((11 + t.tm_sec * 2), 74), 4);
	
}


void handle_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)ctx;	
	
	if (t->tick_time->tm_sec == 0) {
		update_display(t->tick_time);
	}
	
	layer_mark_dirty(&secondDotLayer);

	if(!located || !(t->tick_time->tm_min % 15))
	{
		// Every 15 minutes, request updated weather
		http_location_request();
	}
	
	// Every 15 minutes, request updated time
	http_time_request();
	
	if(!calculated_sunset_sunrise)
    {
	    // Start with some default values
	    text_layer_set_text(&text_sunrise_layer, "Wait!");
	    text_layer_set_text(&text_sunset_layer, "Wait!");
    }
	
	if(!(t->tick_time->tm_min % 2) || data.link_status == LinkStatusUnknown) link_monitor_ping();
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

	window_init(&window, "91 Weather");
	window_stack_push(&window, true /* Animated */);
  
	window_set_background_color(&window, GColorBlack);
  
	resource_init_current_app(&APP_RESOURCES);

	bmp_init_container(RESOURCE_ID_IMAGE_BACKGROUND, &background_image);
	layer_add_child(&window.layer, &background_image.layer.layer);
	

	// Sunrise Text
	text_layer_init(&text_sunrise_layer, window.layer.frame);
	text_layer_set_text_color(&text_sunrise_layer, GColorWhite);
	text_layer_set_background_color(&text_sunrise_layer, GColorClear);
	layer_set_frame(&text_sunrise_layer.layer, GRect(70, 151, 100, 30));
	text_layer_set_font(&text_sunrise_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	layer_add_child(&window.layer, &text_sunrise_layer.layer);

	// Sunset Text
	text_layer_init(&text_sunset_layer, window.layer.frame);
	text_layer_set_text_color(&text_sunset_layer, GColorWhite);
	text_layer_set_background_color(&text_sunset_layer, GColorClear);
	layer_set_frame(&text_sunset_layer.layer, GRect(110, 151, 100, 30));
	text_layer_set_font(&text_sunset_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	layer_add_child(&window.layer, &text_sunset_layer.layer); 
  

	// Day of week text
	text_layer_init(&DayOfWeekLayer, GRect(5, 74, 130 /* width */, 30 /* height */));
	layer_add_child(&background_image.layer.layer, &DayOfWeekLayer.layer);
	text_layer_set_text_color(&DayOfWeekLayer, GColorWhite);
	text_layer_set_background_color(&DayOfWeekLayer, GColorClear);
	text_layer_set_font(&DayOfWeekLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	
	// Timezone one
	text_layer_init(&tzOneLayer, GRect(15, 0, 55 /* width */, 30 /* height */));
	layer_add_child(&background_image.layer.layer, &tzOneLayer.layer);
	text_layer_set_text_color(&tzOneLayer, GColorWhite);
	text_layer_set_background_color(&tzOneLayer, GColorClear);
	text_layer_set_font(&tzOneLayer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	
	// Timezone two
	text_layer_init(&tzTwoLayer, GRect(15, 34, 55 /* width */, 30 /* height */));
	layer_add_child(&background_image.layer.layer, &tzTwoLayer.layer);
	text_layer_set_text_color(&tzTwoLayer, GColorWhite);
	text_layer_set_background_color(&tzTwoLayer, GColorClear);
	text_layer_set_font(&tzTwoLayer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	
	// Calls Info layer
	text_layer_init(&calls_layer, window.layer.frame);
	text_layer_set_text_color(&calls_layer, GColorWhite);
	text_layer_set_background_color(&calls_layer, GColorClear);
	layer_set_frame(&calls_layer.layer, GRect(12, 151, 100, 30));
	text_layer_set_font(&calls_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	layer_add_child(&window.layer, &calls_layer.layer);
	text_layer_set_text(&calls_layer, "?");
	
	// SMS Info layer 
	text_layer_init(&sms_layer, window.layer.frame);
	text_layer_set_text_color(&sms_layer, GColorWhite);
	text_layer_set_background_color(&sms_layer, GColorClear);
	layer_set_frame(&sms_layer.layer, GRect(41, 151, 100, 30));
	text_layer_set_font(&sms_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	layer_add_child(&window.layer, &sms_layer.layer);
	text_layer_set_text(&sms_layer, "?");

	// DEBUG Info layer 
	/*text_layer_init(&debug_layer, window.layer.frame);
	text_layer_set_text_color(&debug_layer, GColorWhite);
	text_layer_set_background_color(&debug_layer, GColorClear);
	layer_set_frame(&debug_layer.layer, GRect(50, 152, 100, 30));
	text_layer_set_font(&debug_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	layer_add_child(&window.layer, &debug_layer.layer);*/
	
	// Second dot
	layer_init(&secondDotLayer, window.layer.frame);
	secondDotLayer.update_proc = &second_dot_layer_update_callback;
	layer_add_child(&window.layer, &secondDotLayer);	
	
	data.link_status = LinkStatusUnknown;
	link_monitor_ping();
	
	// request data refresh on window appear (for example after notification)
	WindowHandlers handlers = { .appear = &link_monitor_ping};
	window_set_window_handlers(&window, handlers);
	
    http_register_callbacks((HTTPCallbacks){.failure=failed,.success=success,.reconnect=reconnect,.location=location,.time=receivedtime}, (void*)ctx);
    register_callbacks();
	
    // Avoids a blank screen on watch start.
    PblTm tick_time;

    get_time(&tick_time);
    update_display(&tick_time);
	
	text_layer_set_text(&tzOneLayer, TIMEZONE_ONE_NAME); 
	text_layer_set_text(&tzTwoLayer, TIMEZONE_TWO_NAME); 

}


void handle_deinit(AppContextRef ctx) {
	(void)ctx;

	bmp_deinit_container(&background_image);
	bmp_deinit_container(&time_format_image);

	for (int i = 0; i < TOTAL_DATE_DIGITS; i++) {
		bmp_deinit_container(&date_digits_images[i]);
	}
	
	for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
		bmp_deinit_container(&time_digits_images[i]);
		bmp_deinit_container(&tzOne_digits_images[i]);
	}
	
	bmp_deinit_container(&tzTwo_digits_images[2]);
	bmp_deinit_container(&tzTwo_digits_images[3]);
	
	bmp_deinit_container(&tzMore[0]);
	bmp_deinit_container(&tzMore[1]);	
	
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .tick_info = {
      .tick_handler = &handle_tick,
      .tick_units = SECOND_UNIT
    },
	.messaging_info = {
		.buffer_sizes = {
			.inbound = 124,
			.outbound = 256,
		}
	}
  };
  app_event_loop(params, &handlers);
}