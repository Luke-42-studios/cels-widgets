# cels-widgets - Backend-agnostic widget component library for CELS
# INTERFACE library: sources compile in the consumer's context
cmake_minimum_required(VERSION 3.21)

# ============================================================================
# INTERFACE library target
# ============================================================================

add_library(cels-widgets INTERFACE)
add_library(cels::widgets ALIAS cels-widgets)

target_sources(cels-widgets INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/focus.c
)

target_include_directories(cels-widgets INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cels-widgets INTERFACE
    cels
)
