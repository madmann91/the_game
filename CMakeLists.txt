cmake_minimum_required(VERSION 3.17)
project(the_game)

set(CMAKE_C_STANDARD 99)

add_library(pcg contrib/pcg-c-basic/pcg_basic.c)
target_include_directories(pcg PUBLIC contrib/pcg-c-basic)

add_library(the_game_lib src/game.c)
target_link_libraries(the_game_lib PUBLIC pcg)
set_property(TARGET the_game_lib PROPERTY PREFIX "")

add_executable(the_game src/main.c)
target_link_libraries(the_game PUBLIC the_game_lib)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED)
if (IPO_SUPPORTED)
    set_property(TARGET the_game     PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET the_game_lib PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif ()
