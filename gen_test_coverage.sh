#!/bin/sh

BUILD_DIR=./builddir
TEST_EXEC=$BUILD_DIR/eagle_test

meson compile -C builddir
LLVM_PROFILE_FILE=$BUILD_DIR/default.profraw $TEST_EXEC
llvm-profdata merge -sparse $BUILD_DIR/default.profraw -o $BUILD_DIR/default.profdata
llvm-cov export $TEST_EXEC -instr-profile=$BUILD_DIR/default.profdata -format=lcov include src > $BUILD_DIR/default.info
genhtml -t "Eagle Test Coverage" $BUILD_DIR/default.info --output-directory coverage
