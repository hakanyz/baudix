# Baudix

Baudix is a modern, high-performance, and professionally designed Serial Port Terminal for embedded systems engineers and developers. Built with Qt/C++, Baudix offers an elegant dark theme, advanced hex/ascii payload parsing, live terminal snapshot capabilities, and an extensible architecture designed for industrial applications.

## Features

- **Modern Dark Interface:** A completely customized UI that prioritizes visual ergonomics, eliminating glaring spaces and cluttered controls.
- **Advanced Connection Handling:** Rapidly switch between COM ports, Baud Rates, Data Bits, Stop Bits, and Parity configurations.
- **Smart Input & History:** Merged Command Input and History controls into a single streamlined row to maximize terminal vertical space. 
- **Macro System:** Easily configure, store, and execute frequently used payloads (e.g., Bootloader commands, Version requests).
- **Embedded-Centric Logging:** Seamless Live Recording (Record/Pause) and instant Terminal Display Snapshots to easily dump UART streams.
- **Highlighting Engine:** Custom RX/TX highlighting to quickly visually distinguish incoming and outgoing data streams.
- **Modbus Ready:** Architected with a multi-tab system preparing for upcoming advanced Modbus communication tools.

## Build Instructions

Baudix uses the standard CMake build system via Qt6. 

### Requirements
- Qt 6.5+ (Widgets, SerialPort, Network modules)
- CMake 3.16+
- A C++17 compatible compiler (MinGW, MSVC, GCC, or Clang)

### Building
1. Clone the repository: `git clone https://github.com/hakanyz/baudix.git`
2. Open the project in Qt Creator or use the command line:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Contributing
Contributions are welcome. Please ensure that UI modifications strictly adhere to the established "Baudix Dark Theme" semantics (e.g., primary actions in blue, destructive/stop actions in red outlines).

## License
Copyright (c) 2026 - Baudix Development Team
