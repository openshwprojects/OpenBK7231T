/***********************************************************
*  File: uni_storge.c 
*  Author: nzy
*  Date: 20170920
***********************************************************/
#define __UNI_STORGE_GLOBALS
#include "drv_model_pub.h"
#include "flash_pub.h"

#include "tuya_hal_storge.h"
#include "../errors_compat.h"

#include "start_type_pub.h"
#include "BkDriverFlash.h"

RESET_SOURCE_STATUS bk_misc_get_start_type();

/***********************************************************
*************************micro define***********************
***********************************************************/
#define PARTITION_SIZE (1 << 12) /* 4KB */
#define FLH_BLOCK_SZ PARTITION_SIZE

// flash map 
#define SIMPLE_FLASH_START (0x200000 - 0x3000 - 0xE000)
#define SIMPLE_FLASH_SIZE 0xE000 // 56k

#define SIMPLE_FLASH_SWAP_START (0x200000 - 0x3000)
#define SIMPLE_FLASH_SWAP_SIZE 0x3000 // 12k

#define SIMPLE_FLASH_KEY_ADDR  (0x200000 - 0x3000 - 0xE000 - 0x1000)            //4k

#define UF_PARTITION_START1     (0x200000 - 0x3000 - 0xE000 - 0x1000) - 0xB000
#define UF_PARTITION_SIZE1      0xB000                                          //44k

#define UF_PARTITION_START2     (0x1E0000 - 0x8000)
#define UF_PARTITION_SIZE2      0x8000                                          //32k

#define UF_PARTITION_START3     (0x132000 - 0x8000)
#define UF_PARTITION_SIZE3      0x8000                                          //32k

/***********************************************************
*************************variable define********************
***********************************************************/
static UNI_STORAGE_DESC_S storage = {
    SIMPLE_FLASH_START,
    SIMPLE_FLASH_SIZE,
    FLH_BLOCK_SZ,
    SIMPLE_FLASH_SWAP_START,
    SIMPLE_FLASH_SWAP_SIZE,
    SIMPLE_FLASH_KEY_ADDR
};

static UF_PARTITION_TABLE_S uf_file = {
    .sector_size = PARTITION_SIZE,
    .uf_partition_num = 3,
    .uf_partition = {
        {UF_PARTITION_START1, UF_PARTITION_SIZE1},                        //44K
        {UF_PARTITION_START2, UF_PARTITION_SIZE2},                        //32K
        {UF_PARTITION_START3, UF_PARTITION_SIZE3},                        //32K
    }
};


/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: tuya_hal_flash_read
*  Input: addr size
*  Output: dst
*  Return: none
***********************************************************/
int tuya_hal_flash_read(const uint32_t addr, uint8_t *dst, const uint32_t size)
{
    uint32_t status;
    if(NULL == dst) {
        return OPRT_INVALID_PARM;
    }
	hal_flash_lock();

    DD_HANDLE flash_handle;
    flash_handle = ddev_open(FLASH_DEV_NAME, (UINT32*)&status, 0);
    ddev_read(flash_handle, (char *)dst, size, addr);
    ddev_close(flash_handle);
    
	hal_flash_unlock();

    return OPRT_OK;
}

static uint32_t __uni_flash_is_protect_all(void)
{
    DD_HANDLE flash_handle;
    uint32_t status;
    uint32_t param;
	
    flash_handle = ddev_open(FLASH_DEV_NAME, (UINT32*)&status, 0);
    ddev_control(flash_handle, CMD_FLASH_GET_PROTECT, (void *)&param);	
	ddev_close(flash_handle);	
	//PR_NOTICE("_uni_flash_is_protect_all:%x\r\n",param);
	return (FLASH_PROTECT_ALL == param);
}


/***********************************************************
*  Function: tuya_hal_flash_write
*  Input: addr src size
*  Output: none
*  Return: none
***********************************************************/
int tuya_hal_flash_write(const uint32_t addr, const uint8_t *src, const uint32_t size)
{
    DD_HANDLE flash_handle;
    uint32_t protect_flag;
    uint32_t status;
    uint32_t param;

    if(NULL == src) 
    {
        return OPRT_INVALID_PARM;
    }

	hal_flash_lock();

    protect_flag = __uni_flash_is_protect_all();
    flash_handle = ddev_open(FLASH_DEV_NAME, (UINT32*)&status, 0);

    if(protect_flag)
    {
        param = FLASH_PROTECT_HALF;
        ddev_control(flash_handle, CMD_FLASH_SET_PROTECT, (void *)&param);
    }
    
    ddev_write(flash_handle, (char *)src, size, addr);

    protect_flag = __uni_flash_is_protect_all();

	if(protect_flag)
	{
        param = FLASH_PROTECT_ALL;
        ddev_control(flash_handle, CMD_FLASH_SET_PROTECT, (void *)&param);
    }



    
    ddev_close(flash_handle);
	hal_flash_unlock();

    return OPRT_OK;
}

/***********************************************************
*  Function: tuya_hal_flash_erase
*  Input: addr size
*  Output: 
*  Return: none
***********************************************************/
int tuya_hal_flash_erase(const uint32_t addr, const uint32_t size)
{
    uint16_t start_sec = (addr/PARTITION_SIZE);
    uint16_t end_sec = ((addr+size-1)/PARTITION_SIZE);
    uint32_t status;
    uint32_t i = 0;
    uint32_t sector_addr;
    DD_HANDLE flash_handle;
    uint32_t  param;
    uint32_t protect_flag;

	hal_flash_lock();

    protect_flag = __uni_flash_is_protect_all();
    flash_handle = ddev_open(FLASH_DEV_NAME, (UINT32*)&status, 0);

    if(protect_flag)
    {
        param = FLASH_PROTECT_HALF;
        ddev_control(flash_handle, CMD_FLASH_SET_PROTECT, (void *)&param);
    }

    for(i = start_sec;i <= end_sec;i++) {
        sector_addr = PARTITION_SIZE*i;
        ddev_control(flash_handle, CMD_FLASH_ERASE_SECTOR,(void*)(&sector_addr));
    }

    protect_flag = __uni_flash_is_protect_all();

    if(protect_flag)
    {
        param = FLASH_PROTECT_ALL;
        ddev_control(flash_handle, CMD_FLASH_SET_PROTECT, (void *)&param);
    }
    
    ddev_close(flash_handle);

	hal_flash_unlock();
     
    return OPRT_OK;
}
UF_PARTITION_TABLE_S *tuya_hal_uf_get_desc(void)
{
    return &uf_file;
}

/***********************************************************
*  Function: uni_get_storge_desc
*  Input: none
*  Output: none
*  Return: UNI_STORGE_DESC_S
***********************************************************/
UNI_STORAGE_DESC_S *tuya_hal_storage_get_desc(void)
{
    return &storage;
}

/***********************************************************
*  Function: tuya_hal_flash_set_protect
*  Input: protect flag
*  Output: 
*  Return: none
***********************************************************/
int tuya_hal_flash_set_protect(const bool enable)
{
    DD_HANDLE flash_handle;
    uint32_t  param;
    uint32_t status;

    flash_handle = ddev_open(FLASH_DEV_NAME, (UINT32 *)&status, 0);

    if(enable)
    {
        param = FLASH_PROTECT_ALL;
        ddev_control(flash_handle, CMD_FLASH_SET_PROTECT, (void *)&param);
    }
    else
    {
        param = FLASH_PROTECT_HALF;
        ddev_control(flash_handle, CMD_FLASH_SET_PROTECT, (void *)&param);
    }
    
    ddev_close(flash_handle);
    return OPRT_OK;
}


