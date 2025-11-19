
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>

#include "rtmon_config.h"
#include "rtmon.h"
#include "private/terminal.h"
#include "port/port.h"

// FreeRTOS:
#ifndef ESP_PLATFORM
  #include "FreeRTOS.h"
  #include "task.h"
#else
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
#endif

#define SCALE    RTMON_CFG_PERCENT_SCALE
#if SCALE == 10
  #define PREFIX_FRACT "01"
#elif SCALE == 100
  #define PREFIX_FRACT "02"
#elif SCALE == 1000
  #define PREFIX_FRACT "03"
#else
  #error "Unsupported RTMON_CFG_PERCENT_SCALE value"
#endif

#if (configRUN_TIME_TYPE_WIDTH == TICK_TYPE_WIDTH_16_BITS)
  #define PRI_RUN_TIME     PRIu16
#elif (configRUN_TIME_TYPE_WIDTH == TICK_TYPE_WIDTH_32_BITS)
  #define PRI_RUN_TIME     PRIu32
#elif (configRUN_TIME_TYPE_WIDTH == TICK_TYPE_WIDTH_64_BITS)
  #define PRI_RUN_TIME     PRIu64
#else
  #error "Unsupported configRUN_TIME_TYPE_WIDTH"
#endif

#if RTMON_CFG_LINE_BUFF_SIZE < 32
  #error RTMON_CFG_LINE_BUFF_SIZE cannot be less than 32
#endif

#define PREFIX_SIZE     ( sizeof(CLEAREOL) - 1 )
#define BUF_SIZE        ( PREFIX_SIZE + RTMON_CFG_LINE_BUFF_SIZE)

static TaskStatus_t s_Tasks[RTMON_CFG_TASKS_MAX_COUNT];

static const char *s_TaskState[] = {
  [eRunning]    "Run",
  [eReady]      "Ready",
  [eBlocked]    "Block",
  [eSuspended]  "Suspend",
  [eDeleted]    "Del", "Unknown" };

#if RTMON_CFG_USE_STATIC_ALOCATION
static StaticTask_t xRtMonTaskTCB;
static StackType_t uxRtMonTaskStack[RTMON_CFG_TASK_STACK_DEPTH];
#endif

static void print(const char* format_msg, ...)
{
  static char s_Buf[BUF_SIZE] = CLEAREOL;
  char *const line = &s_Buf[PREFIX_SIZE];
  const size_t size = RTMON_CFG_LINE_BUFF_SIZE;

  // Message:
  va_list p_args;
  va_start(p_args, format_msg);
  int len = vsnprintf(line, size, format_msg, p_args);
  va_end(p_args);

  // Check and write:
  if (len > 0)
  {
    // Check and fix to max length:
    if (len + 2 > size)
    {
      memcpy(&line[size - 5], "...\r\n", 5);
      len = size;
    }
    else
    {
      line[len++] = '\r';
      line[len++] = '\n';
    }

    // Send:
    rtmon_xmitBuf(s_Buf, PREFIX_SIZE + len);
  }
}

static uint16_t calc_load(configRUN_TIME_COUNTER_TYPE _busy, configRUN_TIME_COUNTER_TYPE _elapsed)
{
  uint32_t load = (uint32_t)_busy * SCALE * 100;
  load = load + _elapsed / 2;
  load = load / _elapsed;
  return (uint16_t)load;
}

static configRUN_TIME_COUNTER_TYPE tasks_stats(configRUN_TIME_COUNTER_TYPE _elapsed, uint16_t *_p_load_acc)
{
  if (_elapsed == 0)
    return 0;

  UBaseType_t task_count;
  task_count = uxTaskGetSystemState(s_Tasks, RTMON_CFG_TASKS_MAX_COUNT, NULL);

  uint16_t load_acc = 0;
  configRUN_TIME_COUNTER_TYPE run_time = 0;
  for (UBaseType_t i = 0; i < task_count; i++)
  {
    TaskStatus_t *task = &s_Tasks[i];

    // TODO add sort by xTaskNumber:
    uint16_t load = calc_load(task->ulRunTimeCounter, _elapsed);

    print("%-16s%u\t%2"PRIu16".%"PREFIX_FRACT""PRIu16"%%\t %u\t:%s",
                task->pcTaskName, task->usStackHighWaterMark,
                load / SCALE, load % SCALE,
                task->uxCurrentPriority, s_TaskState[task->eCurrentState]);

    // TODO - except IDLE task:
    if (task->uxCurrentPriority == 0)
      continue;

    load_acc += load;
    run_time += task->ulRunTimeCounter;
  }

  *_p_load_acc = load_acc;
  return run_time;
}

static void Thread(void *pvParameters)
{
  (void)pvParameters;
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

  configRUN_TIME_COUNTER_TYPE recent = 0;
  while (1)
  {
    // Time interval measurement:
    configRUN_TIME_COUNTER_TYPE now, elapsed;
    now = portGET_RUN_TIME_COUNTER_VALUE();
    elapsed = now - recent;
    recent = now;

    // Clear screen:
    print(CLEAREOS GOTOYX, 0, 0);

    // Header:
    print(BOLD"TASK\tSTACK\tLOAD\tPrior.\tState"NORMAL);
    print("----------------------------------------");

    uint16_t load_acc;
    configRUN_TIME_COUNTER_TYPE run_time;
    run_time = tasks_stats(elapsed, &load_acc);

    // Separator:
    print("========================================");

    // CPU load:
    uint16_t load = calc_load(run_time, elapsed);
    print(BOLD"CPU load:\t%"PRIu16".%"PREFIX_FRACT"u%%"NORMAL,
                load / SCALE, load % SCALE);

    // DEBUG:
    #if RTMON_CFG_VIEW_DEBUG_INFO
    {
      print("ACC load:\t%"PRIu16".%"PREFIX_FRACT""PRIu16"%%",
                  load_acc / SCALE, load_acc % SCALE);

      int16_t load_err = load - load_acc;
      bool sign = load_err < 0;
      if (sign) load_err = -load_err;
      print("Err load:\t%s%"PRIu16".%"PREFIX_FRACT""PRIu16"%%", sign ? "-" : "",
                  load_err / SCALE, load_err % SCALE);

      print("Elapsed: \t%"PRI_RUN_TIME" cnt", elapsed);
    }
    #endif /* RTMON_CFG_VIEW_DEBUG_INFO */

    #if configSUPPORT_DYNAMIC_ALLOCATION
    {
      print("FREE HEAP:\t%u", xPortGetFreeHeapSize());
      print("MIN HEAP:\t%u", xPortGetMinimumEverFreeHeapSize());
    }
    #endif /* configSUPPORT_DYNAMIC_ALLOCATION */

    // Clear end of screen:
    print(CLEAREOS);

    // Delay:
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(RTMON_CFG_UPDATE_PERIOD_MS));
  }
}

void rtmon_Init(void)
{
  rtmon_portInit();
  
#if RTMON_CFG_USE_STATIC_ALOCATION
  TaskHandle_t th = xTaskCreateStatic(Thread, "SMON", RTMON_CFG_TASK_STACK_DEPTH,
     NULL, configMAX_PRIORITIES - 1, uxRtMonTaskStack, &xRtMonTaskTCB);
  assert(th != NULL);
#else
  BaseType_t res = xTaskCreate(Thread, "SMON", RTMON_CFG_TASK_STACK_DEPTH,
     NULL, configMAX_PRIORITIES - 1, NULL);
  assert(res == pdPASS);
#endif
}
