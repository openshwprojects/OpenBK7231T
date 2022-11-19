#include "include.h"
#include "arm_arch.h"
#include "bk_timer_pub.h"
#include "bk_timer.h"
#include "drv_model_pub.h"
#include "intc_pub.h"
#include "icu_pub.h"
#include "uart_pub.h"

#if (CFG_SOC_NAME != SOC_BK7231)
static SDD_OPERATIONS bk_timer_op =
{
    bk_timer_ctrl
};

void (*p_TIMER_Int_Handler[TIMER_CHANNEL_NO])(UINT8) = {NULL,};

static UINT32 bk_timer_cal_endvalue(UINT32 ucChannel, UINT32 time_ms, UINT32 div)
{
    UINT64 value;

    if(div == 0)
        div = 1;

    if(ucChannel < 3)
    {
        /*26m clock*/
        value = time_ms * 26000 / div;
    }
    else
    {
         /*32k clock*/
        value = time_ms * 32 / div;
    }

    if(value > 0xffffffff)
        value = 0xffffffff;

    return (UINT32)value;
}

static UINT32 bk_timer_cal_endvalue_us(UINT32 ucChannel, UINT32 time_us, UINT32 div)
{
    UINT64 value;
    
    if(div == 0)
        div = 1;

    /*26m clock*/
    value = time_us * 26 / div;

    if(value > 0xffffffff)
        value = 0xffffffff;

    return (UINT32)value;
}

static UINT32 init_timer_param(timer_param_t *timer_param)
{
    UINT32 value;
    UINT32 ucChannel = timer_param->channel;

    if (timer_param == NULL)
    {
        return BK_TIMER_FAILURE;
    }

    if (timer_param->channel > 5)
    {
        return BK_TIMER_FAILURE;
    }


    p_TIMER_Int_Handler[ucChannel] = timer_param->t_Int_Handler;

    if (ucChannel < 3)
    {
        value = (PWD_TIMER_26M_CLK_BIT);
        sddev_control(ICU_DEV_NAME, CMD_CLK_PWR_UP, (void *)&value);

        value = bk_timer_cal_endvalue(ucChannel, timer_param->period, timer_param->div);
        REG_WRITE(REG_TIMERCTLA_PERIOD_ADDR(ucChannel), value);

        value = REG_READ(TIMER0_2_CTL);
        value &= ~(TIMERCTLA_CLKDIV_MASK << TIMERCTLA_CLKDIV_POSI);
        value |= ((timer_param->div - 1) << TIMERCTLA_CLKDIV_POSI);
        value |= (1 << ucChannel );
        REG_WRITE(TIMER0_2_CTL, value);
    }
    else
    {
        value = (PWD_TIMER_32K_CLK_BIT);
        sddev_control(ICU_DEV_NAME, CMD_CLK_PWR_UP, (void *)&value);

        value = bk_timer_cal_endvalue(ucChannel, timer_param->period, timer_param->div);
        REG_WRITE(REG_TIMERCTLB_PERIOD_ADDR(ucChannel), value);

        value = REG_READ(TIMER3_5_CTL);
        value &= ~(TIMERCTLB_CLKDIV_MASK << TIMERCTLB_CLKDIV_POSI);
        value |= ((timer_param->div - 1) << TIMERCTLB_CLKDIV_POSI);
        value |= (1 << (ucChannel - 3));
        REG_WRITE(TIMER3_5_CTL, value);
    }

    intc_enable(IRQ_TIMER);

    return BK_TIMER_SUCCESS;
}

static UINT32 init_timer_param_us(timer_param_t *timer_param)
{
    UINT32 value;
    UINT32 ucChannel = timer_param->channel;

    if (timer_param == NULL)
    {
        return BK_TIMER_FAILURE;
    }

    if (timer_param->channel > 2)
    {
        return BK_TIMER_FAILURE;
    }


    p_TIMER_Int_Handler[ucChannel] = timer_param->t_Int_Handler;

    value = (PWD_TIMER_26M_CLK_BIT);
    sddev_control(ICU_DEV_NAME, CMD_CLK_PWR_UP, (void *)&value);

    value = bk_timer_cal_endvalue_us(ucChannel, timer_param->period, timer_param->div);
    REG_WRITE(REG_TIMERCTLA_PERIOD_ADDR(ucChannel), value);

    value = REG_READ(TIMER0_2_CTL);
    value &= ~(TIMERCTLA_CLKDIV_MASK << TIMERCTLA_CLKDIV_POSI);
    value |= ((timer_param->div - 1) << TIMERCTLA_CLKDIV_POSI);
    value |= (1 << ucChannel );
    REG_WRITE(TIMER0_2_CTL, value);

    intc_enable(IRQ_TIMER);

    return BK_TIMER_SUCCESS;
}

