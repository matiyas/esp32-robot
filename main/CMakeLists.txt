idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        robot_core
        motor_control
        servo_control
        http_server
        camera_stream
        wifi_manager
        safety_handler
        hal
        esp_wifi
        esp_http_server
        nvs_flash
        spiffs
        esp_psram
)

# Generate SPIFFS image from spiffs_data directory
spiffs_create_partition_image(spiffs ../spiffs_data FLASH_IN_PROJECT)
