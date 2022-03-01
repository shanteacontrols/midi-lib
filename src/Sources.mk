SOURCES := $(shell find . -regex '.*\.\(cpp\|c\)')

#make sure all objects are located in build directory
OBJECTS := $(addprefix $(BUILD_DIR)/,$(SOURCES))
#also make sure objects have .o extension
OBJECTS := $(addsuffix .o,$(OBJECTS))

#include generated dependency files to allow incremental build when only headers change
-include $(OBJECTS:%.o=%.d)