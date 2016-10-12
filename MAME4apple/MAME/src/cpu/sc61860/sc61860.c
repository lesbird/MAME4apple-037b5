/*****************************************************************************
 *
 *	 sc61860.c
 *	 portable sharp 61860 emulator interface
 *   (sharp pocket computers)
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   peter.trauner@jk.uni-linz.ac.at
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *****************************************************************************/
#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "mamedbg.h"

#include "sc61860.h"
#include "sc.h"

typedef int bool;

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

/* Layout of the registers in the debugger */
static UINT8 sc61860_reg_layout[] = {
	SC61860_P,
	SC61860_Q,
	SC61860_R,
	SC61860_DP, 
	SC61860_PC, 
	-1,

	SC61860_I, 
	SC61860_K, 
	SC61860_V, 
	SC61860_X, 
	SC61860_H, 
	-1,

	SC61860_J, 
	SC61860_L, 
	SC61860_W,
	SC61860_Y,
	SC61860_BA, 
	0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 sc61860_win_layout[] = {
	25, 0,55, 3,	/* register window (top, right rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
	25, 4,55, 8,	/* memory #1 window (right, upper middle) */
	25,14,55, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/****************************************************************************
 * The 6502 registers.
 ****************************************************************************/
typedef struct
{
	SC61860_CONFIG *config;
	UINT8 ram[0x60]; // internal special ram
	UINT8 p, q, r; //6 bits only?

	UINT16 oldpc, pc, dp;
	
	bool carry, zero;

	struct { bool t2ms, t512ms; int count;} timer;
}	SC61860_Regs;

int sc61860_icount = 0;

static SC61860_Regs sc61860;

UINT8 *sc61860_internal_ram(void) { return sc61860.ram; }

void sc61860_2ms_tick(int param)
{
	if (--sc61860.timer.count==0) {
		sc61860.timer.count=256;
		sc61860.timer.t512ms=!sc61860.timer.t512ms;
	}
	sc61860.timer.t2ms=!sc61860.timer.t2ms;
}


READ_HANDLER(sc61860_read_internal)
{
	return sc61860.ram[offset];
}

WRITE_HANDLER(sc61860_write_internal)
{
	sc61860.ram[offset]=data;
}

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/
#include "ops.c"
#include "table.c"

void sc61860_reset(void *param)
{
	if (param) {
		sc61860.config=(SC61860_CONFIG *)param;
	}
	sc61860.timer.t2ms=0;
	sc61860.timer.t512ms=0;
	sc61860.timer.count=256;
	sc61860.pc=0;
	change_pc(sc61860.pc);
}

void sc61860_exit(void)
{
	/* nothing to do yet */
}

unsigned sc61860_get_context (void *dst)
{
	if( dst )
		*(SC61860_Regs*)dst = sc61860;
	return sizeof(SC61860_Regs);
}

void sc61860_set_context (void *src)
{
	if( src )
	{
		sc61860 = *(SC61860_Regs*)src;
		change_pc(sc61860.pc);
	}
}

unsigned sc61860_get_pc (void)
{
	return sc61860.pc;
}

void sc61860_set_pc (unsigned val)
{
	sc61860.pc = val;
	change_pc(sc61860.pc);
}

unsigned sc61860_get_sp (void)
{
	return sc61860.r;
}

void sc61860_set_sp (unsigned val)
{
	sc61860.r=val;
}

unsigned sc61860_get_reg (int regnum)
{
	switch( regnum )
	{
	case SC61860_PC: return sc61860.pc;
	case SC61860_DP: return sc61860.dp;
	case SC61860_P: return sc61860.p;
	case SC61860_Q: return sc61860.q;
	case SC61860_R: return sc61860.r;
	case SC61860_CARRY: return sc61860.carry;
	case SC61860_ZERO: return sc61860.zero;
	case REG_PREVIOUSPC: return sc61860.oldpc;
#if 0
	case SATURN_NMI_STATE: return saturn.nmi_state;
	case SATURN_IRQ_STATE: return saturn.irq_state;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0x1ff )
					return RDMEM( offset ) | ( RDMEM( offset + 1 ) << 8 );
			}
#endif
	}
	return 0;
}

void sc61860_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
	case SC61860_PC: sc61860.pc=val;break;
	case SC61860_DP: sc61860.dp=val;break;
	case SC61860_P: sc61860.p=val;break;
	case SC61860_Q: sc61860.q=val;break;
	case SC61860_R: sc61860.r=val;break;
	case SC61860_CARRY: sc61860.carry=val;break;
	case SC61860_ZERO: sc61860.zero=val;break;
#if 0
	case SATURN_NMI_STATE: saturn.nmi_state=val;break;
	case SATURN_IRQ_STATE: saturn.irq_state=val;break;
	default:
		if( regnum <= REG_SP_CONTENTS )
		{
			unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
			if( offset < 0x1ff )
			{
				WRMEM( offset, val & 0xfff );
				WRMEM( offset + 1, (val >> 8) & 0xff );
			}
		}
#endif
	}
}

INLINE void sc61860_take_irq(void)
{
}

