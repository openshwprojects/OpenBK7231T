/**
 ******************************************************************************
 * @file    platform_init.c
 * @version V1.0.0
 * @date    05-May-2014
 * @brief   This file provide functions called by _BK_ to drive stm32f2xx
 *          platform: - e.g. power save, reboot, platform initialize
 ******************************************************************************
 *  UNPUBLISHED PROPRIETARY SOURCE CODE
 *  Copyright (c) 2016 BEKEN Inc.
 *
 *  The contents of this file may not be disclosed to third parties, copied or
 *  duplicated in any form, in whole or in part, without the prior written
 *  permission of BEKEN Corporation.
 ******************************************************************************
 */
#include "include.h" 
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>

#include "sys_rtos.h"
#include "mem_pub.h"
#include "uart_pub.h"

/************** wrap C library functions **************/
void * __wrap_malloc (size_t size)
{
	return os_malloc(size);
}

void * __wrap__malloc_r (void *p, size_t size)
{
	
	return os_malloc(size);
}

void __wrap_free (void *pv)
{
	vPortFree(pv);
}

void * __wrap_calloc (size_t a, size_t b)
{
	void *pvReturn;

    pvReturn = os_malloc( a*b );
    if (pvReturn)
    {
        os_memset(pvReturn, 0, a*b);
    }

    return pvReturn;
}

void * __wrap_realloc (void* pv, size_t size)
{
	return pvPortRealloc(pv, size);
}

void __wrap__free_r (void *p, void *x)
{
  __wrap_free(x);
}

void* __wrap__realloc_r (void *p, void* x, size_t sz)
{
  return __wrap_realloc (x, sz);
}

void * __wrap_zalloc(size_t size)
{
	return os_zalloc(size);
}

void __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
	os_printf("%s %d func %s expr %s\n", file, line, func, failedexpr);
	ASSERT(0);
}
void __attribute__((weak)) rwip_wakeup(void) {}
void __attribute__((weak)) rwip_wakeup_end(void) {}
uint8_t __attribute__((weak)) ble_switch_mac_sleeped = 0;
uint8_t __attribute__((weak)) ble_active = 0;
void __attribute__((weak)) ble_switch_rf_to_wifi(void) {}
uint8_t __attribute__((weak)) ble_is_start(void)
{
	return 0;
}
uint32_t __attribute__((weak)) ble_in_dut_mode(void)
{
	return 0;
}
uint8 __attribute__((weak)) is_rf_switch_to_ble(void)
{
	return 0;
}
uint32_t ea_time_get_halfslot_rounded(void);
uint32_t ea_timer_next_target_get(void);
uint32_t __attribute__((weak)) rwip_get_current_time(void)
{
	return (ea_time_get_halfslot_rounded());
}
uint32_t __attribute__((weak)) rwip_get_next_target_time(void)
{
	return (ea_timer_next_target_get());
}

// eof

