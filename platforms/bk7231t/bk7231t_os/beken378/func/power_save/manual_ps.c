#include "manual_ps.h"
#include "manual_ps_pub.h"
#include "gpio_pub.h"
#include "power_save_pub.h"
#include "sys_ctrl_pub.h"
#include "target_util_pub.h"
#include "sys_ctrl.h"
#include "ps_debug_pub.h"
#include "icu_pub.h"
#include "pwm_pub.h"


#if PS_SUPPORT_MANUAL_SLEEP
/** @brief  Request power save,and wakeup some time later
 *  @param  sleep_time: Sleep time with milliseconds.
 *              if 0xffffffff not wakeup
 */
void bk_wlan_ps_wakeup_with_timer(UINT32 sleep_time)
{
    deep_sleep_wakeup_with_timer(sleep_time);
}

/** @brief  Request power save,and wakeup by uart if uart2_wk=1,nd wakeup by gpio from bitmap of gpio_index_map.
 */
void bk_wlan_ps_wakeup_with_peri( UINT8 uart2_wk, UINT32 gpio_index_map)
{
    power_save_wakeup_with_peri(uart2_wk, gpio_index_map);
}

void power_save_wakeup_with_peri( UINT8 uart2_wk, UINT32 gpio_index_map)
{
    UINT32 reg, ret;
    UINT32 param = 0;
    UINT32 i;
    UINT32    gpio_stat_cfg[32];

    if(power_save_ps_mode_get() != PS_NO_PS_MODE)
    {
        os_printf("can't peri ps,ps in mode %d!\r\n", power_save_ps_mode_get());
        return ;
    }

    for (i = 0; i < GPIONUM; i++)
    {
        gpio_stat_cfg[i] = REG_READ(0x00802800 + i * 4);

        if (gpio_index_map & (0x01UL << i))
        {
            bk_gpio_config_input_pdwn(i);
            BkGpioEnableIRQ(i, 0x2, NULL, NULL);
            os_printf("set peri wkup gpio %d\r\n", i);
        }
    }

#if(PS_XTAL26M_EN == 1)
    reg = REG_READ(SCTRL_LOW_PWR_CLK);
    reg &= ~(LPO_CLK_MUX_MASK);
    reg |= (LPO_SRC_32K_DIV << LPO_CLK_MUX_POSI);
    REG_WRITE(SCTRL_LOW_PWR_CLK, reg);
#endif
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    PS_DEBUG_DOWN_OUT;
    PS_DEBUG_UP_OUT;

    if(uart2_wk == 1)
    {
        param |= ( UART2_ARM_WAKEUP_EN_BIT);
        os_printf("set peri wkup uart2\r\n");
    }

    if(gpio_index_map)
        param |= ( GPIO_ARM_WAKEUP_EN_BIT);

    os_printf("enter peri ps\r\n");
    sddev_control(ICU_DEV_NAME, CMD_ARM_WAKEUP, &param);
    power_save_ps_mode_set(PS_MCU_PS_MODE);
    param = (0xfffff & ~PWD_UART2_CLK_BIT);
    sddev_control(SCTRL_DEV_NAME, CMD_SCTRL_NORMAL_SLEEP, &param);
    delay(200000);
    sddev_control(SCTRL_DEV_NAME, CMD_SCTRL_NORMAL_WAKEUP, 0);

    for (i = 0; i < GPIONUM; i++)
    {
        if (gpio_index_map & (0x01UL << i))
        {
            bk_gpio_config_input_pdwn(i);
            BkGpioDisableIRQ(i);
        }

        REG_WRITE(0x00802800 + i * 4, gpio_stat_cfg[i]);
    }

    power_save_ps_mode_set(PS_NO_PS_MODE);
    GLOBAL_INT_RESTORE();
    os_printf("exit peri ps\r\n");
}


void power_save_timer1_isr(UINT8 param)
{
    PS_DEBUG_PWM_OUT;
}


void power_save_timer1_init()
{
    UINT32 ret;
    pwm_param_t param;
    param.channel         = PWM1;
    param.cfg.bits.en     = PWM_DISABLE;
    param.cfg.bits.int_en = PWM_INT_EN;
    param.cfg.bits.mode   = PMODE_TIMER;
    param.cfg.bits.clk    = PWM_CLK_32K;
    param.p_Int_Handler   = power_save_timer1_isr;
    param.duty_cycle      = 0x10;
    param.end_value       = 3276;
    ret = sddev_control(PWM_DEV_NAME, CMD_PWM_INIT_PARAM, &param);
    ASSERT(PWM_SUCCESS == ret);
}

