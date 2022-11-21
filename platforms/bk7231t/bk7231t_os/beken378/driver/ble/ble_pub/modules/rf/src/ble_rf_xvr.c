/**
****************************************************************************************
*
* @file rf_xvras.c
*
* @brief Atlas radio initialization and specific functions
*
* Copyright (C) Beken 2009-2015
*
* $Rev: $
*
****************************************************************************************
*/

/**
****************************************************************************************
* @addtogroup RF_XVR
* @ingroup RF
* @brief Radio Driver
*
* This is the driver block for radio
* @{
****************************************************************************************
*/

/**
 *****************************************************************************************
 * INCLUDE FILES
 *****************************************************************************************
 */
#include <string.h>        // for memcpy
#include "common_endian.h"     // endian definitions
#include "common_math.h"       // common math functions
#include "common_utils.h"      // common utility definition
#include "ble_rf.h"            // RF interface
#include "em_map.h"        // RF area mapping

#include "rwip.h"          // for RF API structure definition
#include "reg_blecore.h"   // ble core registers
#include "reg_ble_em_cs.h" // control structure definitions

#include "BK3435_reg.h"           // Platform register
#include "ble_icu.h"
#include "rwip_config.h" // stack configuration

/**
 ****************************************************************************************
 * DEFINES
 ****************************************************************************************
 **/





/**
****************************************************************************************
* MACROS
*****************************************************************************************
*/


#define RPL_POWER_MAX               0x0c

/**
 ****************************************************************************************
 * GLOBAL VARIABLE DEFINITIONS
 *****************************************************************************************
 **/





/**
 ****************************************************************************************
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void CLK32K_AutoCali_init(void);


/**
 *****************************************************************************************
 * @brief Init RF sequence after reset.
 *****************************************************************************************
 */

static void rf_reset(void)
{
}

/**
 ****************************************************************************************
 * @brief ISR to be called in BLE ISR routine when RF Interrupt occurs.
 *****************************************************************************************
 */


/**
 *****************************************************************************************
 * @brief Get TX power in dBm from the index in the control structure
 *
 * @param[in] txpwr_idx  Index of the TX power in the control structure
 * @param[in] modulation Modulation: 1 or 2 or 3 MBPS
 *
 * @return The TX power in dBm
 *
 *****************************************************************************************
 */

static uint8_t rf_txpwr_dbm_get(uint8_t txpwr_idx, uint8_t modulation)
{
    // Power table should be provided
    return(0);
}

/**
 *****************************************************************************************
 * @brief Sleep function for  RF.
 *****************************************************************************************
 */

static void rf_sleep(void)
{
    #if defined(CFG_BLE)
    ble_deepslcntl_set(ble_deepslcntl_get() |
                      BLE_DEEP_SLEEP_ON_BIT |    // RW BLE Core sleep
                      BLE_RADIO_SLEEP_EN_BIT |   // Radio sleep
                      BLE_OSC_SLEEP_EN_BIT);     // Oscillator sleep
    #endif // CFG_BLE
}


/**
 *****************************************************************************************
 * @brief Convert RSSI to dBm
 *
 * @param[in] rssi_reg RSSI read from the HW registers
 *
 * @return The converted RSSI
 *
 *****************************************************************************************
 */

static int8_t rf_rssi_convert (uint8_t rssi_reg)
{
    uint8_t RssidBm = 0; 

    RssidBm = ((rssi_reg) >> 1) - 118;

    return(RssidBm);
}


static uint32_t rf_rpl_reg_rd (uint16_t addr)
{
  uint32_t ret;
	
	ret = REG_PL_RD(((unsigned int)addr));
	
	return ret;
}

static void rf_rpl_reg_wr (uint16_t addr, uint32_t value)
{
		REG_PL_WR(((unsigned int)addr), value);
}
/**
 ****************************************************************************************
 * RADIO FUNCTION INTERFACE
 ****************************************************************************************
 **/

