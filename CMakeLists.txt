cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)

project(lcd_display)

add_definitions("-Wall -g")

include_directories(
)

link_directories(
    ${PROJECT_SOURCE_DIR}/lib
)

add_executable(lcd_display_png png2lcd.c) 
target_link_libraries(lcd_display_png pthread -lpng)

add_executable(lcd_display_bmp lcd_test.c) 
target_link_libraries(lcd_display_bmp pthread)

add_executable(lcd_display_gif gif/gifdec.cpp gif/main.cpp) 
target_link_libraries(lcd_display_gif pthread)

add_executable(lcd_display_2gif gif2/gifdec.cpp gif2/main.cpp) 
target_link_libraries(lcd_display_2gif pthread)
