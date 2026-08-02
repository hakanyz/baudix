<div align="center">
  <img src="resources/baudix_icon.svg" width="120" height="120" alt="Baudix Logo">
  <h1>Baudix</h1>
  <p><b>Professional Serial Terminal & Modbus Utility for Embedded Engineers</b></p>
  
  [![Build Status](https://github.com/hakanyz/baudix/actions/workflows/release.yml/badge.svg)](https://github.com/hakanyz/baudix/actions)
  [![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
  [![Framework](https://img.shields.io/badge/Qt-6.5+-41CD52.svg)](https://www.qt.io/)
  [![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)]()
  [![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
</div>

<br>

## 📌 Overview

Baudix is a high-performance, modern Serial Port Terminal built with **C++17** and **Qt 6**. It was engineered from the ground up to address the lack of modern, ergonomically designed serial monitors in the embedded systems industry. 

Whether you are debugging complex microcontrollers, analyzing raw UART streams, or testing Modbus communications, Baudix provides an ultra-responsive, distraction-free environment packed with professional developer tools.

## ✨ Key Features

- **⚡ High-Performance IO:** Asynchronous serial port handling designed to process high-throughput data streams without GUI freezing.
- **🎨 Modern Dark Ergonomics:** A custom-built UI/UX engine utilizing advanced Qt Stylesheets (QSS) for a premium, eye-friendly dark mode experience.
- **🔄 In-App OTA Updater:** A fully automated update system that queries the GitHub Releases API. It seamlessly downloads and executes silent installations (via Inno Setup) in the background.
- **🛠 Macro System & Payloads:** Easily store, manage, and dispatch frequently used payloads (e.g., Bootloader triggers, firmware version requests) with a single click.
- **📝 Advanced Logging:** Real-time stream recording, session pausing, and instant terminal snapshots for post-analysis debugging.
- **🔍 Hex/ASCII Parsing & Highlighting:** Real-time data conversion and custom RX/TX color highlighting to distinguish incoming vs. outgoing streams visually.
- **🚀 System Tray Integration:** Native OS integration allowing the application to run silently in the background (System Tray) without cluttering the taskbar.

## 📸 Screenshots

*(Add screenshots of your application here to showcase the UI)*
<!-- ![Main Interface](docs/screenshot1.png) -->
<!-- ![Update System](docs/screenshot2.png) -->

## 💻 Tech Stack & Architecture

- **Core Language:** C++17
- **Framework:** Qt 6 (Core, Gui, Widgets, Network, SerialPort)
- **Build System:** CMake
- **CI/CD:** GitHub Actions (Automated multi-platform builds and deployment)
- **Installer:** Inno Setup (Windows)

The architecture heavily relies on Qt's signal-slot mechanism to ensure that the main UI thread remains responsive while the `SerialPortController` handles I/O operations asynchronously. The `Updater` class utilizes `QNetworkAccessManager` for secure HTTPS API queries and binary payload processing.

## 🚀 Build Instructions

### Prerequisites
- **Qt 6.5+** (Ensure `SerialPort` and `Network` modules are installed)
- **CMake 3.16+**
- A C++17 compatible compiler (MSVC 2019+, GCC, or Clang)

### Compilation Steps

1. **Clone the repository:**
   ```bash
   git clone https://github.com/hakanyz/baudix.git
   cd baudix
   ```
2. **Generate build files & compile:**
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . --config Release
   ```
3. **Run:** The executable will be generated in the `build/` (or `build/Release/`) directory.

## 🤝 Contributing

This project is open-source and contributions are welcome. When submitting Pull Requests, please ensure that UI modifications strictly adhere to the established "Baudix Dark Theme" semantics.

## 📄 License

This project is licensed under the MIT License - Copyright (c) 2026 **Hakan** (hakanyz). See the LICENSE file for details.
