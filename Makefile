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
	-mgeneral-regs-only -fno-pic -fno-pie -fno-builtin -O2 -Wall -Wextra -Wno-unused-parameter \
	-IRoboInclude -IRoboGPU -IRoboDesktop -IRoboInput -IRoboArch/X64

BOOT_CFLAGS := -std=c11 -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone \
	-mgeneral-regs-only -fno-builtin -O2 -Wall -Wextra -Wno-unused-parameter \
	-IRoboBootloader -IRoboInclude

KERNEL_SRCS := RoboKernel/RoboKernel.cpp RoboKernel/RoboSupport.cpp RoboGPU/RoboGPU.cpp \
	RoboDesktop/RoboDesktop.cpp RoboInput/RoboInput.cpp RoboInput/RoboPS2Mouse.cpp \
	RoboInput/RoboPS2Keyboard.cpp RoboArch/X64/RoboArch.cpp \
	RoboArch/X64/RoboInterrupt.cpp RoboArch/X64/RoboTimer.cpp RoboArch/X64/RoboMemory.cpp \
	RoboArch/X64/RoboCpu.cpp RoboArch/X64/RoboPci.cpp RoboArch/X64/RoboAta.cpp RoboArch/X64/RoboGPUVendor.cpp 	RoboKernel/RoboSchedule.cpp RoboInput/RoboUsb.cpp RoboFS/RoboFS.cpp RoboDisk/RoboDisk.cpp RoboDisk/RoboVirtioBlk.cpp RoboDisk/RoboAhci.cpp RoboDisk/RoboNvme.cpp
SHELL_DIR := $(OBJ)/Shell
SHELL_CFLAGS := -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector \
	-fno-asynchronous-unwind-tables -fno-unwind-tables -fno-threadsafe-statics -mno-red-zone \
	-fno-pic -fno-pie -msse2 -mfpmath=sse -nostdinc++ -O2 -Wall -Wextra -Wno-unused-parameter \
	-DROBO_KERNEL -IRoboLib/Include -ICompiler/Compiler/Include -ICompiler/Compiler/ThirdParty \
	-ICompiler/RoboPlatforms/RoboticelaOS -IRoboInclude -IRoboGPU -IRoboDesktop -IRoboInput -IRoboArch/X64
