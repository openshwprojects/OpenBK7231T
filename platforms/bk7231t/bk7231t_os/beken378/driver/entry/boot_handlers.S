/**
 ****************************************************************************************
 *
 * @file boot_handlers.s
 *
 * @brief ARM Exception Vector handler functions.
 *
 * Copyright (C) RivieraWaves 2011-2016
 *
 ****************************************************************************************
 */	
	.globl entry_main
	.globl intc_irq
	.globl intc_fiq
	.globl boot_reset
	.globl boot_swi
	.globl boot_undefined
	.globl boot_pabort
	.globl boot_dabort
	.globl boot_reserved
	.globl irq_handler
	.globl fiq_handler
	.globl vPortStartFirstTask
    .globl do_irq
    .globl do_fiq
    .globl do_swi
    .globl do_undefined
    .globl do_pabort
    .globl do_dabort
    .globl do_reserved
    .globl bk_trap_udef
    .globl bk_trap_pabt
    .globl bk_trap_dabt
    .globl bk_trap_resv

#include "sys_config.h"

/* ========================================================================
 *                                Macros
 * ======================================================================== */
#define _FIQ_STACK_SIZE_ 	              0x7F0
#define _IRQ_STACK_SIZE_ 	              0xFF0
#define _SVC_STACK_SIZE_ 	              0x3F0
#define _SYS_STACK_SIZE_ 	              0x3F0
#define _UND_STACK_SIZE_ 	              0x010
#define _ABT_STACK_SIZE_                      0x010


#define BOOT_MODE_MASK     	              0x1F
#define BOOT_MODE_USR       	          0x10
#define BOOT_MODE_FIQ       	          0x11
#define BOOT_MODE_IRQ      	              0x12
#define BOOT_MODE_SVC    	              0x13
#define BOOT_MODE_ABT     	              0x17
#define BOOT_MODE_UND    	              0x1B
#define BOOT_MODE_SYS     	              0x1F
#define BOOT_FIQ_IRQ_MASK 	              0xC0
#define BOOT_IRQ_MASK       	          0x80

#define BOOT_COLOR_UNUSED  	              0xAAAAAAAA      //Pattern to fill UNUSED stack
#define BOOT_COLOR_SVC     	              0xBBBBBBBB      //Pattern to fill SVC stack
#define BOOT_COLOR_IRQ     	              0xCCCCCCCC      //Pattern to fill IRQ stack
#define BOOT_COLOR_FIQ     	              0xDDDDDDDD      //Pattern to fill FIQ stack
#define BOOT_COLOR_SYS     	              0xEEEEEEEE      //Pattern to fill SYS stack

/* ========================================================================
 Context save and restore macro definitions
 * ======================================================================== */


    .macro PUSH_ALL_ARM_REG
        BOOT_CHANGE_MODE  BOOT_MODE_SYS BOOT_MODE_MASK
        LDR R1, =0x400044           // get backup top.
        STMFD   R1!, {R8-R14}       // backup R8-R14
        MRS   R0, CPSR
        STMFD   R1!, {R0}           // backup CPSR

        BOOT_CHANGE_MODE  BOOT_MODE_IRQ BOOT_MODE_MASK
        LDR R1, =0x400068           // get backup top.
        STMFD   R1!, {R8-R14}       // backup R8-R14
        MRS   R0, SPSR
        STMFD   R1!, {R0}           // backup SPSR
        MRS   R0, CPSR
        STMFD   R1!, {R0}           // backup CPSR

        BOOT_CHANGE_MODE  BOOT_MODE_FIQ BOOT_MODE_MASK
        LDR R1, =0x40008C           // get backup top.
        STMFD   R1!, {R8-R14}       // backup R8-R14
        MRS   R0, SPSR
        STMFD   R1!, {R0}           // backup SPSR
        MRS   R0, CPSR
        STMFD   R1!, {R0}           // backup CPSR


        BOOT_CHANGE_MODE  BOOT_MODE_ABT BOOT_MODE_MASK
        LDR R1, =0x4000B0           // get backup top.
        STMFD   R1!, {R8-R14}       // backup R8-R14
        MRS   R0, SPSR
        STMFD   R1!, {R0}           // backup SPSR
        MRS   R0, CPSR
        STMFD   R1!, {R0}           // backup CPSR


        BOOT_CHANGE_MODE  BOOT_MODE_UND BOOT_MODE_MASK
        LDR R1, =0x4000D4           // get backup top.
        STMFD   R1!, {R8-R14}       // backup R8-R14
        MRS   R0, SPSR
        STMFD   R1!, {R0}           // backup SPSR
        MRS   R0, CPSR
        STMFD   R1!, {R0}           // backup CPSR


        BOOT_CHANGE_MODE  BOOT_MODE_SVC BOOT_MODE_MASK
        LDR R1, =0x4000F8           // get backup top.
        STMFD   R1!, {R8-R14}       // backup R8-R14
        MRS   R0, SPSR
        STMFD   R1!, {R0}           // backup SPSR
        MRS   R0, CPSR
        STMFD   R1!, {R0}           // backup CPSR

    .endm

