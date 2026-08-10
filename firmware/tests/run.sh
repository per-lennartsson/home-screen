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

cc -std=c11 -Wall -Wextra \
  -I ../src -I shims \
  ../src/chunk_protocol.c ../src/epaper.c ../src/layout_store.c ../src/rasterizer.c \
  native_epd_stub.c test_chunk_protocol.c \
  -o /tmp/chunk_protocol_native_test

/tmp/chunk_protocol_native_test

cc -std=c11 -Wall -Wextra \
  -I ../src -I shims \
  ../src/chunk_protocol.c ../src/layout_store.c test_layout_store.c \
  -o /tmp/layout_store_native_test

/tmp/layout_store_native_test

cc -std=c11 -Wall -Wextra \
  -I ../src -I shims \
  ../src/layout_store.c ../src/rasterizer.c test_rasterizer.c \
  -o /tmp/rasterizer_native_test

/tmp/rasterizer_native_test
