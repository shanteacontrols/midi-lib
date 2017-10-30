# MIDI library for AVR

Supports USB MIDI via LUFA and MIDI over serial connection. Based on Arduino MIDI library v4.2 by Francois Best.

## Symbols

To use this library, three global symbols must be defined.

### `USE_USB MIDI`

To compile this library, the following symbol must be defined:

- `USE_USB_MIDI`

### SysEx array size

To define size of SysEy array in bytes, use the following symbol (example for size of 45 bytes):

- `MIDI_SYSEX_ARRAY_SIZE=45`

### Serial object

This library uses serial bject to communicate with UART interface. Object must support the following functions:

- `read`: Reads single byte from UART receiver.
- `write`: Writes single byte to UART transmitter.
- `available`: Checks if any data is available on UART receiver.

Serial object must be called `uart`.

## Licencing

This project is based on Arduino MIDI library and LUFA framework which are licenced under MIT. Whole project is also licenced under MIT.