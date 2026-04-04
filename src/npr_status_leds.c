#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/pm/device.h>

#include <math.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>

#include <zephyr/drivers/led_strip.h>

#include <zmk/activity.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/keymap.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/workqueue.h>
#include <drivers/ext_power.h>

#include <zmk_keyboard_npr/status_leds.h>

LOG_MODULE_REGISTER(npr, CONFIG_ZMK_LOG_LEVEL);

#if !DT_HAS_CHOSEN(npr_status_leds)

#error "A npr,status-leds chosen node must be declared"

#endif

#define STRIP_CHOSEN DT_CHOSEN(npr_status_leds)
#define STRIP_NUM_PIXELS DT_PROP(STRIP_CHOSEN, chain_length)

#define DEFAULT_MAX_BRIGHTNESS 10
#define MAX_BRT_STEP 5
#define ANIMATION_SPEED 3

enum npr_status_effect {
    NPR_STATUS_DEFAULT,
    NPR_STATUS_STARTING,
    NPR_STATUS_LAYER_CHANGE,
    NPR_STATUS_ENDPOINT_CHANGE,
};

struct zmk_led_hsb {
    uint16_t h;
    uint8_t s;
    uint8_t b;
};

struct npr_status_leds_config { // Config that will persist between restarts
    uint8_t active_led_profile;
    uint8_t max_brightness;
    struct zmk_led_hsb custom_led_0;
    struct zmk_led_hsb custom_led_1;
    struct zmk_led_hsb custom_led_2;
};

struct npr_status_leds_state {
    uint8_t current_effect;
    uint16_t animation_step;
    uint8_t active_layer;
    bool on;
};


static const struct device *led_strip;

static struct led_rgb pixels[STRIP_NUM_PIXELS];

// Default individual colors
static const struct zmk_led_hsb npr_led_off = {h :   0, s : 0, b : 0};
static const struct zmk_led_hsb npr_led_red = {h :   3, s : 97, b : 75};
static const struct zmk_led_hsb npr_led_orange = {h :  15, s : 96, b : 75};
static const struct zmk_led_hsb npr_led_cyan = {h : 148, s : 65, b : 75};
static const struct zmk_led_hsb npr_led_light_orange = {h : 23, s : 93, b : 75};

static struct zmk_led_hsb npr_lights_profiles[NUM_LED_PROFILES][STRIP_NUM_PIXELS] = {
    { npr_led_off, npr_led_off, npr_led_off}, // placeholder for custom leds
    { npr_led_red, npr_led_light_orange, npr_led_cyan },
    { npr_led_red, npr_led_orange, npr_led_light_orange },
    { npr_led_light_orange, npr_led_red, npr_led_orange },
    { npr_led_cyan, npr_led_light_orange, npr_led_red },
};

static struct npr_status_leds_config config;
static struct npr_status_leds_state state;

static const struct device *const ext_power = DEVICE_DT_GET(DT_INST(0, zmk_ext_power_generic));

struct zmk_led_hsb get_led_default_color(int index) {
    if (config.active_led_profile == 0) {
        switch(index) {
            case 0:
                return config.custom_led_0;
            case 1:
                return config.custom_led_1;
            case 2:
                return config.custom_led_2;
        }
    }

    return npr_lights_profiles[config.active_led_profile][index];
}

static struct zmk_led_hsb hsb_scale_zero_max(struct zmk_led_hsb hsb) {
    hsb.b = hsb.b * config.max_brightness / 100;
    return hsb;
}

static struct led_rgb hsb_to_rgb(struct zmk_led_hsb hsb) {
    float r = 0, g = 0, b = 0;

    uint8_t i = hsb.h / 60;
    float v = hsb.b / ((float)BRT_MAX);
    float s = hsb.s / ((float)SAT_MAX);
    float f = hsb.h / ((float)HUE_MAX) * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
        r = v;
        g = p;
        b = q;
        break;
    }

    struct led_rgb rgb = {r : r * 255, g : g * 255, b : b * 255};

    return rgb;
}

