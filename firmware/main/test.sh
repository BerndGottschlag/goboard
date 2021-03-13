#!/bin/bash
west build -b native_posix_64 --build-dir build_testing -- -DCONFIG_COVERAGE=y
echo ""
echo "Press Ctrl+C once the application has finished."
echo ""
build_testing/zephyr/zephyr.elf
