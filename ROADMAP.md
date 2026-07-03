# Roadmap

Detailed design for each feature lives under `plans/`.

---

## Done

- [x] PISO keyboard scan, layers, Mod-Tap, NKRO/6KRO
- [x] TinyUSB HID composite + CDC serial
- [x] FreeRTOS tasking (scan, USB, OLED)
- [x] OLED status display + Bongo Cat animation
- [x] Host OS detection (Linux / Windows / macOS)
- [x] OS-aware keys (OSK) — runtime keycode resolution per detected OS

---

## Next: Host-Driven Keymap Remapping

Full spec: [plans/change-keymapping.md](plans/change-keymapping.md)

### Phase 1: Flash read/write module
- [ ] `lib/CustomHID/keymap_flash.c` — flash save/load, CRC32, magic validation
- [ ] `keymap_init()` calls `keymap_flash_load()` to overlay factory defaults

### Phase 2: On-device CDC protocol handler
- [ ] `lib/CustomHID/cdc_protocol.c` — command parser, dispatch, response formatter
- [ ] Commands: `PING`, `VERSION`, `KEYMAP_GET`, `KEYMAP_SET`, `KEYMAP_DUMP`, `KEYMAP_SAVE`, `KEYMAP_RESET`, `KEYMAP_RELOAD`

### Phase 3: Integrate into USB_Task
- [ ] Poll `tud_cdc_available()` after `tud_task()` in `src/task_usb.c`
- [ ] Feed incoming bytes to `cdc_protocol_process()`

### Phase 4: Host CLI tool
- [ ] `host/piso_cli/` — Python CLI for connect, dump, load, save, reset
- [ ] Auto-detect CDC port by VID/PID
- [ ] JSON export/import of keymap

### Phase 5: Testing & hardening
- [ ] Boot with empty / valid / corrupted flash
- [ ] End-to-end dump → edit → load → save cycle
- [ ] Concurrent typing + commands, no key events lost

---

## Future

- [ ] Web-based keymap editor
- [ ] Multiple keymap profiles
- [ ] OTA firmware updates
