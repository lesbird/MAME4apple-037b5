/*
	Interface routine for 68kem <-> Mame
*/

#include "driver.h"
#include "m68000.h"
#include "d68k.h"
#include "mamedbg.h"

#ifdef WIN32
#define CONVENTION __cdecl
#else
#define CONVENTION
#endif

/* Use the x86 assembly core */
typedef struct
{
    int d[8];             /* 0x0004 8 Data registers */
    int a[8];             /* 0x0024 8 Address registers */

    int isp;              /* 0x0048 */

    int sr_high;          /* 0x004C System registers */
    int ccr;              /* 0x0050 CCR in Intel Format */
    int x_carry;          /* 0x0054 Extended Carry */

    int pc;               /* 0x0058 Program Counter */

    int IRQ_level;        /* 0x005C IRQ level you want the MC68K process (0=None)  */

    /* Backward compatible with C emulator - Only set in Debug compile */

    int sr;

    int (*irq_callback)(int irqline);

    int previous_pc;      /* last PC used */

    int (*reset_callback)(void);

    int sfc;              /* Source Function Code. (68010) */
    int dfc;              /* Destination Function Code. (68010) */
    int usp;              /* User Stack (All) */
    int vbr;              /* Vector Base Register. (68010) */

    int BankID;			  /* Memory bank in use */
    int CPUtype;		  /* CPU Type 0=68000,1=68010,2=68020 */

} m68k_cpu_context;


static UINT8 m68k_reg_layout[] = {
	M68K_PC, M68K_ISP, -1,
	M68K_SR, M68K_USP, -1,
	M68K_D0, M68K_A0, -1,
	M68K_D1, M68K_A1, -1,
	M68K_D2, M68K_A2, -1,
	M68K_D3, M68K_A3, -1,
	M68K_D4, M68K_A4, -1,
	M68K_D5, M68K_A5, -1,
	M68K_D6, M68K_A6, -1,
	M68K_D7, M68K_A7, 0
};

static UINT8 m68k_win_layout[] = {
	48, 0,32,13,	/* register window (top right) */
	 0, 0,47,13,	/* disassembler window (top left) */
	 0,14,47, 8,	/* memory #1 window (left, middle) */
	48,14,32, 8,	/* memory #2 window (right, middle) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};

extern m68k_cpu_context regs;

extern void CONVENTION M68KRUN(void);
extern void CONVENTION M68KRESET(void);

/********************************************/
/* Interface routines to link Mame -> 68KEM */
/********************************************/

void m68000_reset(void *param)
{
	memset(&regs,0,sizeof(regs));

    regs.a[7] = regs.isp = (( cpu_readop16(0) << 16 ) | cpu_readop16(2));
    regs.pc   = (( cpu_readop16(4) << 16 ) | cpu_readop16(6)) & 0xffffff;
    regs.sr_high = 0x27;

	#ifdef MAME_DEBUG
		regs.sr = 0x2700;
	#endif

    M68KRESET();
}


void m68000_exit(void)
{
	/* nothing to do ? */
}


#ifdef TRACE68K 							/* Trace */
	static int skiptrace=0;
#endif

int m68000_execute(int cycles)
{
	if (regs.IRQ_level == 0x80) return cycles;		/* STOP with no IRQs */

	m68000_ICount = cycles;

#ifdef MAME_DEBUG
    do
    {
		if (mame_debug)
        {
			#ifdef TRACE68K

			int StartCycle = m68000_ICount;

            skiptrace++;

            if (skiptrace > 0)
            {
			    int mycount, areg, dreg;

                areg = dreg = 0;
	            for (mycount=7;mycount>=0;mycount--)
                {
            	    areg = areg + regs.a[mycount];
                    dreg = dreg + regs.d[mycount];
                }

           	    logerror("=> %8x %8x ",areg,dreg);
			    logerror("%6x %4x %d\n",regs.pc,regs.sr & 0x271F,m68000_ICount);
            }
            #endif

			MAME_Debug();
            M68KRUN();

            #ifdef TRACE68K
            if ((regs.IRQ_level & 0x80) || (cpu_getstatus(cpu_getactivecpu()) == 0))
    			m68000_ICount = 0;
            else
				m68000_ICount = StartCycle - 12;
            #endif
        }
        else
			M68KRUN();

    } while (m68000_ICount > 0);

#else

	M68KRUN();

#endif /* MAME_DEBUG */

	return (cycles - m68000_ICount);
}


unsigned m68000_get_context(void *dst)
{
	if( dst )
		*(m68k_cpu_context*)dst = regs;
	return sizeof(m68k_cpu_context);
}

void m68000_set_context(void *src)
{
	if( src )
		regs = *(m68k_cpu_context*)src;
}

unsigned m68000_get_pc(void)
{
    return regs.pc;
}

void m68000_set_pc(unsigned val)
{
	regs.pc = val;
}

unsigned m68000_get_sp(void)
{
	return regs.isp;
}

void m68000_set_sp(unsigned val)
{
	regs.isp = val;
}

unsigned m68000_get_reg(int regnum)
{
    switch( regnum )
    {
		case M68K_PC: return regs.pc;
		case M68K_ISP: return regs.isp;
		case M68K_USP: return regs.usp;
		case M68K_SR: return regs.sr;
		case M68K_VBR: return regs.vbr;
		case M68K_SFC: return regs.sfc;
		case M68K_DFC: return regs.dfc;
		case M68K_D0: return regs.d[0];
		case M68K_D1: return regs.d[1];
		case M68K_D2: return regs.d[2];
		case M68K_D3: return regs.d[3];
		case M68K_D4: return regs.d[4];
		case M68K_D5: return regs.d[5];
		case M68K_D6: return regs.d[6];
		case M68K_D7: return regs.d[7];
		case M68K_A0: return regs.a[0];
		case M68K_A1: return regs.a[1];
		case M68K_A2: return regs.a[2];
		case M68K_A3: return regs.a[3];
		case M68K_A4: return regs.a[4];
		case M68K_A5: return regs.a[5];
		case M68K_A6: return regs.a[6];
		case M68K_A7: return regs.a[7];
		case REG_PREVIOUSPC: return regs.previous_pc;
/* TODO: Verify that this is the right thing to do for the purpose? */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = regs.isp + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return cpu_readmem24bew_dword( offset );
            }
    }
    return 0;
}

