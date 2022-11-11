#include "include.h"
#include "fake_clock_pub.h"
#if (CFG_SOC_NAME == SOC_BK7231)
#include "pwm_pub.h"
#else
#include "bk_timer_pub.h"
#endif
#include "icu_pub.h"
#include "drv_model_pub.h"
#include "uart_pub.h"

#if CFG_SUPPORT_ALIOS
#include "rtos_pub.h"
#include "ll.h"
#include "k_api.h"
#else
#include "sys_rtos.h"
#include "arm_arch.h"
#endif
#include "power_save_pub.h"

#if CFG_USE_MCU_PS
#include "mcu_ps_pub.h"
#endif

#if CFG_BK7221_MDM_WATCHDOG_PATCH
void rc_reset_patch(void);
#endif

static volatile UINT64 current_us = 0;
static volatile UINT32 current_clock = 0;
static volatile UINT32 current_seconds = 0;
static UINT32 second_countdown = FCLK_SECOND;

extern void mcu_ps_increase_clr(void);
#define         ONE_CAL_TIME        15000

typedef struct
{
    UINT32 fclk_tick;
    UINT32 tmp1;
} CAL_TICK_T;
static CAL_TICK_T cal_tick_save;
UINT32 use_cal_net = 0;

void fclk_hdl(UINT8 param)
{
    GLOBAL_INT_DECLARATION();
#if CFG_USE_STA_PS
    if(power_save_use_pwm_isr())
    {
        power_save_pwm_isr(param);
        return;
    }

    if(!mcu_ps_need_pstick())
    {
        return;
    }
#endif

#if CFG_BK7221_MDM_WATCHDOG_PATCH
    rc_reset_patch();
#endif

#if (!CFG_SUPPORT_ALIOS)
    /* Increment the tick counter. */
    GLOBAL_INT_DISABLE();
    current_clock ++;

    if (--second_countdown == 0)
    {
        current_seconds ++;
        second_countdown = FCLK_SECOND;
    }
    
    if( xTaskIncrementTick() != pdFALSE )
    {
        /* Select a new task to run. */
        vTaskSwitchContext();
    }

    GLOBAL_INT_RESTORE();
#endif

#ifdef BEKEN_START_WDT
	/* fresh watch dog if needed */
	if(current_seconds % 5)
	{
		bk_wdg_reload();
	}
#endif

#if CFG_SUPPORT_ALIOS
    krhino_tick_proc();
#endif
}

static UINT32 fclk_freertos_update_tick(UINT32 tick)
{
    current_clock += tick;

    while(tick >= FCLK_SECOND)
    {
        current_seconds ++;
        tick -= FCLK_SECOND;
    }

    if(second_countdown <= tick)
    {
        current_seconds ++;
        second_countdown = FCLK_SECOND - (tick - second_countdown);
    }
    else
    {
        second_countdown -= tick;
    }

    return 0;
}

UINT32 fclk_update_tick(UINT32 tick)
{
#if (CFG_SUPPORT_RTT)
    rtt_update_tick(tick);
#elif (CFG_SUPPORT_ALIOS)
    krhino_update_sys_tick((UINT64)tick);
#else
    GLOBAL_INT_DECLARATION();

    if(tick == 0)
        return 0;

    GLOBAL_INT_DISABLE();
    mcu_ps_increase_clr();
    fclk_freertos_update_tick(tick);
    vTaskStepTick( tick );
    GLOBAL_INT_RESTORE();
    
#endif
    return 0;
}

UINT32 fclk_freertos_get_tick(void)
{
    return current_clock;
}

UINT64 fclk_get_tick(void)
{
    UINT64 fclk;
#if (CFG_SUPPORT_RTT)
    fclk = (UINT64)rt_tick_get();
#elif (CFG_SUPPORT_ALIOS)
    fclk = krhino_sys_tick_get();
#else
    fclk = (UINT64)fclk_freertos_get_tick();
#endif
    return fclk;
}

UINT32 fclk_get_second(void)
{
#if (CFG_SUPPORT_RTT)
    return (rt_tick_get()/FCLK_SECOND);
#elif (CFG_SUPPORT_ALIOS)
    return (krhino_sys_tick_get()/FCLK_SECOND);
#else
    return current_clock/FCLK_SECOND;
#endif
}

UINT32 fclk_from_sec_to_tick(UINT32 sec)
{
    return sec * FCLK_SECOND;
}

void fclk_reset_count(void)
{
    current_clock = 0;
    current_seconds = 0;
}

UINT32 timer_cal_init(void)
{
    UINT32 fclk;

    fclk = BK_TICKS_TO_MS(fclk_get_tick());

    cal_tick_save.fclk_tick = fclk;
    cal_tick_save.tmp1 = 0;
    return 0;
}

