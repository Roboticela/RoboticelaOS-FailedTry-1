The filenames must be PascalCase. The functions, classes, and structs must be PascalCase. Variable names must be camelCase. Constants must be CAPITAL_CASE. Use the uppercase letters for abbrevation words like SDK, XML, CPP etc

Use Robo in every name of the thing like RoboFS, RoboKernel, RoboBootloader and so on.

I want basic Bootloader and Kernel which just Mouse, Touchpad, Touch and Keyboard support. For now, I want 64bit implementation but later on I want ARM64 etc also so write in such a way that it supports it. Also, my final goal is to create a desktop using the Compiler folder thing and the Compiler folder stuff is the apps of the OS. I want completely best kernel and bootloader and runtime so their performance is too much fast and reliable and never crashes. Also, for GPU make a RoboGPU for now which will be the basic driver for GPU by default and make it in such a way that it supports any GPU and can work on any hardware.

Use MakeFile and Powershell and Qemu.

Add powershell for running the OS and for compiling for the Flashdrive to test on real hardware.
Also, add for Build. 

Use C and C++ for building only and use fewer ASM and also, use python in only cases like downloading Unicode data and so on.

# Stages

## Stage 0 — Blank desktop
- [x] RoboBootloader as a UEFI app that reads GOP, loads RoboKernel.elf, passes RoboBootInfo, then exits boot services
- [x] RoboArch/X64 for port access and halt only; desktop and GPU include no x86 headers
- [x] Freestanding RoboKernel, no exceptions, no heap, firmware page tables left in place
- [x] RoboGPU writes the GOP framebuffer in place and speaks both 32-bit pixel orders
- [x] One full desktop fill at boot; every later paint is a damage rectangle
- [x] RoboDesktop solid background and a software arrow cursor, clamped to the screen
- [x] RoboPS2Mouse on the i8042 port, packets folded into one move, timeout leaves the cursor centered
- [x] Makefile, RoboBuild.ps1, RoboRun.ps1 (QEMU + OVMF), RoboFlash.ps1 (only when a drive letter is given)

## Stage 1 — Interrupts and time
- [x] Minimal IDT and interrupt stubs
- [x] Timer (APIC or PIT) as the clock for later scheduling
- [x] Mouse IRQ instead of a spin on the status port
- [x] Idle waits on the timer or the mouse IRQ and still does not repaint when nothing moved

## Stage 2 — Keyboard
- [ ] PS/2 keyboard on the same RoboInput queue as the mouse
- [ ] Key events do not clear the desktop; they dirty only what changed

## Stage 3 — Virtual memory
- [ ] Physical page allocator
- [ ] Kernel page tables, replacing the firmware map
- [ ] Two arenas: one reset each frame, one for long-lived objects
- [ ] Paint and cursor movement still do not call a general allocator
- [ ] User address spaces and a privilege switch
- [ ] Syscall entry only for work that must cross rings; pixels stay damage rectangles

## Stage 4 — Scheduling
- [ ] Run queue and a context switch on the timer
- [ ] Kernel threads, then user processes
- [ ] Sleep until input or timer; wake does not redraw the screen
- [ ] One core first; more cores only after the single-core switch is solid

## Stage 5 — Touchpad, touch, and USB
- [ ] PCI scan for real-hardware controllers
- [ ] xHCI and USB HID mouse on the same input queue
- [ ] Touchpad as absolute points
- [ ] Touch as absolute points
- [ ] A missing USB device still boots the desktop

## Stage 6 — Compiler apps
- [ ] OS platform beside Web and Windows; generated C++ does not change
- [ ] Present copies only the app's dirty rectangle into RoboGPU
- [ ] RoboDesktop hosts those apps as windows
- [ ] A widget repaint appends damage rects and never fills the whole screen

## Stage 7 — Storage
- [ ] Disk or USB mass-storage driver
- [ ] RoboFS so an app can be loaded from media
- [ ] Python only where a host tool needs it, such as downloading Unicode data

## Stage 8 — Other CPUs and faster GPUs
- [ ] RoboArch/ARM64 and its bootloader; GPU, desktop, and damage list stay shared
- [ ] A vendor GPU path behind RoboGPU that still accepts rectangles
- [ ] The firmware framebuffer remains the default when no vendor path exists
