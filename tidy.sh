#!/bin/bash
run-clang-tidy 'src/(.)*.cc$' -checks='-*,modernize-*,-modernize-use-trailing-return-type,-modernize-concat-nested-namespaces,cert-*,bugprone-*,-bugprone-easily-swappable-parameters'