static void npr_status_effect_default(void) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        pixels[i] = hsb_to_rgb(hsb_scale_zero_max(get_led_default_color(i)));
    }
}

static void npr_status_effect_pairing(void) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        struct zmk_led_hsb hsb = get_led_default_color(i);
        uint8_t max_b = hsb.b;
        hsb.b = CLAMP((abs(state.animation_step-100)-(100/3))*1.5, 0, 100);
        hsb.b = hsb.b * max_b / 100;

        pixels[i] = hsb_to_rgb(hsb_scale_zero_max(hsb));
    }              

    state.animation_step += ANIMATION_SPEED * 10;

    if (state.animation_step > 200) {
        state.animation_step = 0;
    }
}

static void npr_status_effect_starting(void) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        struct zmk_led_hsb hsb = get_led_default_color(i);
        uint8_t max_b = hsb.b;
        if (state.animation_step >= (i+1) * 800) {
            hsb.b = 100;
        }
        else if (state.animation_step <= (i+1) * 800  && state.animation_step > i * 800)
        {
            hsb.b = (state.animation_step - i*800)/8;
        }
        else {
            hsb.b = 0;
        }
        hsb.b = hsb.b * max_b / 100;

        pixels[i] = hsb_to_rgb(hsb_scale_zero_max(hsb));
    }

    state.animation_step += ANIMATION_SPEED * 10;

    if (state.animation_step > 2400) {
        state.animation_step = 0;
        state.current_effect = NPR_STATUS_DEFAULT;
    }
}

static void npr_status_effect_layer_change(void) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        struct zmk_led_hsb hsb = get_led_default_color(i);
        uint8_t max_b = hsb.b;
        if (state.active_layer == i
            || (state.active_layer == 3 && i < 2)
            || (state.active_layer == 4 && i != 1)
            || (state.active_layer == 5 && i > 0)) {
            hsb.b = abs((state.animation_step % 800) - 400)/4;
        }
        else {
            hsb.b = 0;
        }
        hsb.b = hsb.b * max_b / 100;

        pixels[i] = hsb_to_rgb(hsb_scale_zero_max(hsb));
    }

    state.animation_step += ANIMATION_SPEED * 10;

    if (state.animation_step > 2400) {
        state.animation_step = 0;
        state.current_effect = NPR_STATUS_DEFAULT;
    }
}

static void npr_status_effect_endpoint_change(void) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        struct zmk_led_hsb hsb = get_led_default_color(i);
        uint8_t max_b = hsb.b;
        hsb.b = CLAMP((abs((state.animation_step%200)-100)-(100/3))*1.5, 0, 100);
        hsb.b = hsb.b * max_b / 100;

        pixels[i] = hsb_to_rgb(hsb_scale_zero_max(hsb));
    }              

    state.animation_step += ANIMATION_SPEED * 15;

    if (state.animation_step > 600) {
        state.animation_step = 0;
        state.current_effect = NPR_STATUS_DEFAULT;
    }
}

static void npr_status_leds_tick(struct k_work *work) {
    switch (state.current_effect) {
    case NPR_STATUS_DEFAULT:
        if (zmk_endpoints_selected().transport != ZMK_TRANSPORT_USB && zmk_ble_active_profile_is_open()) {
            npr_status_effect_pairing();
        }
        else {
            npr_status_effect_default();
        }
        break;
    case NPR_STATUS_STARTING:
        npr_status_effect_starting();
        break;
    case NPR_STATUS_LAYER_CHANGE:
        npr_status_effect_layer_change();
        break;
    case NPR_STATUS_ENDPOINT_CHANGE:
        npr_status_effect_endpoint_change();
        break;
    }

    int err = led_strip_update_rgb(led_strip, pixels, STRIP_NUM_PIXELS);
    if (err < 0) {
        LOG_ERR("Failed to update the RGB strip (%d)", err);
    }
}

K_WORK_DEFINE(status_leds_tick_work, npr_status_leds_tick);

static void npr_status_leds_tick_handler(struct k_timer *timer) {
    if (!state.on) {
        return;
    }

    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &status_leds_tick_work);
}