extern int increase_tick;
UINT32 timer_cal_tick(void)
{
    UINT32 fclk, tmp2;
    UINT32 machw;
    INT32 lost;
    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();
    
    fclk = BK_TICKS_TO_MS(fclk_get_tick());
    cal_tick_save.tmp1 += ONE_CAL_TIME;

    tmp2 = fclk;

    lost = (INT32)(cal_tick_save.tmp1  - (UINT32)tmp2);

    if((lost >= (2*FCLK_DURATION_MS)))
    {
        if(lost > 200)
        {
            //os_printf("m cal_:%x %x\r\n", lost, machw);
        }

        lost -= FCLK_DURATION_MS;
        fclk_update_tick(BK_MS_TO_TICKS(lost));
        increase_tick = 0;
    }
    #if 1
    else
    {
        if(lost <= (-(2*FCLK_DURATION_MS)))
        {
            if(lost < (-50000))
            {
                os_printf("m reset:%x %x\r\n", lost, machw);
            }
            increase_tick = lost + FCLK_DURATION_MS;
        }
    }
    #endif
    //os_printf("tc:%d\r\n",lost);
    
    mcu_ps_machw_init();
    GLOBAL_INT_RESTORE();
    return 0 ;

CAL_RESET:
    timer_cal_init();
    GLOBAL_INT_RESTORE();
    return 0 ;
}


void cal_timer_hdl(UINT8 param)
{
    timer_cal_tick();
}

void cal_timer_set(void)
{
    timer_param_t param;
    UINT32 ret;
    UINT32 timer_channel;

    timer_cal_init();

    param.channel = CAL_TIMER_ID;
    param.div = 1;
    param.period = ONE_CAL_TIME;
    param.t_Int_Handler= cal_timer_hdl;

    ret = sddev_control(TIMER_DEV_NAME, CMD_TIMER_INIT_PARAM, &param);
    //ASSERT(BK_TIMER_SUCCESS == ret);
    if (ret != BK_TIMER_SUCCESS)
        return;
    timer_channel = param.channel;
    ret = sddev_control(TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &timer_channel);
    //ASSERT(BK_TIMER_SUCCESS == ret);    
}

void cal_timer_deset(void)
{
    UINT32 ret;
    UINT32 timer_channel;

    timer_channel = CAL_TIMER_ID;
    ret = sddev_control(TIMER_DEV_NAME, CMD_TIMER_UNIT_DISABLE, &timer_channel);
    //ASSERT(BK_TIMER_SUCCESS == ret);
    if (ret != BK_TIMER_SUCCESS)
        return;
    timer_cal_init();
}

UINT32 bk_cal_init(UINT32 setting)
{
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    
    if(1 == setting)
    {
        cal_timer_deset();
        use_cal_net = 1;
#if CFG_USE_MCU_PS
        mcu_ps_machw_init();
#endif
        os_printf("decset:%d %d %d %d\r\n",use_cal_net,current_clock,fclk_get_second(),xTaskGetTickCount());
    }
    else
    {
#if CFG_USE_MCU_PS
        mcu_ps_machw_cal();
#endif
        cal_timer_set();
        use_cal_net = 0;
#if CFG_USE_MCU_PS
        mcu_ps_machw_reset();
#endif
        os_printf("cset:%d %d %d %d\r\n",use_cal_net,current_clock,fclk_get_second(),xTaskGetTickCount());
    }
    GLOBAL_INT_RESTORE();

	return 0;
}

#if (CFG_SOC_NAME == SOC_BK7231)
UINT32 fclk_cal_endvalue(UINT32 mode)
{
    UINT32 value = 1;

    if(PWM_CLK_32K == mode)
    {
        /*32k clock*/
        value = FCLK_DURATION_MS * 32;
    }
    else if(PWM_CLK_26M == mode)
    {
        /*26m clock*/
#if CFG_SUPPORT_ALIOS
	value = 26000000 / RHINO_CONFIG_TICKS_PER_SECOND;
#else
        value = FCLK_DURATION_MS * 26000;
#endif
    }

    return value;
}
#endif

void fclk_init(void)
{
    UINT32 ret;
    #if (CFG_SOC_NAME == SOC_BK7231)
    pwm_param_t param;

    /*init pwm*/
    param.channel         = FCLK_PWM_ID;
    param.cfg.bits.en     = PWM_ENABLE;
    param.cfg.bits.int_en = PWM_INT_EN;
    param.cfg.bits.mode   = PMODE_TIMER;

#if(CFG_RUNNING_PLATFORM == FPGA_PLATFORM)  // FPGA:PWM0-2-32kCLK, pwm3-5-24CLK
    param.cfg.bits.clk    = PWM_CLK_32K;
#else
    param.cfg.bits.clk    = PWM_CLK_26M;
#endif

    param.p_Int_Handler   = fclk_hdl;
    param.duty_cycle      = 0;
    param.end_value       = fclk_cal_endvalue((UINT32)param.cfg.bits.clk);

    ret = sddev_control(PWM_DEV_NAME, CMD_PWM_INIT_PARAM, &param);
    //ASSERT(PWM_SUCCESS == ret);
    #else
    timer_param_t param;
    param.channel = FCLK_TIMER_ID;
    param.div = 1;
    param.period = FCLK_DURATION_MS;
    param.t_Int_Handler= fclk_hdl;

    ret = sddev_control(TIMER_DEV_NAME, CMD_TIMER_INIT_PARAM, &param);
    //ASSERT(BK_TIMER_SUCCESS == ret);
    if (ret != BK_TIMER_SUCCESS)
        return;
    UINT32 timer_channel;
    timer_channel = param.channel;
    ret = sddev_control(TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &timer_channel);
    //ASSERT(BK_TIMER_SUCCESS == ret);
    if (ret != BK_TIMER_SUCCESS)
        return;
    bk_cal_init(0);
    #endif

}

// eof

