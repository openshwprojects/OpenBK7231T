/**
 ****************************************************************************************
 *
 * @file ll.h
 *
 * @brief Declaration of low level functions.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef LL_H_
#define LL_H_



#include <stdint.h>


#include "ble_compiler.h"

#define KEIL_INTRINSICS_INTERRUPT


/** @brief Enable interrupts globally in the system.
 * This macro must be used when the initialization phase is over and the interrupts
 * can start being handled by the system.
 */
 
/* keil Compiler intrinsics for controlling IRQ and FIQ interrupts
*/
 
#undef GLOBAL_INT_START 
#define GLOBAL_INT_START(); \
do { \
	portENABLE_FIQ(); \
	portENABLE_IRQ(); \
} while(0);

/** @brief Disable interrupts globally in the system.
 * This macro must be used when the system wants to disable all the interrupt
 * it could handle.
 */

#undef GLOBAL_INT_STOP
#define GLOBAL_INT_STOP();		\
do { \
						portDISABLE_FIQ(); \
						portDISABLE_IRQ(); \
} while(0);

/* * @brief Disable interrupts globally in the system.
 * This macro must be used in conjunction with the @ref GLOBAL_INT_RESTORE macro since this
 * last one will close the brace that the current macro opens.  This means that both
 * macros must be located at the same scope level.
 */
#undef GLOBAL_INT_DIS
#define GLOBAL_INT_DIS(); 		\
do { 								\
    uint32_t  fiq_tmp; 				\
	  uint32_t  irq_tmp; 			\
		fiq_tmp = portDISABLE_FIQ();\
		irq_tmp = portDISABLE_IRQ();

#undef GLOBAL_INT_RES
#define GLOBAL_INT_RES();		\
			if(!fiq_tmp)			\
			{                     	\
				portENABLE_FIQ(); 	\
			}                     	\
			if(!irq_tmp)           	\
			{                      	\
				portENABLE_IRQ();	\
			}                     	\
} while(0) ;                                       


/** @brief Invoke the wait for interrupt procedure of the processor.
 *
 * @warning It is suggested that this macro is called while the interrupts are disabled
 * to have performed the checks necessary to decide to move to sleep mode.
 *
 */


#endif // LL_H_