void m68000_set_reg(int regnum, unsigned val)
{
    switch( regnum )
    {
		case M68K_PC: regs.pc = val; break;
		case M68K_ISP: regs.isp = val; break;
		case M68K_USP: regs.usp = val; break;
		case M68K_SR: regs.sr = val; break;
		case M68K_VBR: regs.vbr = val; break;
		case M68K_SFC: regs.sfc = val; break;
		case M68K_DFC: regs.dfc = val; break;
		case M68K_D0: regs.d[0] = val; break;
		case M68K_D1: regs.d[1] = val; break;
		case M68K_D2: regs.d[2] = val; break;
		case M68K_D3: regs.d[3] = val; break;
		case M68K_D4: regs.d[4] = val; break;
		case M68K_D5: regs.d[5] = val; break;
		case M68K_D6: regs.d[6] = val; break;
		case M68K_D7: regs.d[7] = val; break;
		case M68K_A0: regs.a[0] = val; break;
		case M68K_A1: regs.a[1] = val; break;
		case M68K_A2: regs.a[2] = val; break;
		case M68K_A3: regs.a[3] = val; break;
		case M68K_A4: regs.a[4] = val; break;
		case M68K_A5: regs.a[5] = val; break;
		case M68K_A6: regs.a[6] = val; break;
		case M68K_A7: regs.a[7] = val; break;
/* TODO: Verify that this is the right thing to do for the purpose? */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = regs.isp + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					cpu_writemem24bew_dword( offset, val );
            }
    }
}

void m68k_assert_irq(int int_line)
{
	regs.IRQ_level = int_line;

    /* Now check for Interrupt */

	m68000_ICount = -1;
    M68KRUN();
}

void m68k_clear_irq(int int_line)
{
	regs.IRQ_level = 0;
}

void m68000_set_nmi_line(int state)
{
	switch(state)
	{
		case CLEAR_LINE:
			m68k_clear_irq(7);
			return;
		case ASSERT_LINE:
			m68k_assert_irq(7);
			return;
		default:
			m68k_assert_irq(7);
			return;
	}
}

void m68000_set_irq_line(int irqline, int state)
{
	switch(state)
	{
		case CLEAR_LINE:
			m68k_clear_irq(irqline);
			return;
		case ASSERT_LINE:
			m68k_assert_irq(irqline);
			return;
		default:
			m68k_assert_irq(irqline);
			return;
	}
}

void m68000_set_irq_callback(int (*callback)(int irqline))
{
	regs.irq_callback = callback;
}

