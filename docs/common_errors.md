# Common Errors & Troubleshooting

## Build Errors

### `fatal error: tusb_config.h: No such file or directory`
- Ensure the include path adds `lib/CustomHID`, not the file itself:
  ```cmake
  target_include_directories(CUSTOM_HID PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lib/CustomHID)
  ```

### `implicit declaration errors` when splitting files
- Add a forward declaration in the `.c` file or a private header; don't rely on implicit prototypes.

### FreeRTOS Hooks — `undefined reference to 'vApplicationMallocFailedHook'`
```sh
/Users/bo/.pico-sdk/toolchain/14_2_Rel1/bin/../lib/gcc/arm-none-eabi/14.2.1/../../../../arm-none-eabi/bin/ld: CMakeFiles/PISO_HID.dir/lib/FreeRTOS-Kernel/portable/MemMang/heap_4.c.o: in function `pvPortMalloc':
/Users/bo/sandbox/RaspberryPiPico/SDK/PISO_HID/lib/FreeRTOS-Kernel/portable/MemMang/heap_4.c:340:(.text.pvPortMalloc+0x82): undefined reference to `vApplicationMallocFailedHook'
collect2: error: ld returned 1 exit status
```

**Fix — provide the hooks in your source code:**
```c
#include <FreeRTOS.h>

void vApplicationMallocFailedHook(void)
{
    printf("ERROR: FreeRTOS malloc failed!\n");
    portDISABLE_INTERRUPTS();
    for(;;); // Halt
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("ERROR: Stack overflow!\n");
    portDISABLE_INTERRUPTS();
    for(;;); // Halt
}
```

**Or use minimal stubs:**
```c
void vApplicationMallocFailedHook(void) { panic("Malloc failed!"); }
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) { panic("Stack overflow!"); }
```

**Or disable the hook in `FreeRTOSConfig.h`:**
```c
#define configUSE_MALLOC_FAILED_HOOK 0
```

## CI / Release Errors

### `"Resource not accessible by integration"` on release
- Grant Actions "Read and write" permissions in: Settings → Actions → Workflow permissions.
- Or use a PAT secret in the release step.

## Config Notes

- `MT_TAP_TIMEOUT_MS` controls tap-vs-hold for MT keys and influences OLED update cadence.
- FreeRTOS hooks: `vApplicationMallocFailedHook` / `vApplicationStackOverflowHook` must be provided or disabled.
