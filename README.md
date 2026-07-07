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
- lib/tinyusb/ … vendored TinyUSB with host OS detection hook
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
- Re-push an existing tag to trigger a fresh CI build (e.g. after fixing a build error):
  - git tag -d v0.0.5
  - git push origin :refs/tags/v0.0.5
  - git tag v0.0.5
  - git push origin v0.0.5
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
- See [docs/common_errors.md](docs/common_errors.md) for build errors, CI/release issues, and config notes.

### Vendored TinyUSB with Host OS Detection

This project vendors [TinyUSB 0.21.0](https://github.com/hathach/tinyusb) under `lib/tinyusb/` rather than using the SDK copy. The vendored copy has a minimal two-line patch to `src/device/usbd.c` that enables host OS detection on plug-in.

**Why vendor instead of patching the SDK?**

The Pico SDK's `PICO_TINYUSB_PATH` variable lets you override the entire TinyUSB tree. There's no supported way to override a single source file, so we vendor the full tree and set `PICO_TINYUSB_PATH` in `CMakeLists.txt` before `pico_sdk_init()`. This keeps the project self-contained and avoids modifying SDK files.

**How the OS detection works**

Different host OSes (Windows, macOS, Linux) request USB device descriptors with different `wLength` values during enumeration. By intercepting these values, we can fingerprint the connected host:

- **Linux**: requests `0xFF` (255 bytes) for every descriptor
- **Windows**: requests `0xFF` then `0x02` (2 bytes) in a specific pattern
- **macOS**: requests `0x04` (4 bytes) followed by `0xFF`

**The hack — two lines in `lib/tinyusb/src/device/usbd.c`**

> **Line 88** — weak callback stub (fires for every `GET_DESCRIPTOR` request):
```c
TU_ATTR_WEAK void tud_os_detect_wlength_cb(uint16_t wlength) {
  (void) wlength;
}
```

> **Line 1313** — inside `process_get_descriptor()`, before the descriptor-type switch:
```c
tud_os_detect_wlength_cb(p_request->wLength);
```

Our `src/os_detection.c` defines a strong override of `tud_os_detect_wlength_cb()`, collecting the wLength values and matching them against known patterns. The detected OS is displayed on the OLED (`LNX`, `MAC`, or `WIN`).

**Why isn't this in upstream TinyUSB?**

wLength fingerprinting is a heuristic, not a USB specification feature. The values are incidental behavior of each OS's USB stack and can change with updates. Upstream TinyUSB stays spec-compliant and minimal. This approach is adapted from [QMK's OS detection](https://github.com/qmk/qmk_firmware), which uses the same technique for keyboards that need OS-aware behavior.

**Updating TinyUSB**

To update to a newer TinyUSB version:
```bash
# Download latest release
curl -sL https://github.com/hathach/tinyusb/archive/refs/tags/<VERSION>.tar.gz -o tinyusb.tar.gz
tar xzf tinyusb.tar.gz

# Replace vendored sources, preserving our patch
rm -rf lib/tinyusb/src lib/tinyusb/hw
cp -r tinyusb-<VERSION>/src lib/tinyusb/
cp -r tinyusb-<VERSION>/hw lib/tinyusb/

# Re-apply the OS detection hook in src/device/usbd.c
# (see existing patch for reference)
```

### FreeRTOS

```bash
mkdir -p lib
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel ./lib/FreeRTOS-Kernel
# or add as submodule
git submodule add https://github.com/FreeRTOS/FreeRTOS-Kernel lib/FreeRTOS-Kernel

mkdir -p include/FreeRTOS
```

For common FreeRTOS hook errors, see [docs/common_errors.md](docs/common_errors.md).

License
-

Acknowledgments
- This project was developed with the assistance of **Qwen 3.6** (local LLM) and **GitHub Copilot**.
- Original idea of a keyboard using PISO shift registers from [HelloWord-Keyboard](https://github.com/peng-zhihui/HelloWord-Keyboard).
- OLED SSD1306 driver derived from [led.baud-dance.com](https://led.baud-dance.com).
- Bongo Cat animation ported from [OLED-BongoCat-Revision](https://github.com/pedker/OLED-BongoCat-Revision).
- Powered by [FreeRTOS](https://github.com/FreeRTOS/FreeRTOS-Kernel).
- USB stack via [TinyUSB](https://github.com/hathach/tinyusb).
- OS detection approach adapted from [QMK](https://github.com/qmk/qmk_firmware).