/**
 ****************************************************************************************
 *
 * @file architect.h
 *
 * @brief This file contains the definitions of the macros and functions that are
 * architecture dependent.  The implementation of those is implemented in the
 * appropriate architecture directory.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */


#ifndef _ARCHITECT_H_
#define _ARCHITECT_H_

/**
 ****************************************************************************************
 * @defgroup REFIP
 * @brief Reference IP Platform
 *
 * This module contains reference platform components - REFIP.
 *
 *
 * @{
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup DRIVERS
 * @ingroup REFIP
 * @brief Reference IP Platform Drivers
 *
 * This module contains the necessary drivers to run the platform with the
 * RW BT SW protocol stack.
 *
 * This has the declaration of the platform architecture API.
 *
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "generic.h"
#include <stdint.h>        // standard integer definition
#include "ble_compiler.h"      // inline functions

/*
 * CPU WORD SIZE
 ****************************************************************************************
 */
/// ARM is a 32-bit CPU
#define CPU_WORD_SIZE   4

/*
 * CPU Endianness
 ****************************************************************************************
 */
/// ARM is little endian
#define CPU_LE          1

/*
 * DEBUG configuration
 ****************************************************************************************
 */
#if defined(CFG_DBG)
#define PLF_DEBUG          1
#else //CFG_DBG
#define PLF_DEBUG          0
#endif //CFG_DBG


/*
 * NVDS
 ****************************************************************************************
 */

/// NVDS
#ifdef CFG_NVDS
#define PLF_NVDS             1
#else // CFG_NVDS
#define PLF_NVDS             0
#endif // CFG_NVDS


/*
 * UART
 ****************************************************************************************
 */

/// UART
#define PLF_UART             0

/*
 * DEFINES
 ****************************************************************************************
 */

/// Possible errors detected by FW
#define    RESET_NO_ERROR         0x00000000
#define    RESET_MEM_ALLOC_FAIL   0xF2F2F2F2

/// Reset platform and stay in ROM
#define    RESET_TO_ROM           0xA5A5A5A5
/// Reset platform and reload FW
#define    RESET_AND_LOAD_FW      0xC3C3C3C3

/// Exchange memory size limit
#define    EM_SIZE_LIMIT          0x8000
/*
 * EXPORTED FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Compute size of SW stack used.
 *
 * This function is compute the maximum size stack used by SW.
 *
 * @return Size of stack used (in bytes)
 ****************************************************************************************
 */
uint16_t get_stack_usage(void);

/**
 ****************************************************************************************
 * @brief Re-boot FW.
 *
 * This function is used to re-boot the FW when error has been detected, it is the end of
 * the current FW execution.
 * After waiting transfers on UART to be finished, and storing the information that
 * FW has re-booted by itself in a non-loaded area, the FW restart by branching at FW
 * entry point.
 *
 * Note: when calling this function, the code after it will not be executed.
 *
 * @param[in] error      Error detected by FW
 ****************************************************************************************
 */
void platform_reset(uint32_t error);

//#if PLF_DEBUG
/**
 ****************************************************************************************
 * @brief Print the assertion error reason and loop forever.
 *
 * @param condition C string containing the condition.
 * @param file C string containing file where the assertion is located.
 * @param line Line number in the file where the assertion is located.
 ****************************************************************************************
 */
void assert_err(const char *condition, const char * file, int line);

/**
 ****************************************************************************************
 * @brief Print the assertion error reason and loop forever.
 * The parameter value that is causing the assertion will also be disclosed.
 *
 * @param param0 parameter value 0.
 * @param param1 parameter value 1.
 * @param file C string containing file where the assertion is located.
 * @param line Line number in the file where the assertion is located.
 ****************************************************************************************
 */
void assert_param(int param0, int param1, const char * file, int line);

/**
 ****************************************************************************************
 * @brief Print the assertion warning reason.
 *
 * @param param0 parameter value 0.
 * @param param1 parameter value 1.
 * @param file C string containing file where the assertion is located.
 * @param line Line number in the file where the assertion is located.
 ****************************************************************************************
 */
void assert_warn(int param0, int param1, const char * file, int line);


/**
 ****************************************************************************************
 * @brief Dump data value into FW.
 *
 * @param data start pointer of the data.
 * @param length data size to dump
 ****************************************************************************************
 */
void dump_data(uint8_t* data, uint16_t length);
//#endif //PLF_DEBUG


/*
 * ASSERTION CHECK
 ****************************************************************************************
 */
#if PLF_DEBUG
/// Assertions showing a critical error that could require a full system reset
#undef ASSERT_ERR
#define ASSERT_ERR(cond)              ASSERT(cond)

/// Assertions showing a critical error that could require a full system reset
#undef ASSERT_INFO
#define ASSERT_INFO(cond, param0, param1)             \
    do {                                              \
        if (!(cond)) {                                \
            assert_param((int)param0, (int)param1, __MODULE__, __LINE__);  \
        }                                             \
    } while(0)

/// Assertions showing a non-critical problem that has to be fixed by the SW
#undef ASSERT_WARN
#define ASSERT_WARN(cond)             ASSERT(cond)

#undef DUMP_DATA
#define DUMP_DATA(data, length) \
    dump_data((uint8_t*)data, length)

#else
/// Assertions showing a critical error that could require a full system reset
#undef ASSERT_ERR
#define ASSERT_ERR(cond)

/// Assertions showing a critical error that could require a full system reset
#undef ASSERT_INFO
#define ASSERT_INFO(cond, param0, param1)

/// Assertions showing a non-critical problem that has to be fixed by the SW
#undef ASSERT_WARN
#define ASSERT_WARN(cond)

/// DUMP data array present in the SW.
#undef DUMP_DATA
#define DUMP_DATA(data, length)
#endif //PLF_DEBUG

#define FAST_MAIN_ENTRY  __attribute__((section("sys_main_entry")))
#define FAST_IRQ_ENTRY  __attribute__((section("sys_irq_entry")))
#define FAST_FIQ_ENTRY  __attribute__((section("sys_fiq_entry")))


/// Object allocated in shared memory - check linker script
#define __SHARED __attribute__ ((section("shram")))

// required to define GLOBAL_INT_** macros as inline assembly. This file is included after
// definition of ASSERT macros as they are used inside ll.h
#include "ll.h"     // ll definitions
/// @} DRIVERS
#endif // _ARCH_H_