COMPILER_CPPS := $(wildcard Compiler/Compiler/Source/*.cpp)
SHELL_OBJS := $(patsubst Compiler/Compiler/Source/%.cpp,$(SHELL_DIR)/%.o,$(COMPILER_CPPS)) \
	$(SHELL_DIR)/DesktopPage.o $(SHELL_DIR)/RoboAppHost.o $(SHELL_DIR)/RoboticelaOSPlatform.o $(SHELL_DIR)/RoboShell.o \
	$(SHELL_DIR)/RoboVideo.o $(SHELL_DIR)/RoboHeap.o $(SHELL_DIR)/RoboStd.o $(SHELL_DIR)/RoboDebug.o
BLIT_CFLAGS := -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector \
	-fno-asynchronous-unwind-tables -fno-unwind-tables -mno-red-zone \
	-fno-pic -fno-pie -fno-builtin -msse2 -mfpmath=sse -O2 -Wall -Wextra -Wno-unused-parameter \
	-IRoboInclude -IRoboGPU -IRoboDesktop -IRoboInput -IRoboArch/X64
KERNEL_OBJS := $(OBJ)/RoboKernel.o $(OBJ)/RoboSupport.o $(OBJ)/RoboGPU.o \
	$(OBJ)/RoboDesktop.o $(OBJ)/RoboInput.o $(OBJ)/RoboPS2Mouse.o $(OBJ)/RoboPS2Keyboard.o \
	$(OBJ)/RoboArch.o $(OBJ)/RoboInterrupt.o $(OBJ)/RoboTimer.o $(OBJ)/RoboMemory.o \
	$(OBJ)/RoboCpu.o $(OBJ)/RoboPci.o $(OBJ)/RoboAta.o $(OBJ)/RoboGPUVendor.o $(OBJ)/RoboSchedule.o $(OBJ)/RoboUsb.o $(OBJ)/RoboFS.o \
	$(OBJ)/RoboDisk.o $(OBJ)/RoboVirtioBlk.o $(OBJ)/RoboAhci.o $(OBJ)/RoboNvme.o $(OBJ)/RoboPermit.o $(OBJ)/RoboProcess.o $(OBJ)/RoboCall.o $(OBJ)/RoboFrame.o $(OBJ)/RoboNet.o $(OBJ)/RoboSound.o $(OBJ)/RoboPower.o $(SHELL_OBJS)

ARM_OBJ := $(BUILD)/Objects/ARM64
ESP_ARM := $(BUILD)/EspArm
BOOT_ARM := $(ESP_ARM)/EFI/BOOT
ARM_TARGET := --target=aarch64-unknown-none-elf
ARM_CFLAGS := -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector \
	-fno-asynchronous-unwind-tables -fno-unwind-tables -fno-pic -fno-pie -fno-builtin -O2 \
	-Wall -Wextra -Wno-unused-parameter \
	-IRoboInclude -IRoboGPU -IRoboDesktop -IRoboInput -IRoboArch/X64
ARM_SHELL_DIR := $(ARM_OBJ)/Shell
ARM_SHELL_CFLAGS := $(ARM_CFLAGS) -fno-threadsafe-statics -nostdinc++ -DROBO_KERNEL \
	-IRoboLib/Include -ICompiler/Compiler/Include -ICompiler/Compiler/ThirdParty \
	-ICompiler/RoboPlatforms/RoboticelaOS
ARM_SHELL_OBJS := $(patsubst Compiler/Compiler/Source/%.cpp,$(ARM_SHELL_DIR)/%.o,$(COMPILER_CPPS)) \
	$(ARM_SHELL_DIR)/DesktopPage.o $(ARM_SHELL_DIR)/RoboAppHost.o $(ARM_SHELL_DIR)/RoboticelaOSPlatform.o \
	$(ARM_SHELL_DIR)/RoboShell.o $(ARM_SHELL_DIR)/RoboVideo.o $(ARM_SHELL_DIR)/RoboStd.o $(ARM_SHELL_DIR)/RoboDebug.o
ARM_OBJS := $(ARM_OBJ)/RoboKernel.o $(ARM_OBJ)/RoboSupport.o $(ARM_OBJ)/RoboGPU.o \
	$(ARM_OBJ)/RoboDesktop.o $(ARM_OBJ)/RoboInput.o $(ARM_OBJ)/RoboFS.o \
	$(ARM_OBJ)/RoboArch.o $(ARM_OBJ)/RoboVector.o $(ARM_OBJ)/RoboMemory.o \
	$(ARM_OBJ)/RoboCpu.o $(ARM_OBJ)/RoboTimer.o $(ARM_OBJ)/RoboAta.o $(ARM_OBJ)/RoboPci.o \
	$(ARM_OBJ)/RoboDisk.o $(ARM_OBJ)/RoboVirtioBlk.o $(ARM_OBJ)/RoboAhci.o $(ARM_OBJ)/RoboNvme.o $(ARM_OBJ)/RoboPermit.o $(ARM_OBJ)/RoboProcess.o $(ARM_OBJ)/RoboCall.o $(ARM_OBJ)/RoboFrame.o \
	$(ARM_OBJ)/RoboSchedule.o $(ARM_OBJ)/RoboInputDevice.o $(ARM_OBJ)/RoboGPUVendor.o \
	$(ARM_OBJ)/RoboNet.o $(ARM_OBJ)/RoboSound.o $(ARM_OBJ)/RoboPower.o $(ARM_SHELL_OBJS) \
	$(ARM_OBJ)/RoboHeap.o

.PHONY: all clean

TESTAPP_OBJS := $(SHELL_DIR)/TestAppPage.o $(SHELL_DIR)/RoboAppEntry.o

all: $(BOOT)/BOOTX64.EFI $(ESP)/RoboKernel.elf $(ESP)/TestApp.elf $(ESP)/RoboApp.bin $(BOOT_ARM)/BOOTAA64.EFI $(ESP_ARM)/RoboKernel.elf $(ESP_ARM)/RoboApp.bin $(BUILD)/RoboDisk.img

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

$(OBJ)/RoboKernel.o: RoboKernel/RoboKernel.cpp RoboGPU/RoboGPU.hpp RoboDesktop/RoboDesktop.hpp RoboInclude/RoboBootInfo.hpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(OBJ)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboKernel/RoboKernel.cpp -o $(OBJ)/RoboKernel.o

$(OBJ)/RoboSupport.o: RoboKernel/RoboSupport.cpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboKernel/RoboSupport.cpp -o $(OBJ)/RoboSupport.o

$(OBJ)/RoboGPU.o: RoboGPU/RoboGPU.cpp RoboGPU/RoboGPU.hpp RoboInclude/RoboGPUVendor.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(BLIT_CFLAGS) -c RoboGPU/RoboGPU.cpp -o $(OBJ)/RoboGPU.o

$(OBJ)/RoboDesktop.o: RoboDesktop/RoboDesktop.cpp RoboDesktop/RoboDesktop.hpp RoboGPU/RoboGPU.hpp
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

$(OBJ)/RoboNet.o: RoboNet/RoboNet.cpp RoboInclude/RoboNet.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboNet/RoboNet.cpp -o $(OBJ)/RoboNet.o

$(OBJ)/RoboSound.o: RoboSound/RoboSound.cpp RoboInclude/RoboSound.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboSound/RoboSound.cpp -o $(OBJ)/RoboSound.o

$(OBJ)/RoboPower.o: RoboPower/RoboPower.cpp RoboInclude/RoboPower.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboPower/RoboPower.cpp -o $(OBJ)/RoboPower.o

$(OBJ)/RoboAta.o: RoboArch/X64/RoboAta.cpp RoboArch/X64/RoboAta.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboArch/X64/RoboAta.cpp -o $(OBJ)/RoboAta.o

$(OBJ)/RoboGPUVendor.o: RoboArch/X64/RoboGPUVendor.cpp RoboInclude/RoboGPUVendor.hpp RoboGPU/RoboGPU.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboArch/X64/RoboGPUVendor.cpp -o $(OBJ)/RoboGPUVendor.o

$(OBJ)/RoboFS.o: RoboFS/RoboFS.cpp RoboInclude/RoboFS.hpp RoboInclude/RoboPartition.hpp RoboInclude/RoboDisk.hpp RoboInclude/RoboPermit.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboFS/RoboFS.cpp -o $(OBJ)/RoboFS.o

$(OBJ)/RoboProcess.o: RoboKernel/RoboProcess.cpp RoboInclude/RoboProcess.hpp RoboInclude/RoboProgram.hpp RoboInclude/RoboFS.hpp RoboInclude/RoboCall.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboKernel/RoboProcess.cpp -o $(OBJ)/RoboProcess.o

$(OBJ)/RoboCall.o: RoboKernel/RoboCall.cpp RoboInclude/RoboCall.hpp RoboInclude/RoboProgram.hpp RoboInclude/RoboFS.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboKernel/RoboCall.cpp -o $(OBJ)/RoboCall.o

$(OBJ)/RoboFrame.o: RoboKernel/RoboFrame.cpp RoboInclude/RoboFrame.hpp RoboInclude/RoboSchedule.hpp RoboInclude/RoboShell.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboKernel/RoboFrame.cpp -o $(OBJ)/RoboFrame.o

$(OBJ)/RoboPermit.o: RoboFS/RoboPermit.cpp RoboInclude/RoboPermit.hpp RoboInclude/RoboFS.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboFS/RoboPermit.cpp -o $(OBJ)/RoboPermit.o

$(OBJ)/RoboDisk.o: RoboDisk/RoboDisk.cpp RoboInclude/RoboDisk.hpp RoboInclude/RoboPartition.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboDisk/RoboDisk.cpp -o $(OBJ)/RoboDisk.o

$(OBJ)/RoboVirtioBlk.o: RoboDisk/RoboVirtioBlk.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboDisk/RoboVirtioBlk.cpp -o $(OBJ)/RoboVirtioBlk.o

$(OBJ)/RoboAhci.o: RoboDisk/RoboAhci.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboDisk/RoboAhci.cpp -o $(OBJ)/RoboAhci.o

$(OBJ)/RoboNvme.o: RoboDisk/RoboNvme.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboDisk/RoboNvme.cpp -o $(OBJ)/RoboNvme.o

$(BUILD)/RoboImage.exe: RoboImage/RoboImage.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) -std=c++20 -O2 -Wno-deprecated-declarations -IRoboInclude -o $(BUILD)/RoboImage.exe RoboImage/RoboImage.cpp

$(BUILD)/App/X64.bin: RoboApp/RoboAppMain.cpp RoboApp/RoboApp.ld RoboInclude/RoboCall.hpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(BUILD)/App' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-builtin -mno-red-zone -mno-sse -mgeneral-regs-only -mcmodel=large -O2 -IRoboInclude -c RoboApp/RoboAppMain.cpp -o $(BUILD)/App/RoboAppX64.o
	$(LLD) -nostdlib -static -T RoboApp/RoboApp.ld -o $(BUILD)/App/RoboAppX64.elf $(BUILD)/App/RoboAppX64.o
	llvm-objcopy -O binary $(BUILD)/App/RoboAppX64.elf $(BUILD)/App/X64.bin

$(BUILD)/App/ARM.bin: RoboApp/RoboAppMain.cpp RoboApp/RoboApp.ld RoboInclude/RoboCall.hpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(BUILD)/App' | Out-Null"
	$(CLANG) $(ARM_TARGET) -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-builtin -O2 -IRoboInclude -c RoboApp/RoboAppMain.cpp -o $(BUILD)/App/RoboAppARM.o
	$(LLD) -m aarch64elf -nostdlib -static -T RoboApp/RoboApp.ld -o $(BUILD)/App/RoboAppARM.elf $(BUILD)/App/RoboAppARM.o
	llvm-objcopy -O binary $(BUILD)/App/RoboAppARM.elf $(BUILD)/App/ARM.bin

$(BUILD)/RoboDisk.img: $(BUILD)/RoboImage.exe $(ESP)/RoboApp.bin $(ESP)/TestApp.elf $(BUILD)/App/X64.bin $(BUILD)/App/ARM.bin \
	$(ESP)/INTERR.TTF $(ESP)/INTERM.TTF $(ESP)/INTERS.TTF $(ESP)/INTERB.TTF $(ESP)/INTERI.TTF \
	$(ESP)/DESKTOP.BIN $(ESP)/BACKGROU.RJP $(ESP)/ROBOLOGO.SVG $(ESP)/EMPTYFOL.SVG
	$(BUILD)/RoboImage.exe $(BUILD)/RoboDisk.img $(ESP)

$(SHELL_DIR)/%.o: Compiler/Compiler/Source/%.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c $< -o $@

$(SHELL_DIR)/DesktopPage.o: Build/Desktop/DesktopPage.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c Build/Desktop/DesktopPage.cpp -o $(SHELL_DIR)/DesktopPage.o

$(SHELL_DIR)/RoboAppHost.o: RoboKernel/RoboAppHost.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c RoboKernel/RoboAppHost.cpp -o $(SHELL_DIR)/RoboAppHost.o

$(SHELL_DIR)/TestAppPage.o: Build/Desktop/TestAppPage.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c Build/Desktop/TestAppPage.cpp -o $(SHELL_DIR)/TestAppPage.o

$(SHELL_DIR)/RoboAppEntry.o: RoboKernel/RoboAppEntry.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c RoboKernel/RoboAppEntry.cpp -o $(SHELL_DIR)/RoboAppEntry.o

$(ESP)/TestApp.elf: $(TESTAPP_OBJS) $(ESP)/RoboKernel.elf RoboKernel/RoboApp.ld
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(ESP)' | Out-Null"
	$(LLD) -nostdlib -static -T RoboKernel/RoboApp.ld --just-symbols=$(ESP)/RoboKernel.elf -o $(ESP)/TestApp.elf $(TESTAPP_OBJS)

$(SHELL_DIR)/RoboticelaOSPlatform.o: Compiler/RoboPlatforms/RoboticelaOS/RoboticelaOSPlatform.cpp Compiler/RoboPlatforms/RoboticelaOS/RoboticelaOSPlatform.hpp RoboGPU/RoboGPU.hpp RoboDesktop/RoboDesktop.hpp RoboInclude/RoboGPUVendor.hpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c Compiler/RoboPlatforms/RoboticelaOS/RoboticelaOSPlatform.cpp -o $(SHELL_DIR)/RoboticelaOSPlatform.o

$(SHELL_DIR)/RoboShell.o: RoboKernel/RoboShell.cpp RoboInclude/RoboShell.hpp RoboGPU/RoboGPU.hpp RoboDesktop/RoboDesktop.hpp Compiler/RoboPlatforms/RoboticelaOS/RoboticelaOSPlatform.hpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c RoboKernel/RoboShell.cpp -o $(SHELL_DIR)/RoboShell.o

$(SHELL_DIR)/RoboVideo.o: RoboKernel/RoboVideo.cpp RoboInclude/RoboVideo.hpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(SHELL_CFLAGS) -c RoboKernel/RoboVideo.cpp -o $(SHELL_DIR)/RoboVideo.o

$(SHELL_DIR)/RoboHeap.o: RoboLib/RoboHeap.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboLib/RoboHeap.cpp -o $(SHELL_DIR)/RoboHeap.o

$(SHELL_DIR)/RoboDebug.o: RoboLib/RoboDebug.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(SHELL_DIR)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboLib/RoboDebug.cpp -o $(SHELL_DIR)/RoboDebug.o

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

$(ARM_OBJ)/RoboFS.o: RoboFS/RoboFS.cpp RoboInclude/RoboFS.hpp RoboInclude/RoboPartition.hpp RoboInclude/RoboDisk.hpp RoboInclude/RoboPermit.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboFS/RoboFS.cpp -o $(ARM_OBJ)/RoboFS.o

$(ARM_OBJ)/RoboProcess.o: RoboKernel/RoboProcess.cpp RoboInclude/RoboProcess.hpp RoboInclude/RoboProgram.hpp RoboInclude/RoboFS.hpp RoboInclude/RoboCall.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboKernel/RoboProcess.cpp -o $(ARM_OBJ)/RoboProcess.o

$(ARM_OBJ)/RoboCall.o: RoboKernel/RoboCall.cpp RoboInclude/RoboCall.hpp RoboInclude/RoboProgram.hpp RoboInclude/RoboFS.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboKernel/RoboCall.cpp -o $(ARM_OBJ)/RoboCall.o

$(ARM_OBJ)/RoboFrame.o: RoboKernel/RoboFrame.cpp RoboInclude/RoboFrame.hpp RoboInclude/RoboSchedule.hpp RoboInclude/RoboShell.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboKernel/RoboFrame.cpp -o $(ARM_OBJ)/RoboFrame.o

$(ARM_OBJ)/RoboPermit.o: RoboFS/RoboPermit.cpp RoboInclude/RoboPermit.hpp RoboInclude/RoboFS.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboFS/RoboPermit.cpp -o $(ARM_OBJ)/RoboPermit.o

$(ARM_OBJ)/RoboDisk.o: RoboDisk/RoboDisk.cpp RoboInclude/RoboDisk.hpp RoboInclude/RoboPartition.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboDisk/RoboDisk.cpp -o $(ARM_OBJ)/RoboDisk.o

$(ARM_OBJ)/RoboVirtioBlk.o: RoboDisk/RoboVirtioBlk.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboDisk/RoboVirtioBlk.cpp -o $(ARM_OBJ)/RoboVirtioBlk.o

$(ARM_OBJ)/RoboAhci.o: RoboDisk/RoboAhci.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboDisk/RoboAhci.cpp -o $(ARM_OBJ)/RoboAhci.o

$(ARM_OBJ)/RoboNvme.o: RoboDisk/RoboNvme.cpp RoboInclude/RoboPartition.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboDisk/RoboNvme.cpp -o $(ARM_OBJ)/RoboNvme.o

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

$(ARM_SHELL_DIR)/%.o: Compiler/Compiler/Source/%.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(ARM_SHELL_DIR)' | Out-Null"
	$(CLANG) $(ARM_TARGET) $(ARM_SHELL_CFLAGS) -c $< -o $@

$(ARM_SHELL_DIR)/DesktopPage.o: Build/Desktop/DesktopPage.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(ARM_SHELL_DIR)' | Out-Null"
	$(CLANG) $(ARM_TARGET) $(ARM_SHELL_CFLAGS) -c Build/Desktop/DesktopPage.cpp -o $(ARM_SHELL_DIR)/DesktopPage.o

$(ARM_SHELL_DIR)/RoboAppHost.o: RoboKernel/RoboAppHost.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_SHELL_CFLAGS) -c RoboKernel/RoboAppHost.cpp -o $(ARM_SHELL_DIR)/RoboAppHost.o

$(ARM_SHELL_DIR)/RoboticelaOSPlatform.o: Compiler/RoboPlatforms/RoboticelaOS/RoboticelaOSPlatform.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_SHELL_CFLAGS) -c Compiler/RoboPlatforms/RoboticelaOS/RoboticelaOSPlatform.cpp -o $(ARM_SHELL_DIR)/RoboticelaOSPlatform.o

$(ARM_SHELL_DIR)/RoboShell.o: RoboKernel/RoboShell.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_SHELL_CFLAGS) -c RoboKernel/RoboShell.cpp -o $(ARM_SHELL_DIR)/RoboShell.o

$(ARM_SHELL_DIR)/RoboVideo.o: RoboKernel/RoboVideo.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_SHELL_CFLAGS) -c RoboKernel/RoboVideo.cpp -o $(ARM_SHELL_DIR)/RoboVideo.o

$(ARM_SHELL_DIR)/RoboStd.o: RoboLib/RoboStd.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_SHELL_CFLAGS) -c RoboLib/RoboStd.cpp -o $(ARM_SHELL_DIR)/RoboStd.o

$(ARM_SHELL_DIR)/RoboDebug.o: RoboLib/RoboDebug.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -IRoboLib/Include -c RoboLib/RoboDebug.cpp -o $(ARM_SHELL_DIR)/RoboDebug.o

$(ARM_OBJ)/RoboNet.o: RoboNet/RoboNet.cpp RoboInclude/RoboNet.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboNet/RoboNet.cpp -o $(ARM_OBJ)/RoboNet.o

$(ARM_OBJ)/RoboSound.o: RoboSound/RoboSound.cpp RoboInclude/RoboSound.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboSound/RoboSound.cpp -o $(ARM_OBJ)/RoboSound.o

$(ARM_OBJ)/RoboPower.o: RoboPower/RoboPower.cpp RoboInclude/RoboPower.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboPower/RoboPower.cpp -o $(ARM_OBJ)/RoboPower.o

$(ARM_OBJ)/RoboGPUVendor.o: RoboArch/ARM64/RoboGPUVendor.cpp RoboInclude/RoboGPUVendor.hpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -c RoboArch/ARM64/RoboGPUVendor.cpp -o $(ARM_OBJ)/RoboGPUVendor.o

$(ARM_OBJ)/RoboHeap.o: RoboLib/RoboHeap.cpp
	$(CLANG) $(ARM_TARGET) $(ARM_CFLAGS) -IRoboLib/Include -c RoboLib/RoboHeap.cpp -o $(ARM_OBJ)/RoboHeap.o

clean:
	powershell -NoProfile -Command "if (Test-Path '$(BUILD)') { Remove-Item -Recurse -Force '$(BUILD)' }"
