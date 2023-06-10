ROOT_MAKEFILE_DIR := $(realpath $(dir $(realpath $(lastword $(MAKEFILE_LIST)))))
BUILD_DIR_BASE    := $(ROOT_MAKEFILE_DIR)/build
LIB_BUILD_DIR     := $(BUILD_DIR_BASE)

.DEFAULT_GOAL := all

cmake_config:
	@if [ ! -d $(LIB_BUILD_DIR) ]; then \
		echo "Generating CMake files"; \
		cmake \
		-B $(LIB_BUILD_DIR) \
		-S $(ROOT_MAKEFILE_DIR) \
		-DCMAKE_BUILD_TYPE=Debug \
		-DBUILD_TESTING_MIDI=ON \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DCMAKE_CTEST_ARGUMENTS="--verbose"; \
	fi

all: cmake_config
	@cmake --build $(LIB_BUILD_DIR)

lib: cmake_config
	@cmake --build $(LIB_BUILD_DIR) --target midi-lib

test: cmake_config
	@cmake --build $(LIB_BUILD_DIR) --target test

format: cmake_config
	@cmake --build $(LIB_BUILD_DIR) --target midi-format

lint: cmake_config
	@cmake --build $(LIB_BUILD_DIR) --target midi-lint

clean:
	@echo Cleaning up.
	@rm -rf $(BUILD_DIR_BASE)

print-%:
	@echo '$*=$($*)'

.PHONY: cmake_config all lib test format lint clean