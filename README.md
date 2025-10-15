# PISO HID

![Build](https://github.com/bf39l/PISO_HID/actions/workflows/build.yml/badge.svg)

PISO_HID is a Raspberry Pi Pico keyboard firmware using PISO shift registers, TinyUSB (HID + CDC), and FreeRTOS. It supports layers, Mod‑Tap (MT), NKRO/6KRO, and an SSD1306 OLED status view.

Highlights
- PISO keyboard scan via ShiftRegister_PISO
- TinyUSB HID composite with custom descriptors
- FreeRTOS tasks for USB, scan, and OLED
- Layers: MO (momentary), TG (toggle), TO (set), MT (Mod‑Tap layer/mod)
- NKRO/6KRO toggle functional key
- On‑device status: NKRO mode, base/active layer, stack depth

Repository layout
- src/ … app tasks (scan, USB, OLED), common.h
- lib/CustomHID/ … HID, keymap, descriptors, functional keys
- lib/FreeRTOS-Kernel/ … submodule (upstream FreeRTOS)
- lib/OLED_SSD1306/ … driver (if used)
- .github/workflows/build.yml … CI that builds UF2 and releases

Build (macOS/Linux)
```bash
# Clone with submodules
git clone https://github.com/bf39l/PISO_HID
cd PISO_HID
git submodule update --init --recursive

# Or let CMake fetch the Pico SDK:
# cmake -S . -B build -G Ninja -DPICO_SDK_FETCH_FROM_GIT=1

# Configure & build
cmake -S . -B build -G Ninja
cmake --build build -j
# Firmware outputs in build/: .uf2 .elf .bin .map
```

Flash
- BOOTSEL: hold BOOTSEL on Pico, plug in USB, copy build/PISO_HID.uf2 to the RPI‑RP2 drive.
- SWD/OpenOCD: use your VS Code tasks or openocd command.

CI artifacts and releases
- Every push/PR builds firmware and uploads artifacts (UF2, ELF, HEX, BIN, MAP, DIS).
- Tag a version to publish a GitHub Release with firmware:
  - git tag -a v0.0.1 -m "v0.0.1"
  - git push origin v0.0.1
- Ensure GitHub Actions workflow permissions: Settings → Actions → Workflow permissions → Read and write.

Submodule tips (FreeRTOS-Kernel)
- Update pointer to a valid upstream commit:
```bash
git submodule sync --recursive
git submodule update --init --recursive lib/FreeRTOS-Kernel
# or track main:
git -C lib/FreeRTOS-Kernel fetch origin --tags --prune
git -C lib/FreeRTOS-Kernel checkout --detach origin/main
git add lib/FreeRTOS-Kernel .gitmodules
git commit -m "Update FreeRTOS-Kernel submodule pointer"
```
- Fix “not our ref … Direct fetching of that commit failed”: repoint the submodule to a valid tag/branch as above.

Key features overview
- Functional keys:
  - FN_NKRO_TG: toggles NKRO/6KRO (applies on key release)
  - FN_RESET: soft reboot via watchdog (no BOOTSEL)
  - FN_BOOT: reboot to BOOTSEL via reset_usb_boot()
- Mod‑Tap (MT):
  - Tap sends base key; hold after MT_TAP_TIMEOUT_MS applies hold action (mods or momentary layer)
  - custom_hid.c periodically ticks MT to flip holds exactly at timeout
- OLED status:
  - Right‑aligned NKRO/6KRO and layer info
  - Renders state + latest shift‑register snapshot in one frame to avoid flicker

Troubleshooting
- fatal error: tusb_config.h: No such file or directory
  - Ensure the include path adds lib/CustomHID, not the file itself:
    - target_include_directories(CUSTOM_HID PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lib/CustomHID)
- implicit declaration errors when splitting files
  - Add a forward declaration in the .c file or a private header; don’t rely on implicit prototypes.
- “Resource not accessible by integration” on release
  - Grant Actions “Read and write” permissions or use a PAT secret in the release step.

Config notes
- MT_TAP_TIMEOUT_MS controls tap‑vs‑hold for MT keys and influences OLED update cadence.
- FreeRTOS hooks: vApplicationMallocFailedHook / vApplicationStackOverflowHook must be provided or disabled.

### FreeRTOS

```bash
mkdir -p lib
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel ./lib/FreeRTOS-Kernel
# or add as submodule
git submodule add https://github.com/FreeRTOS/FreeRTOS-Kernel lib/FreeRTOS-Kernel

mkdir -p include/FreeRTOS
```

#### Common error
```sh
/Users/bo/.pico-sdk/toolchain/14_2_Rel1/bin/../lib/gcc/arm-none-eabi/14.2.1/../../../../arm-none-eabi/bin/ld: CMakeFiles/PISO_HID.dir/lib/FreeRTOS-Kernel/portable/MemMang/heap_4.c.o: in function `pvPortMalloc':
/Users/bo/sandbox/RaspberryPiPico/SDK/PISO_HID/lib/FreeRTOS-Kernel/portable/MemMang/heap_4.c:340:(.text.pvPortMalloc+0x82): undefined reference to `vApplicationMallocFailedHook'
collect2: error: ld returned 1 exit status
```

```c
// add this function to your source code
#include <FreeRTOS.h>
void vApplicationMallocFailedHook(void)
{
    // This runs if pvPortMalloc() fails
    printf("ERROR: FreeRTOS malloc failed!\n");
    portDISABLE_INTERRUPTS();
    for(;;); // Halt
}
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    // This runs if Stack overflow
    printf("ERROR: Stack overflow!\n");
    portDISABLE_INTERRUPTS();
    for(;;); // Halt
}
// or 
void vApplicationMallocFailedHook(void) { panic("Malloc failed!"); }
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) { panic("Stack overflow!"); }

// or Disable the hook
// FreeRTOSConfig.h
#define configUSE_MALLOC_FAILED_HOOK 0
```

License
-