K_TIMER_DEFINE(status_leds_tick, npr_status_leds_tick_handler, NULL);

#if IS_ENABLED(CONFIG_SETTINGS)
static int npr_status_leds_settings_load(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    int rc;

    if (settings_name_steq(name, "config", &next) && !next) {
        if (len != sizeof(config)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &config, sizeof(config));
        if (rc >= 0) {
            if (state.on) {
                k_timer_start(&status_leds_tick, K_NO_WAIT, K_MSEC(50));
            }

            return 0;
        }

        return rc;
    }

    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(npr_status_leds, "npr/status-leds", NULL, npr_status_leds_settings_load, NULL, NULL);

static void npr_status_leds_save_state_work(struct k_work *_work) {
    settings_save_one("npr/status-leds/config", &config, sizeof(config));
}

static struct k_work_delayable npr_status_leds_save_work;
#endif

static int npr_status_leds_init(void) {
    led_strip = DEVICE_DT_GET(STRIP_CHOSEN);

    if (!device_is_ready(ext_power)) {
        LOG_ERR("External power device \"%s\" is not ready", ext_power->name);
        return -ENODEV;
    }

    config = (struct npr_status_leds_config){
        active_led_profile: 1, // start at 1 because 0 is the custom profile
        max_brightness: DEFAULT_MAX_BRIGHTNESS,
        custom_led_0: npr_led_off,
        custom_led_1: npr_led_off,
        custom_led_2: npr_led_off,
    };

    state = (struct npr_status_leds_state){
        current_effect : NPR_STATUS_STARTING,
        animation_step : 0,
        active_layer: 0,
        on: true
    };

#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_init_delayable(&npr_status_leds_save_work, npr_status_leds_save_state_work);
#endif

    if (state.on) {
        k_timer_start(&status_leds_tick, K_NO_WAIT, K_MSEC(50));
    }

    return 0;
}

static int npr_status_leds_save_state(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    int ret = k_work_reschedule(&npr_status_leds_save_work, K_MSEC(20000));
    return MIN(ret, 0);
#else
    return 0;
#endif
}

int npr_status_leds_on(void) {
    if (!led_strip)
        return -ENODEV;

    if (ext_power != NULL) {
        int rc = ext_power_enable(ext_power);
        if (rc != 0) {
            LOG_ERR("Unable to enable EXT_POWER: %d", rc);
        }
    }

    state.on = true;
    state.animation_step = 0;
    k_timer_start(&status_leds_tick, K_NO_WAIT, K_MSEC(50));

    return 0;
}

static void npr_status_leds_off_handler(struct k_work *work) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        pixels[i] = (struct led_rgb){r : 0, g : 0, b : 0};
    }

    led_strip_update_rgb(led_strip, pixels, STRIP_NUM_PIXELS);
}

K_WORK_DEFINE(npr_status_leds_off_work, npr_status_leds_off_handler);

int npr_status_leds_off(void) {
    if (!led_strip)
        return -ENODEV;

    if (ext_power != NULL) {
        int rc = ext_power_disable(ext_power);
        if (rc != 0) {
            LOG_ERR("Unable to disable EXT_POWER: %d", rc);
        }
    }

    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &npr_status_leds_off_work);

    k_timer_stop(&status_leds_tick);
    state.on = false;

    return 0;
}

static int npr_status_leds_auto_state(bool target_wake_state) {
    // wake up event while awake, or sleep event while sleeping -> no-op
    if (target_wake_state == state.on) {
        return 0;
    }

    if (target_wake_state) {
        return npr_status_leds_on();
    } else {
        return npr_status_leds_off();
    }
}

int npr_status_leds_cycle_profile(void) {
    config.active_led_profile = (config.active_led_profile + 1) % NUM_LED_PROFILES;

    npr_status_leds_save_state();
    return 0;
}

int npr_status_leds_set_profile(int profile) {
    if (profile >= 0 && profile < NUM_LED_PROFILES) {
        config.active_led_profile = profile;
    }

    npr_status_leds_save_state();
    return 0;
}

