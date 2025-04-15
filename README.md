# Requirements
- [STM32Programmer zip](https://www.st.com/en/development-tools/stm32cubeprog.html), place the zip file in `scripts/`
- [nix](https://nixos.org/download.html)

# Setup
1. Modify `STM32_INSTALL_DIR` in `scripts/install_stm32.sh` to point to where you want to install STM32CubeProgrammer

```bash
STM32_INSTALL_DIR="${HOME}/STMicroelectronics/STM32Cube/STM32CubeProgrammer"
```

2. Modify `JLINK_INSTALL_DIR` in `scripts/power_profiler_config.sh` to point to where you want to install JLink
```bash
JLINK_INSTALL_DIR="${HOME}/JLink"
```

1. Find out the group of the device

```bash
$ ls -l /dev/ttyACM0
crw-rw---- 1 root uucp 166, 0 31.03.2025 19:42 /dev/ttyACM0
```

or 

```bash
$ stat /dev/ttyACM0
Access: (0660/crw-rw----)  Uid: (    0/    root)   Gid: (  986/    uucp)
```

2. Add your user to the group, in this case `uucp`

```bash
$ sudo usermod -a -G uucp $USER
```

3. Reboot. Log out and log back in may be enough, but reboot is the safest way.

2. Copy `scripts/99-stlinkv2_1.rules` to `/etc/udev/rules.d/`, check the group in the file and modify it if necessary

```bash
$ sudo cp scripts/99-stlinkv2_1.rules /etc/udev/rules.d/
```

3. Reload udev rules

```bash
$ sudo udevadm control --reload-rules
$ sudo udevadm trigger
```

# Quick Start
1. Start nix shell

```bash
nix develop .
```

2. Using west (zephyr) to build the project

```bash
$ (nix) west config build.board nrf52840dk/nrf52840
$ (nix) west build 
```

3. Update `west.yml` 

```bash
$ (nix) west update
```

# Update `flake.nix`

1. Update `flake.nix` to include new dependencies

2. Update `flake.lock`

```bash
$ nix flake update
```

3. Update `shell.nix`

```bash
$ nix develop .
```

# Using Power Profiler
1. Start the power profiler

```bash
$ sh scripts/run_power_profiler.sh
```

2. If first time, you need to click on `install **Power Profiler**.`

