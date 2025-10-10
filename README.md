# PISO HID

## FreeRTOS

```bash
mkdir -p lib
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel ./lib/FreeRTOS-Kernel
# or add as submodule
git submodule add https://github.com/FreeRTOS/FreeRTOS-Kernel lib/FreeRTOS-Kernel

mkdir -p include/FreeRTOS
```

### Common error
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