/* ========================================================================
 * Push SVC reg
 */
    .macro PUSH_SVC_REG
        SUB     SP, SP, #18 * 4
        STMIA   SP, {R0 - R12}
        MOV     R0, SP
        MRS     R6, SPSR
        STR     LR, [R0, #15*4]
        STR     R6, [R0, #16*4]
        STR     SP, [R0, #13*4]
        STR     LR, [R0, #14*4]
        MRS     R6, CPSR
        STR     R6, [R0, #17*4]
    .endm
	
/* ========================================================================*/
	.macro   portSAVE_CONTEXT 
	//Push R0 as we are going to use the register.
	STMDB	SP!, {R0}	
	
	MRS R0, spsr
	AND R0, R0, #0x1F
	CMP R0, #0x1F
	BNE 10f		
	
	//Set R0 to point to the task stack pointer.
	STMDB	SP, {SP}^
	NOP
	SUB		SP, SP, #4
	LDMIA	SP!, {R0}

	//Push the return address onto the stack.
	STMDB	R0!, {LR}

	//Now we have saved LR we can use it instead of R0.
	MOV		LR, R0

	//Pop R0 so we can save it onto the system mode stack.
	LDMIA	SP!, {R0}

	//Push all the system mode registers onto the task stack.
	STMDB	LR, {R0-R14}^
	NOP
	NOP
	SUB		LR, LR, #60

	//Push the SPSR onto the task stack.
	MRS		R0, SPSR
	STMDB	LR!, {R0}

	LDR		R0, =ulCriticalNesting 
	LDR		R0, [R0]
	STMDB	LR!, {R0}

	//Store the new top of stack for the task.
	LDR		R0, =pxCurrentTCB
	LDR		R0, [R0]
	STR		LR, [R0]
	
	B       11f
	
10:		
	LDMIA  SP!, {R0}
	
	STMDB	r13!, {r0-r12,r14}
	NOP
	NOP	
	
	LDR		R0, =ulCriticalNesting 
	LDR		R0, [R0]
	STMDB	r13!, {R0}
	
	LDR		R0, =pxCurrentTCB 
	LDR		R0, [R0]
	STMDB	r13!, {R0}
		
	SUB r13, r13, #8
	
11:
	MOV R0, R0
	.endm
		
/* ========================================================================*/
	.macro   portRESTORE_CONTEXT 
		
	MRS R0, spsr
	AND R0, R0, #0x1F
	CMP R0, #0x1F
	BNE 20f
			
	//Set the LR to the task stack.	
	LDR		R0, =pxCurrentTCB
	LDR		R0, [R0]
	LDR		LR, [R0]
			
	//The critical nesting depth is the first item on the stack.
	//Load it into the ulCriticalNesting variable.
	LDR		R0, =ulCriticalNesting
	LDMFD	LR!, {R1}
	STR		R1, [R0]

	//Get the SPSR from the stack.
	LDMFD	LR!, {R0}
	MSR		SPSR_cxsf, R0
		
	//Restore all system mode registers for the task.
	LDMFD	LR, {R0-R14}^
	NOP
	NOP

	//Restore the return address.
	LDR		LR, [LR, #+60]

	//And return - correcting the offset in the LR to obtain the
	//correct address.
	SUBS	PC, LR, #4
	
20:		
	ADD r13,r13,#0x8
	
	LDR		R0, =pxCurrentTCB
	LDMFD	r13!, {R1}
	STR		R1, [R0]
		
	LDR		R0, =ulCriticalNesting
	LDMFD	r13!, {R1}
	STR		R1, [R0]
		
	LDMIA	r13!, {r0-r12,r14}
	NOP
	NOP	
		
	SUBS    pc, r14, #0x4
	.endm

/* ========================================================================*/
.macro	 firstRESTORE_CONTEXT 			
	//Set the LR to the task stack.
	LDR 	R0, =pxCurrentTCB
	LDR 	R0, [R0]
	LDR 	LR, [R0]

	//The critical nesting depth is the first item on the stack.
	//Load it into the ulCriticalNesting variable.
	LDR 	R0, =ulCriticalNesting
	LDMFD	LR!, {R1}
	STR 	R1, [R0]

	//Get the SPSR from the stack.
	LDMFD	LR!, {R0}
	MSR 	SPSR_cxsf, R0

	//Restore all system mode registers for the task.
	LDMFD	LR, {R0-R14}^
	NOP
	NOP

	//Restore the return address.
	LDR 	LR, [LR, #+60]

	//And return - correcting the offset in the LR to obtain the
	//correct address.
	SUBS	PC, LR, #4
	.endm

/* ========================================================================
/**
 * Macro for switching ARM mode
 */
	.macro	BOOT_CHANGE_MODE, mode, mode_mask
	MRS   R0, CPSR
	BIC   R0, R0, #\mode_mask
	ORR   R0, R0, #\mode
	MSR   CPSR_c, R0
	.endm

/* ========================================================================
/**
 * Macro for setting the stack
 */
	.macro  BOOT_SET_STACK, stackStart, stackLen, color
	LDR   R0, \stackStart
	LDR   R1, \stackLen

	ADD   R1, R1, R0
	MOV   SP, R1        //Set stack pointer

	LDR   R2, =\color

3:
	CMP   R0, R1        //End of stack?
	STRLT R2, [r0]      //Colorize stack word
	ADDLT R0, R0, #4
	BLT   3b           //branch to previous local label
	.endm


	 .section .data
	 .align 3
	 .global und_stack_start
und_stack_start:
	 .space _UND_STACK_SIZE_
 
	.align 3
	.global abt_stack_start
abt_stack_start:
	 .space _ABT_STACK_SIZE_

	.align 3
	.global fiq_stack_start
fiq_stack_start:
	 .space _FIQ_STACK_SIZE_

	.align 3
	.global irq_stack_start
irq_stack_start:
	 .space _IRQ_STACK_SIZE_
 
	.align 3
	.global sys_stack_start
sys_stack_start:
	 .space _SYS_STACK_SIZE_

	.align 3
	.global svc_stack_start
svc_stack_start:
	 .space _SVC_STACK_SIZE_

/* ========================================================================
 *                                Functions
 * ========================================================================

/* ========================================================================
 * Function to handle reset vector
 */
	.globl	boot_reset
	.section ".boot", "ax"

boot_reset:
    //Disable IRQ and FIQ before starting anything
    MRS   R0, CPSR
    ORR   R0, R0, #0xC0
    MSR   CPSR_c, R0

    //Setup all stacks //Note: Abt and Usr mode are not used
	BOOT_CHANGE_MODE BOOT_MODE_SYS BOOT_MODE_MASK
	BOOT_SET_STACK   boot_stack_base_SYS boot_stack_len_SYS BOOT_COLOR_SYS

	BOOT_CHANGE_MODE BOOT_MODE_ABT BOOT_MODE_MASK
	BOOT_SET_STACK   boot_stack_base_UNUSED boot_stack_len_UNUSED BOOT_COLOR_UNUSED

	BOOT_CHANGE_MODE BOOT_MODE_UND BOOT_MODE_MASK
	BOOT_SET_STACK   boot_stack_base_UNUSED boot_stack_len_UNUSED BOOT_COLOR_UNUSED

	BOOT_CHANGE_MODE BOOT_MODE_IRQ BOOT_MODE_MASK
	BOOT_SET_STACK   boot_stack_base_IRQ boot_stack_len_IRQ BOOT_COLOR_IRQ

	BOOT_CHANGE_MODE BOOT_MODE_FIQ BOOT_MODE_MASK
	BOOT_SET_STACK   boot_stack_base_FIQ boot_stack_len_FIQ BOOT_COLOR_FIQ

	//Clear FIQ banked registers while in FIQ mode
	MOV     R8,  #0
	MOV     R9,  #0
	MOV     R10, #0
	MOV     R11, #0
	MOV     R12, #0

	BOOT_CHANGE_MODE BOOT_MODE_SVC BOOT_MODE_MASK
	BOOT_SET_STACK   boot_stack_base_SVC boot_stack_len_SVC BOOT_COLOR_SVC

    //Stay in Supervisor Mode
    //copy data from binary to ram
    BL _sysboot_copy_data_to_ram
    
    ///*Init the BSS section*/
    BL _sysboot_zi_init
    
    //==================
    //Clear Registers
    MOV R0, #0
    MOV R1, #0
    MOV R2, #0
    MOV R3, #0
    MOV R4, #0
    MOV R5, #0
    MOV R6, #0
    MOV R7, #0
    MOV R8, #0
    MOV R9, #0
    MOV R10, #0
    MOV R11, #0
    MOV R12, #0

    B entry_main

		
/* ========================================================================
 *								  Globals
 * ======================================================================== */
boot_stack_base_UNUSED:
	.word und_stack_start

boot_stack_len_UNUSED:
	.word _UND_STACK_SIZE_

boot_stack_base_IRQ:
	.word irq_stack_start

boot_stack_len_IRQ:
	.word _IRQ_STACK_SIZE_

boot_stack_base_SVC:
	.word svc_stack_start

boot_stack_len_SVC:
	 .word _SVC_STACK_SIZE_

boot_stack_base_FIQ:
	 .word fiq_stack_start

boot_stack_len_FIQ:
	.word _FIQ_STACK_SIZE_

boot_stack_base_SYS:
	 .word sys_stack_start

boot_stack_len_SYS:
	 .word _SYS_STACK_SIZE_  

/*FUNCTION:     _sysboot_copy_data_to_ram*/
/*DESCRIPTION:  copy main stack code from FLASH/ROM to SRAM*/
_sysboot_copy_data_to_ram:
    LDR     R0, =_data_flash_begin
    LDR     R1, =_data_ram_begin
    LDR     R2, =_data_ram_end
	
4: CMP R1, R2
    LDRLO   R4, [R0], #4
    STRLO   R4, [R1], #4
    BLO     4b
    BX LR
        
/*FUNCTION:     _sysboot_zi_init*/
/*DESCRIPTION:  Initialise Zero-Init Data Segment*/
_sysboot_zi_init:
    LDR     R0, =_bss_start
    LDR     R1, =_bss_end
    
    MOV R3, R1
    MOV R4, R0
    MOV R2, #0
5: CMP R4, R3
    STRLO R2, [R4], #4
    BLO 5b
    BX LR

        .align  5
do_undefined:
    LDMFD   SP!, {R0-R1}
    PUSH_SVC_REG

    STMFD  sp!,{r0-r1}
    PUSH_ALL_ARM_REG
    BOOT_CHANGE_MODE  BOOT_MODE_UND BOOT_MODE_MASK
    LDMFD   SP!, {R0-R1}

    B bk_trap_udef

        .align  5
do_pabort:
    LDMFD   SP!, {R0-R1}
    PUSH_SVC_REG

    STMFD  sp!,{r0-r1}
    PUSH_ALL_ARM_REG
    BOOT_CHANGE_MODE  BOOT_MODE_ABT BOOT_MODE_MASK
    LDMFD   SP!, {R0-R1}

    B bk_trap_pabt

        .align  5
do_dabort:
    LDMFD   SP!, {R0-R1}
    PUSH_SVC_REG

    STMFD  sp!,{r0-r1}
    PUSH_ALL_ARM_REG
    BOOT_CHANGE_MODE  BOOT_MODE_ABT BOOT_MODE_MASK
    LDMFD   SP!, {R0-R1}

    B bk_trap_dabt

        .align  5
do_reserved:
    LDMFD   SP!, {R0-R1}
    PUSH_SVC_REG
    B bk_trap_resv


        .align  5
boot_undefined:
    STMFD  sp!,{r0-r1}
    LDR    R1, =0x40000c
    LDR    r0, [R1]
    BX     r0

        .align  5
boot_swi:
    B vPortYieldProcessor

        .align  5
boot_pabort:
    STMFD  sp!,{r0-r1}
    LDR    R1, =0x400010
    LDR    r0, [R1]
    BX     r0

        .align  5
boot_dabort:
    STMFD  sp!,{r0-r1}
    LDR    R1, =0x400014
    LDR    r0, [R1]
    BX     r0

        .align  5
boot_reserved:
    STMFD  sp!,{r0-r1}
    LDR    R1, =0x400018
    LDR    r0, [R1]
    BX     r0

		.align	5 
irq_handler:	
	portSAVE_CONTEXT
	LDR R0, =intc_irq
	MOV LR, PC
	BX R0
	portRESTORE_CONTEXT
	
		.align	5    
fiq_handler:	
	portSAVE_CONTEXT
	LDR R0, =intc_fiq
	MOV LR, PC
	BX R0
	portRESTORE_CONTEXT 
	
/*Starting the first task is just a matter of restoring the context that*/
/*was created by pxPortInitialiseStack().*/
vPortStartFirstTask:
	firstRESTORE_CONTEXT

/*Manual context switch function.  This is the SWI hander.*/
vPortYieldProcessor:
#if (0 == CFG_SUPPORT_BOOTLOADER)
	ADD		LR, LR, #4			//Add 4 to the LR to make the LR appear exactly
								//as if the context was saved during and IRQ
								//handler.
#endif // CFG_SUPPORT_BOOTLOADER

	portSAVE_CONTEXT			//Save the context of the current task...
	
	LDR R0, =vTaskSwitchContext
	MOV LR, PC
	BX R0
	
	portRESTORE_CONTEXT			//Restore the context of the selected task.
	


    .code 32
    .global WFI
    .type WFI,%function
WFI:
	MOV R0, #0
	MCR p15, 0, R0, c7, c0, 4
    MCR p15, 0, R0, c15, c8, 2
    MCR p15, 0, R0, c7, c10, 4
	BX LR
/*EOF*/

