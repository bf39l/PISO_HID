#ifndef __FREERTOS_CONFIG_H__
#define __FREERTOS_CONFIG_H__

#include <assert.h>

/*-----------------------------------------------------------
 * Application specific definitions for RP2040
 *----------------------------------------------------------*/

/* Scheduler configuration */
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configTICK_RATE_HZ                      (1000)    // 1 ms tick
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                256
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

/* Memory allocation */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   (128*1024)
#define configAPPLICATION_ALLOCATED_HEAP        0

/* Hooks */
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* Synchronization */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    1

/* Software timers */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            1024

/* RP2040 specific */
#define configSUPPORT_PICO_SYNC_INTEROP         1
#define configSUPPORT_PICO_TIME_INTEROP         1

/* Assertions */
#define configASSERT(x)                         assert(x)

/* Include or exclude API functions */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_xQueueGetMutexHolder            1

// With current FreeRTOS, can only use one core
/* RP2040 SMP configuration (enable only if using FreeRTOS SMP port) */
#ifdef FREE_RTOS_KERNEL_SMP   /* set by the RP2040 SMP port of FreeRTOS */
#define configNUMBER_OF_CORES       2
#define configNUM_CORES             2       /* number of cores on RP2040 */
#define configTICK_CORE             0       /* core for the tick interrupt */
#define configRUN_MULTIPLE_PRIORITIES 1     /* allow same-priority tasks on multiple cores */
#define configUSE_CORE_AFFINITY     1       // Tasks can run on any available core by default; use vTaskCoreAffinitySet() to pin if needed.
#define configUSE_PASSIVE_IDLE_HOOK 0
#define configUSE_IDLE_HOOK         0
#define configUSE_TICK_HOOK         0
#endif

#define configGENERATE_RUN_TIME_STATS 0
#define configUSE_TRACE_FACILITY      1

#endif /* __FREERTOS_CONFIG_H__ */