void m68000_set_reset_callback(int (*callback)(void))
{
	regs.reset_callback = callback;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *m68000_info(void *context, int regnum)
{
#ifdef MAME_DEBUG
extern int m68k_disassemble(char* str_buff, int pc);
#endif
    static char buffer[32][47+1];
	static int which;
	m68k_cpu_context *r = context;

	which = ++which % 32;
	buffer[which][0] = '\0';
	if( !context )
		r = &regs;

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_PC: sprintf(buffer[which], "PC:%06X", r->pc); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", r->isp); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", r->usp); break;
		case CPU_INFO_REG+M68K_SR: sprintf(buffer[which], "SR:%08X", r->sr); break;
		case CPU_INFO_REG+M68K_VBR: sprintf(buffer[which], "VBR:%08X", r->vbr); break;
		case CPU_INFO_REG+M68K_SFC: sprintf(buffer[which], "SFC:%08X", r->sfc); break;
		case CPU_INFO_REG+M68K_DFC: sprintf(buffer[which], "DFC:%08X", r->dfc); break;
		case CPU_INFO_REG+M68K_D0: sprintf(buffer[which], "D0:%08X", r->d[0]); break;
		case CPU_INFO_REG+M68K_D1: sprintf(buffer[which], "D1:%08X", r->d[1]); break;
		case CPU_INFO_REG+M68K_D2: sprintf(buffer[which], "D2:%08X", r->d[2]); break;
		case CPU_INFO_REG+M68K_D3: sprintf(buffer[which], "D3:%08X", r->d[3]); break;
		case CPU_INFO_REG+M68K_D4: sprintf(buffer[which], "D4:%08X", r->d[4]); break;
		case CPU_INFO_REG+M68K_D5: sprintf(buffer[which], "D5:%08X", r->d[5]); break;
		case CPU_INFO_REG+M68K_D6: sprintf(buffer[which], "D6:%08X", r->d[6]); break;
		case CPU_INFO_REG+M68K_D7: sprintf(buffer[which], "D7:%08X", r->d[7]); break;
		case CPU_INFO_REG+M68K_A0: sprintf(buffer[which], "A0:%08X", r->a[0]); break;
		case CPU_INFO_REG+M68K_A1: sprintf(buffer[which], "A1:%08X", r->a[1]); break;
		case CPU_INFO_REG+M68K_A2: sprintf(buffer[which], "A2:%08X", r->a[2]); break;
		case CPU_INFO_REG+M68K_A3: sprintf(buffer[which], "A3:%08X", r->a[3]); break;
		case CPU_INFO_REG+M68K_A4: sprintf(buffer[which], "A4:%08X", r->a[4]); break;
		case CPU_INFO_REG+M68K_A5: sprintf(buffer[which], "A5:%08X", r->a[5]); break;
		case CPU_INFO_REG+M68K_A6: sprintf(buffer[which], "A6:%08X", r->a[6]); break;
		case CPU_INFO_REG+M68K_A7: sprintf(buffer[which], "A7:%08X", r->a[7]); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->sr & 0x8000 ? 'T':'.',
				r->sr & 0x4000 ? '?':'.',
				r->sr & 0x2000 ? 'S':'.',
				r->sr & 0x1000 ? '?':'.',
				r->sr & 0x0800 ? '?':'.',
				r->sr & 0x0400 ? 'I':'.',
				r->sr & 0x0200 ? 'I':'.',
				r->sr & 0x0100 ? 'I':'.',
				r->sr & 0x0080 ? '?':'.',
				r->sr & 0x0040 ? '?':'.',
				r->sr & 0x0020 ? '?':'.',
				r->sr & 0x0010 ? 'X':'.',
				r->sr & 0x0008 ? 'N':'.',
				r->sr & 0x0004 ? 'Z':'.',
				r->sr & 0x0002 ? 'V':'.',
				r->sr & 0x0001 ? 'C':'.');
            break;
		case CPU_INFO_NAME: return "68000";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "0.16";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1998,99 Mike Coates, Darren Olafson. All rights reserved";
		case CPU_INFO_REG_LAYOUT: return (const char*)m68k_reg_layout;
        case CPU_INFO_WIN_LAYOUT: return (const char*)m68k_win_layout;
	}
	return buffer[which];
}

unsigned m68000_dasm(char *buffer, unsigned pc)
{
	change_pc24bew(pc);
#ifdef MAME_DEBUG
    return m68k_disassemble(buffer, pc);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}

/****************************************************************************
 * M68010 section
 ****************************************************************************/

#if (HAS_M68010)

