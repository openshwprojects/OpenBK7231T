/***********************************************************
*  File: uni_semaphore.c
*  Author: nzy
*  Date: 120427
***********************************************************/
#define _UNI_SEMAPHORE_GLOBAL
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "tuya_hal_semaphore.h"
#include "tuya_hal_system_internal.h"
#include "../errors_compat.h"
#define FALSE    0
#define TRUE     (!FALSE)
/***********************************************************
*************************micro define***********************
***********************************************************/

#if 0
#define LOGD PR_DEBUG
#define LOGT PR_TRACE
#define LOGN PR_NOTICE
#define LOGE PR_ERR
#else
#include "uart_pub.h"
#define LOGD(...) bk_printf("[SEM DEBUG]" __VA_ARGS__)
#define LOGT(...) bk_printf("[SEM TRACE]" __VA_ARGS__)
#define LOGN(...) bk_printf("[SEM NOTICE]" __VA_ARGS__)
#define LOGE(...) bk_printf("[SEM ERROR]" __VA_ARGS__)
#endif



typedef struct
{
    xSemaphoreHandle sem;
}SEM_MANAGE,*P_SEM_MANAGE;


/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: CreateSemaphore 创建一个信号量
*  Input: reqSize->申请的内存大小
*  Output: none
*  Return: NULL失败，非NULL成功
*  Date: 120427
***********************************************************/
static SEM_HANDLE CreateSemaphore(VOID)
{
    P_SEM_MANAGE pSemManage;
    
    pSemManage = (P_SEM_MANAGE)tuya_hal_internal_malloc(sizeof(SEM_MANAGE));

    return (SEM_HANDLE)pSemManage;
}

/***********************************************************
*  Function: InitSemaphore
*  Input: semHandle->信号量操作句柄
*         semCnt
*         sem_max->no use for linux os
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
static int InitSemaphore(const SEM_HANDLE semHandle, const uint32_t semCnt,\
                                 const uint32_t sem_max)
{
    if(!semHandle)
        return OPRT_INVALID_PARM;
        
    P_SEM_MANAGE pSemManage;
    pSemManage = (P_SEM_MANAGE)semHandle;

    pSemManage->sem = xSemaphoreCreateCounting(sem_max, semCnt);
    if(NULL == pSemManage->sem) {
        return OPRT_INIT_SEM_FAILED;
    }

    return OPRT_OK;
}

int tuya_hal_semaphore_create_init(SEM_HANDLE *pHandle, const uint32_t semCnt, \
                          const uint32_t sem_max)
{
    if(NULL == pHandle)
    {
        LOGE("invalid param\n");
        return OPRT_INVALID_PARM;
    }

    *pHandle = CreateSemaphore();
    if(*pHandle == NULL)
    {
        LOGE("malloc fails\n");
        return OPRT_MALLOC_FAILED;
    }

    int ret = InitSemaphore(*pHandle, semCnt, sem_max);

    if(ret != OPRT_OK)
    {
        tuya_hal_internal_free(*pHandle);
        *pHandle = NULL;
        LOGE("semaphore init fails %d %d\n", semCnt, sem_max);
        return ret;
    }

    return OPRT_OK;
}

/***********************************************************
*  Function: WaitSemaphore
*  Input: semHandle->信号量操作句柄
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
int tuya_hal_semaphore_wait(const SEM_HANDLE semHandle)
{
    if(!semHandle)
        return OPRT_INVALID_PARM;

    P_SEM_MANAGE pSemManage;
    pSemManage = (P_SEM_MANAGE)semHandle;

    BaseType_t ret;
    ret = xSemaphoreTake(pSemManage->sem, portMAX_DELAY);
    if(pdTRUE != ret) {
        return OPRT_WAIT_SEM_FAILED;
    }

    return OPRT_OK;
}

/***********************************************************
*  Function: PostSemaphore
*  Input: semHandle->信号量操作句柄
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
int tuya_hal_semaphore_post(const SEM_HANDLE semHandle)
{
    if(!semHandle)
        return OPRT_INVALID_PARM;

    P_SEM_MANAGE pSemManage;
    pSemManage = (P_SEM_MANAGE)semHandle;

    BaseType_t ret;
    if(FALSE == tuya_hal_system_isrstatus()) {
        ret = xSemaphoreGive(pSemManage->sem);
    }else {
        signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        ret = xSemaphoreGiveFromISR(pSemManage->sem,
                                    &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }

    if(pdTRUE != ret) {
        return OPRT_POST_SEM_FAILED;
    }

    return OPRT_OK;
}

/***********************************************************
*  Function: ReleaseSemaphore
*  Input: semHandle->信号量操作句柄
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
int tuya_hal_semaphore_release(const SEM_HANDLE semHandle)
{
    if(!semHandle)
        return OPRT_INVALID_PARM;

    P_SEM_MANAGE pSemManage;
    pSemManage = (P_SEM_MANAGE)semHandle;

    vSemaphoreDelete(pSemManage->sem);
    tuya_hal_internal_free(semHandle); // 释放信号量管理结构

    return OPRT_OK;
}


