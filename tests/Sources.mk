vpath modules/%.cpp ../
vpath modules/%.c ../

SOURCES_COMMON :=

INCLUDE_DIRS_COMMON := \
-I"./" \
-I"../src" \
-I"./src"