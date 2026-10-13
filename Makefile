ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
.SHELLFLAGS := /c
endif

ROOT := .
BUILD := Build
OBJ := $(BUILD)/Objects
ESP := $(BUILD)/Esp
BOOT := $(ESP)/EFI/BOOT

CLANG ?= clang
LLD ?= ld.lld
LLD_LINK ?= lld-link

KERNEL_CFLAGS := -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector \
	-fno-asynchronous-unwind-tables -fno-unwind-tables -mno-red-zone -mno-sse -mno-mmx \
	-mgeneral-regs-only -fno-pic -fno-pie -fno-builtin -Wall -Wextra -Wno-unused-parameter \
	-IRoboInclude -IRoboGPU -IRoboDesktop -IRoboInput -IRoboArch/X64

BOOT_CFLAGS := -std=c11 -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone \
	-mgeneral-regs-only -fno-builtin -Wall -Wextra -Wno-unused-parameter \
	-IRoboBootloader -IRoboInclude

KERNEL_SRCS := RoboKernel/RoboKernel.cpp RoboKernel/RoboSupport.cpp RoboGPU/RoboGPU.cpp \
	RoboDesktop/RoboDesktop.cpp RoboInput/RoboInput.cpp RoboInput/RoboPS2Mouse.cpp \
	RoboInput/RoboPS2Keyboard.cpp RoboArch/X64/RoboArch.cpp \
	RoboArch/X64/RoboInterrupt.cpp RoboArch/X64/RoboTimer.cpp RoboArch/X64/RoboMemory.cpp \
	RoboArch/X64/RoboCpu.cpp RoboArch/X64/RoboPci.cpp RoboArch/X64/RoboAta.cpp RoboArch/X64/RoboGPUVendor.cpp 	RoboKernel/RoboSchedule.cpp RoboInput/RoboUsb.cpp RoboFS/RoboFS.cpp RoboDisk/RoboDisk.cpp RoboDisk/RoboVirtioBlk.cpp RoboDisk/RoboAhci.cpp
SHELL_DIR := $(OBJ)/Shell
SHELL_CFLAGS := -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector \
	-fno-asynchronous-unwind-tables -fno-unwind-tables -fno-threadsafe-statics -mno-red-zone \
	-fno-pic -fno-pie -msse2 -mfpmath=sse -nostdinc++ -O2 -Wall -Wextra -Wno-unused-parameter \
	-DROBO_KERNEL -IRoboLib/Include -ICompiler/Compiler/Include -ICompiler/Compiler/ThirdParty \
	-ICompiler/RoboPlatforms/RoboticelaOS -IRoboInclude -IRoboGPU -IRoboDesktop -IRoboInput