int sc61860_execute(int cycles)
{
	sc61860_icount = cycles;

	change_pc(sc61860.pc);

	do
	{
		sc61860.oldpc = sc61860.pc;

		CALL_MAME_DEBUG;

		sc61860_instruction();

	} while (sc61860_icount > 0);

	return cycles - sc61860_icount;
}

void sc61860_set_nmi_line(int state)
{
}

void sc61860_set_irq_line(int irqline, int state)
{
}

void sc61860_set_irq_callback(int (*callback)(int))
{
}

void sc61860_state_save(void *file)
{
	int cpu = cpu_getactivecpu();
	state_save_UINT16(file,"sc61860",cpu,"PC",&sc61860.pc,2);
	state_save_UINT16(file,"sc61860",cpu,"DP",&sc61860.dp,2);
	state_save_UINT8(file,"sc61860",cpu,"P",&sc61860.p,1);
	state_save_UINT8(file,"sc61860",cpu,"Q",&sc61860.q,1);
	state_save_UINT8(file,"sc61860",cpu,"R",&sc61860.r,1);
//	state_save_UINT8(file,"sc61860",cpu,"C",&sc61860.carry,1);
//	state_save_UINT8(file,"sc61860",cpu,"Z",&sc61860.zero,1);
	// internal ram
}

void sc61860_state_load(void *file)
{
	int cpu = cpu_getactivecpu();
	state_load_UINT16(file,"sc61860",cpu,"PC",&sc61860.pc,2);
	state_load_UINT16(file,"sc61860",cpu,"DP",&sc61860.dp,2);
	state_load_UINT8(file,"sc61860",cpu,"P",&sc61860.p,1);
	state_load_UINT8(file,"sc61860",cpu,"Q",&sc61860.q,1);
	state_load_UINT8(file,"sc61860",cpu,"R",&sc61860.r,1);
//	state_load_UINT8(file,"sc61860",cpu,"C",&sc61860.carry,1);
//	state_load_UINT8(file,"sc61860",cpu,"Z",&sc61860.zero,1);
	// internal ram
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *sc61860_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	SC61860_Regs *r = context;

	which = ++which % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &sc61860;

	switch( regnum )
	{
	case CPU_INFO_REG+SC61860_PC: sprintf(buffer[which],"PC:%.4x",r->pc);break;
	case CPU_INFO_REG+SC61860_DP: sprintf(buffer[which],"DP:%.4x",r->dp);break;
	case CPU_INFO_REG+SC61860_P: sprintf(buffer[which],"P:%.2x",r->p);break;
	case CPU_INFO_REG+SC61860_Q: sprintf(buffer[which],"Q:%.2x",r->q);break;
	case CPU_INFO_REG+SC61860_R: sprintf(buffer[which],"R:%.2x",r->r);break;
	case CPU_INFO_REG+SC61860_I: sprintf(buffer[which],"I:%.2x",r->ram[I]);break;
	case CPU_INFO_REG+SC61860_J: sprintf(buffer[which],"J:%.2x",r->ram[J]);break;
	case CPU_INFO_REG+SC61860_K: sprintf(buffer[which],"K:%.2x",r->ram[K]);break;
	case CPU_INFO_REG+SC61860_L: sprintf(buffer[which],"L:%.2x",r->ram[L]);break;
	case CPU_INFO_REG+SC61860_V: sprintf(buffer[which],"V:%.2x",r->ram[V]);break;
	case CPU_INFO_REG+SC61860_W: sprintf(buffer[which],"W:%.2x",r->ram[W]);break;
	case CPU_INFO_REG+SC61860_H: sprintf(buffer[which],"H:%.2x",r->ram[H]);break;
	case CPU_INFO_REG+SC61860_BA: 
		sprintf(buffer[which],"BA:%.2x%.2x",r->ram[B],r->ram[A]);break;
	case CPU_INFO_REG+SC61860_X: 
		sprintf(buffer[which],"X: %.2x%.2x",r->ram[XH],r->ram[XL]);break;
	case CPU_INFO_REG+SC61860_Y: 
		sprintf(buffer[which],"Y: %.2x%.2x",r->ram[YH],r->ram[YL]);break;
	case CPU_INFO_REG+SC61860_CARRY: sprintf(buffer[which],"Carry: %d",r->carry);break;
	case CPU_INFO_REG+SC61860_ZERO: sprintf(buffer[which],"Carry: %d",r->zero);break;
	case CPU_INFO_FLAGS: sprintf(buffer[which], "%c%c", r->zero?'Z':'.', r->carry ? 'C':'.'); break;
	case CPU_INFO_NAME: return "SC61860";
	case CPU_INFO_FAMILY: return "SC61860";
	case CPU_INFO_VERSION: return "1.0alpha";
	case CPU_INFO_FILE: return __FILE__;
	case CPU_INFO_CREDITS: return "Copyright (c) 2000 Peter Trauner, all rights reserved.";
	case CPU_INFO_REG_LAYOUT: return (const char*)sc61860_reg_layout;
	case CPU_INFO_WIN_LAYOUT: return (const char*)sc61860_win_layout;
	}
	return buffer[which];
}

#ifndef MAME_DEBUG
unsigned sc61860_dasm(char *buffer, unsigned pc)
{
	sprintf( buffer, "$%X", cpu_readop(pc) );
	return 1;
}
#endif
