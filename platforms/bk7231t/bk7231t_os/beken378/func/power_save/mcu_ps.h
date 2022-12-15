#ifndef _MCU_PS_H_
#define _MCU_PS_H_

//#define MCU_PS_DEBUG

#ifdef MCU_PS_DEBUG
#define MCU_PS_PRT                 os_printf

#else
#define MCU_PS_PRT                 os_null_printf

#endif

typedef struct
{
#if (CFG_SUPPORT_ALIOS)
    UINT64 first_tick;
#else
    UINT32 first_tick;
#endif
    UINT64 first_tsf;
} MCU_PS_TSF;
typedef struct
{
#if (CFG_SUPPORT_ALIOS)
    UINT64 fclk_tick;
#else
    UINT32 fclk_tick;
#endif
    UINT32 machw_tm;
} MCU_PS_MACHW_TM;

#define OBK_MCU_SLEEP_METRICS_ENABLE 1
#ifdef OBK_MCU_SLEEP_METRICS_ENABLE
typedef struct
{
    UINT32 sleep_requested_ticks;
    UINT32 slept_ticks;
    UINT32 longest_sleep_1s;
    UINT32 longest_sleep;
    UINT32 longest_sleep_req_1s;
    UINT32 longest_sleep_req;

    UINT32 reasons[10];
    

    char *nexttask;
    char *task;
} OBK_MCU_SLEEP_METRICS;
#endif

#endif

