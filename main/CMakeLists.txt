idf_component_register(
    SRCS "src/i2c_oled_example_main.c" "src/screen_api.c" "src/screen_api.h"
    REQUIRES lmic-esp-idf
    INCLUDE_DIRS ".")