int npr_status_leds_change_brt(int direction) {
    int b = config.max_brightness + (direction * MAX_BRT_STEP);
    config.max_brightness = CLAMP(b, 0, BRT_MAX);

    npr_status_leds_save_state();
    return 0;
}

static struct zmk_led_hsb clone_led(struct zmk_led_hsb led) {
    return (struct zmk_led_hsb){
        h: led.h,
        s: led.s,
        b: led.b
    };
}


static void npr_status_leds_set_custom_led_profile(struct zmk_led_hsb custom_led_profile[STRIP_NUM_PIXELS]) {
    config.custom_led_0 = clone_led(custom_led_profile[0]);
    config.custom_led_1 = clone_led(custom_led_profile[1]);
    config.custom_led_2 = clone_led(custom_led_profile[2]);
}

int npr_status_leds_set_led(int led_index, int param, int value) {
    if (led_index >= 0 && led_index < STRIP_NUM_PIXELS) {
        struct zmk_led_hsb custom_led_profile[STRIP_NUM_PIXELS];
        if (config.active_led_profile == 0) {
            custom_led_profile[0] = clone_led(config.custom_led_0);
            custom_led_profile[1] = clone_led(config.custom_led_1);
            custom_led_profile[2] = clone_led(config.custom_led_2);
        }
        else {
            custom_led_profile[0] = clone_led(npr_lights_profiles[config.active_led_profile][0]);
            custom_led_profile[1] = clone_led(npr_lights_profiles[config.active_led_profile][1]);
            custom_led_profile[2] = clone_led(npr_lights_profiles[config.active_led_profile][2]);
        }
        config.active_led_profile = 0;
        
        switch(param) {
            case PARAM_HUE:
                custom_led_profile[led_index].h = CLAMP(custom_led_profile[led_index].h + value, 0, HUE_MAX);
                npr_status_leds_set_custom_led_profile(custom_led_profile);
                break;
            case PARAM_SAT:
                custom_led_profile[led_index].s = CLAMP(custom_led_profile[led_index].s + value, 0, SAT_MAX);
                npr_status_leds_set_custom_led_profile(custom_led_profile);
                break;
            case PARAM_BRT:
                custom_led_profile[led_index].b = CLAMP(custom_led_profile[led_index].b + value, 0, BRT_MAX);
                npr_status_leds_set_custom_led_profile(custom_led_profile);
                break;
        }
    }

    npr_status_leds_save_state();
    return 0;
}

static int npr_status_leds_layer_change(uint8_t layer) {
    if (state.current_effect != NPR_STATUS_STARTING) {
        state.active_layer = layer;
        state.current_effect = NPR_STATUS_LAYER_CHANGE;
        state.animation_step = 0;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static int npr_status_leds_endpoint_change(void) {
    if (state.current_effect != NPR_STATUS_STARTING && (zmk_usb_is_powered() || zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE)) {
        state.current_effect = NPR_STATUS_ENDPOINT_CHANGE;
        state.animation_step = 0;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static int npr_status_leds_event_listener(const zmk_event_t *eh) {

    if (as_zmk_activity_state_changed(eh) || as_zmk_usb_conn_state_changed(eh)) {
        return npr_status_leds_auto_state(zmk_usb_is_powered() || zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE);
    }

    if (as_zmk_layer_state_changed(eh) != NULL) {
        return npr_status_leds_layer_change(zmk_keymap_highest_layer_active());
    }

    if (as_zmk_endpoint_changed(eh) != NULL) {
        return npr_status_leds_endpoint_change();
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(npr_status_leds, npr_status_leds_event_listener);
ZMK_SUBSCRIPTION(npr_status_leds, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(npr_status_leds, zmk_usb_conn_state_changed);
ZMK_SUBSCRIPTION(npr_status_leds, zmk_activity_state_changed);
ZMK_SUBSCRIPTION(npr_status_leds, zmk_endpoint_changed);

SYS_INIT(npr_status_leds_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);