UINT32 bk_timer_ctrl(UINT32 cmd, void *param)
{
    int i_time_out;
    UINT32 ret = BK_TIMER_SUCCESS;
    UINT32 ucChannel;
    UINT32 value;
    timer_param_t *p_param;

    switch(cmd)
    {
    case CMD_TIMER_UNIT_ENABLE:
        ucChannel = (*(UINT32 *)param);
        if(ucChannel > 5)
        {
            ret = BK_TIMER_FAILURE;
            break;
        }

        if(ucChannel < 3)
        {
            value = REG_READ(TIMER0_2_CTL);
            value |= (1 << ucChannel );
            value &= ~(0x7 << TIMERCTLB_INT_POSI);
            REG_WRITE(TIMER0_2_CTL, value);
        }
        else
        {
            value = REG_READ(TIMER3_5_CTL);
            value |= (1 << (ucChannel - 3));
            value &= ~(0x7 << TIMERCTLB_INT_POSI);
            REG_WRITE(TIMER3_5_CTL, value);
        }
        break;
		
    case CMD_TIMER_UNIT_DISABLE:
        ucChannel = (*(UINT32 *)param);
        if(ucChannel > 5)
        {
            ret = BK_TIMER_FAILURE;
            break;
        }

        if(ucChannel < 3)
        {
            value = REG_READ(TIMER0_2_CTL);
            value &= ~(1 << ucChannel );
            value &= ~(0x7 << TIMERCTLB_INT_POSI);
            REG_WRITE(TIMER0_2_CTL, value);
        }
        else
        {
            value = REG_READ(TIMER3_5_CTL);
            //os_printf("-%08x\r\n", value);
            value &= ~(1 << (ucChannel - 3));
            value &= ~(0x7 << TIMERCTLB_INT_POSI);
            //os_printf("%08x\r\n", value);
            REG_WRITE(TIMER3_5_CTL, value);
        }
        break;
		
    case CMD_TIMER_INIT_PARAM:
        p_param = (timer_param_t *)param;
        ret = init_timer_param(p_param);
        break;
		
    case CMD_TIMER_INIT_PARAM_US:
        p_param = (timer_param_t *)param;
        ret = init_timer_param_us(p_param);
        break;

    case CMD_TIMER_READ_CNT:
        p_param = (timer_param_t *)param;
        i_time_out = 0;
        if (p_param->channel < 3)
        {
            REG_WRITE(TIMER0_2_READ_CTL, ((p_param->channel) << 2) | 1);
            while (REG_READ(TIMER0_2_READ_CTL) & 1)
            {
                i_time_out ++;
                if (i_time_out > (120 * 1000))
                {
                    ret = BK_TIMER_FAILURE;
                    break;
                }
            }
            if (i_time_out <= (120 * 1000))
            {
                p_param->period = REG_READ(TIMERR0_2_READ_VALUE);
            }
        }
        else if (p_param->channel < 6)
        {
            REG_WRITE(TIMER3_5_READ_CTL, ((p_param->channel) << 2) | 1);
            while (REG_READ(TIMER3_5_READ_CTL) & 1)
            {
                i_time_out ++;
                if (i_time_out > (120 * 1000))
                {
                    ret = BK_TIMER_FAILURE;
                    break;
                }
            }
            if (i_time_out <= (120 * 1000))
            {
                p_param->period = REG_READ(TIMER3_5_READ_VALUE);
            }
        }
        break;

    default:
        ret = BK_TIMER_FAILURE;
        break;
    }

    return ret;
}

void bk_timer_init(void)
{
    intc_service_register(IRQ_TIMER, PRI_IRQ_TIMER, bk_timer_isr);
    sddev_register_dev(TIMER_DEV_NAME, &bk_timer_op);
}

void bk_timer_exit(void)
{
    sddev_unregister_dev(TIMER_DEV_NAME);
}

void bk_timer_isr(void)
{
    int i;
    UINT32 status0;
    UINT32 status1;

    status0 = REG_READ(TIMER0_2_CTL) & (0x7 << TIMERCTLA_INT_POSI);
    for(i = 0; i < 3; i++)
    {
        if(status0 & (1 << (i + TIMERCTLA_INT_POSI)))
        {
            if(p_TIMER_Int_Handler[i])
            {
                p_TIMER_Int_Handler[i]((UINT8)i);
            }
        }
    }

    do
    {
        REG_WRITE(TIMER0_2_CTL, (REG_READ(TIMER0_2_CTL) & (~(0x7 << TIMERCTLA_INT_POSI))) | status0);
    } while(REG_READ(TIMER0_2_CTL) & status0 & (0x7 << TIMERCTLA_INT_POSI));

    status1 = REG_READ(TIMER3_5_CTL) & (0x7 << TIMERCTLB_INT_POSI);
    for(i = 0; i < 3; i++)
    {
        if(status1 & (1 << (i + TIMERCTLB_INT_POSI)))
        {
            if(p_TIMER_Int_Handler[i + 3])
            {
                p_TIMER_Int_Handler[i + 3]((UINT8)(i + 3));
            }
        }
    }

    do
    {
        REG_WRITE(TIMER3_5_CTL, (REG_READ(TIMER3_5_CTL) & (~(0x7 << TIMERCTLB_INT_POSI))) | status1);
    } while(REG_READ(TIMER3_5_CTL) & status1 & (0x7 << TIMERCTLB_INT_POSI));
}
#endif

