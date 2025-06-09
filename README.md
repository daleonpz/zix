# Nix-Based Zephyr Development Environment (Zix)

This project is designed to be built and run using Nix, providing a reproducible development environment for Zephyr RTOS. It includes scripts for building the application and MCUboot bootloader.

## Table of Contents

- [About the Project](#about-the-project)
- [Project Status](#project-status)
- [Getting Started](#getting-started)
    - [Requirements](#requirements)
    - [Working with Zix](#working-with-zix)
    - [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [Authors](#authors)
- [License](#license)
- [Extras](#extras)
    - [BLE Sniffer](#ble-sniffer)

## About the Project

This project is ideal for developers working with Zephyr RTOS, especially those focusing on embedded systems and IoT applications. It simplifies the setup process and ensures that all dependencies are managed consistently across different development environments. It includes:

- A Nix-based development environment for reproducibility.
- Scripts for building Zephyr applications and MCUboot.
- BLE sniffer support for monitoring Bluetooth Low Energy communications.
- STM32CubeProgrammer integration for flashing STM32 devices.
- JLink support for debugging and programming.

**[Back to top](#table-of-contents)**

## Project Status

The project is functional. It has been tested on the `nucleo_wb55rg` board and is ready for use in developing Zephyr applications with MCUboot support. The project is actively maintained, and contributions are welcome.

Limitations:
- Twister functionality is not fully implemented.

**[Back to top](#table-of-contents)**

## Getting Started

### Requirements

To build and run this project, you need:

- STM32CubeProgrammer: Download and place the zip file in `scripts/`.

- Nix: A package manager for reproducible builds.

  ```bash
  sh <(curl --proto '=https' --tlsv1.2 -L https://nixos.org/nix/install) --daemon
  ```

- git-lfs: For handling binary files.

  - On Linux: `sudo apt install git-lfs`
  - On macOS: `brew install git-lfs`

- pre-commit: For code quality checks.

  ```bash
  pip install pre-commit
  pre-commit install
  ```

- Zephyr SDK and west (included via Nix environment).

### Working with Zix

1. Fork the repository on GitHub. `app` directory is where you will develop your application.

2. Start Nix shell:

   ```bash
   nix develop .
   ```

3. A `Makefile` is provided in the root directory to build and flash your application and bootloader (MCUBoot).

   ```bash
   make app              # Build otau_example application
   make bootloader       # Build mcuboot bootloader
   make all              # Build both together (default)
   make app-codechecker  # Build application with codechecker
   make flash-app        # Flash application
   make flash-bootloader # Flash bootloader
   make flash            # Flash all (bootloader + application)
   ```
There are several options you can pass to the `make` command:

   ```bash
   make all VERBOSE=1 BOARD=your_board_name APP_DIR=your_app_dir
   make flash APP_DIR=your_app_dir
   ```

   - `VERBOSE`: Show verbose output (0 or 1, default 0).
   - `BOARD`: Target board (default: `nucleo_wb55rg`).
   - `APP_DIR`: Application directory (default: `app`).

4. If you modify `flake.nix` you need to update the Nix flake and restart the Nix shell:

   ```bash
   nix flake update
   git add flake.lock
   git commit -m "Update flake.lock"
   git push

   nix develop .
   ```

**[Back to top](#table-of-contents)**

### Troubleshooting

* If you encounter issues with the Nix environment, try running:

   ```bash
   nix-collect-garbage
   ```

* If you cannot access the serial port from Nix, add your user to the device's group (e.g., `uucp`):

   ```bash
   ls -l /dev/ttyACM0  # Check group (e.g., uucp)
   sudo usermod -a -G uucp $USER
   sudo reboot
   ```

* If you are using ST-Link v2.1, you need to copy the udev rules file to your system:

   ```bash
   sudo cp scripts/99-stlinkv2_1.rules /etc/udev/rules.d/
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   ```

* `nrfutil` searchs for JLinkArm library in /opt/SEGGER/JLink, so you need to create a symlink to the JLink directory:

   ```bash
   sudo mkdir -p /opt/SEGGER/JLink
   sudo ln -s /full-path/to/JLink/* /opt/SEGGER/JLink/
   ```

**[Back to top](#table-of-contents)**

## Contributing

Contributions are welcome! Please create a pull request on GitHub. Use `pre-commit` to ensure code quality before submitting.

**[Back to top](#table-of-contents)**

## Authors

-  Daniel Paredes (daleonpz)

**[Back to top](#table-of-contents)**

## License

Licensed under the Apache License 2.0. See the LICENSE file for details.

Copyright (c) 2025 Daniel Paredes (daleonpz)

**[Back to top](#table-of-contents)**

## Extras
### BLE Sniffer
This project uses BLE as a communication protocol for the wearable device. The [nRF52840 Dongle](https://www.nordicsemi.com/Products/Development-hardware/nRF52840-Dongle) can be configured as a BLE sniffer to capture and analyze Bluetooth Low Energy packets.

1. Start the power profiler:

   ```bash
   sh scripts/run_power_profiler.sh
   ```

2. Open the Programmer app, select the device (`Open DFU bootloader`).

3. Flash the sniffer firmware:

   ```bash
   # Select: nrftools/nrfutil_tools/hex/sniffer_nrf51dongle_nrf51422_4.1.1.hex
   # Click "write" to flash
   ```

4. Verify the device appears as `nRF Sniffer for Bluetooth`.

**[Back to top](#table-of-contents)**
