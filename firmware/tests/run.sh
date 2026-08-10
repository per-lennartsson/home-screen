#!/usr/bin/env bash
# Compiles chunk_protocol.c with the system C compiler against Zephyr header shims
# (shims/) and runs it against reference vectors generated from
# gateway/gateway/protocol.py. This is the one part of the firmware that can be checked
# without the nRF Connect SDK/hardware — see firmware/README.md for what isn't.
#
# To regenerate the reference vectors in test_chunk_protocol.c after a wire-format
# change, run the gateway's venv against a small script calling
# gateway.protocol.encode_chunks/encode_diff/wrap_payload_with_target_hash and update
# the *_HEX constants by hand.
set -euo pipefail
cd "$(dirname "$0")"

# rasterizer.c is NOT in these builds and cannot be: it renders through LVGL now, and
# LVGL is not available to the host compiler here. native_rasterizer_stub.c stands in for
# it so epaper.c still links. What that costs is real — the font-scaling checks that used
# to live in test_rasterizer.c are gone, and rendering is currently only verifiable by
# looking at the panel. Restoring host coverage means building LVGL for the host with its
# own lv_conf.h and asserting on the 1bpp output, which is worth doing but is not a
# five-line change.
cc -std=c11 -Wall -Wextra \
  -I ../src -I shims \
  ../src/chunk_protocol.c ../src/epaper.c ../src/layout_store.c \
  native_epd_stub.c native_rasterizer_stub.c test_chunk_protocol.c \
  -o /tmp/chunk_protocol_native_test

/tmp/chunk_protocol_native_test

cc -std=c11 -Wall -Wextra \
  -I ../src -I shims \
  ../src/chunk_protocol.c ../src/layout_store.c test_layout_store.c \
  -o /tmp/layout_store_native_test

/tmp/layout_store_native_test
