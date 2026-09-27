idf_component_register(SRCS "main.cpp"
                            "audio/Audio.cpp"
                            "leds/Leds.cpp"
                            "display/St7735.cpp"
                            "display/Display.cpp"
                            "btp/btp_commands.cpp"
                            "btp/btp_topics.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES BTPDevice
                       PRIV_REQUIRES esp_adc esp_lcd esp_driver_spi esp_driver_gpio
                                     esp_driver_ledc esp_timer esp_wifi)

# LVGL 8's headers mix enums in bit operations, which C++20 deprecates; the
# code is fine, the warning is not ours to fix.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-deprecated-enum-enum-conversion)
