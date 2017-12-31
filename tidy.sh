#!/bin/bash
clang-tidy -format-style=google -checks="*" -header-filter=".*" -p ./build/ ./src/chord.cc