void m68010_reset(void *param)
{
	m68000_reset(param);
    regs.CPUtype=1;
}

void m68010_exit(void) { m68000_exit(); }
int  m68010_execute(int cycles) { return m68000_execute(cycles); }
unsigned m68010_get_context(void *dst) { return m68000_get_context(dst); }
void m68010_set_context(void *src) { m68000_set_context(src); }
unsigned m68010_get_pc(void) { return m68000_get_pc(); }
void m68010_set_pc(unsigned val) { m68000_set_pc(val); }
unsigned m68010_get_sp(void) { return m68000_get_sp(); }
void m68010_set_sp(unsigned val) { m68000_set_sp(val); }
unsigned m68010_get_reg(int regnum) { return m68000_get_reg(regnum); }
void m68010_set_reg(int regnum, unsigned val) { m68000_set_reg(regnum,val); }
void m68010_set_nmi_line(int state) { m68000_set_nmi_line(state); }
void m68010_set_irq_line(int irqline, int state)  { m68000_set_irq_line(irqline,state); }
void m68010_set_irq_callback(int (*callback)(int irqline))  { m68000_set_irq_callback(callback); }

const char *m68010_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "68010";
	}
	return m68000_info(context,regnum);
}

unsigned m68010_dasm(char *buffer, unsigned pc)
{
	change_pc24bew(pc);
#ifdef MAME_DEBUG
    return m68k_disassemble(buffer, pc);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}
#endif

/****************************************************************************
 * M68020 section
 ****************************************************************************/

#if (HAS_M68EC020)

void m68ec020_reset(void *param)
{
	m68000_reset(param);
    regs.CPUtype=2;
}

void m68ec020_exit(void) { m68000_exit(); }
int  m68ec020_execute(int cycles) { return m68000_execute(cycles); }
unsigned m68ec020_get_context(void *dst) { return m68000_get_context(dst); }
void m68ec020_set_context(void *src) { m68000_set_context(src); }
unsigned m68ec020_get_pc(void) { return m68000_get_pc(); }
void m68ec020_set_pc(unsigned val) { m68000_set_pc(val); }
unsigned m68ec020_get_sp(void) { return m68000_get_sp(); }
void m68ec020_set_sp(unsigned val) { m68000_set_sp(val); }
unsigned m68ec020_get_reg(int regnum) { return m68000_get_reg(regnum); }
void m68ec020_set_reg(int regnum, unsigned val) { m68000_set_reg(regnum,val); }
void m68ec020_set_nmi_line(int state) { m68000_set_nmi_line(state); }
void m68ec020_set_irq_line(int irqline, int state)  { m68000_set_irq_line(irqline,state); }
void m68ec020_set_irq_callback(int (*callback)(int irqline))  { m68000_set_irq_callback(callback); }

const char *m68ec020_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "68EC020";
	}
	return m68000_info(context,regnum);
}

unsigned m68ec020_dasm(char *buffer, unsigned pc)
{
	change_pc24bew(pc);
#ifdef MAME_DEBUG
    return m68k_disassemble(buffer, pc);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}
#endif

#if (HAS_M68020)

void m68020_reset(void *param)
{
	m68000_reset(param);
    regs.CPUtype=2;
}

void m68020_exit(void) { m68000_exit(); }
int  m68020_execute(int cycles) { return m68000_execute(cycles); }
unsigned m68020_get_context(void *dst) { return m68000_get_context(dst); }
void m68020_set_context(void *src) { m68000_set_context(src); }
unsigned m68020_get_pc(void) { return m68000_get_pc(); }
void m68020_set_pc(unsigned val) { m68000_set_pc(val); }
unsigned m68020_get_sp(void) { return m68000_get_sp(); }
void m68020_set_sp(unsigned val) { m68000_set_sp(val); }
unsigned m68020_get_reg(int regnum) { return m68000_get_reg(regnum); }
void m68020_set_reg(int regnum, unsigned val) { m68000_set_reg(regnum,val); }
void m68020_set_nmi_line(int state) { m68000_set_nmi_line(state); }
void m68020_set_irq_line(int irqline, int state)  { m68000_set_irq_line(irqline,state); }
void m68020_set_irq_callback(int (*callback)(int irqline))  { m68000_set_irq_callback(callback); }

const char *m68020_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "68020";
	}
	return m68000_info(context,regnum);
}

unsigned m68020_dasm(char *buffer, unsigned pc)
{
	change_pc24bew(pc);
#ifdef MAME_DEBUG
    return m68k_disassemble(buffer, pc);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}
#endif

