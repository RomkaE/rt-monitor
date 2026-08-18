
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>

#include "rtmon_config.h"
#include "rtmon.h"
#include "private/term_profile.h"
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
  #define COL_LOAD_W   "7"    // "%4u" + '.' + 1 digit + '%'
#elif SCALE == 100
  #define PREFIX_FRACT "02"
  #define COL_LOAD_W   "8"
#elif SCALE == 1000
  #define PREFIX_FRACT "03"
  #define COL_LOAD_W   "9"
#else
  #error "Unsupported RTMON_CFG_PERCENT_SCALE value"
#endif

#if (configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS)
  #define PRI_RUN_TIME     PRIu16
#elif (configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS)
  #define PRI_RUN_TIME     PRIu32
#elif (configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_64_BITS)
  #define PRI_RUN_TIME     PRIu64
#else
  #error "Unsupported configRUN_TIME_TYPE_WIDTH"
#endif

#if RTMON_CFG_LINE_BUFF_SIZE < 32
  #error RTMON_CFG_LINE_BUFF_SIZE cannot be less than 32
#endif

// s_Order keeps task indices as uint8_t:
#if RTMON_CFG_TASKS_MAX_COUNT > 255
  #error RTMON_CFG_TASKS_MAX_COUNT cannot be greater than 255
#endif

#define PREFIX_SIZE     ( sizeof(TERM_LINE_PREFIX) - 1 )
#define BUF_SIZE        ( PREFIX_SIZE + RTMON_CFG_LINE_BUFF_SIZE)

// Table layout: fixed column widths, no tabs (a tab jumps to the next 8-column
// stop, so the columns drift with the number of digits printed).
// NAME(16, left) STACK(6) LOAD(COL_LOAD_W) PRIO(6) 2 spaces STATE(left)
#define ROW_FMT     "%-16s%6u%4"PRIu16".%"PREFIX_FRACT PRIu16"%%%6u  %s"
#define HEAD_FMT    "%-16s%6s%"COL_LOAD_W"s%6s  %s"

#define TABLE_LINE_DASH   "--------------------------------------------"
#define TABLE_LINE_EQ     "============================================"

static TaskStatus_t s_Tasks[RTMON_CFG_TASKS_MAX_COUNT];
static uint8_t s_Order[RTMON_CFG_TASKS_MAX_COUNT];

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
  static char s_Buf[BUF_SIZE] = TERM_LINE_PREFIX;
  char *const line = &s_Buf[PREFIX_SIZE];
  const int size = RTMON_CFG_LINE_BUFF_SIZE;

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
  uint64_t load = (uint64_t)_busy * SCALE * 100;
  load = load + _elapsed / 2;
  load = load / _elapsed;
  return (uint16_t)load;
}

static void sort_by_task_number(UBaseType_t _count)
{
  for (UBaseType_t i = 0; i < _count; i++)
    s_Order[i] = (uint8_t)i;

  for (UBaseType_t i = 1; i < _count; i++)
  {
    const uint8_t task_idx = s_Order[i];
    const UBaseType_t task_num = s_Tasks[task_idx].xTaskNumber;

    UBaseType_t j = i;
    while (j > 0 && s_Tasks[s_Order[j - 1]].xTaskNumber > task_num)
    {
      s_Order[j] = s_Order[j - 1];
      j--;
    }
    s_Order[j] = task_idx;
  }
}

static configRUN_TIME_COUNTER_TYPE tasks_stats(configRUN_TIME_COUNTER_TYPE _elapsed, uint16_t *_p_load_acc)
{
  if (_elapsed == 0)
    return 0;

  UBaseType_t task_count;
  task_count = uxTaskGetSystemState(s_Tasks, RTMON_CFG_TASKS_MAX_COUNT, NULL);
  sort_by_task_number(task_count);

  uint16_t load_acc = 0;
  configRUN_TIME_COUNTER_TYPE run_time = 0;
  for (UBaseType_t i = 0; i < task_count; i++)
  {
    TaskStatus_t *task = &s_Tasks[s_Order[i]];

    uint16_t load = calc_load(task->ulRunTimeCounter, _elapsed);

    print(ROW_FMT, task->pcTaskName, task->usStackHighWaterMark,
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
    print(TERM_FRAME_BEGIN);

    // Header:
    print(TERM_EMPH_ON HEAD_FMT TERM_EMPH_OFF,
                "TASK", "STACK", "LOAD", "Prio", "State");
    print(TABLE_LINE_DASH);

    uint16_t load_acc;
    configRUN_TIME_COUNTER_TYPE run_time;
    run_time = tasks_stats(elapsed, &load_acc);

    // Separator:
    print(TABLE_LINE_EQ);

    // CPU load:
    uint16_t load = calc_load(run_time, elapsed);
    print(TERM_EMPH_ON"CPU load:\t%"PRIu16".%"PREFIX_FRACT"u%%"TERM_EMPH_OFF,
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
    print(TERM_FRAME_END);

    // Delay:
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(RTMON_CFG_UPDATE_PERIOD_MS));
  }
}

void rtmon_Init(void)
{
  rtmon_portInit();
  
#if RTMON_CFG_USE_STATIC_ALOCATION
  TaskHandle_t th = xTaskCreateStatic(Thread, "SMON", RTMON_CFG_TASK_STACK_DEPTH,
     NULL, RTMON_CFG_TASK_PRIO, uxRtMonTaskStack, &xRtMonTaskTCB);
  assert(th != NULL);
#else
  BaseType_t res = xTaskCreate(Thread, "SMON", RTMON_CFG_TASK_STACK_DEPTH,
     NULL, RTMON_CFG_TASK_PRIO, NULL);
  assert(res == pdPASS);
#endif
}
