#define DT_DRV_COMPAT npr_behavior_status_leds

// Dependencies
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk_keyboard_npr/status_leds.h>
#include <dt-bindings/zmk_keyboard_npr/status_leds.h>

LOG_MODULE_DECLARE(npr, CONFIG_ZMK_LOG_LEVEL);


#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

// Set up the values for commands that take no additional parameter.
static const struct behavior_parameter_value_metadata no_arg_values[] = {
    {
        .display_name = "Brightness Up",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = BRT_INC_CMD,
    },
    {
        .display_name = "Brightness Down",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = BRT_DEC_CMD,
    },
    {
        .display_name = "Cycle color presets",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = PRF_CYCLE_CMD,
    },
};

// Set up the "no arg" metadata set.
static const struct behavior_parameter_metadata_set no_args_set = {
    .param1_values = no_arg_values,
    .param1_values_len = ARRAY_SIZE(no_arg_values),
};

// Set up the possible param1 values for commands that take a profile index for param2
static const struct behavior_parameter_value_metadata prof_index_param1_values[] = {
    {
        .display_name = "Choose color preset",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = PRF_SET,
    },
};

// Set up the param2 value metadata for the valid range of possible profiles to pick from.
static const struct behavior_parameter_value_metadata prof_index_param2_values[] = {
    {
        .display_name = "1",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = 1,
    },
    {
        .display_name = "2",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = 2,
    },
    {
        .display_name = "3",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = 3,
    },
    {
        .display_name = "4",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = 4,
    },
    {
        .display_name = "Custom",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = 0,
    },
};

// Set up the metadata set for the commands that take a profile for the second parameter.
static const struct behavior_parameter_metadata_set profile_index_metadata_set = {
    .param1_values = prof_index_param1_values,
    .param1_values_len = ARRAY_SIZE(prof_index_param1_values),
    .param2_values = prof_index_param2_values,
    .param2_values_len = ARRAY_SIZE(prof_index_param2_values),
};

static const struct behavior_parameter_value_metadata led_param_p2_value_metadata_values[] = {
    {
        .display_name = "+10",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = 10,
    },
    {
        .display_name = "+5",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = 5,
    },
    {
        .display_name = "+1",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = 1,
    },
    {
        .display_name = "-1",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = -1,
    },
    {
        .display_name = "-5",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = -5,
    },
    {
        .display_name = "-10",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = -10,
    },
};


static const struct behavior_parameter_value_metadata hue_p1_value_metadata_values[] = {
    {
        .display_name = "Modify left LED hue",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = HSV_HUE_0,
    },
    {
        .display_name = "Modify middle LED hue",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = HSV_HUE_1,
    },
    {
        .display_name = "Modify right LED hue",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = HSV_HUE_2,
    },
};

static const struct behavior_parameter_metadata_set hue_value_metadata_set = {
    .param1_values = hue_p1_value_metadata_values,
    .param1_values_len = ARRAY_SIZE(hue_p1_value_metadata_values),
    .param2_values = led_param_p2_value_metadata_values,
    .param2_values_len = ARRAY_SIZE(led_param_p2_value_metadata_values),
};


static const struct behavior_parameter_value_metadata sat_p1_value_metadata_values[] = {
    {
        .display_name = "Modify left LED saturation",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = HSV_SAT_0,
    },
    {
        .display_name = "Modify middle LED saturation",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = HSV_SAT_1,
    },
    {
        .display_name = "Modify right LED saturation",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = HSV_SAT_2,
    },
};

static const struct behavior_parameter_metadata_set sat_value_metadata_set = {
    .param1_values = sat_p1_value_metadata_values,
    .param1_values_len = ARRAY_SIZE(sat_p1_value_metadata_values),
    .param2_values = led_param_p2_value_metadata_values,
    .param2_values_len = ARRAY_SIZE(led_param_p2_value_metadata_values),
};


static const struct behavior_parameter_value_metadata val_p1_value_metadata_values[] = {
    {
        .display_name = "Modify left LED value/brightness",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = HSV_VAL_0,
    },
    {
        .display_name = "Modify middle LED value/brightness",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = HSV_VAL_1,
    },
    {
        .display_name = "Modify right LED value/brightness",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = HSV_VAL_2,
    },
};

static const struct behavior_parameter_metadata_set val_value_metadata_set = {
    .param1_values = val_p1_value_metadata_values,
    .param1_values_len = ARRAY_SIZE(val_p1_value_metadata_values),
    .param2_values = led_param_p2_value_metadata_values,
    .param2_values_len = ARRAY_SIZE(led_param_p2_value_metadata_values),
};

// Finally, expose all the sets in the top level aggregate structure.
static const struct behavior_parameter_metadata_set metadata_sets[] = {no_args_set,
                                                                       profile_index_metadata_set,
                                                                       hue_value_metadata_set,
                                                                       sat_value_metadata_set,
                                                                       val_value_metadata_set};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(metadata_sets),
    .sets = metadata_sets,
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)


static int on_npr_status_leds_binding_pressed(struct zmk_behavior_binding *binding,
                                                 struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case PRF_CYCLE_CMD:
        return npr_status_leds_cycle_profile();
    case PRF_SET_CMD:
        return npr_status_leds_set_profile(binding->param2);
    case BRT_INC_CMD:
        return npr_status_leds_change_brt(1);
    case BRT_DEC_CMD:
        return npr_status_leds_change_brt(-1);


    case HSV_SET_HUE_0_CMD:
        return npr_status_leds_set_led(0, PARAM_HUE, binding->param2);
    case HSV_SET_HUE_1_CMD:
        return npr_status_leds_set_led(1, PARAM_HUE, binding->param2);
    case HSV_SET_HUE_2_CMD:
        return npr_status_leds_set_led(2, PARAM_HUE, binding->param2);

    case HSV_SET_SAT_0_CMD:
        return npr_status_leds_set_led(0, PARAM_SAT, binding->param2);
    case HSV_SET_SAT_1_CMD:
        return npr_status_leds_set_led(1, PARAM_SAT, binding->param2);
    case HSV_SET_SAT_2_CMD:
        return npr_status_leds_set_led(2, PARAM_SAT, binding->param2);

    case HSV_SET_VAL_0_CMD:
        return npr_status_leds_set_led(0, PARAM_BRT, binding->param2);
    case HSV_SET_VAL_1_CMD:
        return npr_status_leds_set_led(1, PARAM_BRT, binding->param2);
    case HSV_SET_VAL_2_CMD:
        return npr_status_leds_set_led(2, PARAM_BRT, binding->param2);

    case OFF_CMD:
        return npr_status_leds_off();
    }

    return -ENOTSUP;
}

// API struct
static const struct behavior_driver_api npr_status_leds_driver_api = {
    .binding_pressed = on_npr_status_leds_binding_pressed,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif
};

BEHAVIOR_DT_INST_DEFINE(0,                                                // Instance Number (0)
                        NULL,                          // Initialization Function
                        NULL,                                             // Power Management Device Pointer
                        NULL,                         // Behavior Data Pointer
                        NULL,                       // Behavior Configuration Pointer
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,  // Initialization Level, Device Priority
                        &npr_status_leds_driver_api);                  // API struct

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
