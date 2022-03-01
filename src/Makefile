TARGETNAME := libmidi
BUILD_DIR_BASE := ./build
BUILD_DIR := $(BUILD_DIR_BASE)/$(TARGETNAME)
.DEFAULT_GOAL := $(BUILD_DIR)/$(TARGETNAME).a

include Sources.mk

C_COMPILER := clang
CPP_COMPILER := clang++
CLANG_FORMAT := clang-format

#when set to 1, format target will fail if there are any changes to the repository after formatting
CF_FAIL_ON_DIFF := 0

#passed to both c and c++ compiler
COMMON_FLAGS := \
-Wall

#c++ compiler only
CPP_FLAGS := \
-std=c++17

#c compiler only
C_FLAGS := \
-std=c11

#linker
LDFLAGS :=

$(BUILD_DIR)/$(TARGETNAME).a: $(OBJECTS)
	@ar rcs $@ $(OBJECTS)
	@echo Created static library: $@

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(@D)
	@echo Building $<
	@$(C_COMPILER) $(COMMON_FLAGS) $(C_FLAGS) $(addprefix -D,$(DEFINES)) $(INCLUDE_DIRS) $(INCLUDE_FILES) -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -c "$<" -o "$@"

$(BUILD_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	@echo Building $<
	@$(CPP_COMPILER) $(COMMON_FLAGS) $(CPP_FLAGS) $(addprefix -D,$(DEFINES)) $(INCLUDE_DIRS) $(INCLUDE_FILES) -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -c "$<" -o "$@"

format:
	@echo Checking code formatting...
	@find . -regex '.*\.\(cpp\|hpp\|h\|cc\|cxx\)' \
	-exec $(CLANG_FORMAT) -style=file -i {} \;
ifeq ($(CF_FAIL_ON_DIFF), 1)
	git diff -s --exit-code
endif

clean:
	@echo Cleaning up.
	@rm -rf $(BUILD_DIR_BASE)/

#debugging
print-%:
	@echo '$*=$($*)'