void power_save_wakeup_with_timer(UINT32 sleep_time)
{
    UINT32 reg;
    UINT32 param;
    UINT32 wakeup_timer;

    if(power_save_ps_mode_get() != PS_NO_PS_MODE)
    {
        os_printf("can't pwm ps,ps in mode %d!\r\n", power_save_ps_mode_get());
        return ;
    }

    if(sleep_time != 0xffffffff)
    {
        os_printf("sleep with pwm,%d ms\r\n", sleep_time);
#if 1
        reg = REG_READ(SCTRL_LOW_PWR_CLK);
        reg &= ~(LPO_CLK_MUX_MASK);
        reg |= (LPO_SRC_ROSC << LPO_CLK_MUX_POSI);
        REG_WRITE(SCTRL_LOW_PWR_CLK, reg);
#endif
        power_save_timer1_init();
        wakeup_timer = ((sleep_time * 102400) / 3125) ;

        if(wakeup_timer > 65535) //only 16 bit
            wakeup_timer = 65535;
        else if(wakeup_timer < 32)
            wakeup_timer = 32;

        delay(5);
        power_save_pwm1_enable(wakeup_timer);
    }
    else
    {
        os_printf("sleep forever\r\n");
    }

    os_printf("enter pwm ps\r\n");
    param = PWM_ARM_WAKEUP_EN_BIT;
    sddev_control(ICU_DEV_NAME, CMD_ARM_WAKEUP, &param);
    power_save_ps_mode_set(PS_STANDBY_PS_MODE);
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    PS_DEBUG_DOWN_OUT;
    param = (0xfffff & (~PWD_PWM1_CLK_BIT) & (~PWD_UART2_CLK_BIT));
    sddev_control(SCTRL_DEV_NAME, CMD_SCTRL_RTOS_IDLE_SLEEP, &param);
    PS_DEBUG_UP_OUT;
    delay(5);
    sddev_control(SCTRL_DEV_NAME, CMD_SCTRL_RTOS_IDLE_WAKEUP, 0);
    phy_init_after_wakeup();
    GLOBAL_INT_RESTORE();
    power_save_ps_mode_set(PS_NO_PS_MODE);
    power_save_pwm1_disable();
    os_printf("exit pwm ps\r\n");
}


void power_save_wakeup_with_gpio(UINT32 gpio_index)
{
    UINT32 reg;
    UINT32 param;

    if(power_save_ps_mode_get() != PS_NO_PS_MODE)
    {
        os_printf("can't gpio ps,ps in mode %d!\r\n", power_save_ps_mode_get());
        return ;
    }

    os_printf("enter gpio ps\r\n");
    bk_gpio_config_input_pdwn(gpio_index);
    BkGpioEnableIRQ(gpio_index, 0x2, NULL, NULL);
    reg = REG_READ(SCTRL_LOW_PWR_CLK);
    reg &= ~(LPO_CLK_MUX_MASK);
    reg |= (LPO_SRC_ROSC << LPO_CLK_MUX_POSI);
    REG_WRITE(SCTRL_LOW_PWR_CLK, reg);
    param = (GPIO_ARM_WAKEUP_EN_BIT);
    sddev_control(ICU_DEV_NAME, CMD_ARM_WAKEUP, &param);
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    PS_DEBUG_DOWN_OUT;
    PS_DEBUG_UP_OUT;
    param = (0xfffff & (~PWD_UART2_CLK_BIT));
    sddev_control(SCTRL_DEV_NAME, CMD_SCTRL_RTOS_IDLE_SLEEP, &param);
    delay(5);
    sddev_control(SCTRL_DEV_NAME, CMD_SCTRL_RTOS_IDLE_WAKEUP, 0);
    phy_init_after_wakeup();
    GLOBAL_INT_RESTORE();
    os_printf("exit gpio ps\r\n");
}

#endif

#if CFG_USE_DEEP_PS

/*
 *  deep sleep
 *
*/
void deep_sleep_wakeup_with_timer(UINT32 sleep_time)
{
    PS_DEEP_CTRL_PARAM deep_param;

    if(power_save_ps_mode_get() != PS_NO_PS_MODE)
    {
        os_printf("can't pwm ps,ps in mode %d!\r\n", power_save_ps_mode_get());
        return ;
    }


    if(sleep_time != 0xffffffff)
    {
        os_printf("sleep with rtc,%d ms\r\n", sleep_time);
        deep_param.param = ((sleep_time * 102400) / 3125) ;

        if(deep_param.param > 0xffff) //only 16 bit
            deep_param.param = 0xffff;
        else if(deep_param.param < 32)
            deep_param.param = 32;
    }
    else
    {
        os_printf("sleep forever\r\n");
        deep_param.param = 0xffffffff;
    }

    deep_param.deep_wkway = PS_DEEP_WAKEUP_RTC;
    os_printf("enter rtc ps\r\n");
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    sddev_control(SCTRL_DEV_NAME, CMD_SCTRL_RTOS_DEEP_SLEEP, &deep_param);
    delay(5);
    GLOBAL_INT_RESTORE();
    os_printf("exit pwm ps\r\n");
}


/** @brief  Request deep sleep,and wakeup by gpio.
 *          First user must set gpio to GMODE_INPUT,GMODE_INPUT_PULLUP,
 *          or GMODE_INPUT_PULLDOWN,as required.
 *
 *  @param  gpio_index_map:The gpio bitmap which set 1 enable wakeup deep sleep.
 *              gpio_index_map is hex and every bits is map to gpio0-gpio31.
 *          gpio_edge_map:The gpio edge bitmap for wakeup gpios,
 *              gpio_edge_map is hex and every bits is map to gpio0-gpio31.
 *              0:rising,1:falling.
 */
void deep_sleep_wakeup_with_gpio(UINT32 gpio_index_map,UINT32 gpio_edge_map)
{
    PS_DEEP_CTRL_PARAM deep_param;

    if(power_save_ps_mode_get() != PS_NO_PS_MODE)
    {
        os_printf("can't gpio ps,ps in mode %d!\r\n", power_save_ps_mode_get());
        return ;
    }

    os_printf("enter deep ps %x %x\r\n",gpio_index_map,gpio_edge_map);

    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    deep_param.deep_wkway = PS_DEEP_WAKEUP_GPIO;
    deep_param.param = gpio_index_map;
    deep_param.gpio_lv= gpio_edge_map;
    sddev_control(SCTRL_DEV_NAME, CMD_SCTRL_RTOS_DEEP_SLEEP, &deep_param);
    delay(5);
    GLOBAL_INT_RESTORE();
    os_printf("exit gpio ps\r\n");
}
#endif

