add_library(log_vt100 STATIC
    log_vt100.c
)

target_include_directories(log_vt100 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(log_vt100
    pico_stdlib
)
