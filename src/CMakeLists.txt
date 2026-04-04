zephyr_library()

zephyr_include_directories(../include)

zephyr_library_sources(npr_status_leds.c)
zephyr_library_sources(behaviors/behavior_npr_status_leds.c)
