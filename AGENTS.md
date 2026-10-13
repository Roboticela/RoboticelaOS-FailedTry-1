The filenames must be PascalCase. The functions, classes, and structs must be PascalCase. Variable names must be camelCase. Constants must be CAPITAL_CASE. Use the uppercase letters for abbrevation words like SDK, XML, CPP etc

Use Robo in every name of the thing like RoboFS, RoboKernel, RoboBootloader and so on.

I want basic Bootloader and Kernel which just Mouse, Touchpad, Touch and Keyboard support. For now, I want 64bit implementation but later on I want ARM64 etc also so write in such a way that it supports it. Also, my final goal is a complete desktop OS. The Compiler folder stuff is the apps. The kernel, bootloader, and runtime stay fast and reliable and do not crash. A fault in one app ends that app. The desktop, the other apps, and the kernel keep running. X64 and ARM64 run the same GPU, desktop, and damage list. Also, for GPU make a RoboGPU for now which will be the basic driver for GPU by default and make it in such a way that it supports any GPU and can work on any hardware.

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
- [x] PS/2 keyboard on the same RoboInput queue as the mouse
- [x] Key events do not clear the desktop; they dirty only what changed

## Stage 3 — Virtual memory
- [x] Physical page allocator
- [x] Kernel page tables, replacing the firmware map
- [x] Two arenas: one reset each frame, one for long-lived objects
- [x] Paint and cursor movement still do not call a general allocator
- [x] User address spaces and a privilege switch
- [x] Syscall entry only for work that must cross rings; pixels stay damage rectangles

## Stage 4 — Scheduling
- [x] Run queue and a context switch on the timer
- [x] Kernel threads, then user processes
- [x] Sleep until input or timer; wake does not redraw the screen
- [x] One core first; more cores only after the single-core switch is solid

## Stage 5 — Touchpad, touch, and USB
- [x] PCI scan for real-hardware controllers
- [x] xHCI and USB HID mouse on the same input queue
- [x] Touchpad as absolute points
- [x] Touch as absolute points
- [x] A missing USB device still boots the desktop

## Stage 6 — Compiler apps
- [x] OS platform beside Web and Windows; generated C++ does not change
- [x] Present copies only the app's dirty rectangle into RoboGPU
- [x] RoboDesktop hosts those apps as windows
- [x] A widget repaint appends damage rects and never fills the whole screen

## Stage 7 — Storage
- [x] Disk or USB mass-storage driver
- [x] RoboFS so an app can be loaded from media
- [x] Python only where a host tool needs it, such as downloading Unicode data

## Stage 8 — Other CPUs and faster GPUs
- [x] RoboArch/ARM64 and its bootloader; GPU, desktop, and damage list stay shared
- [x] A vendor GPU path behind RoboGPU that still accepts rectangles
- [x] The firmware framebuffer remains the default when no vendor path exists

## Stage 9 — ARM64 matches X64
- [x] Kernel page tables replace the firmware map, with the same two arenas
- [x] Timer IRQ, syscall entry, and a one-core schedule
- [x] Idle waits on the timer and still does not repaint when nothing changed
- [x] GPU, desktop, and the damage list stay the shared code

## Stage 10 — Disks on real machines
- [x] AHCI or NVMe beside ATA, and virtio-blk on ARM64
- [x] RoboFS mounts the first disk that answers
- [x] A missing disk still boots the desktop

## Stage 11 — Files an app can write
- [x] Create, read, write, and delete a file
- [x] Directories, so an app can keep its own folder
- [x] A failed write returns an error and does not panic the kernel

## Stage 12 — App processes
- [x] Load a Compiler app from RoboFS into its own user address space
- [x] The app heap is not the kernel heap; paint still does not call a general allocator
- [x] Exit, or a fault, ends that process and leaves the desktop up

## Stage 13 — Syscalls for the OS platform
- [ ] Present, input, clock, files, and exit cross rings; generated C++ does not change
- [ ] Present still copies only the app's dirty rectangle
- [ ] Keys and the pointer reach the app that owns the focused window

## Stage 14 — Many windows
- [ ] More than one window, with focus, drag, resize, and close
- [ ] Z-order; a repaint dirties only the rectangles that changed
- [ ] Closing a window stops its process and restores only the uncovered pixels

## Stage 15 — Text
- [ ] Unicode data stays a host download
- [ ] Glyphs are drawn into damage rectangles
- [ ] Windows show text and never fill the whole screen to do it

## Stage 16 — Launcher
- [ ] The desktop lists Compiler apps stored on RoboFS
- [ ] Choosing one starts it in a window
- [ ] An empty disk still shows the desktop

## Stage 17 — USB keyboard and more pointers
- [ ] USB keyboard on the same RoboInput queue
- [ ] More than one mouse, touchpad, or touch device
- [ ] A missing controller still boots

## Stage 18 — More cores
- [x] Further cores join only after the one-core switch stays solid
- [x] A core with nothing to run sleeps
- [x] One painter owns the framebuffer

## Stage 19 — GPUs that stay behind rectangles
- [x] Each vendor path, including virtio-gpu, accepts the same rectangles
- [x] An unknown GPU uses the firmware framebuffer
- [x] No vendor path blocks boot

## Stage 20 — Network
- [ ] One NIC, virtio-net or a common PCI Ethernet device
- [ ] TCP and UDP sockets for an app
- [ ] A missing NIC still boots the desktop

## Stage 21 — Sound
- [ ] One playback path
- [ ] An app writes samples; audio does not repaint the desktop
- [ ] A missing device still boots

## Stage 22 — Power
- [ ] Shutdown and reboot through the firmware tables
- [ ] Halt remains the fallback when those tables are missing

## Stage 23 — Install and boot
- [ ] The installed volume holds the bootloader, the kernel, and the apps
- [ ] RoboFlash.ps1 writes that layout, and still refuses the system drive
- [ ] QEMU boots that image the same way it boots the build folder
