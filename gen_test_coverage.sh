#!/bin/sh

TEST_EXEC=./builddir/eagle_test

meson compile -C builddir
$TEST_EXEC
llvm-profdata merge -sparse default.profraw -o default.profdata
llvm-cov report $TEST_EXEC -instr-profile=default.profdata
llvm-cov export $TEST_EXEC -instr-profile=default.profdata -format=lcov include src > default.info
genhtml -t "Eagle Test Coverage" default.info --output-directory test_coverage_result
