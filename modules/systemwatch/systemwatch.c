#include "systemwatch.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tim.h"
#include <stdint.h>
#include <string.h>

#define LOG_TAG  "systemwatch"
#include "elog.h"

// 静态变量
static TaskMonitor_t taskList[MAX_MONITORED_TASKS];
static uint8_t taskCount = 0;
static osThreadId_t watchTaskHandle;
static volatile uint32_t watch_task_last_active = 0;

// 辅助函数声明
static void PrintTaskInfo(TaskStatus_t *pxTaskStatus, TaskMonitor_t *pxTaskMonitor);
static void PrintSystemStatus(void);
static const char* GetTaskStateString(eTaskState state);

static void SystemWatch_Task(void *argument)
{
    UNUSED(argument);
    uint32_t lastCounter[MAX_MONITORED_TASKS] = {0};
    
    while(1) {
        watch_task_last_active = HAL_GetTick();

        // 检查所有任务
        for(uint8_t i = 0; i < taskCount; i++) {
            if(taskList[i].isActive) {
                // 检查计数器是否更新
                if(lastCounter[i] == taskList[i].counter) {
                    taskENTER_CRITICAL();
                    
                    // 打印系统状态头部
                    log_e("\r\n**** Task Blocked Detected! System State Dump ****");
                    log_e("Time: %lu ms", HAL_GetTick());
                    log_e("----------------------------------------");
                    
                    // 获取并打印阻塞任务信息
                    TaskStatus_t taskStatus;
                    vTaskGetInfo(taskList[i].handle,
                               &taskStatus,
                               pdTRUE,
                               eInvalid);
                    
                    log_e("Blocked Task Information:");
                    PrintTaskInfo(&taskStatus, &taskList[i]);
                    
                    // 打印系统整体状态
                    PrintSystemStatus();
                    
                    taskEXIT_CRITICAL();
                    HAL_NVIC_SystemReset();
                }
                lastCounter[i] = taskList[i].counter;
            }
        }
        osDelay(MONITOR_PERIOD);
    }
}

// 定时器中断回调，用于监控 SystemWatch 任务本身
void sysytemwatch_it_callback(void){
    if(xTaskGetTickCount() - watch_task_last_active > MONITOR_PERIOD * 5) {
        taskENTER_CRITICAL();
        
        log_e("\r\n**** SystemWatch Task Blocked! ****");
        log_e("Last active: %lu ms ago", xTaskGetTickCount() - watch_task_last_active);
        PrintSystemStatus();
                
        taskEXIT_CRITICAL();
        HAL_NVIC_SystemReset();
    }
}


void SystemWatch_Init(void)
{
    HAL_TIM_Base_Start_IT(&htim6);
    memset(taskList, 0, sizeof(taskList));
    
    const osThreadAttr_t watchTask_attributes = {
        .name = "WatchTask",
        .stack_size = 256 * 4,
        .priority = (osPriority_t)osPriorityHigh,
    };
    
    watchTaskHandle = osThreadNew(SystemWatch_Task, NULL, &watchTask_attributes);
    log_i("SystemWatch initialized, watch task created.");
}

static void PrintTaskInfo(TaskStatus_t *pxTaskStatus, TaskMonitor_t *pxTaskMonitor)
{
    log_e("Name: %s", pxTaskMonitor->name);
    log_e("Handle: 0x%x", (unsigned int)pxTaskMonitor->handle);
    log_e("Counter: %lu", pxTaskMonitor->counter);
    log_e("Stack HWM: %lu words (%lu bytes)", 
          pxTaskStatus->usStackHighWaterMark,
          pxTaskStatus->usStackHighWaterMark * sizeof(StackType_t));
    log_e("Base Priority: %lu", pxTaskStatus->uxBasePriority);
    log_e("Current Priority: %lu", pxTaskStatus->uxCurrentPriority);
    log_e("State: %s", GetTaskStateString(pxTaskStatus->eCurrentState));
}

static void PrintSystemStatus(void)
{
    log_e("\r\nSystem Status:");
    log_e("----------------------------------------");
    
    // 内存使用情况
    log_e("Memory Usage:");
    log_e("- Free Heap: %u bytes", xPortGetFreeHeapSize());
    log_e("- Minimum Ever Free: %u bytes", xPortGetMinimumEverFreeHeapSize());
    // 所有任务状态
    log_e("\r\nAll Tasks Status:");
    for(uint8_t i = 0; i < taskCount; i++) {
        TaskStatus_t xTaskDetails;
        
        vTaskGetInfo(taskList[i].handle,
                    &xTaskDetails,
                    pdTRUE,
                    eInvalid);
        
        log_e("\r\nTask %d:", i);
        PrintTaskInfo(&xTaskDetails, &taskList[i]);
    }
    log_e("----------------------------------------\r\n");
}

static const char* GetTaskStateString(eTaskState state)
{
    switch (state) {
        case eRunning:   return "Running";
        case eReady:     return "Ready";
        case eBlocked:   return "Blocked";
        case eSuspended: return "Suspended";
        case eDeleted:   return "Deleted";
        default:         return "Unknown";
    }
}

int8_t SystemWatch_RegisterTask(osThreadId_t taskHandle, const char* taskName)
{
    if(taskCount >= MAX_MONITORED_TASKS) {
        return -1;
    }
    
    taskList[taskCount].handle = taskHandle;
    taskList[taskCount].name = taskName;
    taskList[taskCount].counter = 0;
    taskList[taskCount].isActive = 1;
    taskCount++;

    log_i( "Task %s (handle: 0x%x) registered for monitoring.", 
           taskName, 
           (unsigned int)taskHandle);
    return 0;
}

void SystemWatch_ReportTaskAlive(osThreadId_t taskHandle)
{
    for(uint8_t i = 0; i < taskCount; i++) {
        if(taskList[i].handle == taskHandle) {
            taskList[i].counter++;
            break;
        }
    }
}
