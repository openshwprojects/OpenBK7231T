#include "include.h"
#include "func_pub.h"
#include "intc.h"
#include "rwnx.h"
#include "uart_pub.h"
#include "lwip_intf.h"
#include "param_config.h"
#include "saradc_pub.h"

#if CFG_ROLE_LAUNCH
#include "role_launch.h"
#endif

#if CFG_SUPPORT_CALIBRATION
#include "bk7011_cal_pub.h"
#endif

#if CFG_UART_DEBUG 
#include "uart_debug_pub.h"
#endif

#if CFG_SDIO
#include "sdio_intf_pub.h"
#endif

#if CFG_USB
#include "fusb_pub.h"
#endif
#include "start_type_pub.h"
#include "BkDriverFlash.h"

RESET_SOURCE_STATUS bk_misc_get_start_type();

UINT32 func_init_extended(void)
{
    char temp_mac[6];
    
	cfg_param_init();
    // load mac, init mac first
    wifi_get_mac_address(temp_mac, CONFIG_ROLE_NULL);
	
    FUNC_PRT("[FUNC]rwnxl_init\r\n");
    rwnxl_init();

#if CFG_UART_DEBUG 
	#ifndef KEIL_SIMULATOR
    FUNC_PRT("[FUNC]uart_debug_init\r\n");   
    uart_debug_init();
	#endif
#endif

#if (!CFG_SUPPORT_RTT)
    FUNC_PRT("[FUNC]intc_init\r\n");
    intc_init();
#endif

#if CFG_SUPPORT_CALIBRATION
	UINT32 is_tab_inflash = 0;
    FUNC_PRT("[FUNC]calibration_main\r\n");
    calibration_main();
    #if CFG_SUPPORT_MANUAL_CALI
	is_tab_inflash = manual_cal_load_txpwr_tab_flash();
    manual_cal_load_default_txpwr_tab(is_tab_inflash);
    #endif
    #if CFG_SARADC_CALIBRATE
    manual_cal_load_adc_cali_flash();
    #endif
    #if CFG_USE_TEMPERATURE_DETECT
    manual_cal_load_temp_tag_flash();
    #endif
	
    #if (CFG_SOC_NAME != SOC_BK7231)
    manual_cal_load_lpf_iq_tag_flash();
    manual_cal_load_xtal_tag_flash();
    #endif // (CFG_SOC_NAME != SOC_BK7231)
	
    rwnx_cal_initial_calibration();

	#if CFG_SUPPORT_MANUAL_CALI
	if (0)//(is_tab_inflash == 0)
	{
		manual_cal_fitting_txpwr_tab();
		manual_cal_save_chipinfo_tab_to_flash();
		manual_cal_save_txpwr_tab_to_flash();
	}
	#endif // CFG_SUPPORT_MANUAL_CALI
#endif    

#if CFG_SDIO
    FUNC_PRT("[FUNC]sdio_intf_init\r\n");
    sdio_intf_init();
#endif

#if CFG_SDIO_TRANS
    FUNC_PRT("[FUNC]sdio_intf_trans_init\r\n");
    sdio_trans_init();
#endif


#if CFG_USB
    FUNC_PRT("[FUNC]fusb_init\r\n");
    fusb_init();
#endif

#if  CFG_USE_STA_PS
    FUNC_PRT("[FUNC]ps_init\r\n");
#endif

#if CFG_ROLE_LAUNCH
    rl_init();
#endif

	#if CFG_ENABLE_BUTTON
	key_initialization();
	#endif

#ifdef BEKEN_START_WDT
	bk_wdg_initialize(10000);
#endif

    FUNC_PRT("[FUNC]func_init_extended OVER!!!\r\n\r\n");
    os_printf("start_type:%d\r\n",bk_misc_get_start_type());
    return 0;
}

UINT32 func_init_basic(void)
{
    intc_init();
    hal_flash_init();

    return 0;
}

// eof
