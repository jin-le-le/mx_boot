# ============================================================
# MCU_BOOT 顶层 Makefile
# ------------------------------------------------------------
# 一键编译三款芯片的 bootloader 和签名工具 smoke test。
# 详细工程配置在各自 projects/<chip>/ 下，本文件只是薄包装。
# ============================================================

# 默认目标：编译三款芯片
.DEFAULT_GOAL := all

# ---- 路径 ----
ROOT_DIR     := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PROJECTS_DIR := $(ROOT_DIR)/projects
SIGN_TOOL    := $(ROOT_DIR)/tool/sign_tool

# ---- 工具链（可被环境变量覆盖） ----
CMAKE        ?= cmake
NINJA        ?= ninja
PYTHON       ?= python3

# ---- 颜色 ----
ifneq (,$(findstring xterm,$(TERM)))
	C_RESET  := \033[0m
	C_GREEN  := \033[32m
	C_YELLOW := \033[33m
	C_CYAN   := \033[36m
else
	C_RESET  :=
	C_GREEN  :=
	C_YELLOW :=
	C_CYAN   :=
endif

# ============================================================
# 顶层目标
# ============================================================

.PHONY: all f407 f103 hc32 sign-smoke clean help

all: f407 f103 hc32

## 编译 STM32F407 bootloader
f407:
	@printf "$(C_CYAN)=== [1/3] Build STM32F407 bootloader ===$(C_RESET)\n"
	cd $(PROJECTS_DIR)/stm32f407 && $(CMAKE) -B build -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
	cd $(PROJECTS_DIR)/stm32f407 && $(CMAKE) --build build
	@printf "$(C_GREEN)✓ F407 done. Artifacts: projects/stm32f407/build/MCU_BOOT.{elf,bin,hex}$(C_RESET)\n"

## 编译 STM32F103 bootloader
f103:
	@printf "$(C_CYAN)=== [2/3] Build STM32F103 bootloader ===$(C_RESET)\n"
	cd $(PROJECTS_DIR)/stm32f103 && $(CMAKE) -B build -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
	cd $(PROJECTS_DIR)/stm32f103 && $(CMAKE) --build build
	@printf "$(C_GREEN)✓ F103 done. Artifacts: projects/stm32f103/build/MCU_BOOT.{elf,bin,hex}$(C_RESET)\n"

## 编译 HC32F460 bootloader
hc32:
	@printf "$(C_CYAN)=== [3/3] Build HC32F460 bootloader ===$(C_RESET)\n"
	cd $(PROJECTS_DIR)/hc32f460/boot && $(CMAKE) -B build -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-hc32f460.cmake
	cd $(PROJECTS_DIR)/hc32f460/boot && $(CMAKE) --build build
	@printf "$(C_GREEN)✓ HC32 done. Artifacts: projects/hc32f460/boot/build/hc32_boot.{elf,bin,hex}$(C_RESET)\n"

## Python 签名工具冒烟测试
sign-smoke:
	@printf "$(C_CYAN)=== Sign tool smoke test ===$(C_RESET)\n"
	cd $(SIGN_TOOL) && $(PYTHON) generate_keys.py --outdir /tmp/mcu_boot_keys
	cd $(SIGN_TOOL) && $(PYTHON) -c "open('/tmp/mcu_boot_app.bin','wb').write(b'\\xAA'*1024)"
	cd $(SIGN_TOOL) && $(PYTHON) sign_firmware.py /tmp/mcu_boot_app.bin \
		--key /tmp/mcu_boot_keys/private_key.pem \
		--version 1.0.0 \
		--addr 0x08010000 \
		--out /tmp/mcu_boot_app_signed.bin
	@printf "$(C_GREEN)✓ Sign tool OK$(C_RESET)\n"

## 清理所有构建产物
clean:
	@printf "$(C_YELLOW)=== Cleaning build directories ===$(C_RESET)\n"
	rm -rf $(PROJECTS_DIR)/stm32f407/build
	rm -rf $(PROJECTS_DIR)/stm32f103/build
	rm -rf $(PROJECTS_DIR)/hc32f460/boot/build
	rm -rf $(PROJECTS_DIR)/hc32f460/app_printf/build
	rm -rf $(ROOT_DIR)/tool/mcu_boot_tool/bin
	rm -rf $(ROOT_DIR)/tool/mcu_boot_tool/obj

## 显示帮助
help:
	@echo "MCU_BOOT build targets:"
	@echo ""
	@echo "  make all         Build all 3 chip bootloaders (default)"
	@echo "  make f407        Build STM32F407 bootloader"
	@echo "  make f103        Build STM32F103 bootloader"
	@echo "  make hc32        Build HC32F460 bootloader"
	@echo "  make sign-smoke  Run Python signing tool smoke test"
	@echo "  make clean       Remove all build directories"
	@echo "  make help        Show this help"
	@echo ""
	@echo "Environment overrides:"
	@echo "  CMAKE=$(CMAKE)"
	@echo "  NINJA=$(NINJA)"
	@echo "  PYTHON=$(PYTHON)"
