set(src_files
<<<<<<< HEAD
    app_main_with_light_control_1
=======
    app_main_with_light_control.cpp
>>>>>>> d24d51a1576b81c226506d2801fe2b6d5863e881
    light_controller.cpp
    frame_cap_pipeline.cpp
)

set(include_dirs ./)

set(requires
    log
    esp_timer
    fatfs
    sdmmc
    vfs
    esp-dl
    esp_lcd
    driver
    who_frame_cap
    esp_driver_rmt
)

set(detect_component ${DETECT_MODEL} espressif__${DETECT_MODEL})

idf_component_register(
    SRCS         ${src_files}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES     ${requires}
)

idf_component_optional_requires(PUBLIC ${detect_component})
