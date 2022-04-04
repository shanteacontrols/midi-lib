TARGET         := midi
BUILD_DIR_BASE := ./build
BUILD_DIR      := $(BUILD_DIR_BASE)/$(TARGET)
SCRIPTS_DIR    := ../scripts
CLANG_TIDY_OUT := $(BUILD_DIR_BASE)/clang-tidy-fixes.yaml
TEST_BINARIES  := $(addprefix -object ,$(shell $(FIND) $(BUILD_DIR_BASE) -name "*.out" 2>/dev/null))

.DEFAULT_GOAL := test

include Defines.mk
include Sources.mk
-include Objects.mk

CC  := clang
CXX := clang++

# when set to 1, format target will fail if there are any changes to the repository after formatting
CF_FAIL_ON_DIFF := 0

# when set to 1, lint target will fail if there are any changes to the repository after linting
CL_FAIL_ON_DIFF := 0

#passed to both c and c++ compiler
CPPFLAGS := \
-fprofile-instr-generate \
-fcoverage-mapping \
-O0 \
-g \
-Wall \
-fdata-sections \
-ffunction-sections

#c++ compiler only
CXXFLAGS := \
-std=c++17

#c compiler only
CFLAGS := \
-std=c11

#linker
LDFLAGS := \
-lglog \
-pthread \
-lgtest

$(BUILD_DIR)/%.c.o $(BUILD_DIR_BASE)/%.c.o: %.c
	@mkdir -p $(@D)
	@echo Building: $<
	@ccache $(CC) $(CPPFLAGS) $(CFLAGS) $(addprefix -D,$(DEFINES)) $(INCLUDE_DIRS_COMMON) $(INCLUDE_FILES_COMMON) -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -c "$<" -o "$@"

$(BUILD_DIR)/%.cpp.o $(BUILD_DIR_BASE)/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	@echo Building: $<
	@ccache $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(addprefix -D,$(DEFINES)) $(INCLUDE_DIRS_COMMON) $(INCLUDE_FILES_COMMON) -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -c "$<" -o "$@"

define LINK_OBJECTS
	@echo Creating executable: $@
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
endef

pre-build:
	@$(SCRIPTS_DIR)/gen_test_targets.sh

exec:
	@echo Running all compiled tests.
	@$(SCRIPTS_DIR)/tests_exec.sh $(BUILD_DIR_BASE)

coverage:
	@echo Creating coverage report.
	@llvm-profdata merge $(wildcard $(BUILD_DIR_BASE)/*.profraw) -o $(BUILD_DIR_BASE)/tests.profdata
	@llvm-cov report $(TEST_BINARIES) -instr-profile=$(BUILD_DIR_BASE)/tests.profdata -ignore-filename-regex="test|modules" -use-color
	@llvm-cov show $(TEST_BINARIES) -instr-profile=$(BUILD_DIR_BASE)/tests.profdata -ignore-filename-regex="test|modules" -use-color --format html > $(BUILD_DIR_BASE)/coverage.html
	@echo ***Generated HTML coverage report in $(BUILD_DIR_BASE)/coverage.html***

format:
	@echo Checking code formatting...
	@find . -regex '.*\.\(cpp\|hpp\|h\|cc\|cxx\)' \
	-exec clang-format -style=file -i {} \;
ifeq ($(CF_FAIL_ON_DIFF), 1)
	git diff -s --exit-code
endif

lint:
	@$(SCRIPTS_DIR)/tidy.sh $(CLANG_TIDY_OUT)
ifeq ($(CL_FAIL_ON_DIFF), 1)
	@if [ -s $(CLANG_TIDY_OUT) ]; then \
		echo Lint issues found:; \
		cat $(CLANG_TIDY_OUT); \
		false; \
	fi
endif

clean:
	@echo Cleaning up.
	@rm -rf $(BUILD_DIR_BASE)/

#debugging
print-%:
	@echo '$($*)'
