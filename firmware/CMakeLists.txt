cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(homescreen_display)

# The LVGL fonts the panel renders with, generated from one TTF by tools/fonts/generate.mjs
# together with the frontend's glyph-metrics table. Globbed rather than listed: the set is
# defined by that generator's SIZES/WEIGHTS, and a stale hand-maintained list here would
# silently drop a font the wire format can still reference. See src/fonts/hs_fonts.h.
file(GLOB HS_FONT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/fonts/lv_font_hs_*.c)

target_sources(app PRIVATE
  ${HS_FONT_SOURCES}
  src/fonts/hs_fonts.c
  src/main.c
  src/ble_service.c
  src/chunk_protocol.c
  src/battery.c
  src/button.c
  src/epaper.c
  src/epaper_ssd1683.c
  src/layout_store.c
  src/rasterizer.c
)

target_include_directories(app PRIVATE src src/fonts)
