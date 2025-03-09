/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

#define TRAPHANDLER(name, num)                \
    .globl name;                              \
    .type name, @function;                    \
    .align 2;                                 \
    name:                                         \
    pushl $(num);        /* Push trap number */       \
    jmp _alltraps

#define TRAPHANDLER_NOEC(name, num)           \
    .globl name;                              \
    .type name, @function;                    \
    .align 2;                                 \
    name:                                         \
	pushl $0;            /* Push dummy error code */  \
    pushl $(num);        /* Push trap number */       \
    jmp _alltraps

.text

###################################################################
# exceptions/interrupts
###################################################################

/* Define trap handlers */
TRAPHANDLER_NOEC(th0, 0)        /* T_DIVIDE: Divide Error */
TRAPHANDLER_NOEC(th1, 1)        /* T_DEBUG: Debug Exception */
//TRAPHANDLER_NOEC(th2, 2)        /* T_NMI: Non-Maskable Interrupt */
TRAPHANDLER_NOEC(th3, 3)        /* T_BRKPT: Breakpoint */
TRAPHANDLER_NOEC(th4, 4)        /* T_OFLOW: Overflow */
TRAPHANDLER_NOEC(th5, 5)        /* T_BOUND: Bounds Check */
TRAPHANDLER_NOEC(th6, 6)        /* T_ILLOP: Illegal Opcode */
TRAPHANDLER_NOEC(th7, 7)        /* T_DEVICE: Device Not Available */
TRAPHANDLER(th8, 8)             /* T_DBLFLT: Double Fault */
TRAPHANDLER_NOEC(th9, 9)        /* T_COPROC: Reserved */
TRAPHANDLER(th10, 10)           /* T_TSS: Invalid Task Switch Segment */
TRAPHANDLER(th11, 11)           /* T_SEGNP: Segment Not Present */
TRAPHANDLER(th12, 12)           /* T_STACK: Stack Exception */
TRAPHANDLER(th13, 13)           /* T_GPFLT: General Protection Fault */
TRAPHANDLER(th14, 14)           /* T_PGFLT: Page Fault */
TRAPHANDLER_NOEC(th16, 16)      /* T_FPERR: Floating Point Error */
//TRAPHANDLER(th17, 17)           /* T_ALIGN: Alignment Check */
//TRAPHANDLER_NOEC(th18, 18)      /* T_MCHK: Machine Check */
//TRAPHANDLER_NOEC(th19, 19)      /* T_SIMDERR: SIMD Floating Point Error */
TRAPHANDLER_NOEC(t_syscall, T_SYSCALL)      /* T_SYSCALL: System Call */


###################################################################
# IRQ Handlers (IRQs 0-15)
###################################################################

TRAPHANDLER_NOEC(th32, IRQ_OFFSET)
TRAPHANDLER_NOEC(th33, IRQ_OFFSET + 1)
TRAPHANDLER_NOEC(th34, IRQ_OFFSET + 2)
TRAPHANDLER_NOEC(th35, IRQ_OFFSET + 3)
TRAPHANDLER_NOEC(th36, IRQ_OFFSET + 4)
TRAPHANDLER_NOEC(th37, IRQ_OFFSET + 5)
TRAPHANDLER_NOEC(th38, IRQ_OFFSET + 6)
TRAPHANDLER_NOEC(th39, IRQ_OFFSET + 7)
TRAPHANDLER_NOEC(th40, IRQ_OFFSET + 8)
TRAPHANDLER_NOEC(th41, IRQ_OFFSET + 9)
TRAPHANDLER_NOEC(th42, IRQ_OFFSET + 10)
TRAPHANDLER_NOEC(th43, IRQ_OFFSET + 11)
TRAPHANDLER_NOEC(th44, IRQ_OFFSET + 12)
TRAPHANDLER_NOEC(th45, IRQ_OFFSET + 13)
TRAPHANDLER_NOEC(th46, IRQ_OFFSET + 14)
TRAPHANDLER_NOEC(th47, IRQ_OFFSET + 15)


// HINT 1 : TRAPHANDLER_NOEC(t_divide, T_DIVIDE);
//          Do something like this if there is no error code for the trap
// HINT 2 : TRAPHANDLER(t_dblflt, T_DBLFLT);
//          Do something like this if the trap includes an error code..
// HINT 3 : READ Intel's manual to check if the trap includes an error code
//          or not...

###################################################################
# _alltraps
###################################################################

_alltraps:
	pushl %ds
	pushl %es
	pushal
	pushl $GD_KD
	popl %ds
	pushl $GD_KD
	popl %es
	pushl %esp
	call trap









