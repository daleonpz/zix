# Copyright 2025 Daniel Paredes (daleonpz)
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Verbosity control
VERBOSE ?= 0
ifeq ($(VERBOSE),1)
export Q :=
else
export Q := @
endif

# Board definition 
DEFAULT_BOARD ?= nucleo_wb55rg
BOARD ?= $(DEFAULT_BOARD)

# Application Directory
DEFAULT_APP_DIR ?= app
APP_DIR ?= $(DEFAULT_APP_DIR)
APP_DOMAIN ?= $(APP_DIR)

# West build directories
APP_BUILD_DIR ?= build

.PHONY: app
app:
	$(Q)west build -b $(BOARD) -s $(APP_DIR) --domain $(APP_DOMAIN) --sysbuild -d $(APP_BUILD_DIR)

.PHONY: app-codechecker
app-codechecker:
	$(Q)west build -b $(BOARD) -s $(APP_DIR) --domain $(APP_DOMAIN) --pristine --sysbuild -d $(APP_BUILD_DIR) -- -DZEPHYR_SCA_VARIANT=codechecker

.PHONY: bootloader
bootloader:
	$(Q)west build -b $(BOARD) -s $(APP_DIR) --domain mcuboot --sysbuild -d $(APP_BUILD_DIR)

.PHONY: all 
combined:
	$(Q)west build -b $(BOARD) -s $(APP_DIR) --pristine --sysbuild -d $(APP_BUILD_DIR)

.PHONY: test
test:
	$(Q)west twister -T $(APP_DIR) --platform $(BOARD)

.PHONY: flash-app
flash-app: app
	$(Q)west flash -d $(APP_BUILD_DIR)

.PHONY: flash-bootloader
flash-bootloader: bootloader
	$(Q)west flash -d $(APP_BUILD_DIR) --domain mcuboot

.PHONY: flash
flash: all
	$(Q)west flash -d $(APP_BUILD_DIR)

.PHONY: clean
clean:
	$(Q)rm -rf $(APP_BUILD_DIR)

.PHONY: flake-update
flake-update:
	$(Q)nix flake update

.PHONY: help
help:
	@echo "usage: make [OPTIONS] <target>"
	@echo "Options:"
	@echo "  VERBOSE: Show verbose output (0 or 1, default 0)"
	@echo "  BOARD: Target board (default: $(DEFAULT_BOARD))"
	@echo "  APP_DIR: Application directory (default: ${APP_DIR})"
	@echo "Targets:"
	@echo "  all: Build both application and bootloader"
	@echo "  app: Build application"
	@echo "  app-codechecker: Build application with codechecker"
	@echo "  bootloader: Build mcuboot bootloader"
	@echo "  test: Run twister tests for application"
	@echo "  flash: Flash both application and bootloader"
	@echo "  flash-app: Flash the application"
	@echo "  flash-bootloader: Flash the bootloader"
	@echo "  clean: Remove all build directories"
	@echo "  flake-update: Update Nix flake"
	@echo "  help: Show this help message"
