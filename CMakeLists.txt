# cels-widgets - Widget component library with Clay layout integration
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/behavioral.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/layouts.c
)

target_include_directories(cels-widgets INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cels-widgets INTERFACE
    cels
)

# Link cels-clay when available (provides Clay layout macros)
if(TARGET cels-clay)
    target_link_libraries(cels-widgets INTERFACE
        cels-clay
    )
endif()

# Link cels-layout when available (provides CEL_* layout types)
if(TARGET cels-layout)
    target_link_libraries(cels-widgets INTERFACE cels-layout)
endif()
