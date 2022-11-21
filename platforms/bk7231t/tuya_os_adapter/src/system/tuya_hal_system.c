/***********************************************************
*  File: uni_system.c
*  Author: nzy
*  Date: 120427
***********************************************************/
#define _UNI_SYSTEM_GLOBAL

#include "tuya_hal_system.h"
#include "tuya_hal_wifi.h"
#include "../errors_compat.h"

#include "wlan_ui_pub.h"
#include "mem_pub.h"
#include "wdt_pub.h"
#include "drv_model_pub.h"

#include "start_type_pub.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
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
#define LOGD(...) bk_printf("[SYS DEBUG]" __VA_ARGS__)
#define LOGT(...) bk_printf("[SYS TRACE]" __VA_ARGS__)
#define LOGN(...) bk_printf("[SYS NOTICE]" __VA_ARGS__)
#define LOGE(...) bk_printf("[SYS ERROR]" __VA_ARGS__)
#endif


#define SERIAL_NUM_LEN 32

RESET_SOURCE_STATUS bk_misc_get_start_type();
void bk_reboot(void);

/***********************************************************
*************************variable define********************
***********************************************************/
static char serial_no[SERIAL_NUM_LEN+1] = {0};

/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: GetSystemTickCount 
*  Input: none
*  Output: none
*  Return: system tick count
***********************************************************/


SYS_TICK_T tuya_hal_get_systemtickcount(void)
{
    return (SYS_TICK_T)xTaskGetTickCount();
}

/***********************************************************
*  Function: GetTickRateMs 
*  Input: none
*  Output: none
*  Return: tick rate spend how many ms
***********************************************************/
uint32_t tuya_hal_get_tickratems(void)
{
    return (uint32_t)portTICK_RATE_MS;
}

/***********************************************************
*  Function: tuya_hal_system_sleep 
*  Input: msTime
*  Output: none 
*  Return: none
*  Date: 120427
***********************************************************/
void tuya_hal_system_sleep(const unsigned long msTime)
{
    vTaskDelay((msTime)/(portTICK_RATE_MS));
}

/***********************************************************
*  Function: SystemIsrStatus->direct system interrupt status
*  Input: none
*  Output: none 
*  Return: bool
***********************************************************/
bool tuya_hal_system_isrstatus(void)
{
    if(0 !=  bk_wlan_get_INT_status()) {
        return TRUE;
    }

    return FALSE;
}

/***********************************************************
*  Function: SystemReset 
*  Input: msTime
*  Output: none 
*  Return: none
*  Date: 120427
***********************************************************/
void tuya_hal_system_reset(void)
{
    bk_printf("\r\n****SystemReset****\r\n");
    bk_reboot();
}

/***********************************************************
*  Function: uni_watchdog_init_and_start 
*  Input: timeval
*  Output: none 
*  Return: void *
***********************************************************/
void tuya_hal_watchdog_init_start(int timeval)
{
    //init 
    int cyc_cnt = timeval * 1000;
    
    if(cyc_cnt > 0xFFFF) {
        cyc_cnt = 0xFFFF;
    }
    //init wdt
    sddev_control(WDT_DEV_NAME, WCMD_SET_PERIOD, &cyc_cnt);
    
    // start wdt timer
    sddev_control(WDT_DEV_NAME, WCMD_POWER_UP, NULL);
}

/***********************************************************
*  Function: uni_watchdog_refresh 
*  Input: none
*  Output: none 
*  Return: void *
***********************************************************/
void tuya_hal_watchdog_refresh(void)
{
    sddev_control(WDT_DEV_NAME, WCMD_RELOAD_PERIOD, NULL);
}

/***********************************************************
*  Function: uni_watchdog_stop 
*  Input: none
*  Output: none 
*  Return: void *
***********************************************************/
void tuya_hal_watchdog_stop(void)
{
    sddev_control(WDT_DEV_NAME, WCMD_POWER_DOWN, NULL);
}

