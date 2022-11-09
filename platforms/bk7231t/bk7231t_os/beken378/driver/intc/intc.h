/**
 ****************************************************************************************
 *
 * @file intc.h
 *
 * @brief Declaration of the Reference Interrupt Controller (INTC) API.
 *
 * Copyright (C) RivieraWaves 2011-2016
 *
 ****************************************************************************************
 */

#ifndef _INTC_H_
#define _INTC_H_

#include "compiler.h"
#include "doubly_list.h"
#include "generic.h"

//#define INTC_DEBUG

#ifdef INTC_DEBUG
#define INTC_PRT       os_printf
#define INTC_WPRT      warning_prf
#else
#define INTC_PRT       os_null_printf
#define INTC_WPRT      os_null_printf
#endif

#define INTC_MAX_COUNT                    32


struct arm_registers
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t fp;
    uint32_t ip;
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
    uint32_t spsr;
    uint32_t cpsr;
};

typedef struct
{
    LIST_HEADER_T list;

    FUNCPTR isr_func;

    UINT8  pri;            // priority
    UINT8  int_num;
    UINT16 resv;
} ISR_T;

typedef struct
{
    LIST_HEADER_T isr;

    UINT8 max;     // maximum interrupt number
    UINT8 min;     // minum interrupt number
} ISR_LIST_T;
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize and configure the reference INTCTRL.
 * This function configures the INTC according to the system needs.
 ****************************************************************************************
 */
extern void intc_init(void);
extern void intc_irq(void);
extern void intc_fiq(void);
extern void do_irq( void );
extern void do_fiq( void );
extern void do_swi( void );
extern void do_undefined( void );
extern void do_pabort( void );
extern void do_dabort( void );
extern void do_reserved( void );
extern void intc_service_change_handler(UINT8 int_num, FUNCPTR isr);

#endif // _INTC_H_
