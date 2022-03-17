TARGET         := libmidi
BUILD_DIR_BASE := ./build
BUILD_DIR      := $(BUILD_DIR_BASE)/$(TARGET)
SCRIPTS_DIR    := ../scripts
CLANG_TIDY_OUT := $(BUILD_DIR_BASE)/clang-tidy-fixes.yaml

.DEFAULT_GOAL  := $(BUILD_DIR)/$(TARGET).a

include Sources.mk

CC  := clang
CXX := clang++

# when set to 1, format target will fail if there are any changes to the repository after formatting
CF_FAIL_ON_DIFF := 0

# when set to 1, lint target will fail if there are any changes to the repository after linting
CL_FAIL_ON_DIFF := 0

#passed to both c and c++ compiler
CPPFLAGS := \
-Wall

#c++ compiler only
CXXFLAGS := \
-std=c++17

#c compiler only
CFLAGS := \
-std=c11

#linker
LDFLAGS :=

$(BUILD_DIR)/$(TARGET).a: $(OBJECTS)
	@ar rcs $@ $(OBJECTS)
	@echo Created static library: $@

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(@D)
	@echo Building $<
	@$(CC) $(CPPFLAGS) $(CCFLAGS) $(addprefix -D,$(DEFINES)) $(INCLUDE_DIRS) $(INCLUDE_FILES) -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -c "$<" -o "$@"

$(BUILD_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	@echo Building $<
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(addprefix -D,$(DEFINES)) $(INCLUDE_DIRS) $(INCLUDE_FILES) -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -c "$<" -o "$@"

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
	@rm -rf $(BUILD_DIR_BASE)/ compile_commands.json

#debugging
print-%:
	@echo '$*=$($*)'