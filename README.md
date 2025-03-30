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
