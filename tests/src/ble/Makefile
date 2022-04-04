vpath src/%.cpp ../

SOURCES_$(shell basename $(dir $(lastword $(MAKEFILE_LIST)))) := \
src/MIDI.cpp \
src/transport/ble/BLE.cpp

DEFINES_$(shell basename $(dir $(lastword $(MAKEFILE_LIST)))) := \
MIDI_BLE_MAX_PACKET_SIZE=16