/***********************************************************
*  Function: SysGetHeapSize 
*  Input: none
*  Output: none 
*  Return: int-> <0 means don't support to get heapsize
***********************************************************/
int tuya_hal_system_getheapsize(void)
{
    return (int)xPortGetFreeHeapSize();
}

/***********************************************************
*  Function: GetSerialNo 
*  Input: none
*  Output: none 
*  Return: char *->serial number
***********************************************************/
char *tuya_hal_get_serialno(void)
{
    // if the device have unique serial number
    // then add get serial number code to serial_no array

    // if don't have unique serial number,then use mac addr
   
    int op_ret = OPRT_OK;
    NW_MAC_S mac1;
    op_ret = tuya_hal_wifi_get_mac(WF_STATION,&mac1);
    if(op_ret != OPRT_OK) {
        return NULL;
    }

    memset(serial_no,'\0',sizeof(serial_no));
    sprintf(serial_no,"%02x%02x%02x%02x%02x%02x",mac1.mac[0],mac1.mac[1],\
            mac1.mac[2],mac1.mac[3],mac1.mac[4],mac1.mac[5]);

    if(0 == serial_no[0]) {
        return NULL;
    }
    return serial_no;
}

/***********************************************************
*  Function: system_get_rst_info 
*  Input: none
*  Output: none 
*  Return: char *->reset reason
***********************************************************/
TY_RST_REASON_E tuya_hal_system_get_rst_info(void)
{
    unsigned char value = bk_misc_get_start_type() & 0xFF;
    TY_RST_REASON_E bk_value;
    
    switch(value) {
        case RESET_SOURCE_POWERON:
            bk_value = TY_RST_POWER_OFF;
            break;

        case RESET_SOURCE_REBOOT:
            bk_value = TY_RST_SOFTWARE;
            break;

        case RESET_SOURCE_WATCHDOG:
            bk_value = TY_RST_HARDWARE_WATCHDOG;
            break;

        case RESET_SOURCE_CRASH_XAT0:
        case RESET_SOURCE_CRASH_UNDEFINED:
        case RESET_SOURCE_CRASH_PREFETCH_ABORT:
        case RESET_SOURCE_CRASH_DATA_ABORT:
        case RESET_SOURCE_CRASH_UNUSED:
            bk_value = TY_RST_FATAL_EXCEPTION;
            break;

        default:
            bk_value = TY_RST_POWER_OFF;
            break;

    }

    bk_printf("bk_rst:%d tuya_rst:%d",value, bk_value);
    
    return bk_value;
}

/***********************************************************
*  Function: tuya_random
*  Input: none
*  Output: none
*  Return: random data in INT
***********************************************************/
uint32_t tuya_hal_random(void)
{
    uint8_t data[4] = {0};
    tuya_hal_get_random_data(data, 4, 0);
    return data[0] + (data[1]<<8) + (data[2]<<16) + (data[3]<<24);
}

/***********************************************************
*  Function: tuya_get_random_data
*  Input: dst size
*  Output: none
*  Return: void
***********************************************************/
int tuya_hal_get_random_data(uint8_t* dst, int size, uint8_t range)
{
    if(range == 0)
        range = 0xFF;

    int i;
    static unsigned char exec_flag = FALSE;

    if(!exec_flag) {
//        srand(random_seed);
        exec_flag = TRUE;
    }
    for(i = 0; i< size; i++) {
        int val =  rand();// + random_seed;
        dst[i] = val % range;
    }
    return OPRT_OK;
}


/***********************************************************
*  Function: tuya_set_lp_mode
*  Input: lp_enable
*  Output: none
*  Return: void
***********************************************************/
static bool is_lp_enable = FALSE;
void tuya_hal_set_lp_mode(bool lp_enable)
{
    is_lp_enable = lp_enable;
}

/***********************************************************
*  Function: tuya_get_lp_mode
*  Input: none
*  Output: none
*  Return: bool
***********************************************************/
bool tuya_hal_get_lp_mode(void)
{
   return is_lp_enable;
}
