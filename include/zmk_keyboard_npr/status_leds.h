#pragma once

#define NUM_LED_PROFILES 5 // index 0 is reserved for the custom profile

// Max hsb values used when converting to rgb
#define HUE_MAX 360
#define SAT_MAX 100
#define BRT_MAX 100

#define PARAM_HUE 0
#define PARAM_SAT 1
#define PARAM_BRT 2

int npr_status_leds_cycle_profile(void);
int npr_status_leds_set_profile(int profile);
int npr_status_leds_change_brt(int direction);
int npr_status_leds_set_led(int led_index, int param, int value);
int npr_status_leds_off(void);