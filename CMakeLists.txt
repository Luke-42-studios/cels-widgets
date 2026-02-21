# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