static void rf_force_agc_enable(bool en)
{
  
    ble_forceagc_en_setf(en);
  
}
void rf_init(struct rwip_rf_api *api)
{
    // Initialize the RF driver API structure
    api->reg_rd = rf_rpl_reg_rd;
    api->reg_wr = rf_rpl_reg_wr;
    api->txpwr_dbm_get = rf_txpwr_dbm_get;
    api->txpwr_max = RPL_POWER_MAX;
    api->sleep = rf_sleep;
    api->reset = rf_reset;

    #if defined(CFG_BLE)
        api->force_agc_enable = rf_force_agc_enable;
    #endif //CFG_BLE

    api->rssi_convert = rf_rssi_convert;
	
	//initial_xver_BK3435V2_openLoop();
}

void Delay_us(int num)
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 10; x++);
    }
}

void Delay(int num)
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 50; x++);
    }
}

void Delay_ms(int num) //sync from svn revision 18
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 3260; x++);
    }
		
}

volatile uint32_t XVR_ANALOG_REG_BAK[16] = {0};


#define RF_TEST 			0
#define TEST_RC 			0


void init_analog_xvr_reg(void)
{
  	Delay_ms(20);
	mHWreg_Assign_XVR_Regsiter(00, 0xC3A03210);
	XVR_ANALOG_REG_BAK[0x00] = 0xC3A03210;

	mHWreg_Assign_XVR_Regsiter(01, 0x8295C100);//
	XVR_ANALOG_REG_BAK[0x01] = 0x8295C100;

	mHWreg_Assign_XVR_Regsiter(02, 0x2f42AF00); 
	XVR_ANALOG_REG_BAK[0x02] = 0x2f42AF00;

	mHWreg_Assign_XVR_Regsiter(03, 0x6A1CD462); //audio
	XVR_ANALOG_REG_BAK[0x03] = 0x6A1CD462;

	
	mHWreg_Assign_XVR_Regsiter(04, 0x7F9156B3);// 0x5F915727
	XVR_ANALOG_REG_BAK[0x04] = 0x5F9156B3;

	//mHWreg_Assign_XVR_Regsiter(04, 0x7F9116B3);// 0x5F915727
	//XVR_ANALOG_REG_BAK[0x04] = 0x5F9116B3;

	mHWreg_Assign_XVR_Regsiter(05, 0x48295540);// 0x48285540
	XVR_ANALOG_REG_BAK[0x05] = 0x48295540;

	//mHWreg_Assign_XVR_Regsiter(06, 0x88D68C00);   //
	//XVR_ANALOG_REG_BAK[0x06] = 0x88D68C00;//
	mHWreg_Assign_XVR_Regsiter(06, 0x88D68C10);   // for adc
	XVR_ANALOG_REG_BAK[0x06] = 0x88D68C10;//

	mHWreg_Assign_XVR_Regsiter(07, 0x0010Bfc7);//0x0000Bfc7); 
	XVR_ANALOG_REG_BAK[0x07] = 0x0010Bfc7;//0x0000Bfc7;
	mHWreg_Assign_XVR_Regsiter(08, 0X2DD04005);//0x3FB84005);   
	XVR_ANALOG_REG_BAK[0x08] = 0X2DD04005;//0x3FB84005;

#if (MCU_DEFAULT_CLK == MCU_CLK_16M)
	mHWreg_Assign_XVR_Regsiter(09, 0x74203C08);//0x70203C08 rc // 0x74203C08 xtal
	XVR_ANALOG_REG_BAK[0x09] = 0x74203C08; // 0x74203C08

#elif (MCU_DEFAULT_CLK == MCU_CLK_32M )	
	mHWreg_Assign_XVR_Regsiter(09, 0x74203C4A);//0x70203C08 rc // 0x74203C08 xtal
	XVR_ANALOG_REG_BAK[0x09] = 0x74203C4A; // 0x74203C08

#elif (MCU_DEFAULT_CLK == MCU_CLK_64M )	
	mHWreg_Assign_XVR_Regsiter(09, 0x74203C4A);//0x70203C08 rc // 0x74203C08 xtal
	XVR_ANALOG_REG_BAK[0x09] = 0x74203C4A; // 0x74203C08
#endif

	//mHWreg_Assign_XVR_Regsiter(0A, 0x1C07EC25); //0x1C07ED35  0x1C07EC25  
	//XVR_ANALOG_REG_BAK[0x0A] = 0x1C07EC25;
	mHWreg_Assign_XVR_Regsiter(0A, 0x1C07EC27); //for adc
	XVR_ANALOG_REG_BAK[0x0A] = 0x1C07EC27;
	
	mHWreg_Assign_XVR_Regsiter(0B, 0xd1Fd240C); //0xD3DD240C  71FD240C
	XVR_ANALOG_REG_BAK[0x0B] = 0xd1Fd240C;
	
	mHWreg_Assign_XVR_Regsiter(0C, 0x80009008);  //0x8000D008  0x80009008
	XVR_ANALOG_REG_BAK[0x0C] = 0x80009008;
	
	mHWreg_Assign_XVR_Regsiter(0D, 0x04413F23);//0x84413F23
	XVR_ANALOG_REG_BAK[0x0D] = 0x04413F23;
	
	mHWreg_Assign_XVR_Regsiter(0E, 0x00309350);  //-------------------------------------------------------
	XVR_ANALOG_REG_BAK[0x0E] = 0x00309350;
	
	mHWreg_Assign_XVR_Regsiter(0F, 0x3126E978);  
	XVR_ANALOG_REG_BAK[0x0F] = 0x3126E978;

	Delay_ms(20);
}
void initial_xver_BK3435V2_openLoop(void)
{
    
	uint32_t cfg ;
	Delay_ms(20);

	init_analog_xvr_reg();

	//mHWreg_Assign_XVR_Regsiter(10, 0x00083435);
	//mHWreg_Assign_XVR_Regsiter(11, 0x16810020);  
	//mHWreg_Assign_XVR_Regsiter(12, 0x00003C00); 
	//mHWreg_Assign_XVR_Regsiter(13, 0x00000000);  
	//mHWreg_Assign_XVR_Regsiter(14, 0x00080000);                                                                                              
	//mHWreg_Assign_XVR_Regsiter(15, 0x00000000);  
	//mHWreg_Assign_XVR_Regsiter(16, 0x00000000);  
	//mHWreg_Assign_XVR_Regsiter(17, 0x00000000);
	//mHWreg_Assign_XVR_Regsiter(18, 0x000001FF);  
	//mHWreg_Assign_XVR_Regsiter(19, 0x00000000);    
	//mHWreg_Assign_XVR_Regsiter(1A, 0x00000000);
	//mHWreg_Assign_XVR_Regsiter(1B, 0x00000000); 
	//mHWreg_Assign_XVR_Regsiter(1C, 0x00000000); 
	//mHWreg_Assign_XVR_Regsiter(1D, 0x00000000); 
	//mHWreg_Assign_XVR_Regsiter(1E, 0x00000000); 
	//mHWreg_Assign_XVR_Regsiter(1F, 0x00000000); 
	mHWreg_Assign_XVR_Regsiter(20, 0x8E89BED6);
	mHWreg_Assign_XVR_Regsiter(21, 0x96000000);
	//mHWreg_Assign_XVR_Regsiter(22, 0x78000000);
	mHWreg_Assign_XVR_Regsiter(22, 0x78010000);
	mHWreg_Assign_XVR_Regsiter(23, 0xA0000000);
	mHWreg_Assign_XVR_Regsiter(24, 0x001E0782);   
	mHWreg_Assign_XVR_Regsiter(25, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(26, 0x10200505);  
	mHWreg_Assign_XVR_Regsiter(27, 0x0008C900);
	mHWreg_Assign_XVR_Regsiter(28, 0x01011010);
	mHWreg_Assign_XVR_Regsiter(29, 0x3C104E00);  
	mHWreg_Assign_XVR_Regsiter(2A, 0x0e103D68); 
	mHWreg_Assign_XVR_Regsiter(2B, 0x00000408);
	mHWreg_Assign_XVR_Regsiter(2C, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(2D, 0x082CC444);
	mHWreg_Assign_XVR_Regsiter(2E, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(2F, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(30, 0x10010001);  
	mHWreg_Assign_XVR_Regsiter(31, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(32, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(33, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(34, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(35, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(36, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(37, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(38, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(39, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(3A, 0x00128000);   
	mHWreg_Assign_XVR_Regsiter(3B, 0x22341048);  
	mHWreg_Assign_XVR_Regsiter(3C, 0x01FF1c00);
	mHWreg_Assign_XVR_Regsiter(3D, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(3E, 0x00000000);
	mHWreg_Assign_XVR_Regsiter(3F, 0x00000000); 
	mHWreg_Assign_XVR_Regsiter(40, 0x01000000);
	mHWreg_Assign_XVR_Regsiter(41, 0x07050402);
	mHWreg_Assign_XVR_Regsiter(42, 0x120F0C0A);
	mHWreg_Assign_XVR_Regsiter(43, 0x221E1A16);
	mHWreg_Assign_XVR_Regsiter(44, 0x35302B26);
	mHWreg_Assign_XVR_Regsiter(45, 0x4B45403A);
	mHWreg_Assign_XVR_Regsiter(46, 0x635D5751);
	mHWreg_Assign_XVR_Regsiter(47, 0x7C767069);
	mHWreg_Assign_XVR_Regsiter(48, 0x968F8983);
	mHWreg_Assign_XVR_Regsiter(49, 0xAEA8A29C);
	mHWreg_Assign_XVR_Regsiter(4A, 0xC5BFBAB4);
	mHWreg_Assign_XVR_Regsiter(4B, 0xD9D4CFCA);
	mHWreg_Assign_XVR_Regsiter(4C, 0xE9E5E1DD);
	mHWreg_Assign_XVR_Regsiter(4D, 0xF5F3F0ED);
	mHWreg_Assign_XVR_Regsiter(4E, 0xFDFBFAF8);
	mHWreg_Assign_XVR_Regsiter(4F, 0xFFFFFFFE); 

#if RF_TEST		
	cfg = REG_AHB0_ICU_ANALOG_MODE;
	cfg |= ((0x01 << 18 ) | (0x01 << 17 ) | (0x01 << 21 ) | (0x01 << 23 ));

	REG_AHB0_ICU_ANALOG_MODE = cfg;
	
	cfg = REG_AHB0_ICU_ANALOG0_PWD;
	cfg &= ~ ((0x01 << 18 ) | (0x01 << 17 ) | (0x01 << 21 )| (0x01 << 23 ));
	REG_AHB0_ICU_ANALOG0_PWD = cfg;

#else	
  	cfg = REG_AHB0_ICU_ANALOG_MODE;
	cfg |= ((0x01 << 18 ));
	REG_AHB0_ICU_ANALOG_MODE = cfg;
	
	cfg = REG_AHB0_ICU_ANALOG0_PWD;

	cfg &= ~ ((0x01 << 18 ) );
	REG_AHB0_ICU_ANALOG0_PWD = cfg;
		
#endif	

	#if (HZ32000)
	CLK32K_AutoCali_init();
	#endif	
	Delay_ms(200);
	
		
}  


void CLK32K_AutoCali_init(void)
{
   XVR_ANALOG_REG_BAK[0x09] &= ~(0x01 << 26);
   XVR_REG09 = XVR_ANALOG_REG_BAK[0x09];
   
   XVR_ANALOG_REG_BAK[0x0C] &= ~(0X01 << 14);
   XVR_REG0C = XVR_ANALOG_REG_BAK[0x0C];
   

   XVR_ANALOG_REG_BAK[0x0C] |= (0X01 << 15);
   XVR_REG0C = XVR_ANALOG_REG_BAK[0x0C];
   
}


