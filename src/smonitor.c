
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "sys_monitor_cfg.h"
#include "smonitor.h"
#include "inc/terminal.h"
#include "port/inc/port.h"
#include "log/log.h"

// FreeRTOS:
#include "FreeRTOS.h"
#include "task.h"

#define SCALE    SYS_MON_PERCENT_SCALE
#if SCALE == 10
  #define PREFIX_FRACT "01"
#elif SCALE == 100
  #define PREFIX_FRACT "02"
#elif SCALE == 1000
  #define PREFIX_FRACT "03"
#else
  #error "Unsupported SYS_MON_PERCENT_SCALE value"
#endif

#if (configRUN_TIME_TYPE_WIDTH == TICK_TYPE_WIDTH_16_BITS)
  #define PRI_FRACT     PRIu16
#elif (configRUN_TIME_TYPE_WIDTH == TICK_TYPE_WIDTH_32_BITS)
  #define PRI_FRACT     PRIu32
#elif (configRUN_TIME_TYPE_WIDTH == TICK_TYPE_WIDTH_64_BITS)
  #define PRI_FRACT     PRIu64
#else
  #error "Unsupported configRUN_TIME_TYPE_WIDTH"
#endif

static char s_Buf[SYS_MONITOR_BUFF_SIZE];
static uint16_t s_Len;

static TaskStatus_t s_Tasks[SMON_TASKS_MAX_COUNT];

static const char *s_TaskState[] = {
  [eRunning]    "Run",
  [eReady]      "Ready",
  [eBlocked]    "Block",
  [eSuspended]  "Suspend",
  [eDeleted]    "Del", "Unknown" };

void smon_printf(const char* format_msg, ...)
{
  char line[SYS_MON_LINE_BUFF_SIZE];
  const size_t size = SYS_MON_LINE_BUFF_SIZE;
  int len = 0;

  // Message:
  va_list p_args;
  va_start(p_args, format_msg);
  len = vsnprintf(line, size - len, format_msg, p_args);
  va_end(p_args);

  // Check and write:
  if (len > 0)
  {
    // Check and fix to max length:
    if (len > size)
      len = size;

    // Add \r\n:
    if (size - len < 2)
      len = size - 2;
    line[len++] = '\r';
    line[len++] = '\n';

    // Write to buffer:
    uint16_t avaliable = SYS_MONITOR_BUFF_SIZE - s_Len;
    if (len > avaliable)
    {
      LOG_WARNING("[SMON] Buffer overflow!");
      len = avaliable;  
    }
    memcpy(&s_Buf[s_Len], line, len);
    s_Len += len;
  }
}

uint16_t calc_load(configRUN_TIME_COUNTER_TYPE _busy, configRUN_TIME_COUNTER_TYPE _elapsed)
{
  uint32_t load = (uint32_t)_busy * SCALE * 100;
  load = load + _elapsed / 2;
  load = load / _elapsed;
  return (uint16_t)load;
}

configRUN_TIME_COUNTER_TYPE tasks_stats(configRUN_TIME_COUNTER_TYPE _elapsed, uint16_t *_p_load_acc)
{
  if (_elapsed == 0)
    return 0;

  UBaseType_t task_count;
  task_count = uxTaskGetSystemState(s_Tasks, SMON_TASKS_MAX_COUNT, NULL);

  uint16_t load_acc = 0;
  configRUN_TIME_COUNTER_TYPE run_time = 0;
  for (UBaseType_t i = 0; i < task_count; i++)
  {
    TaskStatus_t *task = &s_Tasks[i];

    // TODO add sort by xTaskNumber:
    uint16_t load = calc_load(task->ulRunTimeCounter, _elapsed);

    smon_printf("%s\t%u\t%2"PRIu16".%"PREFIX_FRACT""PRIu16"%%\t %u\t:%s",
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

void Thread(void *pvParameters)
{
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
    smon_printf(CLEARSCR);
    smon_printf(GOTOYX, 0, 0);

    // Header:
    smon_printf(BOLD"TASK\tSTACK\tLOAD\tPrior.\tState"NORMAL
                "\r\n----------------------------------------");

    uint16_t load_acc;
    configRUN_TIME_COUNTER_TYPE run_time;
    run_time = tasks_stats(elapsed, &load_acc);

    // Separator:
    smon_printf("========================================\r\n");

    // CPU load:
    uint16_t load = calc_load(run_time, elapsed);
    smon_printf(BOLD"CPU load:\t%"PRIu16".%"PREFIX_FRACT"u%%"NORMAL,
                load / SCALE, load % SCALE);

    // DEBUG:
    #if SYS_MON_VIEW_DEBUG_INFO
    {
      smon_printf("ACC load:\t%"PRIu16".%"PREFIX_FRACT""PRIu16"%%",
                  load_acc / SCALE, load_acc % SCALE);

      int16_t load_err = load - load_acc;
      bool sign = load_err < 0;
      if (sign) load_err = -load_err;
      smon_printf("Err load:\t%s%"PRIu16".%"PREFIX_FRACT""PRIu16"%%", sign ? "-" : "",
                  load_err / SCALE, load_err % SCALE);

      smon_printf("Elapsed: \t%"PRI_FRACT" cnt", elapsed);

      uint16_t remaining = SYS_MONITOR_BUFF_SIZE - s_Len;
      smon_printf("Free in buf:\t%"PRIu16" bytes", remaining);
    }
    #endif /* SYS_MON_VIEW_DEBUG_INFO */

    #if configSUPPORT_DYNAMIC_ALLOCATION
    {
      smon_printf("FREE HEAP:\t%u\r\n", xPortGetFreeHeapSize());
      smon_printf("MIN HEAP:\t%u\r\n", xPortGetMinimumEverFreeHeapSize());
    }
    #endif /* configSUPPORT_DYNAMIC_ALLOCATION */

    // Send:
    portSysMonitor_TxBuff(s_Buf, s_Len);
    s_Len = 0;

    // Delay:
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SYS_MONITOR_UPDATE_PERIOD_MS));
  }
}

void smonitor_Init(void)
{
  static StaticTask_t xSMonTaskTCB;
  static StackType_t uxSMonTaskStack[SMON_TASK_STACK_DEPTH];

  portSysMonitor_Init();
  xTaskCreateStatic(Thread, "SMON", SMON_TASK_STACK_DEPTH,
     NULL, configMAX_PRIORITIES - 1, uxSMonTaskStack, &xSMonTaskTCB);
}
