# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
    SRCS
        "main.c"
        "dht22.c"
        "oled.c"
        "alarm.c"
        "rtos_objects.cpp"
        "system_state.cpp"
        "sensors.cpp"
        "motion.cpp"
        "input.cpp"
        "display.cpp"
        "alarm.cpp"
    INCLUDE_DIRS
        "."
        "../include"
)