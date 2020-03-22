#!/bin/bash
clang-tidy -format-style=google -checks="*,-llvm-header-guard,-fuchsia-overloaded-operator,-modernize-use-trailing-return-type,-modernize-use-nodiscard,-fuchsia-default-arguments-calls,-fuchsia-default-arguments-declarations,-google-runtime-references,-readability-avoid-const-params-in-decls" -header-filter=".*" -p ./build/ ./src/chord.cc
