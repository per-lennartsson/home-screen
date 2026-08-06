cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(homescreen_display)

target_sources(app PRIVATE
  src/main.c
  src/ble_service.c
  src/chunk_protocol.c
  src/battery.c
  src/epaper.c
  src/epaper_ssd1683.c
)

target_include_directories(app PRIVATE src)
