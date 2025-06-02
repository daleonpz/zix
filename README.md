# Zephyr Development Environment

This project is designed to be built and run using Nix, providing a reproducible development environment for Zephyr RTOS. It includes scripts for building the application and MCUboot bootloader.

## Table of Contents

- [About the Project](#about-the-project)
- [Project Status](#project-status)
- [Getting Started](#getting-started)
    - [Requirements](#requirements)
    - [Getting the Source](#getting-the-source)
    - [Building](#building)
    - [Testing](#testing)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Authors](#authors)
- [License](#license)
- [Acknowledgments](#acknowledgments)
-[Extras](#extras)
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

**Hardware Setup**:

1. Set `STM32_INSTALL_DIR` in `scripts/install_stm32.sh`:

   ```bash
   STM32_INSTALL_DIR="${HOME}/STMicroelectronics/STM32Cube/STM32CubeProgrammer"
   ```

2. Set `JLINK_INSTALL_DIR` in `scripts/nrf_tools_config.sh`:

   ```bash
   JLINK_INSTALL_DIR="${HOME}/JLink"
   ```

3. Add your user to the device's group (e.g., `uucp`):

   ```bash
   ls -l /dev/ttyACM0  # Check group (e.g., uucp)
   sudo usermod -a -G uucp $USER
   sudo reboot
   ```

4. Copy and configure udev rules:

   ```bash
   sudo cp scripts/99-stlinkv2_1.rules /etc/udev/rules.d/
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   ```

5. Create symlinks for STM32CubeProgrammer and JLink:

   ```bash
   sudo mkdir -p /opt/SEGGER/JLink
   sudo ln -s /path/to/JLink/ /opt/SEGGER/JLink/
   ``` 

**[Back to top](#table-of-contents)**

### Getting the Source

Clone the repository with `git-lfs` installed:

```bash
git clone --recursive git@github.com:daleonpz/zephyr_nix_devenv.git
```

If you cloned without `--recursive`, run:

```bash
git submodule update --init
git lfs pull
```

**[Back to top](#table-of-contents)**

### Building

1. Start the Nix shell:

   ```bash
   nix develop .
   ```

2. Clone your application:
   
   ```bash
   west init -m https://github.com/your-name/your-application your-app-worskpace
   cd your-app-workspace
   west update
   ```

3. Build the application, bootloader (MCUBoot), or both, using the provided Makefile. The Makefile is located in the root directory of the project **NOT in the application directory**.

   ```bash
   make app              # Build otau_example application
   make bootloader       # Build mcuboot bootloader
   make all              # Build both together (default)
   make app-codechecker  # Build application with codechecker
   ```

4. Flash to the board:

   ```bash
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


5. Clean build directories:

   ```bash
   make clean
   ```

6. See all available targets:

   ```bash
   make help
   ```

**Updating Dependencies**:

- Update `west.yml`: `west update`
- Update `flake.nix` and `flake.lock`: `nix flake update`
- Reload Nix shell: `nix develop .`

**[Back to top](#table-of-contents)**

### Testing

Run tests using Zephyr's Twister framework:

```bash
make test
```

Test results are saved in JUnit XML format in the respective build directories.

**[Back to top](#table-of-contents)**

## Documentation

Documentation is not currently built locally (Makefile does not include a `docs` target). Check the project GitHub page for any available documentation or contribute to add a `docs` target.

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

## Acknowledgments

- Uses Zephyr RTOS for real-time operation and mcuboot for OTA updates.

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
   # Select: scripts/sniffer_fw/hex/sniffer_nrf51dongle_nrf51422_4.1.1.hex
   # Click "write" to flash
   ```

4. Verify the device appears as `nRF Sniffer for Bluetooth`.

**[Back to top](#table-of-contents)**
