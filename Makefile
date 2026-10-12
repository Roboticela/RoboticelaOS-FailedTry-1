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
	RoboArch/X64/RoboCpu.cpp RoboArch/X64/RoboPci.cpp RoboKernel/RoboSchedule.cpp RoboInput/RoboUsb.cpp
KERNEL_OBJS := $(OBJ)/RoboKernel.o $(OBJ)/RoboSupport.o $(OBJ)/RoboGPU.o \
	$(OBJ)/RoboDesktop.o $(OBJ)/RoboInput.o $(OBJ)/RoboPS2Mouse.o $(OBJ)/RoboPS2Keyboard.o \
	$(OBJ)/RoboArch.o $(OBJ)/RoboInterrupt.o $(OBJ)/RoboTimer.o $(OBJ)/RoboMemory.o \
	$(OBJ)/RoboCpu.o $(OBJ)/RoboPci.o $(OBJ)/RoboSchedule.o $(OBJ)/RoboUsb.o

.PHONY: all clean

all: $(BOOT)/BOOTX64.EFI $(ESP)/RoboKernel.elf

$(BOOT)/BOOTX64.EFI: $(OBJ)/RoboBootloader.o
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(BOOT)' | Out-Null"
	$(LLD_LINK) -subsystem:efi_application -nodefaultlib -dll -entry:EfiMain -out:$(BOOT)/BOOTX64.EFI $(OBJ)/RoboBootloader.o

$(ESP)/RoboKernel.elf: $(KERNEL_OBJS) RoboKernel/RoboLinker.ld
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(ESP)' | Out-Null"
	$(LLD) -nostdlib -static -T RoboKernel/RoboLinker.ld -z max-page-size=0x1000 -o $(ESP)/RoboKernel.elf $(KERNEL_OBJS)

$(OBJ)/RoboBootloader.o: RoboBootloader/RoboBootloader.c RoboBootloader/RoboEfi.hpp RoboInclude/RoboBootInfo.hpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(OBJ)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-windows $(BOOT_CFLAGS) -c RoboBootloader/RoboBootloader.c -o $(OBJ)/RoboBootloader.o

$(OBJ)/RoboKernel.o: RoboKernel/RoboKernel.cpp
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(OBJ)' | Out-Null"
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboKernel/RoboKernel.cpp -o $(OBJ)/RoboKernel.o

$(OBJ)/RoboSupport.o: RoboKernel/RoboSupport.cpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboKernel/RoboSupport.cpp -o $(OBJ)/RoboSupport.o

$(OBJ)/RoboGPU.o: RoboGPU/RoboGPU.cpp RoboGPU/RoboGPU.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboGPU/RoboGPU.cpp -o $(OBJ)/RoboGPU.o

$(OBJ)/RoboDesktop.o: RoboDesktop/RoboDesktop.cpp RoboDesktop/RoboDesktop.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboDesktop/RoboDesktop.cpp -o $(OBJ)/RoboDesktop.o

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

$(OBJ)/RoboSchedule.o: RoboKernel/RoboSchedule.cpp RoboInclude/RoboSchedule.hpp
	$(CLANG) --target=x86_64-unknown-none-elf $(KERNEL_CFLAGS) -c RoboKernel/RoboSchedule.cpp -o $(OBJ)/RoboSchedule.o

clean:
	powershell -NoProfile -Command "if (Test-Path '$(BUILD)') { Remove-Item -Recurse -Force '$(BUILD)' }"