COMPILER_CPPS := $(wildcard Compiler/Compiler/Source/*.cpp)
SHELL_OBJS := $(patsubst Compiler/Compiler/Source/%.cpp,$(SHELL_DIR)/%.o,$(COMPILER_CPPS)) \
	$(SHELL_DIR)/DesktopPage.o $(SHELL_DIR)/RoboticelaOSPlatform.o $(SHELL_DIR)/RoboShell.o \
	$(SHELL_DIR)/RoboVideo.o $(SHELL_DIR)/RoboHeap.o $(SHELL_DIR)/RoboStd.o
BLIT_CFLAGS := -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector \
	-fno-asynchronous-unwind-tables -fno-unwind-tables -mno-red-zone \
	-fno-pic -fno-pie -fno-builtin -msse2 -mfpmath=sse -O2 -Wall -Wextra -Wno-unused-parameter \
	-IRoboInclude -IRoboGPU -IRoboDesktop -IRoboInput -IRoboArch/X64
KERNEL_OBJS := $(OBJ)/RoboKernel.o $(OBJ)/RoboSupport.o $(OBJ)/RoboGPU.o \
	$(OBJ)/RoboDesktop.o $(OBJ)/RoboInput.o $(OBJ)/RoboPS2Mouse.o $(OBJ)/RoboPS2Keyboard.o \
	$(OBJ)/RoboArch.o $(OBJ)/RoboInterrupt.o $(OBJ)/RoboTimer.o $(OBJ)/RoboMemory.o \
	$(OBJ)/RoboCpu.o $(OBJ)/RoboPci.o $(OBJ)/RoboAta.o $(OBJ)/RoboGPUVendor.o $(OBJ)/RoboSchedule.o $(OBJ)/RoboUsb.o $(OBJ)/RoboFS.o \
	$(OBJ)/RoboDisk.o $(OBJ)/RoboVirtioBlk.o $(OBJ)/RoboAhci.o $(SHELL_OBJS)

ARM_OBJ := $(BUILD)/Objects/ARM64
ESP_ARM := $(BUILD)/EspArm
BOOT_ARM := $(ESP_ARM)/EFI/BOOT
ARM_TARGET := --target=aarch64-unknown-none-elf
ARM_CFLAGS := -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector \
	-fno-asynchronous-unwind-tables -fno-unwind-tables -fno-pic -fno-pie -fno-builtin \
	-Wall -Wextra -Wno-unused-parameter \
	-IRoboInclude -IRoboGPU -IRoboDesktop -IRoboInput -IRoboArch/X64
ARM_OBJS := $(ARM_OBJ)/RoboKernel.o $(ARM_OBJ)/RoboSupport.o $(ARM_OBJ)/RoboGPU.o \
	$(ARM_OBJ)/RoboDesktop.o $(ARM_OBJ)/RoboInput.o $(ARM_OBJ)/RoboFS.o \
	$(ARM_OBJ)/RoboArch.o $(ARM_OBJ)/RoboVector.o $(ARM_OBJ)/RoboMemory.o \
	$(ARM_OBJ)/RoboCpu.o $(ARM_OBJ)/RoboTimer.o $(ARM_OBJ)/RoboAta.o $(ARM_OBJ)/RoboPci.o \
	$(ARM_OBJ)/RoboDisk.o $(ARM_OBJ)/RoboVirtioBlk.o $(ARM_OBJ)/RoboAhci.o \
	$(ARM_OBJ)/RoboSchedule.o $(ARM_OBJ)/RoboInputDevice.o $(ARM_OBJ)/RoboGPUVendor.o $(ARM_OBJ)/RoboShellStub.o

.PHONY: all clean

all: $(BOOT)/BOOTX64.EFI $(ESP)/RoboKernel.elf $(ESP)/RoboApp.bin $(BOOT_ARM)/BOOTAA64.EFI $(ESP_ARM)/RoboKernel.elf $(ESP_ARM)/RoboApp.bin $(BUILD)/RoboDisk.img

$(BOOT)/BOOTX64.EFI: $(OBJ)/RoboBootloader.o
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(BOOT)' | Out-Null"
	$(LLD_LINK) -subsystem:efi_application -nodefaultlib -dll -entry:EfiMain -out:$(BOOT)/BOOTX64.EFI $(OBJ)/RoboBootloader.o

$(ESP)/RoboKernel.elf: $(KERNEL_OBJS) RoboKernel/RoboLinker.ld
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(ESP)' | Out-Null"
	$(LLD) -nostdlib -static -T RoboKernel/RoboLinker.ld -z max-page-size=0x1000 -o $(ESP)/RoboKernel.elf $(KERNEL_OBJS)

$(ESP)/RoboApp.bin: $(ESP)/RoboKernel.elf
	powershell -NoProfile -Command "[IO.File]::WriteAllBytes('$(ESP)/RoboApp.bin', [byte[]](0x52,0x4F,0x42,0x4F,0x4A,0x3B,0xC2,0x00))"

$(OBJ)/RoboBootloader.o: RoboBootloader/RoboBootloader.c RoboBootloader/RoboEfi.hpp RoboInclude/RoboBootInfo.hpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(OBJ)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-windows $(BOOT_CFLAGS) -c RoboBootloader/RoboBootloader.c -o $(OBJ)/RoboBootloader.o

$(OBJ)/RoboKernel.o: RoboKernel/RoboKernel.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(OBJ)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboKernel/RoboKernel.cpp -o $(OBJ)/RoboKernel.o

$(OBJ)/RoboSupport.o: RoboKernel/RoboSupport.cpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboKernel/RoboSupport.cpp -o $(OBJ)/RoboSupport.o

$(OBJ)/RoboGPU.o: RoboGPU/RoboGPU.cpp RoboGPU/RoboGPU.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(BLIT_CFLAGS) -c RoboGPU/RoboGPU.cpp -o $(OBJ)/RoboGPU.o

$(OBJ)/RoboDesktop.o: RoboDesktop/RoboDesktop.cpp RoboDesktop/RoboDesktop.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(BLIT_CFLAGS) -c RoboDesktop/RoboDesktop.cpp -o $(OBJ)/RoboDesktop.o

$(OBJ)/RoboInput.o: RoboInput/RoboInput.cpp RoboInput/RoboInput.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboInput/RoboInput.cpp -o $(OBJ)/RoboInput.o

$(OBJ)/RoboPS2Mouse.o: RoboInput/RoboPS2Mouse.cpp RoboInput/RoboPS2Mouse.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboInput/RoboPS2Mouse.cpp -o $(OBJ)/RoboPS2Mouse.o

$(OBJ)/RoboPS2Keyboard.o: RoboInput/RoboPS2Keyboard.cpp RoboInput/RoboPS2Keyboard.hpp RoboInput/RoboInput.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboInput/RoboPS2Keyboard.cpp -o $(OBJ)/RoboPS2Keyboard.o

$(OBJ)/RoboArch.o: RoboArch/X64/RoboArch.cpp RoboArch/X64/RoboArch.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboArch/X64/RoboArch.cpp -o $(OBJ)/RoboArch.o

$(OBJ)/RoboInterrupt.o: RoboArch/X64/RoboInterrupt.cpp RoboArch/X64/RoboTimer.hpp RoboInput/RoboPS2Mouse.hpp RoboInput/RoboPS2Keyboard.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboArch/X64/RoboInterrupt.cpp -o $(OBJ)/RoboInterrupt.o

$(OBJ)/RoboTimer.o: RoboArch/X64/RoboTimer.cpp RoboArch/X64/RoboTimer.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboArch/X64/RoboTimer.cpp -o $(OBJ)/RoboTimer.o

$(OBJ)/RoboMemory.o: RoboArch/X64/RoboMemory.cpp RoboArch/X64/RoboMemory.hpp RoboInclude/RoboBootInfo.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboArch/X64/RoboMemory.cpp -o $(OBJ)/RoboMemory.o

$(OBJ)/RoboCpu.o: RoboArch/X64/RoboCpu.cpp RoboArch/X64/RoboCpu.hpp RoboArch/X64/RoboMemory.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboArch/X64/RoboCpu.cpp -o $(OBJ)/RoboCpu.o

$(OBJ)/RoboPci.o: RoboArch/X64/RoboPci.cpp RoboArch/X64/RoboPci.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboArch/X64/RoboPci.cpp -o $(OBJ)/RoboPci.o

$(OBJ)/RoboUsb.o: RoboInput/RoboUsb.cpp RoboInput/RoboUsb.hpp RoboInput/RoboInput.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboInput/RoboUsb.cpp -o $(OBJ)/RoboUsb.o

$(OBJ)/RoboAta.o: RoboArch/X64/RoboAta.cpp RoboArch/X64/RoboAta.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboArch/X64/RoboAta.cpp -o $(OBJ)/RoboAta.o

$(OBJ)/RoboGPUVendor.o: RoboArch/X64/RoboGPUVendor.cpp RoboInclude/RoboGPUVendor.hpp RoboGPU/RoboGPU.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboArch/X64/RoboGPUVendor.cpp -o $(OBJ)/RoboGPUVendor.o

$(OBJ)/RoboFS.o: RoboFS/RoboFS.cpp RoboInclude/RoboFS.hpp RoboInclude/RoboPartition.hpp RoboInclude/RoboDisk.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboFS/RoboFS.cpp -o $(OBJ)/RoboFS.o

$(OBJ)/RoboDisk.o: RoboDisk/RoboDisk.cpp RoboInclude/RoboDisk.hpp RoboInclude/RoboPartition.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboDisk/RoboDisk.cpp -o $(OBJ)/RoboDisk.o

$(OBJ)/RoboVirtioBlk.o: RoboDisk/RoboVirtioBlk.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboDisk/RoboVirtioBlk.cpp -o $(OBJ)/RoboVirtioBlk.o

$(OBJ)/RoboAhci.o: RoboDisk/RoboAhci.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboDisk/RoboAhci.cpp -o $(OBJ)/RoboAhci.o

$(BUILD)/RoboImage.exe: RoboImage/RoboImage.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) -std=c++20 -O2 -Wno-deprecated-declarations -IRoboInclude -o $(BUILD)/RoboImage.exe RoboImage/RoboImage.cpp

$(BUILD)/RoboDisk.img: $(BUILD)/RoboImage.exe $(ESP)/RoboApp.bin
	$(BUILD)/RoboImage.exe $(BUILD)/RoboDisk.img $(ESP)

$(SHELL_DIR)/%.o: Compiler/Compiler/Source/%.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c $< -o $@

$(SHELL_DIR)/DesktopPage.o: Build/Desktop/DesktopPage.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c Build/Desktop/DesktopPage.cpp -o $(SHELL_DIR)/DesktopPage.o

$(SHELL_DIR)/RoboticelaOSPlatform.o: Compiler/RoboPlatforms/RoboticelaOS/RoboticelaOSPlatform.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c Compiler/RoboPlatforms/RoboticelaOS/RoboticelaOSPlatform.cpp -o $(SHELL_DIR)/RoboticelaOSPlatform.o

$(SHELL_DIR)/RoboShell.o: RoboKernel/RoboShell.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c RoboKernel/RoboShell.cpp -o $(SHELL_DIR)/RoboShell.o

$(SHELL_DIR)/RoboVideo.o: RoboKernel/RoboVideo.cpp RoboInclude/RoboVideo.hpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c RoboKernel/RoboVideo.cpp -o $(SHELL_DIR)/RoboVideo.o

$(SHELL_DIR)/RoboHeap.o: RoboLib/RoboHeap.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboLib/RoboHeap.cpp -o $(SHELL_DIR)/RoboHeap.o

$(SHELL_DIR)/RoboStd.o: RoboLib/RoboStd.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c RoboLib/RoboStd.cpp -o $(SHELL_DIR)/RoboStd.o

$(OBJ)/RoboSchedule.o: RoboKernel/RoboSchedule.cpp RoboInclude/RoboSchedule.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboKernel/RoboSchedule.cpp -o $(OBJ)/RoboSchedule.o

$(ESP_ARM)/RoboKernel.elf: $(ARM_OBJS) RoboKernel/RoboLinkerARM64.ld
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(ESP_ARM)' | Out-Null"
	$(LLD) -m aarch64elf -nostdlib -static -T RoboKernel/RoboLinkerARM64.ld -z max-page-size=0x1000 -o $(ESP_ARM)/RoboKernel.elf $(ARM_OBJS)

$(ESP_ARM)/RoboApp.bin: $(ESP_ARM)/RoboKernel.elf
	powershell -NoProfile -Command "[IO.File]::WriteAllBytes('$(ESP_ARM)/RoboApp.bin', [byte[]](0x52,0x4F,0x42,0x4F,0x4A,0x3B,0xC2,0x00))"

$(BOOT_ARM)/BOOTAA64.EFI: $(ARM_OBJ)/RoboBootloader.o
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(BOOT_ARM)' | Out-Null"
	$(LLD_LINK) -subsystem:efi_application -nodefaultlib -dll -entry:EfiMain -out:$(BOOT_ARM)/BOOTAA64.EFI $(ARM_OBJ)/RoboBootloader.o

$(ARM_OBJ)/RoboBootloader.o: RoboBootloader/RoboBootloader.c RoboBootloader/RoboEfi.hpp RoboInclude/RoboBootInfo.hpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(ARM_OBJ)' | Out-Null"
	$(CLANG) --target=aarch64-unknown-windows -std=c11 -ffreestanding -fno-stack-protector -fshort-wchar -fno-builtin -Wall -Wextra -Wno-unused-parameter -IRoboBootloader -IRoboInclude -DROBO_ARCH_ARM64 -c RoboBootloader/RoboBootloader.c -o $(ARM_OBJ)/RoboBootloader.o

$(ARM_OBJ)/RoboKernel.o: RoboKernel/RoboKernel.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(ARM_OBJ)' | Out-Null"
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboKernel/RoboKernel.cpp -o $(ARM_OBJ)/RoboKernel.o

$(ARM_OBJ)/RoboSupport.o: RoboKernel/RoboSupport.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboKernel/RoboSupport.cpp -o $(ARM_OBJ)/RoboSupport.o

$(ARM_OBJ)/RoboGPU.o: RoboGPU/RoboGPU.cpp RoboGPU/RoboGPU.hpp RoboInclude/RoboGPUVendor.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboGPU/RoboGPU.cpp -o $(ARM_OBJ)/RoboGPU.o

$(ARM_OBJ)/RoboDesktop.o: RoboDesktop/RoboDesktop.cpp RoboDesktop/RoboDesktop.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboDesktop/RoboDesktop.cpp -o $(ARM_OBJ)/RoboDesktop.o

$(ARM_OBJ)/RoboInput.o: RoboInput/RoboInput.cpp RoboInput/RoboInput.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboInput/RoboInput.cpp -o $(ARM_OBJ)/RoboInput.o

$(ARM_OBJ)/RoboFS.o: RoboFS/RoboFS.cpp RoboInclude/RoboFS.hpp RoboInclude/RoboPartition.hpp RoboInclude/RoboDisk.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboFS/RoboFS.cpp -o $(ARM_OBJ)/RoboFS.o

$(ARM_OBJ)/RoboDisk.o: RoboDisk/RoboDisk.cpp RoboInclude/RoboDisk.hpp RoboInclude/RoboPartition.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboDisk/RoboDisk.cpp -o $(ARM_OBJ)/RoboDisk.o

$(ARM_OBJ)/RoboVirtioBlk.o: RoboDisk/RoboVirtioBlk.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboDisk/RoboVirtioBlk.cpp -o $(ARM_OBJ)/RoboVirtioBlk.o

$(ARM_OBJ)/RoboAhci.o: RoboDisk/RoboAhci.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboDisk/RoboAhci.cpp -o $(ARM_OBJ)/RoboAhci.o

$(ARM_OBJ)/RoboPci.o: RoboArch/ARM64/RoboPci.cpp RoboArch/X64/RoboPci.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboArch/ARM64/RoboPci.cpp -o $(ARM_OBJ)/RoboPci.o

$(ARM_OBJ)/RoboArch.o: RoboArch/ARM64/RoboArch.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboArch/ARM64/RoboArch.cpp -o $(ARM_OBJ)/RoboArch.o

$(ARM_OBJ)/RoboVector.o: RoboArch/ARM64/RoboVector.S
	$(CLANG) $(ARM_TARGET) -c RoboArch/ARM64/RoboVector.S -o $(ARM_OBJ)/RoboVector.o

$(ARM_OBJ)/RoboMemory.o: RoboArch/ARM64/RoboMemory.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboArch/ARM64/RoboMemory.cpp -o $(ARM_OBJ)/RoboMemory.o

$(ARM_OBJ)/RoboCpu.o: RoboArch/ARM64/RoboCpu.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboArch/ARM64/RoboCpu.cpp -o $(ARM_OBJ)/RoboCpu.o

$(ARM_OBJ)/RoboTimer.o: RoboArch/ARM64/RoboTimer.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboArch/ARM64/RoboTimer.cpp -o $(ARM_OBJ)/RoboTimer.o

$(ARM_OBJ)/RoboAta.o: RoboArch/ARM64/RoboAta.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboArch/ARM64/RoboAta.cpp -o $(ARM_OBJ)/RoboAta.o

$(ARM_OBJ)/RoboSchedule.o: RoboArch/ARM64/RoboSchedule.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboArch/ARM64/RoboSchedule.cpp -o $(ARM_OBJ)/RoboSchedule.o

$(ARM_OBJ)/RoboInputDevice.o: RoboArch/ARM64/RoboInputDevice.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboArch/ARM64/RoboInputDevice.cpp -o $(ARM_OBJ)/RoboInputDevice.o

$(ARM_OBJ)/RoboShellStub.o: RoboArch/ARM64/RoboShellStub.cpp RoboInclude/RoboShell.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboArch/ARM64/RoboShellStub.cpp -o $(ARM_OBJ)/RoboShellStub.o

$(ARM_OBJ)/RoboGPUVendor.o: RoboArch/ARM64/RoboGPUVendor.cpp RoboInclude/RoboGPUVendor.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboArch/ARM64/RoboGPUVendor.cpp -o $(ARM_OBJ)/RoboGPUVendor.o

clean:
	powershell -NoProfile -Command "if (Test-Path '$(BUILD)') { Remove-Item -Recurse -Force '$(BUILD)' }"
