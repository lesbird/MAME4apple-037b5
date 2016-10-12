/****************************************************************************
*               real mode NEC Vx emulator v1.0 by Oliver Bergmann           *
*             based on the i286 emulator by Fabrice Frances                 *
*	        (initial work based on David Hedley's pcemu)                *
*   any ideas/help/bugs/contributions to Raul_Bloodworth@hotmail.com        *
*   DO WHAT YOU WANT WITH THIS CODE BUT THE BEST IS EMULATION               *
****************************************************************************/
/* NEC V-Models overview
   ---------------------
   NEC V20/V30	-	i80186 upward instruction compatible. No protected mode.
   				extra features:		fewer cycles than i80186
   				extra instructions:	- BCD	(add4s,sub4s,cpm4s,rol4,ror4)
   									- Bitmanipulation (test1,set1,clr1)
   									- Loop on C-Flag (repc,repnc)
   									- Bitfields	(ins,ext)
   									- i80186 emulation (brkem,retem.calln,reti,
   									  additional MD-Flag (bit 15) in statusreg.

  NEC V20		- 	8/10 MHz
  					µpd70108
  					i8088 pin compatible

  NEC V20H		-   10/12/16 MHz
  					µpd70108H
  					packages: 40 pin IYP, 44 pin PLCC, 52 pin PFP (QFP)

  NEC V25		-	extra instructions for register bank switches and task switches
  					µpd70320, mPD70322
  					packages: 84 pin PLCC, 94 pin QFP

  NEC V25 Plus	- 	NEC V25 with integrated peripherals: PIC,DMA,ports,serial,timer
  					256 Bytes RAM, 8 kByte ROM
  					µpd70325

  NEC V25 Software Guard - NEC V25 with 8086 emulation and security mode
  					µpd70327

  NEC V30		- 	10,12,16 MHz
  					µpd70116
  					i8086 pin compatible

  NEC V30H		-   see V25H (but with V30 core)
  					µpd70116H

  NEC V33		-   ???	used in IREM M92 (R-Type Leo !)
  					16 MHz
  					µpd 70136

  NEC V35		- 	same as V25 but V30 core.
  					µpd70330, mPD70332

  NEC V35 Plus	-   same as V25 Plus but with V30 core
  					µpd70335

  NEC V35 Sofware Guard - take a guess....yep !
  					µpd70337


  NEC V40		-	undefined opcode triggers INT6, same speeds as V20
  					packages: 68 pin PGA, 68 pin PLCC, 80 pin PFP (QFP)
  					µpd70208

  NEC V40H		-	10,12,16 MHz
					µpd70208H
					packages: 68 pin PGA, 80 pin PFP (QFP)

  NEC V45		-	???

  NEC V50		-	undifed opcode triggers INT6, V30 speeds
  					µpd70216

  NEC V50H		-   10,12,16 MHz
  					µpd70216H

  NEC V53a		-	???

  NEC V55		- 	speeds up to 16 MHz (????)

  NEC V60 		- 	16 MHz (???)	[Sonic,Spiderman,Golden Axe 2...]

  NEC V70 		- 	16 MHz (???)
  					µpd70616
  					used in Sega System 32



   History:
   --------
   20.07.1999 v1.0: 90% of Basic V20 is done. Used in IREM M72 (?) (Raiden,R-Type,...)
					No peripherals !

   2do: - EXT & INS Instruction:
		I have no clue about how they work
	    - RETEM & CALLN & RETI and the MD-Flag:
	    	I have added the MD-Flag and BRKEM clears it (set by reset and is normally =1).
	    	The Mapping of the registers ond the whole CALLN/RETI adress-sheme is missing.
	    	No big deal (adress spache is narrowed to 8080/8086) but it has to be
	    	waved in here.
	    - All other V models
	    - adjust clocks for all other instructions than the nec_pre() ones. !!!

	22.07.1999 v1.5: Information-flood - Found much info in the OPCODE.LST

	    fixed:	- timings ! (All clocks correspond to V20 !!!)
	    fixed:	- found almost every opcode for all NEC-CPUS. No code though but opcodes
			  are known now. I inserted the comments from OPCODE.LST.

	  2do: - Timings for other NEC'S. Think the timings should come from a table (depending on R/M)
	         maybe rewrite this part.
	       - add some code to identify which cpu we are trying to emulate (for correct timing)
	       - strip unnecessary code

	09.11.1999 v1.6: Additions by Bryan McPhail
		Fixed bug in i_rotshft_bd8 and i_rotshft_wd8 (thanks Nao).
		Fixed bug in i_imul_d16
		Moved prefix_base & seg_prefix inside reg struct
		Removed amask stuff
		To do:  Double check int3(), into(), lea()

	10.11.1999 v1.7: Additions by Bryan McPhail
		Adjusted IncWord,DecWord, IYV, IIYV

		NB: STOSW - Implementing this as two byte writes causes
		Lethal Thunder to break, but using a single word write works.

		Why?  Because PutMemW does NOT use the DefaultBase macro!

		However I have no idea what is right and what the DefaultBase
		macro even does...  For now I'm using PutMemW.

		This 'bug' may also affect i_insw.

		All my changes are flagged 'MISH'.

*/

#include <stdio.h>
#include <string.h>
#include "nechost.h"
#include "cpuintrf.h"
#include "memory.h"
#include "mamedbg.h"
#include "nec.h"
#include "necintrf.h"
#include "driver.h"

static UINT8 nec_reg_layout[] = {
	NEC_IP,NEC_SP,NEC_FLAGS,NEC_AW,NEC_CW,NEC_DW,NEC_BW,NEC_BP,NEC_IX,NEC_IY, -1,
	NEC_ES,NEC_CS,NEC_SS,NEC_DS,NEC_VECTOR,NEC_NMI_STATE,NEC_IRQ_STATE, 0
};

static UINT16 bytes[] = {
	1,2,4,8,16,32,64,128,256,
	512,1024,2048,4096,8192,16384,32768,65336
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 nec_win_layout[] = {
     0, 0,80, 2,    /* register window (top rows) */
     0, 3,34,19,    /* disassembler window (left colums) */
    35, 3,45, 9,    /* memory #1 window (right, upper middle) */
    35,13,45, 9,    /* memory #2 window (right, lower middle) */
     0,23,80, 1,    /* command line window (bottom rows) */
};

/* NEC registers */
typedef union
{                   /* eight general registers */
    UINT16 w[8];    /* viewed as 16 bits registers */
    UINT8  b[16];   /* or as 8 bit registers */
} necbasicregs;

typedef struct
{
    necbasicregs regs;
    int     ip;
	UINT16	flags;
	UINT32	base[4];
	UINT16	sregs[4];
    int     (*irq_callback)(int irqline);
    int     AuxVal, OverVal, SignVal, ZeroVal, CarryVal, ParityVal; /* 0 or non-0 valued flags */
	UINT8	TF, IF, DF, MF; 	/* 0 or 1 valued flags */	/* OB[19.07.99] added Mode Flag V30 */
	UINT8	int_vector;
	UINT8	pending_irq;
	INT8	nmi_state;
	INT8	irq_state;

	unsigned prefix_base;	/* base address of the latest prefix segment */
	char seg_prefix;		/* prefix segment indicator */
} nec_Regs;

/***************************************************************************/
/* cpu state                                                               */
/***************************************************************************/

int nec_ICount;

static nec_Regs I;

/* The interrupt number of a pending external interrupt pending NMI is 2.	*/
/* For INTR interrupts, the level is caught on the bus during an INTA cycle */

#define INT_IRQ 0x01
#define NMI_IRQ 0x02

#include "necinstr.h"
#include "necea.h"
#include "necmodrm.h"

static UINT8 parity_table[256];
/***************************************************************************/

void nec_reset (void *param)
{
    unsigned int i,j,c;
    BREGS reg_name[8]={ AL, CL, DL, BL, AH, CH, DH, BH };

	memset( &I, 0, sizeof(I) );

     I.sregs[CS] = 0xffff;
	I.base[CS] = I.sregs[CS] << 4;

	change_pc20( (I.base[CS] + I.ip));

    for (i = 0;i < 256; i++)
    {
		for (j = i, c = 0; j > 0; j >>= 1)
			if (j & 1) c++;
		parity_table[i] = !(c & 1);
    }

	I.ZeroVal = I.ParityVal = 1;
	SetMD(1);						/* set the mode-flag = native mode */

    for (i = 0; i < 256; i++)
    {
		Mod_RM.reg.b[i] = reg_name[(i & 0x38) >> 3];
		Mod_RM.reg.w[i] = (WREGS) ( (i & 0x38) >> 3) ;
    }

    for (i = 0xc0; i < 0x100; i++)
    {
		Mod_RM.RM.w[i] = (WREGS)( i & 7 );
		Mod_RM.RM.b[i] = (BREGS)reg_name[i & 7];
    }
}

void nec_exit (void)
{
	/* nothing to do ? */
}


static void nec_interrupt(unsigned int_num,BOOLEAN md_flag)
{
    unsigned dest_seg, dest_off;

#if 0
	logerror("PC=%06x : NEC Interrupt %02d",cpu_get_pc(),int_num);
#endif

    i_pushf();
	I.TF = I.IF = 0;
	if (md_flag) SetMD(0);	/* clear Mode-flag = start 8080 emulation mode */

	if (int_num == -1)
	{
		int_num = (*I.irq_callback)(0);
//		logerror(" (indirect ->%02d) ",int_num);
	}

    dest_off = ReadWord(int_num*4);
    dest_seg = ReadWord(int_num*4+2);

	PUSH(I.sregs[CS]);
	PUSH(I.ip);
	I.ip = (WORD)dest_off;
	I.sregs[CS] = (WORD)dest_seg;
	I.base[CS] = SegBase(CS);
	change_pc20((I.base[CS]+I.ip));
//	logerror("=%06x\n",cpu_get_pc());
}



void nec_trap(void)
{
	nec_instruction[FETCHOP]();
	nec_interrupt(1,0);
}


static void external_int(void)
{
	if( I.pending_irq & NMI_IRQ )
	{
		nec_interrupt(NEC_NMI_INT,0);
		I.pending_irq &= ~NMI_IRQ;
	}
	else
	if( I.pending_irq )
	{
		/* the actual vector is retrieved after pushing flags */
		/* and clearing the IF */
		nec_interrupt(-1,0);
	}
}

/****************************************************************************/
/*                             OPCODES                                      */
/****************************************************************************/

static void i_add_br8(void)    /* Opcode 0x00 - ADD */
{
    DEF_br8(dst,src);
	nec_ICount-=3;
    ADDB(dst,src);
    PutbackRMByte(ModRM,dst);
}

static void i_add_wr16(void)    /* Opcode 0x01 */
{
    DEF_wr16(dst,src);
	nec_ICount-=3;
    ADDW(dst,src);
    PutbackRMWord(ModRM,dst);
}

static void i_add_r8b(void)    /* Opcode 0x02 */
{
    DEF_r8b(dst,src);
	nec_ICount-=3;
    ADDB(dst,src);
    RegByte(ModRM)=dst;
}

static void i_add_r16w(void)    /* Opcode 0x03 */
{
    DEF_r16w(dst,src);
	nec_ICount-=3;
    ADDW(dst,src);
    RegWord(ModRM)=dst;
}

static void i_add_ald8(void)    /* Opcode 0x04 */
{
    DEF_ald8(dst,src);
	nec_ICount-=4;
    ADDB(dst,src);
	I.regs.b[AL]=dst;
}

static void i_add_axd16(void)    /* Opcode 0x05 */
{
    DEF_axd16(dst,src);
	nec_ICount-=4;
    ADDW(dst,src);
	I.regs.w[AW]=dst;
}

static void i_push_es(void)    /* Opcode 0x06 */
{
	nec_ICount-=3;
	PUSH(I.sregs[ES]);
}

static void i_pop_es(void)    /* Opcode 0x07 */
{
	POP(I.sregs[ES]);
	I.base[ES] = SegBase(ES);
	nec_ICount-=2;
}

static void i_or_br8(void)    /* Opcode 0x08 */
{
    DEF_br8(dst,src);
	nec_ICount-=3;
    ORB(dst,src);
    PutbackRMByte(ModRM,dst);
}

static void i_or_wr16(void)    /* Opcode 0x09 */
{
    DEF_wr16(dst,src);
	nec_ICount-=3;
    ORW(dst,src);
    PutbackRMWord(ModRM,dst);
}

static void i_or_r8b(void)    /* Opcode 0x0a */
{
    DEF_r8b(dst,src);
	nec_ICount-=3;
    ORB(dst,src);
    RegByte(ModRM)=dst;
}

static void i_or_r16w(void)    /* Opcode 0x0b */
{
    DEF_r16w(dst,src);
	nec_ICount-=3;
    ORW(dst,src);
    RegWord(ModRM)=dst;
}

static void i_or_ald8(void)    /* Opcode 0x0c */
{
    DEF_ald8(dst,src);
	nec_ICount-=4;
    ORB(dst,src);
	I.regs.b[AL]=dst;
}

static void i_or_axd16(void)    /* Opcode 0x0d */
{
    DEF_axd16(dst,src);
	nec_ICount-=4;
    ORW(dst,src);
	I.regs.w[AW]=dst;
}

static void i_push_cs(void)    /* Opcode 0x0e */
{
	nec_ICount-=3;
	PUSH(I.sregs[CS]);
}

static void i_pre_nec(void) /* Opcode 0x0f */
{
    unsigned Opcode = FETCH;
    unsigned ModRM;
    unsigned tmp;
    unsigned tmp2;

	switch (Opcode) {
		case 0x10 : // 0F 10 47 30 - TEST1 [bx+30h],cl
			ModRM = FETCH;
		    if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		    	nec_ICount-=3;
		    }
		    else {
		    	int old=nec_ICount;
		    	(*GetEA[ModRM])();;
		    	tmp=ReadByte(EA);
   				nec_ICount=old-12;			/* my source says 14 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
		    }
			tmp2 = I.regs.b[CL] & 0x7;
			I.ZeroVal = tmp & bytes[tmp2] ? 1 : 0;
//			SetZF(tmp & (1<<tmp2));
			break;
		case 0x11 : // 0F 11 47 30 - TEST1 [bx+30h],cl
			ModRM = FETCH;
		    //tmp = GetRMWord(ModRM);
		    if (ModRM >= 0xc0) {
				tmp=I.regs.w[Mod_RM.RM.w[ModRM]];
				nec_ICount-=3;
			}
			else {
				int old=nec_ICount;
				(*GetEA[ModRM])();;
				tmp=ReadWord(EA);
				nec_ICount=old-12;			/* my source says 14 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
			}
			tmp2 = I.regs.b[CL] & 0xF;
			I.ZeroVal = tmp & bytes[tmp2] ? 1 : 0;
//			SetZF(tmp & (1<<tmp2));
			break;


		case 0x12 : // 0F 12 [mod:000:r/m] - CLR1 reg/m8,cl
			ModRM = FETCH;
		    /* need the long if due to correct cycles OB[19.07.99] */
		    if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		    	nec_ICount-=5;
		    }
		    else {
		    	int old=nec_ICount;
		    	(*GetEA[ModRM])();;
		    	tmp=ReadByte( EA);
   				nec_ICount=old-14;			/* my source says 14 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
		    }
			tmp2 = I.regs.b[CL] & 0x7;		/* hey its a Byte so &07 NOT &0f */
			tmp &= ~(bytes[tmp2]);
			PutbackRMByte(ModRM,tmp);
			break;

		case 0x13 : // 0F 13 [mod:000:r/m] - CLR1 reg/m16,cl
			ModRM = FETCH;
		    //tmp = GetRMWord(ModRM);
			if (ModRM >= 0xc0) {
				tmp=I.regs.w[Mod_RM.RM.w[ModRM]];
				nec_ICount-=5;
			}
			else {
				int old=nec_ICount;
				(*GetEA[ModRM])();;
				tmp=ReadWord(EA);
				nec_ICount=old-14;			/* my source says 14 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
			}
			tmp2 = I.regs.b[CL] & 0xF;		/* this time its a word */
			tmp &= ~(bytes[tmp2]);
			PutbackRMWord(ModRM,tmp);
			break;

		case 0x14 : // 0F 14 47 30 - SET1 [bx+30h],cl
			ModRM = FETCH;
			if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		    	nec_ICount-=4;
		    }
		    else {
		    	int old=nec_ICount;
		    	 (*GetEA[ModRM])();;
		    	tmp=ReadByte(EA);
   				nec_ICount=old-13;
		    }
			tmp2 = I.regs.b[CL] & 0x7;
			tmp |= (bytes[tmp2]);
			PutbackRMByte(ModRM,tmp);
			break;
		case 0x15 : // 0F 15 C6 - SET1 si,cl
			ModRM = FETCH;
		   //tmp = GetRMWord(ModRM);
		   if (ModRM >= 0xc0) {
				tmp=I.regs.w[Mod_RM.RM.w[ModRM]];
				nec_ICount-=4;
			}
			else {
				int old=nec_ICount;
				(*GetEA[ModRM])();;
				tmp=ReadWord(EA);
				nec_ICount=old-13;
			}
			tmp2 = I.regs.b[CL] & 0xF;
			tmp |= (bytes[tmp2]);
			PutbackRMWord(ModRM,tmp);
			break;
		case 0x16 : // 0F 16 C6 - NOT1 si,cl
			ModRM = FETCH;
		    /* need the long if due to correct cycles OB[19.07.99] */
		    if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		    	nec_ICount-=4;
		    }
		    else {
		    	int old=nec_ICount;
		    	(*GetEA[ModRM])();;
		    	tmp=ReadByte(EA);
   				nec_ICount=old-18;			/* my source says 18 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
		    }
			tmp2 = I.regs.b[CL] & 0x7;	/* hey its a Byte so &07 NOT &0f */
			if (tmp & bytes[tmp2])
				tmp &= ~(bytes[tmp2]);
			else
				tmp |= (bytes[tmp2]);
			PutbackRMByte(ModRM,tmp);
			break;
		case 0x17 : // 0F 17 C6 - NOT1 si,cl
			ModRM = FETCH;
		    //tmp = GetRMWord(ModRM);
			if (ModRM >= 0xc0) {
				tmp=I.regs.w[Mod_RM.RM.w[ModRM]];
				nec_ICount-=4;
			}
			else {
				int old=nec_ICount;
				(*GetEA[ModRM])();;
				tmp=ReadWord(EA);
				nec_ICount=old-18;			/* my source says 14 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
			}
			tmp2 = I.regs.b[CL] & 0xF;		/* this time its a word */
			if (tmp & bytes[tmp2])
				tmp &= ~(bytes[tmp2]);
			else
				tmp |= (bytes[tmp2]);
			PutbackRMWord(ModRM,tmp);
			break;
		case 0x18 : // 0F 18 XX - TEST1 [bx+30h],07
			ModRM = FETCH;
		   //tmp = GetRMByte(ModRM);
		    if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		    	nec_ICount-=4;
		    }
		    else {
		    	int old=nec_ICount;
		    	(*GetEA[ModRM])();;
		    	tmp=ReadByte(EA);
   				nec_ICount=old-13;			/* my source says 15 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
		    }
			tmp2 = FETCH;
			tmp2 &= 0xF;
			I.ZeroVal = tmp & (bytes[tmp2]) ? 1 : 0;
//			SetZF(tmp & (1<<tmp2));
			break;
		case 0x19 : // 0F 19 XX - TEST1 [bx+30h],07
			ModRM = FETCH;
		   	//tmp = GetRMWord(ModRM);
		   	if (ModRM >= 0xc0) {
				tmp=I.regs.w[Mod_RM.RM.w[ModRM]];
				nec_ICount-=4;
			}
			else {
				int old=nec_ICount;
				(*GetEA[ModRM])();;
				tmp=ReadWord(EA);
				nec_ICount=old-13;			/* my source says 14 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
			}
			tmp2 = FETCH;
			tmp2 &= 0xf;
			I.ZeroVal = tmp & (bytes[tmp2]) ? 1 : 0;
//			SetZF(tmp & (1<<tmp2));
			break;
		case 0x1a : // 0F 1A 06 - CLR1 si,cl
			ModRM = FETCH;
		   	//tmp = GetRMByte(ModRM);
		   	if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		    	nec_ICount-=6;
		    }
		    else {
		    	int old=nec_ICount;
		    	(*GetEA[ModRM])();;
		    	tmp=ReadByte(EA);
   				nec_ICount=old-15;			/* my source says 15 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
		    }
			tmp2 = FETCH;
			tmp2 &= 0x7;
			tmp &= ~(bytes[tmp2]);
			PutbackRMByte(ModRM,tmp);
			break;
		case 0x1B : // 0F 1B 06 - CLR1 si,cl
			ModRM = FETCH;
		   	//tmp = GetRMWord(ModRM);
		   	if (ModRM >= 0xc0) {
				tmp=I.regs.w[Mod_RM.RM.w[ModRM]];
				nec_ICount-=6;
			}
			else {
				int old=nec_ICount;
				(*GetEA[ModRM])();;
				tmp=ReadWord(EA);
				nec_ICount=old-15;			/* my source says 15 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
			}
			tmp2 = FETCH;
			tmp2 &= 0xF;
			tmp &= ~(bytes[tmp2]);
			PutbackRMWord(ModRM,tmp);
			break;
		case 0x1C : // 0F 1C 47 30 - SET1 [bx+30h],cl
			ModRM = FETCH;
		   //tmp = GetRMByte(ModRM);
		    if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		    	nec_ICount-=5;
		    }
		    else {
		    	int old=nec_ICount;
		    	(*GetEA[ModRM])();;
		    	tmp=ReadByte(EA);
   				nec_ICount=old-14;			/* my source says 15 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
		    }
			tmp2 = FETCH;
			tmp2 &= 0x7;
			tmp |= (bytes[tmp2]);
			PutbackRMByte(ModRM,tmp);
			break;
		case 0x1D : // 0F 1D C6 - SET1 si,cl
			//logerror("PC=%06x : Set1 ",cpu_get_pc()-2);
			ModRM = FETCH;
		    if (ModRM >= 0xc0) {
				tmp=I.regs.w[Mod_RM.RM.w[ModRM]];
				nec_ICount-=5;
				//logerror("reg=%04x ->",tmp);
			}
			else {
				int old=nec_ICount;
				(*GetEA[ModRM])();	// calculate EA
				tmp=ReadWord(EA);	// read from EA
				nec_ICount=old-14;
				//logerror("[%04x]=%04x ->",EA,tmp);
			}
			tmp2 = FETCH;
			tmp2 &= 0xF;
			tmp |= (bytes[tmp2]);
			//logerror("%04x",tmp);
			PutbackRMWord(ModRM,tmp);
			break;
		case 0x1e : // 0F 1e C6 - NOT1 si,07
			ModRM = FETCH;
		   	//tmp = GetRMByte(ModRM);
		   	if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		    	nec_ICount-=5;
		    }
		    else {
		    	int old=nec_ICount;
		    	(*GetEA[ModRM])();;
		    	tmp=ReadByte(EA);
   				nec_ICount=old-19;
		    }
			tmp2 = FETCH;
			tmp2 &= 0x7;
			if (tmp & bytes[tmp2])
				tmp &= ~(bytes[tmp2]);
			else
				tmp |= (bytes[tmp2]);
			PutbackRMByte(ModRM,tmp);
			break;
		case 0x1f : // 0F 1f C6 - NOT1 si,07
			ModRM = FETCH;
		   	//tmp = GetRMWord(ModRM);
		   	if (ModRM >= 0xc0) {
				tmp=I.regs.w[Mod_RM.RM.w[ModRM]];
				nec_ICount-=5;
			}
			else {
				int old=nec_ICount;
				(*GetEA[ModRM])();;
				tmp=ReadWord(EA);
				nec_ICount=old-19;			/* my source says 15 cycles everytime and not
   											   ModRM-dependent like GetEA[] does..hmmm */
			}
			tmp2 = FETCH;
			tmp2 &= 0xF;
			if (tmp & bytes[tmp2])
				tmp &= ~(bytes[tmp2]);
			else
				tmp |= (bytes[tmp2]);
			PutbackRMWord(ModRM,tmp);
			break;
		case 0x20 : { // 0F 20 59 - add4s
			int count = (I.regs.b[CL]+1)/2;	// length in words !
			int i;
	      	unsigned di = I.regs.w[IY];
			unsigned si = I.regs.w[IX];
			I.ZeroVal = 0;
			I.CarryVal = 0; // NOT ADC
			for (i=0;i<count;i++) {
				int v1,v2;
				int result;
				tmp = GetMemB(DS, si);
				tmp2 = GetMemB(ES, di);

				v1 = (tmp>>4)*10 + (tmp&0xf);
				v2 = (tmp2>>4)*10 + (tmp2&0xf);
				result = v1+v2+I.CarryVal;
				I.CarryVal = result > 99 ? 1 : 0;
				result = result % 100;
				v1 = ((result/10)<<4) | (result % 10);
				PutMemB(ES, di,v1)
				if (v1) I.ZeroVal = 1;
				si++;
				di++;
			}
			I.OverVal = I.CarryVal;
			nec_ICount-=7+19*count;	// 7+19n, n #operand words
			} break;

		case 0x22 : { // 0F 22 59 - sub4s
			int count = (I.regs.b[CL]+1)/2;
			int i;
	      unsigned di = I.regs.w[IY];
			unsigned si = I.regs.w[IX];
			I.ZeroVal = 0;
			I.CarryVal = 0; // NOT ADC
			for (i=0;i<count;i++) {
				int v1,v2;
				int result;
				tmp = GetMemB(ES, di);
				tmp2 = GetMemB(DS, si);

				v1 = (tmp>>4)*10 + (tmp&0xf);
				v2 = (tmp2>>4)*10 + (tmp2&0xf);
				if (v1 < (v2+I.CarryVal)) {
					v1+=100;
					result = v1-(v2+I.CarryVal);
					I.CarryVal = 1;
				} else {
					result = v1-(v2+I.CarryVal);
					I.CarryVal = 0;
				}
				v1 = ((result/10)<<4) | (result % 10);
				PutMemB(ES, di,v1)
				if (v1) I.ZeroVal = 1;
				si++;
				di++;
			}
			I.OverVal = I.CarryVal;
			nec_ICount-=7+19*count;
			} break;

		case 0x25 :
			/*
			----------O-MOVSPA---------------------------------
			OPCODE MOVSPA	 -  Move Stack Pointer After Bank Switched

			CPU:  NEC V25,V35,V25 Plus,V35 Plus,V25 Software Guard
			Type of Instruction: System

			Instruction:  MOVSPA

			Description:  This instruction transfer	 both SS and SP	 of the old register
				      bank to new register bank after the bank has been switched by
				      interrupt or BRKCS instruction.

			Flags Affected:	 None

			CPU mode: RM

			+++++++++++++++++++++++
			Physical Form:	MOVSPA
			COP (Code of Operation)	 : 0Fh 25h

			Clocks:	 16
			*/
			logerror("PC=%06x : MOVSPA\n",cpu_get_pc()-2);
			nec_ICount-=16;
			break;
		case 0x26 : { // 0F 22 59 - cmp4s
			int count = (I.regs.b[CL]+1)/2;
			int i;
	        unsigned di = I.regs.w[IY];
			unsigned si = I.regs.w[IX];
			I.ZeroVal = 0;
			I.CarryVal = 0; // NOT ADC
			for (i=0;i<count;i++) {
				int v1,v2;
				int result;
				tmp = GetMemB(ES, di);
				tmp2 = GetMemB(DS, si);

				v1 = (tmp>>4)*10 + (tmp&0xf);
				v2 = (tmp2>>4)*10 + (tmp2&0xf);
				if (v1 < (v2+I.CarryVal)) {
					v1+=100;
					result = v1-(v2+I.CarryVal);
					I.CarryVal = 1;
				} else {
					result = v1-(v2+I.CarryVal);
					I.CarryVal = 0;
				}
				v1 = ((result/10)<<4) | (result % 10);
//				PutMemB(ES, di,v1)	/* no store, only compare */
				if (v1) I.ZeroVal = 1;
				si++;
				di++;
			}
			I.OverVal = I.CarryVal;
			nec_ICount-=7+19*count;	// 7+19n, n #operand bytes
			} break;
		case 0x28 : // 0F 28 C7 - ROL4 bh
			ModRM = FETCH;
		   	//tmp = GetRMByte(ModRM);
		   	if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		    	nec_ICount-=25;
		    }
		    else {
		    	int old=nec_ICount;
		    	(*GetEA[ModRM])();;
		    	tmp=ReadByte(EA);
   				nec_ICount=old-28;
   			}
			tmp <<= 4;
			tmp |= I.regs.b[AL] & 0xF;
			I.regs.b[AL] = (I.regs.b[AL] & 0xF0) | ((tmp>>8)&0xF);
			tmp &= 0xff;
			PutbackRMByte(ModRM,tmp);
			break;
		// Is this a REAL instruction??
		case 0x29 : // 0F 29 C7 - ROL4 bx

			ModRM = FETCH;
		    /*
		    if (ModRM >= 0xc0) {
				tmp=I.regs.w[Mod_RM.RM.w[ModRM]];
				nec_ICount-=29;
			}
			else {
				int old=nec_ICount;
				(*GetEA[ModRM])();;
				tmp=ReadWord(EA);
				nec_ICount=old-33;
			}
			tmp <<= 4;
			tmp |= I.regs.b[AL] & 0xF;
			I.regs.b[AL] = (I.regs.b[AL] & 0xF0) | ((tmp>>8)&0xF);
			tmp &= 0xffff;
			PutbackRMWord(ModRM,tmp);
			*/
			logerror("PC=%06x : ROL4 %02x\n",cpu_get_pc()-3,ModRM);
			break;

		case 0x2A : // 0F 2a c2 - ROR4 bh
			ModRM = FETCH;
		   	//tmp = GetRMByte(ModRM);
		   	if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		    	nec_ICount-=29;
		    }
		    else {
		    	int old=nec_ICount;
		    	(*GetEA[ModRM])();;
		    	tmp=ReadByte(EA);
   				nec_ICount=old-33;
   			}
			tmp2 = (I.regs.b[AL] & 0xF)<<4;
			I.regs.b[AL] = (I.regs.b[AL] & 0xF0) | (tmp&0xF);
			tmp = tmp2 | (tmp>>4);
			PutbackRMByte(ModRM,tmp);
			break;

		case 0x2B : // 0F 2b c2 - ROR4 bx
			ModRM = FETCH;
			/*
			//tmp = GetRMWord(ModRM);
			if (ModRM >= 0xc0) {
				tmp=I.regs.w[Mod_RM.RM.w[ModRM]];
				nec_ICount-=29;
			}
			else {
				int old=nec_ICount;
				(*GetEA[ModRM])();;
				tmp=ReadWord(EA);
				nec_ICount=old-33;
			}
			tmp2 = (I.regs.b[AL] & 0xF)<<4;
			I.regs.b[AL] = (I.regs.b[AL] & 0xF0) | (tmp&0xF);

			tmp = tmp2 | (tmp>>4);
			PutbackRMWord(ModRM,tmp);
			*/
			logerror("PC=%06x : ROR4 %02x\n",cpu_get_pc()-3,ModRM);
			break;
		case 0x2D : // 0Fh 2Dh <1111 1RRR>
			/* OPCODE BRKCS  -	 Break with Contex Switch
			   CPU:  NEC V25,V35,V25 Plus,V35 Plus,V25 Software Guard
			   Description:

				Perform a High-Speed Software Interrupt with contex-switch to
				register bank indicated by the lower 3-bits of 'bank'.

			Info:	NEC V25/V35/V25 Plus/V35 Plus Bank System

				This Chips have	 8 32bytes register banks, which placed in
				Internal chip RAM by addresses:
				xxE00h..xxE1Fh Bank 0
				xxE20h..xxE3Fh Bank 1
				   .........
				xxEC0h..xxEDFh Bank 6
				xxEE0h..xxEFFh Bank 7
				xxF00h..xxFFFh Special Functions Register
				Where xx is Value of IDB register.
				IBD is Byte Register contained Internal data area base
				IBD addresses is FFFFFh and xxFFFh where xx is data in IBD.

				Format of Bank:
				+0	Reserved
				+2	Vector PC
				+4	Save   PSW
				+6	Save   PC
				+8	DS0		;DS
				+A	SS		;SS
				+C	PS		;CS
				+E	DS1		;ES
				+10	IY		;IY
				+11	IX		;IX
				+14	BP		;BP
				+16	SP		;SP
				+18	BW		;BW
				+1A	DW		;DW
				+1C	CW		;CW
				+1E	AW		;AW

				Format of V25 etc. PSW (FLAGS):
				Bit	Description
				15	1
				14	RB2 \
				13	RB1  >	Current Bank Number
				12	RB0 /
				11	V	;OF
				10	IYR	;DF
				9	IE	;IF
				8	BRK	;TF
				7	S	;SF
				6	Z	;ZF
				5	F1	General Purpose user flag #1
						(accessed by Flag Special Function Register)
				4	AC	;AF
				3	F0	General purpose user flag #0
						(accessed by Flag Special Function Register)
				2	P	;PF
				1	BRKI	I/O Trap Enable Flag
				0	CY	;CF

			Flags Affected:	 None
			*/
			ModRM = FETCH;
			logerror("PC=%06x : BRKCS %02x\n",cpu_get_pc()-3,ModRM);
			nec_ICount-=15;// checked !
			break;

		case 0x31: // 0F 31 [mod:reg:r/m] - INS reg8,reg8 or INS reg8,imm4

			ModRM = FETCH;
			logerror("PC=%06x : INS ",cpu_get_pc()-2);
		   	if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		        logerror("ModRM=%04x \n",ModRM);
		    	nec_ICount-=29;
		    }
		    else {
		    	int old=nec_ICount;
		    	(*GetEA[ModRM])();;
		    	tmp=ReadByte(EA);
		    	logerror("ModRM=%04x  Byte=%04x\n",EA,tmp);
   				nec_ICount=old-33;
   			}

			// more to come
			//bfl=tmp2 & 0xf;		// bit field length
			//bfs=tmp & 0xf;		// bit field start (bit offset in DS:IX)
			//I.regs.b[AH] =0;	// AH =0

			/*2do: the rest is silence....yet
			----------O-INS------------------------------------
			OPCODE INS  -  Insert Bit String

			CPU: NEC/Sony  all V-series
			Type of Instruction: User

			Instruction:  INS  start,len

			Description:

				  BitField [	     BASE =  ES:IY
					 START BIT OFFSET =  start
						   LENGTH =  len
						 ]   <-	 AW [ bits= (len-1)..0]

			Note:	di and start automatically UPDATE
			Note:	Alternative Name of this instruction is NECINS

			Flags Affected: None

			CPU mode: RM

			+++++++++++++++++++++++
			Physical Form		 : INS	reg8,reg8
			COP (Code of Operation)	 : 0FH 31H  PostByte
			*/

			//nec_ICount-=31; /* 31 -117 clocks ....*/
			break;
		case 0x33: // 0F 33 [mod:reg:r/m] - EXT reg8,reg8 or EXT reg8,imm4

			ModRM = FETCH;
			logerror("PC=%06x : EXT ",cpu_get_pc()-2);
		   	if (ModRM >= 0xc0) {
		    	tmp=I.regs.b[Mod_RM.RM.b[ModRM]];
		        logerror("ModRM=%04x \n",ModRM);
		    	nec_ICount-=29;
		    }
		    else {
		    	int old=nec_ICount;
		    	(*GetEA[ModRM])();;
		    	tmp=ReadByte(EA);
		    	logerror("ModRM=%04x  Byte=%04x\n",EA,tmp);
   				nec_ICount=old-33;
   			}
			/*2do: the rest is silence....yet */
			//bfl=tmp2 & 0xf;		// bit field length
			//bfs=tmp & 0xf;		// bit field start (bit offset in DS:IX)
			//I.regs.b[AH] =0;	// AH =0

			/*

			----------O-EXT------------------------------------
			OPCODE EXT  -  Extract Bit Field

			CPU: NEC/Sony all  V-series
			Type of Instruction: User

			Instruction:  EXT  start,len

			Description:

				  AW <- BitField [
						     BASE =  DS:IX
					 START BIT OFFSET =  start
						   LENGTH =  len
						 ];

			Note:	si and start automatically UPDATE

			Flags Affected: None

			CPU mode: RM

			+++++++++++++++++++++++
			Physical Form		 : EXT	reg8,reg8
			COP (Code of Operation)	 : 0FH 33H  PostByte

			Clocks:		EXT  reg8,reg8
			NEC V20:	26-55
			*/

			//NEC_ICount-=26; /* 26 -55 clocks ....*/
			break;
		case 0x91:
			/*
			----------O-RETRBI---------------------------------
			OPCODE RETRBI	 -  Return from Register Bank Context
				     Switch  Interrupt.

			CPU:  NEC V25,V35,V25 Plus,V35 Plus,V25 Software Guard
			Type of Instruction: System

			Instruction:  RETRBI

			Description:

				PC  <- Save PC;
				PSW <- Save PSW;

			Flags Affected:	 All

			CPU mode: RM

			+++++++++++++++++++++++
			Physical Form:	RETRBI
			COP (Code of Operation)	 : 0Fh 91h

			Clocks:	 12
			*/
			logerror("PC=%06x : RETRBI\n",cpu_get_pc()-2);
			nec_ICount-=12;
			break;

		case 0x94:
			/*
			----------O-TSKSW----------------------------------
			OPCODE TSKSW  -	  Task Switch

			CPU:  NEC V25,V35,V25 Plus,V35 Plus,V25 Software Guard
			Type of Instruction: System

			Instruction:  TSKSW   reg16

			Description:  Perform a High-Speed task switch to the register bank indicated
				      by lower 3 bits of reg16. The PC and PSW are saved in the old
				      banks. PC and PSW save Registers and the new PC and PSW values
				      are retrived from the new register bank's save area.

			Note:	     See BRKCS instruction for more Info about banks.

			Flags Affected:	 All

			CPU mode: RM

			+++++++++++++++++++++++
			Physical Form:	TSCSW reg16
			COP (Code of Operation)	 : 0Fh 94h <1111 1RRR>

			Clocks:	 11
			*/
			ModRM = FETCH;

			logerror("PC=%06x : TSCSW %02x\n",cpu_get_pc()-3,ModRM);
			nec_ICount-=11;
			break;
		case 0x95:
			/*
			----------O-MOVSPB---------------------------------
			OPCODE MOVSPB	 -  Move Stack Pointer Before Bamk Switching

			CPU:  NEC V25,V35,V25 Plus,V35 Plus,V25 Software Guard
			Type of Instruction: System

			Instruction:  MOVSPB  Number_of_bank

			Description:  The MOVSPB instruction transfers the current SP and SS before
				      the bank switching to new register bank.

			Note:	      New Register Bank Number indicated by lower 3bit of Number_of_
				      _bank.

			Note:	      See BRKCS instruction for more info about banks.

			Flags Affected:	 None

			CPU mode: RM

			+++++++++++++++++++++++
			Physical Form:	MOVSPB	  reg16
			COP (Code of Operation)	 : 0Fh 95h <1111 1RRR>

			Clocks:	 11
			*/
			ModRM = FETCH;
			logerror("PC=%06x : MOVSPB %02x\n",cpu_get_pc()-3,ModRM);
			nec_ICount-=11;
			break;
		case 0xbe:
			/*
			----------O-STOP-----------------------------------
			OPCODE STOP    -  Stop CPU

			CPU:  NEC V25,V35,V25 Plus,V35 Plus,V25 Software Guard
			Type of Instruction: System

			Instruction:  STOP

			Description:
					PowerDown instruction, Stop Oscillator,
					Halt CPU.

			Flags Affected:	 None

			CPU mode: RM

			+++++++++++++++++++++++
			Physical Form:	STOP
			COP (Code of Operation)	 : 0Fh BEh

			Clocks:	 N/A
			*/
			logerror("PC=%06x : STOP\n",cpu_get_pc()-2);
			nec_ICount-=2; /* of course this is crap */
			break;
		case 0xe0:
			/*
			----------O-BRKXA----------------------------------
			OPCODE BRKXA   -  Break to Expansion Address

			CPU:  NEC V33/V53  only
			Type of Instruction: System

			Instruction:  BRKXA int_vector

			Description:
				     [sp-1,sp-2] <- PSW		; PSW EQU FLAGS
				     [sp-3,sp-4] <- PS		; PS  EQU CS
				     [sp-5,sp-6] <- PC		; PC  EQU IP
				     SP	 <-  SP -6
				     IE	 <-  0
				     BRK <-  0
				     MD	 <-  0
				     PC	 <- [int_vector*4 +0,+1]
				     PS	 <- [int_vector*4 +2,+3]
				     Enter Expansion Address Mode.

			Note:	In NEC V53 Memory Space dividing into 1024 16K pages.
				The programming model is Same as in Normal mode.

				Mechanism is:
				20 bit Logical Address:	 19..14 Page Num  13..0 Offset

				page Num convertin by internal table to 23..14 Page Base
				tHE pHYIXCAL ADDRESS is both Base and Offset.

				Address Expansion Registers:
				logical Address A19..A14	I/O Address
				0				FF00h
				1				FF02h
				...				...
				63				FF7Eh

				Register XAM aliased with port # FF80h indicated current mode
				of operation.
				Format of XAM register (READ ONLY):
				15..1	reserved
				0	XA Flag, if=1 then in XA mode.

			Format	of  V53 PSW:
				15..12	1
				11	V
				10	IYR
				9	IE
				8	BRK
				7	S
				6	Z
				5	0
				4	AC
				3	0
				2	P
				1	1
				0	CY

			Flags Affected:	 None

			CPU mode: RM

			+++++++++++++++++++++++
			Physical Form:	BRKXA  imm8
			COP (Code of Operation)	 : 0Fh E0h imm8
			*/

			ModRM = FETCH;
			logerror("PC=%06x : BRKXA %02x\n",cpu_get_pc()-3,ModRM);
			nec_ICount-=12;
			break;
		case 0xf0:
			/*
			----------O-RETXA----------------------------------
			OPCODE RETXA   -  Return from  Expansion Address

			CPU:  NEC V33/V53 only
			Type of Instruction: System

			Instruction:  RETXA int_vector

			Description:
				     [sp-1,sp-2] <- PSW		; PSW EQU FLAGS
				     [sp-3,sp-4] <- PS		; PS  EQU CS
				     [sp-5,sp-6] <- PC		; PC  EQU IP
				     SP	 <-  SP -6
				     IE	 <-  0
				     BRK <-  0
				     MD	 <-  0
				     PC	 <- [int_vector*4 +0,+1]
				     PS	 <- [int_vector*4 +2,+3]
				     Disable EA mode.

			Flags Affected:	 None

			CPU mode: RM

			+++++++++++++++++++++++
			Physical Form:	RETXA  imm8
			COP (Code of Operation)	 : 0Fh F0h imm8

			Clocks:	 12
			*/
			ModRM = FETCH;
			logerror("PC=%06x : RETXA %02x\n",cpu_get_pc()-3,ModRM);
			nec_ICount-=12;
			break;
		case 0xff: /* 0F ff imm8 - BRKEM */
			/*
			OPCODE BRKEM  -	 Break for Emulation

			CPU: NEC/Sony V20/V30/V40/V50
			Description:

					PUSH	FLAGS
					PUSH	CS
					PUSH	IP
					MOV	CS,0:[intnum*4+2]
					MOV	IP,0:[intnum*4]
					MD <- 0;	// Enable 8080 emulation

			Note:	BRKEM instruction do software interrupt and then New CS,IP loaded
				it switch to 8080 mode i.e. CPU will execute 8080 code.
				Mapping Table of Registers in 8080 Mode
				8080 Md.   A  B	 C  D  E  H  L	SP PC  F
				native.	   AL CH CL DH DL BH BL BP IP  FLAGS(low)
				For Return of 8080 mode use CALLN instruction.
			Note:	I.e. 8080 addressing only 64KB then "Real Address" is CS*16+PC

			Flags Affected: MD
			*/
			ModRM=FETCH;
			nec_ICount-=38;
			logerror("PC=%06x : BRKEM %02x\n",cpu_get_pc()-3,ModRM);
			nec_interrupt(ModRM,1);
			break;
		default :
			break;
	}
}


static void i_adc_br8(void)    /* Opcode 0x10 */
{
    DEF_br8(dst,src);
    src+=CF;
    ADDB(dst,src);
    PutbackRMByte(ModRM,dst);
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_adc_wr16(void)    /* Opcode 0x11 */
{
    DEF_wr16(dst,src);
    src+=CF;
    ADDW(dst,src);
    PutbackRMWord(ModRM,dst);
    nec_ICount-=(ModRM >=0xc0 )?2:24;
}

static void i_adc_r8b(void)    /* Opcode 0x12 */
{
    DEF_r8b(dst,src);
    src+=CF;
    ADDB(dst,src);
    RegByte(ModRM)=dst;
    nec_ICount-=(ModRM >=0xc0 )?2:11;
}

static void i_adc_r16w(void)    /* Opcode 0x13 */
{
    DEF_r16w(dst,src);
	nec_ICount-=3;
    src+=CF;
    ADDW(dst,src);
    RegWord(ModRM)=dst;
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_adc_ald8(void)    /* Opcode 0x14 */
{
    DEF_ald8(dst,src);
    src+=CF;
    ADDB(dst,src);
	I.regs.b[AL] = dst;
	nec_ICount-=4;
}

static void i_adc_axd16(void)    /* Opcode 0x15 */
{
    DEF_axd16(dst,src);
    src+=CF;
    ADDW(dst,src);
	I.regs.w[AW]=dst;
	nec_ICount-=4;
}

static void i_push_ss(void)    	/* Opcode 0x16 */
{
	PUSH(I.sregs[SS]);
	nec_ICount-=10;			   	/* OPCODE.LST says 8-12...so 10 */
}

static void i_pop_ss(void)    	/* Opcode 0x17 */
{
	POP(I.sregs[SS]);
	I.base[SS] = SegBase(SS);
	nec_instruction[FETCHOP](); 	/* no interrupt before next instruction */
	nec_ICount-=10;				/* OPCODE.LST says 8-12...so 10 */
}

static void i_sbb_br8(void)    /* Opcode 0x18 */
{
    DEF_br8(dst,src);
    src+=CF;
    SUBB(dst,src);
    PutbackRMByte(ModRM,dst);
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_sbb_wr16(void)    /* Opcode 0x19 */
{
    DEF_wr16(dst,src);
    src+=CF;
    SUBW(dst,src);
    PutbackRMWord(ModRM,dst);
    nec_ICount-=(ModRM >=0xc0 )?2:24;
}

static void i_sbb_r8b(void)    /* Opcode 0x1a */
{
    DEF_r8b(dst,src);
    src+=CF;
    SUBB(dst,src);
    RegByte(ModRM)=dst;
    nec_ICount-=(ModRM >=0xc0 )?2:11;
}

static void i_sbb_r16w(void)    /* Opcode 0x1b */
{
    DEF_r16w(dst,src);
    src+=CF;
    SUBW(dst,src);
    RegWord(ModRM)= dst;
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_sbb_ald8(void)    /* Opcode 0x1c */
{
    DEF_ald8(dst,src);
    src+=CF;
    SUBB(dst,src);
	I.regs.b[AL] = dst;
	nec_ICount-=4;
}

static void i_sbb_axd16(void)    /* Opcode 0x1d */
{
    DEF_axd16(dst,src);
    src+=CF;
    SUBW(dst,src);
	I.regs.w[AW]=dst;
	nec_ICount-=4;
}

static void i_push_ds(void)    /* Opcode 0x1e */
{
	PUSH(I.sregs[DS]);
	nec_ICount-=10;
}

static void i_pop_ds(void)    /* Opcode 0x1f */
{
	POP(I.sregs[DS]);
	I.base[DS] = SegBase(DS);
	nec_ICount-=10;
}

static void i_and_br8(void)    /* Opcode 0x20 */
{
    DEF_br8(dst,src);
    ANDB(dst,src);
    PutbackRMByte(ModRM,dst);
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_and_wr16(void)    /* Opcode 0x21 */
{
    DEF_wr16(dst,src);
    ANDW(dst,src);
    PutbackRMWord(ModRM,dst);
    nec_ICount-=(ModRM >=0xc0 )?2:24;
}

static void i_and_r8b(void)    /* Opcode 0x22 */
{
    DEF_r8b(dst,src);
    ANDB(dst,src);
    RegByte(ModRM)=dst;
    nec_ICount-=(ModRM >=0xc0 )?2:11;
}

static void i_and_r16w(void)    /* Opcode 0x23 */
{
    DEF_r16w(dst,src);
    ANDW(dst,src);
    RegWord(ModRM)=dst;
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_and_ald8(void)    /* Opcode 0x24 */
{
    DEF_ald8(dst,src);
    ANDB(dst,src);
	I.regs.b[AL] = dst;
	nec_ICount-=4;
}

static void i_and_axd16(void)    /* Opcode 0x25 */
{
    DEF_axd16(dst,src);
    ANDW(dst,src);
	I.regs.w[AW]=dst;
	nec_ICount-=4;
}

static void i_es(void)    /* Opcode 0x26 */
{
    I.seg_prefix=TRUE;
	I.prefix_base=I.base[ES];
	nec_ICount-=2;
	nec_instruction[FETCHOP]();
}

static void i_daa(void)    /* Opcode 0x27 */
{
	if (AF || ((I.regs.b[AL] & 0xf) > 9))
	{
		int tmp;
		I.regs.b[AL] = tmp = I.regs.b[AL] + 6;
		I.AuxVal = 1;
		I.CarryVal |= tmp & 0x100;
	}

	if (CF || (I.regs.b[AL] > 0x9f))
	{
		I.regs.b[AL] += 0x60;
		I.CarryVal = 1;
	}

	SetSZPF_Byte(I.regs.b[AL]);
	nec_ICount-=3;
}


static void i_sub_br8(void)    /* Opcode 0x28 */
{
    DEF_br8(dst,src);
	SUBB(dst,src);
    PutbackRMByte(ModRM,dst);
    nec_ICount-=(ModRM >=0xc0 )?2:15;

}

static void i_sub_wr16(void)    /* Opcode 0x29 */
{
    DEF_wr16(dst,src);
    SUBW(dst,src);
    PutbackRMWord(ModRM,dst);
    nec_ICount-=(ModRM >=0xc0 )?2:24;
}

static void i_sub_r8b(void)    /* Opcode 0x2a */
{
    DEF_r8b(dst,src);
    SUBB(dst,src);
    RegByte(ModRM)=dst;
    nec_ICount-=(ModRM >=0xc0 )?2:11;
}

static void i_sub_r16w(void)    /* Opcode 0x2b */
{
    DEF_r16w(dst,src);
    SUBW(dst,src);
    RegWord(ModRM)=dst;
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_sub_ald8(void)    /* Opcode 0x2c */
{
    DEF_ald8(dst,src);
    SUBB(dst,src);
	I.regs.b[AL] = dst;
	nec_ICount-=4;
}

static void i_sub_axd16(void)    /* Opcode 0x2d */
{
    DEF_axd16(dst,src);
    SUBW(dst,src);
	I.regs.w[AW]=dst;
	nec_ICount-=4;
}

static void i_cs(void)    /* Opcode 0x2e */
{
    I.seg_prefix=TRUE;
	I.prefix_base=I.base[CS];
	nec_ICount-=2;
	nec_instruction[FETCHOP]();
}

static void i_das(void)    /* Opcode 0x2f */
{
	if (AF || ((I.regs.b[AL] & 0xf) > 9))
	{
		int tmp;
		I.regs.b[AL] = tmp = I.regs.b[AL] - 6;
		I.AuxVal = 1;
		I.CarryVal |= tmp & 0x100;
	}

	if (CF || (I.regs.b[AL] > 0x9f))
	{
		I.regs.b[AL] -= 0x60;
		I.CarryVal = 1;
	}

	SetSZPF_Byte(I.regs.b[AL]);
	nec_ICount-=7;
}

static void i_xor_br8(void)    /* Opcode 0x30 */
{
    DEF_br8(dst,src);
    XORB(dst,src);
    PutbackRMByte(ModRM,dst);
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_xor_wr16(void)    /* Opcode 0x31 */
{
    DEF_wr16(dst,src);
    XORW(dst,src);
    PutbackRMWord(ModRM,dst);
    nec_ICount-=(ModRM >=0xc0 )?2:24;
}

static void i_xor_r8b(void)    /* Opcode 0x32 */
{
    DEF_r8b(dst,src);
	XORB(dst,src);
    RegByte(ModRM)=dst;
    nec_ICount-=(ModRM >=0xc0 )?2:11;
}

static void i_xor_r16w(void)    /* Opcode 0x33 */
{
    DEF_r16w(dst,src);
    XORW(dst,src);
    RegWord(ModRM)=dst;
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_xor_ald8(void)    /* Opcode 0x34 */
{
    DEF_ald8(dst,src);
    XORB(dst,src);
	I.regs.b[AL] = dst;
	nec_ICount-=4;
}

static void i_xor_axd16(void)    /* Opcode 0x35 */
{
    DEF_axd16(dst,src);
    XORW(dst,src);
	I.regs.w[AW]=dst;
	nec_ICount-=4;
}

static void i_ss(void)    /* Opcode 0x36 */
{
    I.seg_prefix=TRUE;
	I.prefix_base=I.base[SS];
	nec_ICount-=2;
	nec_instruction[FETCHOP]();
}

static void i_aaa(void)    /* Opcode 0x37 */
{
	if (AF || ((I.regs.b[AL] & 0xf) > 9))
    {
		I.regs.b[AL] += 6;
		I.regs.b[AH] += 1;
		I.AuxVal = 1;
		I.CarryVal = 1;
    }
	else
	{
		I.AuxVal = 0;
		I.CarryVal = 0;
    }
	I.regs.b[AL] &= 0x0F;
	nec_ICount-=3;
}

static void i_cmp_br8(void)    /* Opcode 0x38 */
{
    DEF_br8(dst,src);
    SUBB(dst,src);
    nec_ICount-=(ModRM >=0xc0 )?2:11;
}

static void i_cmp_wr16(void)    /* Opcode 0x39 */
{
    DEF_wr16(dst,src);
    SUBW(dst,src);
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_cmp_r8b(void)    /* Opcode 0x3a */
{
    DEF_r8b(dst,src);
    SUBB(dst,src);
    nec_ICount-=(ModRM >=0xc0 )?2:11;
}

static void i_cmp_r16w(void)    /* Opcode 0x3b */
{
    DEF_r16w(dst,src);
    SUBW(dst,src);
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_cmp_ald8(void)    /* Opcode 0x3c */
{
    DEF_ald8(dst,src);
    SUBB(dst,src);
    nec_ICount-=4;
}

static void i_cmp_axd16(void)    /* Opcode 0x3d */
{
    DEF_axd16(dst,src);
    SUBW(dst,src);
    nec_ICount-=4;
}

static void i_ds(void)    /* Opcode 0x3e */
{
    I.seg_prefix=TRUE;
	I.prefix_base=I.base[DS];
	nec_ICount-=2;
	nec_instruction[FETCHOP]();
}

static void i_aas(void)    /* Opcode 0x3f */
{
	if (AF || ((I.regs.b[AL] & 0xf) > 9))
    {
		I.regs.b[AL] -= 6;
		I.regs.b[AH] -= 1;
		I.AuxVal = 1;
		I.CarryVal = 1;
    }
	else
	{
		I.AuxVal = 0;
		I.CarryVal = 0;
    }
	I.regs.b[AL] &= 0x0F;
	nec_ICount-=3;
}


#define IncWordReg(Reg) 					\
{											\
	unsigned tmp = (unsigned)I.regs.w[Reg]; \
	unsigned tmp1 = tmp+1;					\
	/*SetOFW_Add(tmp1,tmp,1);*/				\
	I.OverVal = (tmp == 0x7fff); /* MISH */ \
	SetAF(tmp1,tmp,1);						\
	SetSZPF_Word(tmp1); 					\
	I.regs.w[Reg]=tmp1; 					\
	nec_ICount-=2;							\
}


static void i_inc_ax(void)    /* Opcode 0x40 */
{
    IncWordReg(AW);
}

static void i_inc_cx(void)    /* Opcode 0x41 */
{
    IncWordReg(CW);
}

static void i_inc_dx(void)    /* Opcode 0x42 */
{
    IncWordReg(DW);
}

static void i_inc_bx(void)    /* Opcode 0x43 */
{
    IncWordReg(BW);
}

static void i_inc_sp(void)    /* Opcode 0x44 */
{
    IncWordReg(SP);
}

static void i_inc_bp(void)    /* Opcode 0x45 */
{
    IncWordReg(BP);
}

static void i_inc_si(void)    /* Opcode 0x46 */
{
    IncWordReg(IX);
}

static void i_inc_di(void)    /* Opcode 0x47 */
{
    IncWordReg(IY);
}

#define DecWordReg(Reg) 					\
{ 											\
	unsigned tmp = (unsigned)I.regs.w[Reg]; \
    unsigned tmp1 = tmp-1; 					\
    /*SetOFW_Sub(tmp1,1,tmp);*/ 			\
	I.OverVal = (tmp == 0x8000); /* MISH */ \
    SetAF(tmp1,tmp,1); 						\
    SetSZPF_Word(tmp1); 					\
	I.regs.w[Reg]=tmp1; 					\
	nec_ICount-=2; 							\
}

static void i_dec_ax(void)    /* Opcode 0x48 */
{
    DecWordReg(AW);
}

static void i_dec_cx(void)    /* Opcode 0x49 */
{
    DecWordReg(CW);
}

static void i_dec_dx(void)    /* Opcode 0x4a */
{
    DecWordReg(DW);
}

static void i_dec_bx(void)    /* Opcode 0x4b */
{
    DecWordReg(BW);
}

static void i_dec_sp(void)    /* Opcode 0x4c */
{
    DecWordReg(SP);
}

static void i_dec_bp(void)    /* Opcode 0x4d */
{
    DecWordReg(BP);
}

static void i_dec_si(void)    /* Opcode 0x4e */
{
    DecWordReg(IX);
}

static void i_dec_di(void)    /* Opcode 0x4f */
{
    DecWordReg(IY);
}

static void i_push_ax(void)    /* Opcode 0x50 */
{
	PUSH(I.regs.w[AW]);
	nec_ICount-=10;
}

static void i_push_cx(void)    /* Opcode 0x51 */
{
	PUSH(I.regs.w[CW]);
	nec_ICount-=10;
}

static void i_push_dx(void)    /* Opcode 0x52 */
{
	PUSH(I.regs.w[DW]);
	nec_ICount-=10;
}

static void i_push_bx(void)    /* Opcode 0x53 */
{
	PUSH(I.regs.w[BW]);
	nec_ICount-=10;
}

static void i_push_sp(void)    /* Opcode 0x54 */
{
	PUSH(I.regs.w[SP]);
	nec_ICount-=10;
}

static void i_push_bp(void)    /* Opcode 0x55 */
{
	PUSH(I.regs.w[BP]);
	nec_ICount-=10;
}


static void i_push_si(void)    /* Opcode 0x56 */
{
	PUSH(I.regs.w[IX]);
	nec_ICount-=10;
}

static void i_push_di(void)    /* Opcode 0x57 */
{
	PUSH(I.regs.w[IY]);
	nec_ICount-=10;
}

static void i_pop_ax(void)    /* Opcode 0x58 */
{
	POP(I.regs.w[AW]);
	nec_ICount-=10;
}

static void i_pop_cx(void)    /* Opcode 0x59 */
{
	POP(I.regs.w[CW]);
	nec_ICount-=10;
}

static void i_pop_dx(void)    /* Opcode 0x5a */
{
	POP(I.regs.w[DW]);
	nec_ICount-=10;
}

static void i_pop_bx(void)    /* Opcode 0x5b */
{
	POP(I.regs.w[BW]);
	nec_ICount-=10;
}

static void i_pop_sp(void)    /* Opcode 0x5c */
{
	POP(I.regs.w[SP]);
	nec_ICount-=10;
}

static void i_pop_bp(void)    /* Opcode 0x5d */
{
	POP(I.regs.w[BP]);
	nec_ICount-=10;
}

static void i_pop_si(void)    /* Opcode 0x5e */
{
	POP(I.regs.w[IX]);
	nec_ICount-=10;
}

static void i_pop_di(void)    /* Opcode 0x5f */
{
	POP(I.regs.w[IY]);
	nec_ICount-=10;
}

static void i_pusha(void)    /* Opcode 0x60 */
{
	unsigned tmp=I.regs.w[SP];
	PUSH(I.regs.w[AW]);
	PUSH(I.regs.w[CW]);
	PUSH(I.regs.w[DW]);
	PUSH(I.regs.w[BW]);
    PUSH(tmp);
	PUSH(I.regs.w[BP]);
	PUSH(I.regs.w[IX]);
	PUSH(I.regs.w[IY]);
	nec_ICount-=51;
}

static void i_popa(void)    /* Opcode 0x61 */
{
    unsigned tmp;
	POP(I.regs.w[IY]);
	POP(I.regs.w[IX]);
	POP(I.regs.w[BP]);
    POP(tmp);
	POP(I.regs.w[BW]);
	POP(I.regs.w[DW]);
	POP(I.regs.w[CW]);
	POP(I.regs.w[AW]);
	nec_ICount-=59;
}

static void i_bound(void)    /* Opcode 0x62  BOUND or CHKIND (on NEC)*/
{
	unsigned ModRM = FETCH;
    int low = (INT16)GetRMWord(ModRM);
    int high= (INT16)GetnextRMWord;
    int tmp= (INT16)RegWord(ModRM);
    if (tmp<low || tmp>high) {
		/* OB: on NECs CS:IP points to instruction
		       FOLLOWING the BOUND instruction ! */
		// I.ip-=2;
		nec_interrupt(5,0);
    }
 	nec_ICount-=20;
}

static void i_brkn(void)	/* Opcode 0x63 BRKN -  Break to Native Mode */
{
	/*
	CPU:  NEC (V25/V35) Software Guard only
	Instruction:  BRKN int_vector

	Description:
		     [sp-1,sp-2] <- PSW		; PSW EQU FLAGS
		     [sp-3,sp-4] <- PS		; PS  EQU CS
		     [sp-5,sp-6] <- PC		; PC  EQU IP
		     SP	 <-  SP -6
		     IE	 <-  0
		     BRK <-  0
		     MD	 <-  1
		     PC	 <- [int_vector*4 +0,+1]
		     PS	 <- [int_vector*4 +2,+3]

	Note:	The BRKN instruction switches operations in Native Mode
		from Security Mode via Interrupt call. In Normal Mode
		Instruction executed as	 mPD70320/70322 (V25) operation mode.

	Flags Affected:	 None

	CPU mode: RM

	+++++++++++++++++++++++
	Physical Form:	BRKN  imm8
	COP (Code of Operation)	 : 63h imm8

	Clocks:	 56+10T [44+10T]
	*/
	//nec_ICount-=56;
	unsigned int_vector;
	int_vector = FETCH;
	logerror("PC=%06x : BRKN %02x\n",cpu_get_pc()-2,int_vector);
}



static void repc(int flagval)
{
    /* Handles repc- and repnc- prefixes. flagval is the value of ZF for the
       loop  to continue for CMPS and SCAS instructions. */

	unsigned next = FETCHOP;
	unsigned count = I.regs.w[CW];

    switch(next)
    {
    case 0x26:  /* ES: */
        I.seg_prefix=TRUE;
		I.prefix_base=I.base[ES];
		nec_ICount-=2;
		repc(flagval);
		break;
    case 0x2e:  /* CS: */
        I.seg_prefix=TRUE;
		I.prefix_base=I.base[CS];
		nec_ICount-=2;
		repc(flagval);
		break;
    case 0x36:  /* SS: */
        I.seg_prefix=TRUE;
		I.prefix_base=I.base[SS];
		nec_ICount-=2;
		repc(flagval);
		break;
    case 0x3e:  /* DS: */
        I.seg_prefix=TRUE;
		I.prefix_base=I.base[DS];
		nec_ICount-=2;
		repc(flagval);
		break;
    case 0x6c:  /* REP INSB */
		nec_ICount-=9-count;
		for (; (CF==flagval)&&(count > 0); count--)
           i_insb();
		I.regs.w[CW]=count;
		break;
    case 0x6d:  /* REP INSW */
		nec_ICount-=9-count;
		for (;(CF==flagval)&&(count > 0); count--)
           i_insw();
		I.regs.w[CW]=count;
		break;
    case 0x6e:  /* REP OUTSB */
		nec_ICount-=9-count;
		for (;(CF==flagval)&&(count > 0); count--)
            i_outsb();
		I.regs.w[CW]=count;
		break;
    case 0x6f:  /* REP OUTSW */
		nec_ICount-=9-count;
		for (; (CF==flagval)&&(count > 0); count--)
            i_outsw();
		I.regs.w[CW]=count;
		break;
    case 0xa4:  /* REP MOVSB */
		nec_ICount-=9-count;
		for (;(CF==flagval)&&(count > 0); count--)
			i_movsb();
		I.regs.w[CW]=count;
		break;
    case 0xa5:  /* REP MOVSW */
		nec_ICount-=9-count;
		for (;(CF==flagval)&&(count > 0); count--)
			i_movsw();
		I.regs.w[CW]=count;
		break;
    case 0xa6:  /* REP(N)E CMPSB */
		nec_ICount-=9;
		for (I.ZeroVal = !flagval; (ZF == flagval) && (CF==flagval)&&(count > 0); count--)
			i_cmpsb();
		I.regs.w[CW]=count;
		break;
    case 0xa7:  /* REP(N)E CMPSW */
		nec_ICount-=9;
		for (I.ZeroVal = !flagval; (ZF == flagval) && (CF==flagval)&&(count > 0); count--)
			i_cmpsw();
		I.regs.w[CW]=count;
		break;
    case 0xaa:  /* REP STOSB */
		nec_ICount-=9-count;
		for (;(CF==flagval)&&(count > 0); count--)
			i_stosb();
		I.regs.w[CW]=count;
		break;
    case 0xab:  /* REP STOSW */
		nec_ICount-=9-count;
		for (;(CF==flagval)&&(count > 0); count--)
			i_stosw();
		I.regs.w[CW]=count;
		break;
    case 0xac:  /* REP LODSB */
		nec_ICount-=9;
		for (;(CF==flagval)&&(count > 0); count--)
			i_lodsb();
		I.regs.w[CW]=count;
		break;
    case 0xad:  /* REP LODSW */
		nec_ICount-=9;
		for (;(CF==flagval)&&(count > 0); count--)
			i_lodsw();
		I.regs.w[CW]=count;
		break;
    case 0xae:  /* REP(N)E SCASB */
		nec_ICount-=9;
		for (I.ZeroVal = !flagval; (ZF == flagval) && (CF==flagval)&&(count > 0); count--)
			i_scasb();
		I.regs.w[CW]=count;
		break;
    case 0xaf:  /* REP(N)E SCASW */
		nec_ICount-=9;
		for (I.ZeroVal = !flagval; (ZF == flagval) && (CF==flagval)&&(count > 0); count--)
			i_scasw();
		I.regs.w[CW]=count;
		break;
    default:
		nec_instruction[next]();
    }
}

static void i_repnc(void)    /* Opcode 0x64 */
{
    repc(0);
}

static void i_repc(void)    /* Opcode 0x65 */
{
    repc(1);
}

static void i_push_d16(void)    /* Opcode 0x68 */
{
    unsigned tmp = FETCH;
    tmp += FETCH << 8;
    PUSH(tmp);
    nec_ICount-=12;
}

static void i_imul_d16(void)    /* Opcode 0x69 */
{
    DEF_r16w(dst,src);
    unsigned src2=FETCH;
    src2+=(FETCH<<8);
    dst = (INT32)((INT16)src)*(INT32)((INT16)src2);
	I.CarryVal = I.OverVal = (((INT32)dst) >> 15 != 0) && (((INT32)dst) >> 15 != -1);
    RegWord(ModRM)=(WORD)dst;
    nec_ICount-=(ModRM >=0xc0 )?38:47;
}

static void i_push_d8(void)    /* Opcode 0x6a */
{
    unsigned tmp = (WORD)((INT16)((INT8)FETCH));
    PUSH(tmp);
    nec_ICount-=7;
}

static void i_imul_d8(void)    /* Opcode 0x6b */
{
    DEF_r16w(dst,src);
    unsigned src2= (WORD)((INT16)((INT8)FETCH));
    dst = (INT32)((INT16)src)*(INT32)((INT16)src2);
	I.CarryVal = I.OverVal = (((INT32)dst) >> 15 != 0) && (((INT32)dst) >> 15 != -1);
    RegWord(ModRM)=(WORD)dst;
    nec_ICount-=(ModRM >=0xc0 )?31:39;
}

static void i_insb(void)    /* Opcode 0x6c */
{
	nec_ICount-=5;
	PutMemB(ES,I.regs.w[IY],read_port(I.regs.w[DW]));
	I.regs.w[IY]+= -2 * I.DF + 1;
}

static void i_insw(void)    /* Opcode 0x6d */
{
	PutMemB(ES,I.regs.w[IY],read_port(I.regs.w[DW]));
	PutMemB(ES,I.regs.w[IY]+1,read_port(I.regs.w[DW]+1));
//logerror("%04x:  insw\n",cpu_get_pc());
	I.regs.w[IY]+= -4 * I.DF + 2;
	nec_ICount-=8;
}

static void i_outsb(void)    /* Opcode 0x6e */
{
	write_port(I.regs.w[DW],GetMemB(DS,I.regs.w[IX]));
	I.regs.w[IY]+= -2 * I.DF + 1;
	nec_ICount-=8;
}

static void i_outsw(void)    /* Opcode 0x6f */
{
	write_port(I.regs.w[DW],GetMemB(DS,I.regs.w[IX]));
	write_port(I.regs.w[DW]+1,GetMemB(DS,I.regs.w[IX]+1));
	I.regs.w[IY]+= -4 * I.DF + 2;
	nec_ICount-=8;
}

static void i_jo(void)    /* Opcode 0x70 */
{
	int tmp = (int)((INT8)FETCH);
	if (OF)
	{
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jno(void)    /* Opcode 0x71 */
{
	int tmp = (int)((INT8)FETCH);
	if (!OF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jb(void)    /* Opcode 0x72 */
{
	int tmp = (int)((INT8)FETCH);
	if (CF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jnb(void)    /* Opcode 0x73 */
{
	int tmp = (int)((INT8)FETCH);
	if (!CF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jz(void)    /* Opcode 0x74 */
{
	int tmp = (int)((INT8)FETCH);
	if (ZF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jnz(void)    /* Opcode 0x75 */
{
	int tmp = (int)((INT8)FETCH);
	if (!ZF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jbe(void)    /* Opcode 0x76 */
{
	int tmp = (int)((INT8)FETCH);
    if (CF || ZF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jnbe(void)    /* Opcode 0x77 */
{
	int tmp = (int)((INT8)FETCH);
    if (!(CF || ZF)) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_js(void)    /* Opcode 0x78 */
{
	int tmp = (int)((INT8)FETCH);
    if (SF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jns(void)    /* Opcode 0x79 */
{
	int tmp = (int)((INT8)FETCH);
    if (!SF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jp(void)    /* Opcode 0x7a */
{
	int tmp = (int)((INT8)FETCH);
    if (PF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jnp(void)    /* Opcode 0x7b */
{
	int tmp = (int)((INT8)FETCH);
    if (!PF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jl(void)    /* Opcode 0x7c */
{
	int tmp = (int)((INT8)FETCH);
    if ((SF!=OF)&&!ZF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jnl(void)    /* Opcode 0x7d */
{
	int tmp = (int)((INT8)FETCH);
    if (ZF||(SF==OF)) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jle(void)    /* Opcode 0x7e */
{
	int tmp = (int)((INT8)FETCH);
    if (ZF||(SF!=OF)) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_jnle(void)    /* Opcode 0x7f */
{
	int tmp = (int)((INT8)FETCH);
    if ((SF==OF)&&!ZF) {
		I.ip = (WORD)(I.ip+tmp);
		nec_ICount-=14;
		change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=4;
}

static void i_80pre(void)    /* Opcode 0x80 */
{
	unsigned ModRM = FETCH;
    unsigned dst = GetRMByte(ModRM);
    unsigned src = FETCH;
	nec_ICount-=(ModRM >=0xc0 )?4:18;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD eb,d8 */
        ADDB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x08:  /* OR eb,d8 */
        ORB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x10:  /* ADC eb,d8 */
        src+=CF;
        ADDB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x18:  /* SBB eb,b8 */
        src+=CF;
        SUBB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x20:  /* AND eb,d8 */
        ANDB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x28:  /* SUB eb,d8 */
        SUBB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x30:  /* XOR eb,d8 */
        XORB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x38:  /* CMP eb,d8 */
        SUBB(dst,src);
	break;
    }
}


static void i_81pre(void)    /* Opcode 0x81 */
{
	unsigned ModRM = FETCH;
    unsigned dst = GetRMWord(ModRM);
    unsigned src = FETCH;
    src+= (FETCH << 8);
	nec_ICount-=(ModRM >=0xc0 )?4:26;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD ew,d16 */
        ADDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x08:  /* OR ew,d16 */
        ORW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x10:  /* ADC ew,d16 */
        src+=CF;
		ADDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x18:  /* SBB ew,d16 */
        src+=CF;
        SUBW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x20:  /* AND ew,d16 */
        ANDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x28:  /* SUB ew,d16 */
        SUBW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x30:  /* XOR ew,d16 */
        XORW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x38:  /* CMP ew,d16 */
        SUBW(dst,src);
	break;
    }
}

static void i_82pre(void)	 /* Opcode 0x82 */
{
	unsigned ModRM = FETCH;
	unsigned dst = GetRMByte(ModRM);
	unsigned src = FETCH;
	nec_ICount-=(ModRM >=0xc0 )?4:18;

    switch (ModRM & 0x38)
    {
	case 0x00:	/* ADD eb,d8 */
		ADDB(dst,src);
		PutbackRMByte(ModRM,dst);
	break;
	case 0x08:	/* OR eb,d8 */
		ORB(dst,src);
		PutbackRMByte(ModRM,dst);
	break;
	case 0x10:	/* ADC eb,d8 */
        src+=CF;
		ADDB(dst,src);
		PutbackRMByte(ModRM,dst);
	break;
	case 0x18:	/* SBB eb,d8 */
        src+=CF;
		SUBB(dst,src);
		PutbackRMByte(ModRM,dst);
	break;
	case 0x20:	/* AND eb,d8 */
		ANDB(dst,src);
		PutbackRMByte(ModRM,dst);
	break;
	case 0x28:	/* SUB eb,d8 */
		SUBB(dst,src);
		PutbackRMByte(ModRM,dst);
	break;
	case 0x30:	/* XOR eb,d8 */
		XORB(dst,src);
		PutbackRMByte(ModRM,dst);
	break;
	case 0x38:	/* CMP eb,d8 */
		SUBB(dst,src);
	break;
    }
}

static void i_83pre(void)    /* Opcode 0x83 */
{
	unsigned ModRM = FETCH;
    unsigned dst = GetRMWord(ModRM);
    unsigned src = (WORD)((INT16)((INT8)FETCH));
	nec_ICount-=(ModRM >=0xc0 )?4:26;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD ew,d16 */
        ADDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x08:  /* OR ew,d16 */
        ORW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x10:  /* ADC ew,d16 */
        src+=CF;
        ADDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x18:  /* SBB ew,d16 */
        src+=CF;
        SUBW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x20:  /* AND ew,d16 */
        ANDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x28:  /* SUB ew,d16 */
        SUBW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x30:  /* XOR ew,d16 */
        XORW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x38:  /* CMP ew,d16 */
        SUBW(dst,src);
	break;
    }
}

static void i_test_br8(void)    /* Opcode 0x84 */
{
    DEF_br8(dst,src);
    ANDB(dst,src);
    nec_ICount-=(ModRM >=0xc0 )?2:10;
}

static void i_test_wr16(void)    /* Opcode 0x85 */
{
    DEF_wr16(dst,src);
    ANDW(dst,src);
    nec_ICount-=(ModRM >=0xc0 )?2:14;
}

static void i_xchg_br8(void)    /* Opcode 0x86 */
{
    DEF_br8(dst,src);
    RegByte(ModRM)=dst;
    PutbackRMByte(ModRM,src);
    // V30
    if (ModRM >= 0xc0) nec_ICount-=3;
    else nec_ICount-=(EO & 1 )?24:16;
}

static void i_xchg_wr16(void)    /* Opcode 0x87 */
{
    DEF_wr16(dst,src);
    RegWord(ModRM)=dst;
    PutbackRMWord(ModRM,src);
    nec_ICount-=(ModRM >=0xc0 )?3:24;
}

static void i_mov_br8(void)    /* Opcode 0x88 */
{
	unsigned ModRM = FETCH;
    BYTE src = RegByte(ModRM);
    PutRMByte(ModRM,src);
    nec_ICount-=(ModRM >=0xc0 )?2:9;
}

static void i_mov_wr16(void)    /* Opcode 0x89 */
{
	unsigned ModRM = FETCH;
    WORD src = RegWord(ModRM);
    PutRMWord(ModRM,src);
    nec_ICount-=(ModRM >=0xc0 )?2:13;
}

static void i_mov_r8b(void)    /* Opcode 0x8a */
{
	unsigned ModRM = FETCH;
    BYTE src = GetRMByte(ModRM);
    RegByte(ModRM)=src;
    nec_ICount-=(ModRM >=0xc0 )?2:11;
}

static void i_mov_r16w(void)    /* Opcode 0x8b */
{
	unsigned ModRM = FETCH;
    WORD src = GetRMWord(ModRM);
    RegWord(ModRM)=src;
    nec_ICount-=(ModRM >=0xc0 )?2:15;
}

static void i_mov_wsreg(void)    /* Opcode 0x8c */
{
	unsigned ModRM = FETCH;
	if (ModRM & 0x20) return;	/* HJB 12/13/98 1xx is invalid */
	PutRMWord(ModRM,I.sregs[(ModRM & 0x38) >> 3]);
	nec_ICount-=(ModRM >=0xc0 )?2:12;
}

static void i_lea(void)    /* Opcode 0x8d */
{
	unsigned ModRM = FETCH;
	(void)(*GetEA[ModRM])();
	RegWord(ModRM)=EO;	/* HJB 12/13/98 effective offset (no segment part) */
	nec_ICount-=4;
}

static void i_mov_sregw(void)    /* Opcode 0x8e */
{
	unsigned ModRM = FETCH;
    WORD src = GetRMWord(ModRM);
	nec_ICount-=(ModRM >=0xc0 )?2:13;
    switch (ModRM & 0x38)
    {
    case 0x00:  /* mov es,ew */
	I.sregs[ES] = src;
	I.base[ES] = SegBase(ES);
	break;
    case 0x18:  /* mov ds,ew */
	I.sregs[DS] = src;
	I.base[DS] = SegBase(DS);
	break;
    case 0x10:  /* mov ss,ew */
	I.sregs[SS] = src;
	I.base[SS] = SegBase(SS); /* no interrupt allowed before next instr */
	nec_instruction[FETCHOP]();
	break;
    case 0x08:  /* mov cs,ew */
	break;  /* doesn't do a jump far */
    }
}

static void i_popw(void)    /* Opcode 0x8f */
{
	unsigned ModRM = FETCH;
    WORD tmp;
    POP(tmp);
    PutRMWord(ModRM,tmp);
    nec_ICount-=21;
}


#define XchgAWReg(Reg) \
{ \
    WORD tmp; \
	tmp = I.regs.w[Reg]; \
	I.regs.w[Reg] = I.regs.w[AW]; \
	I.regs.w[AW] = tmp; \
	nec_ICount-=3; \
}


static void i_nop(void)    /* Opcode 0x90 */
{
    /* this is XchgAWReg(AW); */
	nec_ICount-=2;
}

static void i_xchg_axcx(void)    /* Opcode 0x91 */
{
    XchgAWReg(CW);
}

static void i_xchg_axdx(void)    /* Opcode 0x92 */
{
    XchgAWReg(DW);
}

static void i_xchg_axbx(void)    /* Opcode 0x93 */
{
    XchgAWReg(BW);
}

static void i_xchg_axsp(void)    /* Opcode 0x94 */
{
    XchgAWReg(SP);
}

static void i_xchg_axbp(void)    /* Opcode 0x95 */
{
    XchgAWReg(BP);
}

static void i_xchg_axsi(void)    /* Opcode 0x96 */
{
    XchgAWReg(IX);
}

static void i_xchg_axdi(void)    /* Opcode 0x97 */
{
    XchgAWReg(IY);
}

static void i_cbw(void)    /* Opcode 0x98 */
{
	nec_ICount-=2;
	I.regs.b[AH] = (I.regs.b[AL] & 0x80) ? 0xff : 0;
}

static void i_cwd(void)    /* Opcode 0x99 */
{
	nec_ICount-=5;
	I.regs.w[DW] = (I.regs.b[AH] & 0x80) ? 0xffff : 0;
}

static void i_call_far(void)
{
    unsigned tmp, tmp2;

	tmp = FETCH;
	tmp += FETCH << 8;

	tmp2 = FETCH;
	tmp2 += FETCH << 8;

	PUSH(I.sregs[CS]);
	PUSH(I.ip);

	I.ip = (WORD)tmp;
	I.sregs[CS] = (WORD)tmp2;
	I.base[CS] = SegBase(CS);
	change_pc20((I.base[CS]+I.ip));
	nec_ICount-=39;
}

static void i_wait(void)    /* Opcode 0x9b */
{
	nec_ICount-=7;   /* 2+5n (n = number of times POLL pin sampled) */
}

static void i_pushf(void)    /* Opcode 0x9c */
{
    PUSH( CompressFlags() | 0xf000 );
    nec_ICount-=10;
}

static void i_popf(void)    /* Opcode 0x9d */
{
    unsigned tmp;
    POP(tmp);
    ExpandFlags(tmp);
	nec_ICount-=10;
	if (I.TF) nec_trap();
}

static void i_sahf(void)    /* Opcode 0x9e */
{
	unsigned tmp = (CompressFlags() & 0xff00) | (I.regs.b[AH] & 0xd5);
    ExpandFlags(tmp);
    nec_ICount-=3;
}

static void i_lahf(void)    /* Opcode 0x9f */
{
	I.regs.b[AH] = CompressFlags() & 0xff;
	nec_ICount-=2;
}

static void i_mov_aldisp(void)    /* Opcode 0xa0 */
{
    unsigned addr;

	addr = FETCH;
	addr += FETCH << 8;
	I.regs.b[AL] = GetMemB(DS, addr);
	nec_ICount-=10;
}

static void i_mov_axdisp(void)    /* Opcode 0xa1 */
{
    unsigned addr;

	addr = FETCH;
	addr += FETCH << 8;
	I.regs.b[AL] = GetMemB(DS, addr);
	I.regs.b[AH] = GetMemB(DS, addr+1);
	nec_ICount-=14;
}


static void i_mov_dispal(void)    /* Opcode 0xa2 */
{
    unsigned addr;
	addr = FETCH;
	addr += FETCH << 8;
	PutMemB(DS, addr, I.regs.b[AL]);
	nec_ICount-=9;
}

static void i_mov_dispax(void)    /* Opcode 0xa3 */
{
    unsigned addr;
	addr = FETCH;
	addr += FETCH << 8;
	PutMemB(DS, addr, I.regs.b[AL]);
	PutMemB(DS, addr+1, I.regs.b[AH]);
	nec_ICount-=13;
}

static void i_movsb(void)    /* Opcode 0xa4 */
{
	BYTE tmp = GetMemB(DS,I.regs.w[IX]);
	PutMemB(ES,I.regs.w[IY], tmp);
	I.regs.w[IY] += -2 * I.DF + 1;
	I.regs.w[IX] += -2 * I.DF + 1;
	nec_ICount-=19;	// 11+8n
}

static void i_movsw(void)    /* Opcode 0xa5 */
{
	WORD tmp = GetMemW(DS,I.regs.w[IX]);
	PutMemW(ES,I.regs.w[IY], tmp);
	I.regs.w[IY] += -4 * I.DF + 2;
	I.regs.w[IX] += -4 * I.DF + 2;
	nec_ICount-=19; // 11+8n
}

static void i_cmpsb(void)    /* Opcode 0xa6 */
{
	unsigned dst = GetMemB(ES, I.regs.w[IY]);
	unsigned src = GetMemB(DS, I.regs.w[IX]);
    SUBB(src,dst); /* opposite of the usual convention */
	I.regs.w[IY] += -2 * I.DF + 1;
	I.regs.w[IX] += -2 * I.DF + 1;
	nec_ICount-=14;
}

static void i_cmpsw(void)    /* Opcode 0xa7 */
{
	unsigned dst = GetMemW(ES, I.regs.w[IY]);
	unsigned src = GetMemW(DS, I.regs.w[IX]);
    SUBW(src,dst); /* opposite of the usual convention */
	I.regs.w[IY] += -4 * I.DF + 2;
	I.regs.w[IX] += -4 * I.DF + 2;
	nec_ICount-=14;
}

static void i_test_ald8(void)    /* Opcode 0xa8 */
{
    DEF_ald8(dst,src);
    ANDB(dst,src);
    nec_ICount-=4;
}

static void i_test_axd16(void)    /* Opcode 0xa9 */
{
    DEF_axd16(dst,src);
    ANDW(dst,src);
    nec_ICount-=4;
}

static void i_stosb(void)    /* Opcode 0xaa */
{
	PutMemB(ES,I.regs.w[IY],I.regs.b[AL]);
	I.regs.w[IY] += -2 * I.DF + 1;
	nec_ICount-=5;
}

static void i_stosw(void)    /* Opcode 0xab */
{
	PutMemW(ES,I.regs.w[IY],I.regs.w[AW]);
//	PutMemB(ES,I.regs.w[IY],I.regs.b[AL]); /* MISH */
//	PutMemB(ES,I.regs.w[IY]+1,I.regs.b[AH]);
	I.regs.w[IY] += -4 * I.DF + 2;
	nec_ICount-=5;
}

static void i_lodsb(void)    /* Opcode 0xac */
{
	I.regs.b[AL] = GetMemB(DS,I.regs.w[IX]);
	I.regs.w[IX] += -2 * I.DF + 1;
	nec_ICount-=10;
}

static void i_lodsw(void)    /* Opcode 0xad */
{
	I.regs.w[AW] = GetMemW(DS,I.regs.w[IX]);
	I.regs.w[IX] +=  -4 * I.DF + 2;
	nec_ICount-=10;
}

static void i_scasb(void)    /* Opcode 0xae */
{
	unsigned src = GetMemB(ES, I.regs.w[IY]);
	unsigned dst = I.regs.b[AL];
    SUBB(dst,src);
	I.regs.w[IY] += -2 * I.DF + 1;
	nec_ICount-=12;
}

static void i_scasw(void)    /* Opcode 0xaf */
{
	unsigned src = GetMemW(ES, I.regs.w[IY]);
	unsigned dst = I.regs.w[AW];
    SUBW(dst,src);
	I.regs.w[IY] += -4 * I.DF + 2;
	nec_ICount-=12;
}

static void i_mov_ald8(void)    /* Opcode 0xb0 */
{
	I.regs.b[AL] = FETCH;
	nec_ICount-=4;
}

static void i_mov_cld8(void)    /* Opcode 0xb1 */
{
	I.regs.b[CL] = FETCH;
	nec_ICount-=4;
}

static void i_mov_dld8(void)    /* Opcode 0xb2 */
{
	I.regs.b[DL] = FETCH;
	nec_ICount-=4;
}

static void i_mov_bld8(void)    /* Opcode 0xb3 */
{
	I.regs.b[BL] = FETCH;
	nec_ICount-=4;
}

static void i_mov_ahd8(void)    /* Opcode 0xb4 */
{
	I.regs.b[AH] = FETCH;
	nec_ICount-=4;
}

static void i_mov_chd8(void)    /* Opcode 0xb5 */
{
	I.regs.b[CH] = FETCH;
	nec_ICount-=4;
}

static void i_mov_dhd8(void)    /* Opcode 0xb6 */
{
	I.regs.b[DH] = FETCH;
	nec_ICount-=4;
}

static void i_mov_bhd8(void)    /* Opcode 0xb7 */
{
	I.regs.b[BH] = FETCH;
	nec_ICount-=4;
}

static void i_mov_axd16(void)    /* Opcode 0xb8 */
{
	I.regs.b[AL] = FETCH;
	I.regs.b[AH] = FETCH;
	nec_ICount-=4;
}

static void i_mov_cxd16(void)    /* Opcode 0xb9 */
{
	I.regs.b[CL] = FETCH;
	I.regs.b[CH] = FETCH;
	nec_ICount-=4;
}

static void i_mov_dxd16(void)    /* Opcode 0xba */
{
	I.regs.b[DL] = FETCH;
	I.regs.b[DH] = FETCH;
	nec_ICount-=4;
}

static void i_mov_bxd16(void)    /* Opcode 0xbb */
{
	I.regs.b[BL] = FETCH;
	I.regs.b[BH] = FETCH;
	nec_ICount-=4;
}

static void i_mov_spd16(void)    /* Opcode 0xbc */
{
	I.regs.b[SPL] = FETCH;
	I.regs.b[SPH] = FETCH;
	nec_ICount-=4;
}

static void i_mov_bpd16(void)    /* Opcode 0xbd */
{
	I.regs.b[BPL] = FETCH;
	I.regs.b[BPH] = FETCH;
	nec_ICount-=4;
}

static void i_mov_sid16(void)    /* Opcode 0xbe */
{
	I.regs.b[IXL] = FETCH;
	I.regs.b[IXH] = FETCH;
	nec_ICount-=4;
}

static void i_mov_did16(void)    /* Opcode 0xbf */
{
	I.regs.b[IYL] = FETCH;
	I.regs.b[IYH] = FETCH;
	nec_ICount-=4;
}

void nec_rotate_shift_Byte(unsigned ModRM, int count)
{
  unsigned src = (unsigned)GetRMByte(ModRM);
  unsigned dst=src;

	if (count < 0) /* FETCH must come _after_ GetRMWord */
		count = FETCH;

  if (count==0)
  {
	nec_ICount-=8; /* or 7 if dest is in memory */
  }
  else if (count==1)
  {
    nec_ICount-=(ModRM >=0xc0 )?2:16;
    switch (ModRM & 0x38)
    {
      case 0x00:  /* ROL eb,1 */
	I.CarryVal = src & 0x80;
        dst=(src<<1)+CF;
        PutbackRMByte(ModRM,dst);
	I.OverVal = (src^dst)&0x80;
	break;
      case 0x08:  /* ROR eb,1 */
	I.CarryVal = src & 0x01;
        dst = ((CF<<8)+src) >> 1;
        PutbackRMByte(ModRM,dst);
	I.OverVal = (src^dst)&0x80;
	break;
      case 0x10:  /* RCL eb,1 */
        dst=(src<<1)+CF;
        PutbackRMByte(ModRM,dst);
        SetCFB(dst);
	I.OverVal = (src^dst)&0x80;
	break;
      case 0x18:  /* RCR eb,1 */
        dst = ((CF<<8)+src) >> 1;
        PutbackRMByte(ModRM,dst);
	I.CarryVal = src & 0x01;
	I.OverVal = (src^dst)&0x80;
	break;
      case 0x20:  /* SHL eb,1 */
      case 0x30:
        dst = src << 1;
        PutbackRMByte(ModRM,dst);
        SetCFB(dst);
	I.OverVal = (src^dst)&0x80;
	I.AuxVal = 1;
        SetSZPF_Byte(dst);
	break;
      case 0x28:  /* SHR eb,1 */
        dst = src >> 1;
        PutbackRMByte(ModRM,dst);
	I.CarryVal = src & 0x01;
	I.OverVal = src & 0x80;
	I.AuxVal = 1;
        SetSZPF_Byte(dst);
	break;
      case 0x38:  /* SAR eb,1 */
        dst = ((INT8)src) >> 1;
        PutbackRMByte(ModRM,dst);
	I.CarryVal = src & 0x01;
	I.OverVal = 0;
	I.AuxVal = 1;
        SetSZPF_Byte(dst);
	break;
    }
  }
  else
  {
    nec_ICount-=(ModRM >=0xc0 )?7+4*count:19+4*count;
    switch (ModRM & 0x38)
    {
      case 0x00:  /* ROL eb,count */
	for (; count > 0; count--)
	{
	  I.CarryVal = dst & 0x80;
          dst = (dst << 1) + CF;
	}
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
     case 0x08:  /* ROR eb,count */
	for (; count > 0; count--)
	{
	  I.CarryVal = dst & 0x01;
          dst = (dst >> 1) + (CF << 7);
	}
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
      case 0x10:  /* RCL eb,count */
	for (; count > 0; count--)
	{
          dst = (dst << 1) + CF;
          SetCFB(dst);
	}
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
      case 0x18:  /* RCR eb,count */
	for (; count > 0; count--)
	{
          dst = (CF<<8)+dst;
	  I.CarryVal = dst & 0x01;
          dst >>= 1;
	}
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
      case 0x20:
      case 0x30:  /* SHL eb,count */
        dst <<= count;
        SetCFB(dst);
	I.AuxVal = 1;
        SetSZPF_Byte(dst);
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
      case 0x28:  /* SHR eb,count */
        dst >>= count-1;
	I.CarryVal = dst & 0x1;
        dst >>= 1;
        SetSZPF_Byte(dst);
	I.AuxVal = 1;
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
      case 0x38:  /* SAR eb,count */
        dst = ((INT8)dst) >> (count-1);
	I.CarryVal = dst & 0x1;
        dst = ((INT8)((BYTE)dst)) >> 1;
        SetSZPF_Byte(dst);
	I.AuxVal = 1;
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
    }
  }
}


void nec_rotate_shift_Word(unsigned ModRM, int count)
{
	unsigned src = GetRMWord(ModRM);
	unsigned dst=src;

	if (count < 0) /* FETCH must come _after_ GetRMWord */
		count = FETCH;

  if (count==0)
  {
	nec_ICount-=8; /* or 7 if dest is in memory */
  }
  else if (count==1)
  {
    nec_ICount-=(ModRM >=0xc0 )?2:24;
    switch (ModRM & 0x38)
    {
#if 0
      case 0x00:  /* ROL ew,1 */
        tmp2 = (tmp << 1) + CF;
	SetCFW(tmp2);
	I.OverVal = !(!(tmp & 0x4000)) != CF;
	PutbackRMWord(ModRM,tmp2);
	break;
      case 0x08:  /* ROR ew,1 */
	I.CarryVal = tmp & 0x01;
	tmp2 = (tmp >> 1) + ((unsigned)CF << 15);
	I.OverVal = !(!(tmp & 0x8000)) != CF;
	PutbackRMWord(ModRM,tmp2);
	break;
      case 0x10:  /* RCL ew,1 */
	tmp2 = (tmp << 1) + CF;
	SetCFW(tmp2);
	I.OverVal = (tmp ^ (tmp << 1)) & 0x8000;
	PutbackRMWord(ModRM,tmp2);
	break;
      case 0x18:  /* RCR ew,1 */
	tmp2 = (tmp >> 1) + ((unsigned)CF << 15);
	I.OverVal = !(!(tmp & 0x8000)) != CF;
	I.CarryVal = tmp & 0x01;
	PutbackRMWord(ModRM,tmp2);
	break;
      case 0x20:  /* SHL ew,1 */
      case 0x30:
	tmp <<= 1;
	SetCFW(tmp);
	SetOFW_Add(tmp,tmp2,tmp2);
	I.AuxVal = 1;
	SetSZPF_Word(tmp);
	PutbackRMWord(ModRM,tmp);
	break;
      case 0x28:  /* SHR ew,1 */
	I.CarryVal = tmp & 0x01;
	I.OverVal = tmp & 0x8000;
	tmp2 = tmp >> 1;
	SetSZPF_Word(tmp2);
	I.AuxVal = 1;
	PutbackRMWord(ModRM,tmp2);
	break;
      case 0x38:  /* SAR ew,1 */
	I.CarryVal = tmp & 0x01;
	I.OverVal = 0;
	tmp2 = (tmp >> 1) | (tmp & 0x8000);
	SetSZPF_Word(tmp2);
	I.AuxVal = 1;
	PutbackRMWord(ModRM,tmp2);
	break;
#else
      case 0x00:  /* ROL ew,1 */
	I.CarryVal = src & 0x8000;
        dst=(src<<1)+CF;
        PutbackRMWord(ModRM,dst);
	I.OverVal = (src^dst)&0x8000;
	break;
      case 0x08:  /* ROR ew,1 */
	I.CarryVal = src & 0x01;
        dst = ((CF<<16)+src) >> 1;
        PutbackRMWord(ModRM,dst);
	I.OverVal = (src^dst)&0x8000;
	break;
      case 0x10:  /* RCL ew,1 */
        dst=(src<<1)+CF;
        PutbackRMWord(ModRM,dst);
        SetCFW(dst);
	I.OverVal = (src^dst)&0x8000;
	break;
      case 0x18:  /* RCR ew,1 */
        dst = ((CF<<16)+src) >> 1;
        PutbackRMWord(ModRM,dst);
	I.CarryVal = src & 0x01;
	I.OverVal = (src^dst)&0x8000;
	break;
      case 0x20:  /* SHL ew,1 */
      case 0x30:
        dst = src << 1;
        PutbackRMWord(ModRM,dst);
        SetCFW(dst);
	I.OverVal = (src^dst)&0x8000;
	I.AuxVal = 1;
        SetSZPF_Word(dst);
	break;
      case 0x28:  /* SHR ew,1 */
        dst = src >> 1;
        PutbackRMWord(ModRM,dst);
	I.CarryVal = src & 0x01;
	I.OverVal = src & 0x8000;
	I.AuxVal = 1;
        SetSZPF_Word(dst);
	break;
      case 0x38:  /* SAR ew,1 */
        dst = ((INT16)src) >> 1;
        PutbackRMWord(ModRM,dst);
	I.CarryVal = src & 0x01;
	I.OverVal = 0;
	I.AuxVal = 1;
        SetSZPF_Word(dst);
	break;
#endif
    }
  }
  else
  {

    nec_ICount-=(ModRM >=0xc0 )?7+count*4:27+count*4;
    switch (ModRM & 0x38)
    {
      case 0x00:  /* ROL ew,count */
     	for (; count > 0; count--)
	{
	  I.CarryVal = dst & 0x8000;
          dst = (dst << 1) + CF;
	}
        PutbackRMWord(ModRM,dst);
	break;
      case 0x08:  /* ROR ew,count */
	for (; count > 0; count--)
	{
	  I.CarryVal = dst & 0x01;
          dst = (dst >> 1) + (CF << 15);
	}
        PutbackRMWord(ModRM,dst);
	break;
      case 0x10:  /* RCL ew,count */
	for (; count > 0; count--)
	{
          dst = (dst << 1) + CF;
          SetCFW(dst);
	}
        PutbackRMWord(ModRM,dst);
	break;
      case 0x18:  /* RCR ew,count */
	for (; count > 0; count--)
	{
          dst = dst + (CF << 16);
	  I.CarryVal = dst & 0x01;
           dst >>= 1;
	}
        PutbackRMWord(ModRM,dst);
	break;
      case 0x20:
      case 0x30:  /* SHL ew,count */
        dst <<= count;
        SetCFW(dst);
	I.AuxVal = 1;
        SetSZPF_Word(dst);
        PutbackRMWord(ModRM,dst);
	break;
      case 0x28:  /* SHR ew,count */
        dst >>= count-1;
	I.CarryVal = dst & 0x1;
        dst >>= 1;
        SetSZPF_Word(dst);
	I.AuxVal = 1;
        PutbackRMWord(ModRM,dst);
	break;
      case 0x38:  /* SAR ew,count */
        dst = ((INT16)dst) >> (count-1);
	I.CarryVal = dst & 0x01;
        dst = ((INT16)((WORD)dst)) >> 1;
        SetSZPF_Word(dst);
	I.AuxVal = 1;
        PutbackRMWord(ModRM,dst);
	break;
    }
  }
}


static void i_rotshft_bd8(void)    /* Opcode 0xc0 */
{
    unsigned ModRM = FETCH;
    nec_rotate_shift_Byte(ModRM,-1);
}

static void i_rotshft_wd8(void)    /* Opcode 0xc1 */
{
    unsigned ModRM = FETCH;
    nec_rotate_shift_Word(ModRM,-1);
}


static void i_ret_d16(void)    /* Opcode 0xc2 */
{
	unsigned count = FETCH;
	count += FETCH << 8;
	POP(I.ip);
	I.regs.w[SP]+=count;
	change_pc20((I.base[CS]+I.ip));
	nec_ICount-=22;	// near 20-24
}

static void i_ret(void)    /* Opcode 0xc3 */
{
	POP(I.ip);
	change_pc20((I.base[CS]+I.ip));
	nec_ICount-=17; // near 15-19
}

static void i_les_dw(void)    /* Opcode 0xc4 */
{
	unsigned ModRM = FETCH;
    WORD tmp = GetRMWord(ModRM);

    RegWord(ModRM)= tmp;
    I.sregs[ES] = GetnextRMWord;
    I.base[ES] = SegBase(ES);
    nec_ICount-=22;   /* 18-26 */
}

static void i_lds_dw(void)    /* Opcode 0xc5 */
{
	unsigned ModRM = FETCH;
    WORD tmp = GetRMWord(ModRM);

    RegWord(ModRM)=tmp;
    I.sregs[DS] = GetnextRMWord;
    I.base[DS] = SegBase(DS);
    nec_ICount-=22;   /* 18-26 */
}

static void i_mov_bd8(void)    /* Opcode 0xc6 */
{
    unsigned ModRM = FETCH;
    PutImmRMByte(ModRM);
    nec_ICount-=(ModRM >=0xc0 )?4:11;
}

static void i_mov_wd16(void)    /* Opcode 0xc7 */
{
    unsigned ModRM = FETCH;
    PutImmRMWord(ModRM);
    nec_ICount-=(ModRM >=0xc0 )?4:15;
}

static void i_enter(void)    /* Opcode 0xc8 */
{
    unsigned nb = FETCH;
    unsigned i,level;

    nec_ICount-=23;
    nb += FETCH << 8;
    level = FETCH;
    PUSH(I.regs.w[BP]);
    I.regs.w[BP]=I.regs.w[SP];
    I.regs.w[SP] -= nb;
    for (i=1;i<level;i++) {
	PUSH(GetMemW(SS,I.regs.w[BP]-i*2));
	nec_ICount-=16;
    }
    if (level) PUSH(I.regs.w[BP]);
}

static void i_leave(void)    /* Opcode 0xc9 */
{
	I.regs.w[SP]=I.regs.w[BP];
	POP(I.regs.w[BP]);
	nec_ICount-=8;
}

static void i_retf_d16(void)    /* Opcode 0xca */
{
	unsigned count = FETCH;
	count += FETCH << 8;
	POP(I.ip);
	POP(I.sregs[CS]);
	I.base[CS] = SegBase(CS);
	I.regs.w[SP]+=count;
	change_pc20((I.base[CS]+I.ip));
	nec_ICount-=25; // 21-29
}

static void i_retf(void)    /* Opcode 0xcb */
{
	POP(I.ip);
	POP(I.sregs[CS]);
	I.base[CS] = SegBase(CS);
	change_pc20((I.base[CS]+I.ip));
	nec_ICount-=28;	// 24-32
}

static void i_int3(void)    /* Opcode 0xcc */
{
	nec_ICount-=38;	// 38-50
	nec_interrupt(3,0);
}

static void i_int(void)    /* Opcode 0xcd */
{
	unsigned int_num = FETCH;
	nec_ICount-=38;	// 38-50
	nec_interrupt(int_num,0);
}

static void i_into(void)    /* Opcode 0xce */
{
    if (OF) {
	nec_ICount-=52;
	nec_interrupt(4,0);
    } else nec_ICount-=3;   /* 3 or 52! */
}

static void i_iret(void)    /* Opcode 0xcf */
{
	POP(I.ip);
	POP(I.sregs[CS]);
	I.base[CS] = SegBase(CS);
    	i_popf();
	change_pc20((I.base[CS]+I.ip));
	nec_ICount-=32;	// 27-39
}

static void i_rotshft_b(void)    /* Opcode 0xd0 */
{
	nec_rotate_shift_Byte(FETCH,1);
}


static void i_rotshft_w(void)    /* Opcode 0xd1 */
{
	nec_rotate_shift_Word(FETCH,1);
}


static void i_rotshft_bcl(void)    /* Opcode 0xd2 */
{
	nec_rotate_shift_Byte(FETCH,I.regs.b[CL]);
}

static void i_rotshft_wcl(void)    /* Opcode 0xd3 */
{
	nec_rotate_shift_Word(FETCH,I.regs.b[CL]);
}

/* OB: Opcode works on NEC V-Series but not the Variants 		*/
/*     one could specify any byte value as operand but the NECs */
/*     always substitute 0x0a.									*/
static void i_aam(void)   	/* Opcode 0xd4 */
{
	unsigned mult=FETCH;

	if (mult == 0)
		nec_interrupt(0,0);
    else
    {
		I.regs.b[AH] = I.regs.b[AL] / 10;
		I.regs.b[AL] %= 10;
		SetSZPF_Word(I.regs.w[AW]);
		nec_ICount-=15;
    }
}

/* OB: Opcode works on NEC V-Series but not the Variants 	*/
/*     one could specify any byte value as operand but the NECs */
/*     always substitute 0x0a.					*/
static void i_aad(void)    	/* Opcode 0xd5 */
{
	unsigned mult=FETCH;				/* eat operand = ignore ! */

	I.regs.b[AL] = I.regs.b[AH] * 10 + I.regs.b[AL];
	I.regs.b[AH] = 0;

	SetZF(I.regs.b[AL]);
	SetPF(I.regs.b[AL]);
	I.SignVal = 0;
	nec_ICount-=7;
	mult=0;
}

static void i_setalc(void)  /* Opcode 0xd6 */
{
	/*
	----------O-SETALC---------------------------------
	OPCODE SETALC  - Set AL to Carry Flag

	CPU:  Intel 8086 and all its clones and upward
	    compatibility chips.
	Type of Instruction: User

	Instruction: SETALC

	Description:

		IF (CF=0) THEN AL:=0 ELSE AL:=FFH;

	Flags Affected: None

	CPU mode: RM,PM,VM,SMM

	Physical Form:		 SETALC
	COP (Code of Operation): D6H
	Clocks:	      80286    : n/a   [3]
		      80386    : n/a   [3]
		     Cx486SLC  : n/a   [2]
		      i486     : n/a   [3]
		      Pentium  : n/a   [3]
	Note: n/a is Time that Intel etc not say.
	      [3] is real time it executed.

	*/
	I.regs.b[AL] = (CF)?0xff:0x00;
	nec_ICount-=3;	// V30
	logerror("PC=%06x : SETALC\n",cpu_get_pc()-1);
}

static void i_xlat(void)    /* Opcode 0xd7 */
{
	unsigned dest = I.regs.w[BW]+I.regs.b[AL];
	I.regs.b[AL] = GetMemB(DS, dest);
	nec_ICount-=9;	// V30
}

static void i_escape(void)    /* Opcodes 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde and 0xdf */
{
	unsigned ModRM = FETCH;
	nec_ICount-=2;	// dont found any info :-(, set same as hlt
    	GetRMByte(ModRM);
}

static void i_loopne(void)    /* Opcode 0xe0 */
{
    int disp = (int)((INT8)FETCH);
    unsigned tmp = I.regs.w[CW]-1;

    I.regs.w[CW]=tmp;

    if (!ZF && tmp) {
	nec_ICount-=14;
	I.ip = (WORD)(I.ip+disp);
	change_pc20((I.base[CS]+I.ip));
    } else nec_ICount-=5;
}

static void i_loope(void)    /* Opcode 0xe1 */
{
    int disp = (int)((INT8)FETCH);
    unsigned tmp = I.regs.w[CW]-1;
    I.regs.w[CW]=tmp;

    if (ZF && tmp) {
	nec_ICount-=14;
	I.ip = (WORD)(I.ip+disp);
	change_pc20((I.base[CS]+I.ip));
   } else nec_ICount-=5;
}

static void i_loop(void)    /* Opcode 0xe2 */
{
	int disp = (int)((INT8)FETCH);
	unsigned tmp = I.regs.w[CW]-1;

	I.regs.w[CW]=tmp;

    if (tmp) {
	nec_ICount-=13;
	I.ip = (WORD)(I.ip+disp);
	change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=5;
}

static void i_jcxz(void)    /* Opcode 0xe3 */
{
	int disp = (int)((INT8)FETCH);
	if (I.regs.w[CW] == 0) {
	nec_ICount-=13;
	I.ip = (WORD)(I.ip+disp);
	change_pc20((I.base[CS]+I.ip));
	} else nec_ICount-=5;
}

static void i_inal(void)    /* Opcode 0xe4 */
{
	unsigned port = FETCH;
	I.regs.b[AL] = read_port(port);
	nec_ICount-=9;
}

static void i_inax(void)    /* Opcode 0xe5 */
{
	unsigned port = FETCH;
	I.regs.b[AL] = read_port(port);
	I.regs.b[AH] = read_port(port+1);
	nec_ICount-=13;
}

static void i_outal(void)    /* Opcode 0xe6 */
{
	unsigned port = FETCH;
	write_port(port, I.regs.b[AL]);
	nec_ICount-=8;
}

static void i_outax(void)    /* Opcode 0xe7 */
{
	unsigned port = FETCH;
	write_port(port, I.regs.b[AL]);
	write_port(port+1, I.regs.b[AH]);
	nec_ICount-=12;
}

static void i_call_d16(void)    /* Opcode 0xe8 */
{
	unsigned tmp = FETCH;
	tmp += FETCH << 8;

	PUSH(I.ip);
	I.ip = (WORD)(I.ip+(INT16)tmp);
	change_pc20((I.base[CS]+I.ip));
	nec_ICount-=24; // 21-29
}


static void i_jmp_d16(void)    /* Opcode 0xe9 */
{
	int tmp = FETCH;
	tmp += FETCH << 8;

	I.ip = (WORD)(I.ip+(INT16)tmp);
	change_pc20((I.base[CS]+I.ip));
	nec_ICount-=15;
}

static void i_jmp_far(void)    /* Opcode 0xea */
{
    unsigned tmp,tmp1;

	tmp = FETCH;
	tmp += FETCH << 8;

	tmp1 = FETCH;
	tmp1 += FETCH << 8;

	I.sregs[CS] = (WORD)tmp1;
	I.base[CS] = SegBase(CS);
	I.ip = (WORD)tmp;
	change_pc20((I.base[CS]+I.ip));
	nec_ICount-=27; // 27-35
}

static void i_jmp_d8(void)    /* Opcode 0xeb */
{
	int tmp = (int)((INT8)FETCH);
	I.ip = (WORD)(I.ip+tmp);
	nec_ICount-=12;
}

static void i_inaldx(void)    /* Opcode 0xec */
{
	I.regs.b[AL] = read_port(I.regs.w[DW]);
	nec_ICount-=8;
}

static void i_inaxdx(void)    /* Opcode 0xed */
{
	unsigned port = I.regs.w[DW];

	I.regs.b[AL] = read_port(port);
	I.regs.b[AH] = read_port(port+1);
	nec_ICount-=12;
}

static void i_outdxal(void)    /* Opcode 0xee */
{
	write_port(I.regs.w[DW], I.regs.b[AL]);
	nec_ICount-=8;
}

static void i_outdxax(void)    /* Opcode 0xef */
{
	unsigned port = I.regs.w[DW];
	write_port(port, I.regs.b[AL]);
	write_port(port+1, I.regs.b[AH]);
	nec_ICount-=12;
}

/* I think thats not a V20 instruction...*/
static void i_lock(void)    /* Opcode 0xf0 */
{
	nec_ICount-=2;
	nec_instruction[FETCHOP]();  /* un-interruptible */
}


static void i_brks(void) 	/* Opcode 0xf1 - Break to Security Mode */
{
	/*
	CPU:  NEC (V25/V35) Software Guard  only
	Instruction:  BRKS int_vector

	Description:
		     [sp-1,sp-2] <- PSW		; PSW EQU FLAGS
		     [sp-3,sp-4] <- PS		; PS  EQU CS
		     [sp-5,sp-6] <- PC		; PC  EQU IP
		     SP	 <-  SP -6
		     IE	 <-  0
		     BRK <-  0
		     MD	 <-  0
		     PC	 <- [int_vector*4 +0,+1]
		     PS	 <- [int_vector*4 +2,+3]

	Note:	The BRKS instruction switches operations in Security Mode
		via Interrupt call. In Security Mode the fetched operation
		code is executed after conversion in accordance with build-in
		translation table

	Flags Affected:	 None

	CPU mode: RM

	+++++++++++++++++++++++
	Physical Form:	BRKS  imm8
	Clocks:	 56+10T [44+10T]
*/
	unsigned int_vector;
	int_vector=FETCH;
	logerror("PC=%06x : BRKS %02x\n",cpu_get_pc()-2,int_vector);
}


static void rep(int flagval)
{
    /* Handles rep- and repnz- prefixes. flagval is the value of ZF for the
       loop  to continue for CMPS and SCAS instructions. */

	unsigned next = FETCHOP;
	unsigned count = I.regs.w[CW];

    switch(next)
    {
    	case 0x26:  /* ES: */
			I.seg_prefix=TRUE;
			I.prefix_base=I.base[ES];
			nec_ICount-=2;
			rep(flagval);
			break;
	    case 0x2e:  /* CS: */
			I.seg_prefix=TRUE;
			I.prefix_base=I.base[CS];
			nec_ICount-=2;
			rep(flagval);
			break;
    case 0x36:  /* SS: */
        I.seg_prefix=TRUE;
	I.prefix_base=I.base[SS];
	nec_ICount-=2;
	rep(flagval);
	break;
    case 0x3e:  /* DS: */
        I.seg_prefix=TRUE;
	I.prefix_base=I.base[DS];
	nec_ICount-=2;
	rep(flagval);
	break;
    case 0x6c:  /* REP INSB */
	nec_ICount-=9-count;
	for (; count > 0; count--)
        	i_insb();
	I.regs.w[CW]=count;
	break;
    case 0x6d:  /* REP INSW */
	nec_ICount-=9-count;
	for (; count > 0; count--)
        	i_insw();
	I.regs.w[CW]=count;
	break;
    case 0x6e:  /* REP OUTSB */
	nec_ICount-=9-count;
	for (; count > 0; count--)
           i_outsb();
	I.regs.w[CW]=count;
	break;
    case 0x6f:  /* REP OUTSW */
	nec_ICount-=9-count;
	for (; count > 0; count--)
            i_outsw();
	I.regs.w[CW]=count;
	break;
    case 0xa4:  /* REP MOVSB */
	nec_ICount-=9-count;
	for (; count > 0; count--)
		i_movsb();
	I.regs.w[CW]=count;
	break;
    case 0xa5:  /* REP MOVSW */
	nec_ICount-=9-count;
	for (; count > 0; count--)
		i_movsw();
	I.regs.w[CW]=count;
	break;
    case 0xa6:  /* REP(N)E CMPSB */
	nec_ICount-=9;
	for (I.ZeroVal = !flagval; (ZF == flagval) && (count > 0); count--)
		i_cmpsb();
	I.regs.w[CW]=count;
	break;
    case 0xa7:  /* REP(N)E CMPSW */
	nec_ICount-=9;
	for (I.ZeroVal = !flagval; (ZF == flagval) && (count > 0); count--)
		i_cmpsw();
	I.regs.w[CW]=count;
	break;
    case 0xaa:  /* REP STOSB */
	nec_ICount-=9-count;
	for (; count > 0; count--)
		i_stosb();
	I.regs.w[CW]=count;
	break;
    case 0xab:  /* REP STOSW */
	nec_ICount-=9-count;
	for (; count > 0; count--)
		i_stosw();
	I.regs.w[CW]=count;
	break;
    case 0xac:  /* REP LODSB */
	nec_ICount-=9;
	for (; count > 0; count--)
		i_lodsb();
	I.regs.w[CW]=count;
	break;
    case 0xad:  /* REP LODSW */
	nec_ICount-=9;
	for (; count > 0; count--)
		i_lodsw();
	I.regs.w[CW]=count;
	break;
    case 0xae:  /* REP(N)E SCASB */
	nec_ICount-=9;
	for (I.ZeroVal = !flagval; (ZF == flagval) && (count > 0); count--)
		i_scasb();
	I.regs.w[CW]=count;
	break;
    case 0xaf:  /* REP(N)E SCASW */
	nec_ICount-=9;
	for (I.ZeroVal = !flagval; (ZF == flagval) && (count > 0); count--)
		i_scasw();
	I.regs.w[CW]=count;
	break;
    default:
	nec_instruction[next]();
    }
}


static void i_repne(void)    /* Opcode 0xf2 */
{
    rep(0);
}

static void i_repe(void)    /* Opcode 0xf3 */
{
    rep(1);
}

static void i_hlt(void)    /* Opcode 0xf4 */
{
	nec_ICount=0;
}

static void i_cmc(void)    /* Opcode 0xf5 */
{
	I.CarryVal = !CF;
	nec_ICount-=2;
}

static void i_f6pre(void)
{
	/* Opcode 0xf6 */
	unsigned ModRM = FETCH;
    unsigned tmp = (unsigned)GetRMByte(ModRM);
    unsigned tmp2;


    switch (ModRM & 0x38)
    {
    case 0x00:  /* TEST Eb, data8 */
    case 0x08:  /* ??? */
		tmp &= FETCH;
		I.CarryVal = I.OverVal = I.AuxVal = 0;
		SetSZPF_Byte(tmp);
		nec_ICount-=(ModRM >=0xc0 )?4:11;
		break;

    case 0x10:  /* NOT Eb */
		PutbackRMByte(ModRM,~tmp);
		nec_ICount-=(ModRM >=0xc0 )?2:16;
		break;
    case 0x18:  /* NEG Eb */
        	tmp2=0;
        	SUBB(tmp2,tmp);
        	PutbackRMByte(ModRM,tmp2);
        	nec_ICount-=(ModRM >=0xc0 )?2:16;
		break;
    case 0x20:  /* MUL AL, Eb */
		{
			UINT16 result;
			tmp2 = I.regs.b[AL];

			SetSF((INT8)tmp2);
			SetPF(tmp2);

			result = (UINT16)tmp2*tmp;
			I.regs.w[AW]=(WORD)result;

			SetZF(I.regs.w[AW]);
			I.CarryVal = I.OverVal = (I.regs.b[AH] != 0);
		}
		nec_ICount-=(ModRM >=0xc0 )?30:36;
		break;
    case 0x28:  /* IMUL AL, Eb */
		{
			INT16 result;

			tmp2 = (unsigned)I.regs.b[AL];

			SetSF((INT8)tmp2);
			SetPF(tmp2);

			result = (INT16)((INT8)tmp2)*(INT16)((INT8)tmp);
			I.regs.w[AW]=(WORD)result;

			SetZF(I.regs.w[AW]);

			I.CarryVal = I.OverVal = (result >> 7 != 0) && (result >> 7 != -1);
		}
		nec_ICount-=(ModRM >=0xc0 )?30:39;
		break;
    case 0x30:  /* IYV AL, Ew */
		{
			UINT16 result;

			result = I.regs.w[AW];

			if (tmp)
			{
				tmp2 = result % tmp;

				if ((result /= tmp) > 0xff)
				{
					nec_interrupt(0,0);
					break;
				}
				else
				{
					I.regs.b[AL] = result;
					I.regs.b[AH] = tmp2;
				}
			}
			else
			{
				nec_interrupt(0,0);
				break;
			}
		}
		nec_ICount-=(ModRM >=0xc0 )?25:35;
		break;
    case 0x38:  /* IIYV AL, Ew */
		{

			INT16 result;

			result = (INT16)I.regs.w[AW];

			if (tmp)
			{
				tmp2 = result % (INT16)((INT8)tmp);

				if ((result /= (INT16)((INT8)tmp)) > 0xff)
				{
					nec_interrupt(0,0);
					break;
				}
				else
				{
					I.regs.b[AL] = result;
					I.regs.b[AH] = tmp2;
				}
			}
			else
			{
				nec_interrupt(0,0);
				break;
			}
		}
		nec_ICount-=(ModRM >=0xc0 )?43:53;
		break;
    }
}


static void i_f7pre(void)
{
	/* Opcode 0xf7 */
	unsigned ModRM = FETCH;
    unsigned tmp = GetRMWord(ModRM);
    unsigned tmp2;


    switch (ModRM & 0x38)
    {
    case 0x00:  /* TEST Ew, data16 */
    case 0x08:  /* ??? */
		tmp2 = FETCH;
		tmp2 += FETCH << 8;

		tmp &= tmp2;

		I.CarryVal = I.OverVal = I.AuxVal = 0;
		SetSZPF_Word(tmp);
		nec_ICount-=(ModRM >=0xc0 )?4:15;
		break;
    case 0x10:  /* NOT Ew */
		tmp = ~tmp;
		PutbackRMWord(ModRM,tmp);
		nec_ICount-=(ModRM >=0xc0 )?2:24;
		break;

    case 0x18:  /* NEG Ew */
        tmp2 = 0;
        SUBW(tmp2,tmp);
        PutbackRMWord(ModRM,tmp2);
	nec_ICount-=(ModRM >=0xc0 )?2:24;
	break;
    case 0x20:  /* MUL AW, Ew */
		{
			UINT32 result;
			tmp2 = I.regs.w[AW];

			SetSF((INT16)tmp2);
			SetPF(tmp2);

			result = (UINT32)tmp2*tmp;
			I.regs.w[AW]=(WORD)result;
            result >>= 16;
			I.regs.w[DW]=result;

			SetZF(I.regs.w[AW] | I.regs.w[DW]);
			I.CarryVal = I.OverVal = (I.regs.w[DW] != 0);
		}
		nec_ICount-=(ModRM >=0xc0 )?30:36;
		break;

    case 0x28:  /* IMUL AW, Ew */
		nec_ICount-=150;
		{
			INT32 result;

			tmp2 = I.regs.w[AW];

			SetSF((INT16)tmp2);
			SetPF(tmp2);

			result = (INT32)((INT16)tmp2)*(INT32)((INT16)tmp);
			I.CarryVal = I.OverVal = (result >> 15 != 0) && (result >> 15 != -1);

			I.regs.w[AW]=(WORD)result;
			result = (WORD)(result >> 16);
			I.regs.w[DW]=result;

			SetZF(I.regs.w[AW] | I.regs.w[DW]);
		}
		nec_ICount-=(ModRM >=0xc0 )?34:44;
		break;
    case 0x30:  /* IYV AW, Ew */
		{
			UINT32 result;

            result = (((UINT32)I.regs.w[DW]) << 16) | I.regs.w[AW];

			if (tmp)
			{
				tmp2 = result % tmp;
				if ((result /= tmp) > 0xffff)
				{
					nec_interrupt(0,0);
					break;
				}
				else
				{
                    I.regs.w[AW]=result;
                    I.regs.w[DW]=tmp2;
				}
			}
			else
			{
				nec_interrupt(0,0);
				break;
			}
		}
		nec_ICount-=(ModRM >=0xc0 )?25:35;
		break;
    case 0x38:  /* IIYV AW, Ew */
		{
			INT32 result;

			result = ((UINT32)I.regs.w[DW] << 16) + I.regs.w[AW];

			if (tmp)
			{
				tmp2 = result % (INT32)((INT16)tmp);
				if ((result /= (INT32)((INT16)tmp)) > 0xffff)
				{
					nec_interrupt(0,0);
					break;
				}
				else
				{
					I.regs.w[AW]=result;
					I.regs.w[DW]=tmp2;
				}
			}
			else
			{
				nec_interrupt(0,0);
				break;
			}
		}
		nec_ICount-=(ModRM >=0xc0 )?43:53;
		break;
    }
}


static void i_clc(void)    /* Opcode 0xf8 */
{
	I.CarryVal = 0;
	nec_ICount-=2;
}

static void i_stc(void)    /* Opcode 0xf9 */
{
	I.CarryVal = 1;
	nec_ICount-=2;
}

static void i_di(void)    /* Opcode 0xfa */
{
	SetIF(0);
	nec_ICount-=2;
}

static void i_ei(void)    /* Opcode 0xfb */
{
	SetIF(1);
	nec_ICount-=2;
}

static void i_cld(void)    /* Opcode 0xfc */
{
	SetDF(0);
	nec_ICount-=2;
}

static void i_std(void)    /* Opcode 0xfd */
{
	SetDF(1);
	nec_ICount-=2;
}

static void i_fepre(void)    /* Opcode 0xfe */
{
    unsigned ModRM = FETCH;
    unsigned tmp = GetRMByte(ModRM);
    unsigned tmp1;

    if ((ModRM & 0x38) == 0)  /* INC eb */
    {
		tmp1 = tmp+1;
		SetOFB_Add(tmp1,tmp,1);
    }
    else  /* DEC eb */
    {
		tmp1 = tmp-1;
		SetOFB_Sub(tmp1,1,tmp);
    }

    SetAF(tmp1,tmp,1);
    SetSZPF_Byte(tmp1);

    PutbackRMByte(ModRM,(BYTE)tmp1);
    nec_ICount-=(ModRM >=0xc0 )?2:16;
}


static void i_ffpre(void)    /* Opcode 0xff */
{
	unsigned ModRM = FETCH;
    unsigned tmp;
    unsigned tmp1;

    switch(ModRM & 0x38)
    {
    case 0x00:  /* INC ew */
		tmp = GetRMWord(ModRM);
		tmp1 = tmp+1;

		/*SetOFW_Add(tmp1,tmp,1);*/
        I.OverVal = (tmp==0x7fff); /* Mish */
		SetAF(tmp1,tmp,1);
		SetSZPF_Word(tmp1);

		PutbackRMWord(ModRM,(WORD)tmp1);
		nec_ICount-=(ModRM >=0xc0 )?2:24;
		break;

    case 0x08:  /* DEC ew */
		tmp = GetRMWord(ModRM);
		tmp1 = tmp-1;

		/*SetOFW_Sub(tmp1,1,tmp);*/
		I.OverVal = (tmp==0x8000);	 /* Mish */
		SetAF(tmp1,tmp,1);
		SetSZPF_Word(tmp1);

		PutbackRMWord(ModRM,(WORD)tmp1);
		nec_ICount-=(ModRM >=0xc0 )?2:24;
		break;

    case 0x10:  /* CALL ew */
		tmp = GetRMWord(ModRM);
		PUSH(I.ip);
		I.ip = (WORD)tmp;
		change_pc20((I.base[CS]+I.ip));
		nec_ICount-=(ModRM >=0xc0 )?16:20;
		break;

	case 0x18:  /* CALL FAR ea */
		tmp = I.sregs[CS];	/* HJB 12/13/98 need to skip displacements of EA */
		tmp1 = GetRMWord(ModRM);
		I.sregs[CS] = GetnextRMWord;
		I.base[CS] = SegBase(CS);
		PUSH(tmp);
		PUSH(I.ip);
		I.ip = tmp1;
		change_pc20((I.base[CS]+I.ip));
		nec_ICount-=(ModRM >=0xc0 )?16:26;
		break;

    case 0x20:  /* JMP ea */
		nec_ICount-=13;
		I.ip = GetRMWord(ModRM);
		change_pc20((I.base[CS]+I.ip));
		break;

    case 0x28:  /* JMP FAR ea */
		nec_ICount-=15;
		I.ip = GetRMWord(ModRM);
		I.sregs[CS] = GetnextRMWord;
		I.base[CS] = SegBase(CS);
		change_pc20((I.base[CS]+I.ip));
		break;

    case 0x30:  /* PUSH ea */
		nec_ICount-=4;
		tmp = GetRMWord(ModRM);
		PUSH(tmp);
		break;
    }
}


static void i_invalid(void)
{
    /* makes the cpu loops forever until user resets it */
	/*	{ extern int debug_key_pressed; debug_key_pressed = 1; } */
	I.ip--;
	nec_ICount-=10;
	logerror("PC=%06x : Invalid Opcode %02x\n",cpu_get_pc(),(BYTE)cpu_readop((I.base[CS]+I.ip)));
}

/* ASG 971222 -- added these interface functions */

unsigned nec_get_context(void *dst)
{
	if( dst )
		*(nec_Regs*)dst = I;
    return sizeof(nec_Regs);
}

void nec_set_context(void *src)
{
	if( src )
	{
		I = *(nec_Regs*)src;
		I.base[CS] = SegBase(CS);
		I.base[DS] = SegBase(DS);
		I.base[ES] = SegBase(ES);
		I.base[SS] = SegBase(SS);
		change_pc20((I.base[CS]+I.ip));
	}
}

unsigned nec_get_pc(void)
{
	return (I.base[CS] + (WORD)I.ip);
}

void nec_set_pc(unsigned val)
{
	if( val - I.base[CS] < 0x10000 )
	{
		I.ip = val - I.base[CS];
	}
	else
	{
		I.base[CS] = val & 0xffff0;
		I.sregs[CS] = I.base[CS] >> 4;
		I.ip = val & 0x0000f;
	}
}

unsigned nec_get_sp(void)
{
	return I.base[SS] + I.regs.w[SP];
}

void nec_set_sp(unsigned val)
{
	if( val - I.base[SS] < 0x10000 )
	{
		I.regs.w[SP] = val - I.base[SS];
	}
	else
	{
		I.base[SS] = val & 0xffff0;
		I.sregs[SS] = I.base[SS] >> 4;
		I.regs.w[SP] = val & 0x0000f;
	}
}

unsigned nec_get_reg(int regnum)
{
	switch( regnum )
	{
		case NEC_IP: return I.ip;
		case NEC_SP: return I.regs.w[SP];
		case NEC_FLAGS: CompressFlags(); return I.flags;
        case NEC_AW: return I.regs.w[AW];
		case NEC_CW: return I.regs.w[CW];
		case NEC_DW: return I.regs.w[DW];
		case NEC_BW: return I.regs.w[BW];
		case NEC_BP: return I.regs.w[BP];
		case NEC_IX: return I.regs.w[IX];
		case NEC_IY: return I.regs.w[IY];
		case NEC_ES: return I.sregs[ES];
		case NEC_CS: return I.sregs[CS];
		case NEC_SS: return I.sregs[SS];
		case NEC_DS: return I.sregs[DS];
		case NEC_VECTOR: return I.int_vector;
		case NEC_PENDING: return I.pending_irq;
		case NEC_NMI_STATE: return I.nmi_state;
		case NEC_IRQ_STATE: return I.irq_state;
		case REG_PREVIOUSPC: return 0;	/* not supported */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = ((I.base[SS] + I.regs.w[SP])) + 2 * (REG_SP_CONTENTS - regnum);
				return cpu_readmem20( offset ) | ( cpu_readmem20( offset + 1) << 8 );
			}
	}
	return 0;
}

void nec_set_nmi_line(int state);
void nec_set_irq_line(int irqline, int state);

void nec_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case NEC_IP: I.ip = val; break;
		case NEC_SP: I.regs.w[SP] = val; break;
		case NEC_FLAGS: I.flags = val; ExpandFlags(val); break;
        case NEC_AW: I.regs.w[AW] = val; break;
		case NEC_CW: I.regs.w[CW] = val; break;
		case NEC_DW: I.regs.w[DW] = val; break;
		case NEC_BW: I.regs.w[BW] = val; break;
		case NEC_BP: I.regs.w[BP] = val; break;
		case NEC_IX: I.regs.w[IX] = val; break;
		case NEC_IY: I.regs.w[IY] = val; break;
		case NEC_ES: I.sregs[ES] = val; break;
		case NEC_CS: I.sregs[CS] = val; break;
		case NEC_SS: I.sregs[SS] = val; break;
		case NEC_DS: I.sregs[DS] = val; break;
		case NEC_VECTOR: I.int_vector = val; break;
		case NEC_PENDING: I.pending_irq = val; break;
		case NEC_NMI_STATE: nec_set_nmi_line(val); break;
		case NEC_IRQ_STATE: nec_set_irq_line(0,val); break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = ((I.base[SS] + I.regs.w[SP])) + 2 * (REG_SP_CONTENTS - regnum);
				cpu_writemem20( offset, val & 0xff );
				cpu_writemem20( offset+1, (val >> 8) & 0xff );
			}
    }
}

void nec_set_nmi_line(int state)
{
	if( I.nmi_state == state ) return;
    I.nmi_state = state;
	if (state != CLEAR_LINE)
	{
		I.pending_irq |= NMI_IRQ;
	}
}

void nec_set_irq_line(int irqline, int state)
{
	I.irq_state = state;
	if (state == CLEAR_LINE)
	{
		if (!I.IF)
			I.pending_irq &= ~INT_IRQ;
	}
	else
	{
		if (I.IF)
			I.pending_irq |= INT_IRQ;
	}
}

void nec_set_irq_callback(int (*callback)(int))
{
	I.irq_callback = callback;
}

int nec_execute(int cycles)
{
	nec_ICount=cycles;	/* ASG 971222 cycles_per_run;*/
	while(nec_ICount>0)
    {

#ifdef VERBOSE_DEBUG
printf("[%04x:%04x]=%02x\tAW=%04x\tBW=%04x\tCW=%04x\tDW=%04x\n",sregs[CS],I.ip,GetMemB(CS,I.ip),I.regs.w[AW],I.regs.w[BW],I.regs.w[CW],I.regs.w[DW]);
#endif

	if ((I.pending_irq && I.IF) || (I.pending_irq & NMI_IRQ))
		external_int(); 	 /* HJB 12/15/98 */

	CALL_MAME_DEBUG;

	I.seg_prefix=FALSE;
#if defined(BIGCASE) && !defined(RS6000)
  /* Some compilers cannot handle large case statements */
	switch(FETCHOP)
	{
	case 0x00:    i_add_br8(); break;
	case 0x01:    i_add_wr16(); break;
	case 0x02:    i_add_r8b(); break;
	case 0x03:    i_add_r16w(); break;
	case 0x04:    i_add_ald8(); break;
	case 0x05:    i_add_axd16(); break;
	case 0x06:    i_push_es(); break;
	case 0x07:    i_pop_es(); break;
	case 0x08:    i_or_br8(); break;
	case 0x09:    i_or_wr16(); break;
	case 0x0a:    i_or_r8b(); break;
	case 0x0b:    i_or_r16w(); break;
	case 0x0c:    i_or_ald8(); break;
	case 0x0d:    i_or_axd16(); break;
	case 0x0e:    i_push_cs(); break;
	case 0x0f:    i_pre_nec(); break;
	case 0x10:    i_adc_br8(); break;
	case 0x11:    i_adc_wr16(); break;
	case 0x12:    i_adc_r8b(); break;
	case 0x13:    i_adc_r16w(); break;
	case 0x14:    i_adc_ald8(); break;
	case 0x15:    i_adc_axd16(); break;
	case 0x16:    i_push_ss(); break;
	case 0x17:    i_pop_ss(); break;
	case 0x18:    i_sbb_br8(); break;
	case 0x19:    i_sbb_wr16(); break;
	case 0x1a:    i_sbb_r8b(); break;
	case 0x1b:    i_sbb_r16w(); break;
	case 0x1c:    i_sbb_ald8(); break;
	case 0x1d:    i_sbb_axd16(); break;
	case 0x1e:    i_push_ds(); break;
	case 0x1f:    i_pop_ds(); break;
	case 0x20:    i_and_br8(); break;
	case 0x21:    i_and_wr16(); break;
	case 0x22:    i_and_r8b(); break;
	case 0x23:    i_and_r16w(); break;
	case 0x24:    i_and_ald8(); break;
	case 0x25:    i_and_axd16(); break;
	case 0x26:    i_es(); break;
	case 0x27:    i_daa(); break;
	case 0x28:    i_sub_br8(); break;
	case 0x29:    i_sub_wr16(); break;
	case 0x2a:    i_sub_r8b(); break;
	case 0x2b:    i_sub_r16w(); break;
	case 0x2c:    i_sub_ald8(); break;
	case 0x2d:    i_sub_axd16(); break;
	case 0x2e:    i_cs(); break;
	case 0x2f:    i_das(); break;
	case 0x30:    i_xor_br8(); break;
	case 0x31:    i_xor_wr16(); break;
	case 0x32:    i_xor_r8b(); break;
	case 0x33:    i_xor_r16w(); break;
	case 0x34:    i_xor_ald8(); break;
	case 0x35:    i_xor_axd16(); break;
	case 0x36:    i_ss(); break;
	case 0x37:    i_aaa(); break;
	case 0x38:    i_cmp_br8(); break;
	case 0x39:    i_cmp_wr16(); break;
	case 0x3a:    i_cmp_r8b(); break;
	case 0x3b:    i_cmp_r16w(); break;
	case 0x3c:    i_cmp_ald8(); break;
	case 0x3d:    i_cmp_axd16(); break;
	case 0x3e:    i_ds(); break;
	case 0x3f:    i_aas(); break;
	case 0x40:    i_inc_ax(); break;
	case 0x41:    i_inc_cx(); break;
	case 0x42:    i_inc_dx(); break;
	case 0x43:    i_inc_bx(); break;
	case 0x44:    i_inc_sp(); break;
	case 0x45:    i_inc_bp(); break;
	case 0x46:    i_inc_si(); break;
	case 0x47:    i_inc_di(); break;
	case 0x48:    i_dec_ax(); break;
	case 0x49:    i_dec_cx(); break;
	case 0x4a:    i_dec_dx(); break;
	case 0x4b:    i_dec_bx(); break;
	case 0x4c:    i_dec_sp(); break;
	case 0x4d:    i_dec_bp(); break;
	case 0x4e:    i_dec_si(); break;
	case 0x4f:    i_dec_di(); break;
	case 0x50:    i_push_ax(); break;
	case 0x51:    i_push_cx(); break;
	case 0x52:    i_push_dx(); break;
	case 0x53:    i_push_bx(); break;
	case 0x54:    i_push_sp(); break;
	case 0x55:    i_push_bp(); break;
	case 0x56:    i_push_si(); break;
	case 0x57:    i_push_di(); break;
	case 0x58:    i_pop_ax(); break;
	case 0x59:    i_pop_cx(); break;
	case 0x5a:    i_pop_dx(); break;
	case 0x5b:    i_pop_bx(); break;
	case 0x5c:    i_pop_sp(); break;
	case 0x5d:    i_pop_bp(); break;
	case 0x5e:    i_pop_si(); break;
	case 0x5f:    i_pop_di(); break;
        case 0x60:    i_pusha(); break;
        case 0x61:    i_popa(); break;
        case 0x62:    i_bound(); break;
	case 0x63:    i_invalid(); break;
	case 0x64:    i_repnc(); break;
	case 0x65:	  i_repc(); break;
	case 0x66:    i_invalid(); break;
	case 0x67:    i_invalid(); break;
        case 0x68:    i_push_d16(); break;
        case 0x69:    i_imul_d16(); break;
        case 0x6a:    i_push_d8(); break;
        case 0x6b:    i_imul_d8(); break;
        case 0x6c:    i_insb(); break;
        case 0x6d:    i_insw(); break;
        case 0x6e:    i_outsb(); break;
        case 0x6f:    i_outsw(); break;
	case 0x70:    i_jo(); break;
	case 0x71:    i_jno(); break;
	case 0x72:    i_jb(); break;
	case 0x73:    i_jnb(); break;
	case 0x74:    i_jz(); break;
	case 0x75:    i_jnz(); break;
	case 0x76:    i_jbe(); break;
	case 0x77:    i_jnbe(); break;
	case 0x78:    i_js(); break;
	case 0x79:    i_jns(); break;
	case 0x7a:    i_jp(); break;
	case 0x7b:    i_jnp(); break;
	case 0x7c:    i_jl(); break;
	case 0x7d:    i_jnl(); break;
	case 0x7e:    i_jle(); break;
	case 0x7f:    i_jnle(); break;
	case 0x80:    i_80pre(); break;
	case 0x81:    i_81pre(); break;
	case 0x82:	  i_82pre(); break;
	case 0x83:    i_83pre(); break;
	case 0x84:    i_test_br8(); break;
	case 0x85:    i_test_wr16(); break;
	case 0x86:    i_xchg_br8(); break;
	case 0x87:    i_xchg_wr16(); break;
	case 0x88:    i_mov_br8(); break;
	case 0x89:    i_mov_wr16(); break;
	case 0x8a:    i_mov_r8b(); break;
	case 0x8b:    i_mov_r16w(); break;
	case 0x8c:    i_mov_wsreg(); break;
	case 0x8d:    i_lea(); break;
	case 0x8e:    i_mov_sregw(); break;
	case 0x8f:    i_popw(); break;
	case 0x90:    i_nop(); break;
	case 0x91:    i_xchg_axcx(); break;
	case 0x92:    i_xchg_axdx(); break;
	case 0x93:    i_xchg_axbx(); break;
	case 0x94:    i_xchg_axsp(); break;
	case 0x95:    i_xchg_axbp(); break;
	case 0x96:    i_xchg_axsi(); break;
	case 0x97:    i_xchg_axdi(); break;
	case 0x98:    i_cbw(); break;
	case 0x99:    i_cwd(); break;
	case 0x9a:    i_call_far(); break;
	case 0x9b:    i_wait(); break;
	case 0x9c:    i_pushf(); break;
	case 0x9d:    i_popf(); break;
	case 0x9e:    i_sahf(); break;
	case 0x9f:    i_lahf(); break;
	case 0xa0:    i_mov_aldisp(); break;
	case 0xa1:    i_mov_axdisp(); break;
	case 0xa2:    i_mov_dispal(); break;
	case 0xa3:    i_mov_dispax(); break;
	case 0xa4:    i_movsb(); break;
	case 0xa5:    i_movsw(); break;
	case 0xa6:    i_cmpsb(); break;
	case 0xa7:    i_cmpsw(); break;
	case 0xa8:    i_test_ald8(); break;
	case 0xa9:    i_test_axd16(); break;
	case 0xaa:    i_stosb(); break;
	case 0xab:    i_stosw(); break;
	case 0xac:    i_lodsb(); break;
	case 0xad:    i_lodsw(); break;
	case 0xae:    i_scasb(); break;
	case 0xaf:    i_scasw(); break;
	case 0xb0:    i_mov_ald8(); break;
	case 0xb1:    i_mov_cld8(); break;
	case 0xb2:    i_mov_dld8(); break;
	case 0xb3:    i_mov_bld8(); break;
	case 0xb4:    i_mov_ahd8(); break;
	case 0xb5:    i_mov_chd8(); break;
	case 0xb6:    i_mov_dhd8(); break;
	case 0xb7:    i_mov_bhd8(); break;
	case 0xb8:    i_mov_axd16(); break;
	case 0xb9:    i_mov_cxd16(); break;
	case 0xba:    i_mov_dxd16(); break;
	case 0xbb:    i_mov_bxd16(); break;
	case 0xbc:    i_mov_spd16(); break;
	case 0xbd:    i_mov_bpd16(); break;
	case 0xbe:    i_mov_sid16(); break;
	case 0xbf:    i_mov_did16(); break;
        case 0xc0:    i_rotshft_bd8(); break;
        case 0xc1:    i_rotshft_wd8(); break;
	case 0xc2:    i_ret_d16(); break;
	case 0xc3:    i_ret(); break;
	case 0xc4:    i_les_dw(); break;
	case 0xc5:    i_lds_dw(); break;
	case 0xc6:    i_mov_bd8(); break;
	case 0xc7:    i_mov_wd16(); break;
        case 0xc8:    i_enter(); break;
        case 0xc9:    i_leave(); break;
	case 0xca:    i_retf_d16(); break;
	case 0xcb:    i_retf(); break;
	case 0xcc:    i_int3(); break;
	case 0xcd:    i_int(); break;
	case 0xce:    i_into(); break;
	case 0xcf:    i_iret(); break;
        case 0xd0:    i_rotshft_b(); break;
        case 0xd1:    i_rotshft_w(); break;
        case 0xd2:    i_rotshft_bcl(); break;
        case 0xd3:    i_rotshft_wcl(); break;
	case 0xd4:    i_aam(); break;
	case 0xd5:    i_aad(); break;
	case 0xd6:    i_setalc(); break;
	case 0xd7:    i_xlat(); break;
	case 0xd8:    i_escape(); break;
	case 0xd9:    i_escape(); break;
	case 0xda:    i_escape(); break;
	case 0xdb:    i_escape(); break;
	case 0xdc:    i_escape(); break;
	case 0xdd:    i_escape(); break;
	case 0xde:    i_escape(); break;
	case 0xdf:    i_escape(); break;
	case 0xe0:    i_loopne(); break;
	case 0xe1:    i_loope(); break;
	case 0xe2:    i_loop(); break;
	case 0xe3:    i_jcxz(); break;
	case 0xe4:    i_inal(); break;
	case 0xe5:    i_inax(); break;
	case 0xe6:    i_outal(); break;
	case 0xe7:    i_outax(); break;
	case 0xe8:    i_call_d16(); break;
	case 0xe9:    i_jmp_d16(); break;
	case 0xea:    i_jmp_far(); break;
	case 0xeb:    i_jmp_d8(); break;
	case 0xec:    i_inaldx(); break;
	case 0xed:    i_inaxdx(); break;
	case 0xee:    i_outdxal(); break;
	case 0xef:    i_outdxax(); break;
	case 0xf0:    i_lock(); break;
	case 0xf1:    i_invalid(); break;
	case 0xf2:    i_repne(); break;
	case 0xf3:    i_repe(); break;
	case 0xf4:    i_hlt(); break;
	case 0xf5:    i_cmc(); break;
	case 0xf6:    i_f6pre(); break;
	case 0xf7:    i_f7pre(); break;
	case 0xf8:    i_clc(); break;
	case 0xf9:    i_stc(); break;
	case 0xfa:    i_di(); break;
	case 0xfb:    i_ei(); break;
	case 0xfc:    i_cld(); break;
	case 0xfd:    i_std(); break;
	case 0xfe:    i_fepre(); break;
	case 0xff:    i_ffpre(); break;
	};
#else
	nec_instruction[FETCHOP]();
#endif

//if (cpu_get_pc()>0xc0000) logerror("CPU %05x\n",cpu_get_pc());

    }
	return cycles - nec_ICount;
}


unsigned nec_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
    return Dasmnec(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}

/* Wrappers for the different CPU types */
void v20_reset(void *param) { nec_reset(param); }
void v20_exit(void) { nec_exit(); }
int v20_execute(int cycles) { return nec_execute(cycles); }
unsigned v20_get_context(void *dst) { return nec_get_context(dst); }
void v20_set_context(void *src) { nec_set_context(src); }
unsigned v20_get_pc(void) { return nec_get_pc(); }
void v20_set_pc(unsigned val) { nec_set_pc(val); }
unsigned v20_get_sp(void) { return nec_get_sp(); }
void v20_set_sp(unsigned val) { nec_set_sp(val); }
unsigned v20_get_reg(int regnum) { return nec_get_reg(regnum); }
void v20_set_reg(int regnum, unsigned val)	{ nec_set_reg(regnum,val); }
void v20_set_nmi_line(int state) { nec_set_nmi_line(state); }
void v20_set_irq_line(int irqline, int state) { nec_set_irq_line(irqline,state); }
void v20_set_irq_callback(int (*callback)(int irqline)) { nec_set_irq_callback(callback); }
const char *v20_info(void *context, int regnum)
{
    static char buffer[32][63+1];
    static int which = 0;
    nec_Regs *r = context;

    which = ++which % 32;
    buffer[which][0] = '\0';
    if( !context )
        r = &I;

    switch( regnum )
    {
        case CPU_INFO_REG+NEC_IP: sprintf(buffer[which], "IP:%04X", r->ip); break;
        case CPU_INFO_REG+NEC_SP: sprintf(buffer[which], "SP:%04X", r->regs.w[SP]); break;
        case CPU_INFO_REG+NEC_FLAGS: sprintf(buffer[which], "F:%04X", r->flags); break;
        case CPU_INFO_REG+NEC_AW: sprintf(buffer[which], "AW:%04X", r->regs.w[AW]); break;
        case CPU_INFO_REG+NEC_CW: sprintf(buffer[which], "CW:%04X", r->regs.w[CW]); break;
        case CPU_INFO_REG+NEC_DW: sprintf(buffer[which], "DW:%04X", r->regs.w[DW]); break;
        case CPU_INFO_REG+NEC_BW: sprintf(buffer[which], "BW:%04X", r->regs.w[BW]); break;
        case CPU_INFO_REG+NEC_BP: sprintf(buffer[which], "BP:%04X", r->regs.w[BP]); break;
        case CPU_INFO_REG+NEC_IX: sprintf(buffer[which], "IX:%04X", r->regs.w[IX]); break;
        case CPU_INFO_REG+NEC_IY: sprintf(buffer[which], "IY:%04X", r->regs.w[IY]); break;
        case CPU_INFO_REG+NEC_ES: sprintf(buffer[which], "ES:%04X", r->sregs[ES]); break;
        case CPU_INFO_REG+NEC_CS: sprintf(buffer[which], "CS:%04X", r->sregs[CS]); break;
        case CPU_INFO_REG+NEC_SS: sprintf(buffer[which], "SS:%04X", r->sregs[SS]); break;
        case CPU_INFO_REG+NEC_DS: sprintf(buffer[which], "DS:%04X", r->sregs[DS]); break;
        case CPU_INFO_REG+NEC_VECTOR: sprintf(buffer[which], "V:%02X", r->int_vector); break;
        case CPU_INFO_REG+NEC_PENDING: sprintf(buffer[which], "P:%X", r->pending_irq); break;
        case CPU_INFO_REG+NEC_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
        case CPU_INFO_REG+NEC_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state); break;
        case CPU_INFO_FLAGS:
            r->flags = CompressFlags();
            sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                r->flags & 0x8000 ? 'M':'.',
                r->flags & 0x4000 ? '?':'.',
                r->flags & 0x2000 ? '?':'.',
                r->flags & 0x1000 ? '?':'.',
                r->flags & 0x0800 ? 'O':'.',
                r->flags & 0x0400 ? 'D':'.',
                r->flags & 0x0200 ? 'I':'.',
                r->flags & 0x0100 ? 'T':'.',
                r->flags & 0x0080 ? 'S':'.',
                r->flags & 0x0040 ? 'Z':'.',
                r->flags & 0x0020 ? '?':'.',
                r->flags & 0x0010 ? 'A':'.',
                r->flags & 0x0008 ? '?':'.',
                r->flags & 0x0004 ? 'P':'.',
                r->flags & 0x0002 ? 'N':'.',
                r->flags & 0x0001 ? 'C':'.');
            break;
        case CPU_INFO_NAME: return "V20";
        case CPU_INFO_FAMILY: return "NEC V-Series";
        case CPU_INFO_VERSION: return "1.6";
        case CPU_INFO_FILE: return __FILE__;
        case CPU_INFO_CREDITS: return "Real mode NEC emulator v1.3 by Oliver Bergmann\n(initial work based on Fabrice Fabian's i86 core)";
        case CPU_INFO_REG_LAYOUT: return (const char*)nec_reg_layout;
        case CPU_INFO_WIN_LAYOUT: return (const char*)nec_win_layout;
    }
    return buffer[which];
}
unsigned v20_dasm(char *buffer, unsigned pc) { return nec_dasm(buffer,pc); }

void v30_reset(void *param) { nec_reset(param); }
void v30_exit(void) { nec_exit(); }
int v30_execute(int cycles) { return nec_execute(cycles); }
unsigned v30_get_context(void *dst) { return nec_get_context(dst); }
void v30_set_context(void *src) { nec_set_context(src); }
unsigned v30_get_pc(void) { return nec_get_pc(); }
void v30_set_pc(unsigned val) { nec_set_pc(val); }
unsigned v30_get_sp(void) { return nec_get_sp(); }
void v30_set_sp(unsigned val) { nec_set_sp(val); }
unsigned v30_get_reg(int regnum) { return nec_get_reg(regnum); }
void v30_set_reg(int regnum, unsigned val)	{ nec_set_reg(regnum,val); }
void v30_set_nmi_line(int state) { nec_set_nmi_line(state); }
void v30_set_irq_line(int irqline, int state) { nec_set_irq_line(irqline,state); }
void v30_set_irq_callback(int (*callback)(int irqline)) { nec_set_irq_callback(callback); }
const char *v30_info(void *context, int regnum)
{
    static char buffer[32][63+1];
    static int which = 0;
    nec_Regs *r = context;

    which = ++which % 32;
    buffer[which][0] = '\0';
    if( !context )
        r = &I;

    switch( regnum )
    {
        case CPU_INFO_REG+NEC_IP: sprintf(buffer[which], "IP:%04X", r->ip); break;
        case CPU_INFO_REG+NEC_SP: sprintf(buffer[which], "SP:%04X", r->regs.w[SP]); break;
        case CPU_INFO_REG+NEC_FLAGS: sprintf(buffer[which], "F:%04X", r->flags); break;
        case CPU_INFO_REG+NEC_AW: sprintf(buffer[which], "AW:%04X", r->regs.w[AW]); break;
        case CPU_INFO_REG+NEC_CW: sprintf(buffer[which], "CW:%04X", r->regs.w[CW]); break;
        case CPU_INFO_REG+NEC_DW: sprintf(buffer[which], "DW:%04X", r->regs.w[DW]); break;
        case CPU_INFO_REG+NEC_BW: sprintf(buffer[which], "BW:%04X", r->regs.w[BW]); break;
        case CPU_INFO_REG+NEC_BP: sprintf(buffer[which], "BP:%04X", r->regs.w[BP]); break;
        case CPU_INFO_REG+NEC_IX: sprintf(buffer[which], "IX:%04X", r->regs.w[IX]); break;
        case CPU_INFO_REG+NEC_IY: sprintf(buffer[which], "IY:%04X", r->regs.w[IY]); break;
        case CPU_INFO_REG+NEC_ES: sprintf(buffer[which], "ES:%04X", r->sregs[ES]); break;
        case CPU_INFO_REG+NEC_CS: sprintf(buffer[which], "CS:%04X", r->sregs[CS]); break;
        case CPU_INFO_REG+NEC_SS: sprintf(buffer[which], "SS:%04X", r->sregs[SS]); break;
        case CPU_INFO_REG+NEC_DS: sprintf(buffer[which], "DS:%04X", r->sregs[DS]); break;
        case CPU_INFO_REG+NEC_VECTOR: sprintf(buffer[which], "V:%02X", r->int_vector); break;
        case CPU_INFO_REG+NEC_PENDING: sprintf(buffer[which], "P:%X", r->pending_irq); break;
        case CPU_INFO_REG+NEC_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
        case CPU_INFO_REG+NEC_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state); break;
        case CPU_INFO_FLAGS:
            r->flags = CompressFlags();
            sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                r->flags & 0x8000 ? 'M':'.',
                r->flags & 0x4000 ? '?':'.',
                r->flags & 0x2000 ? '?':'.',
                r->flags & 0x1000 ? '?':'.',
                r->flags & 0x0800 ? 'O':'.',
                r->flags & 0x0400 ? 'D':'.',
                r->flags & 0x0200 ? 'I':'.',
                r->flags & 0x0100 ? 'T':'.',
                r->flags & 0x0080 ? 'S':'.',
                r->flags & 0x0040 ? 'Z':'.',
                r->flags & 0x0020 ? '?':'.',
                r->flags & 0x0010 ? 'A':'.',
                r->flags & 0x0008 ? '?':'.',
                r->flags & 0x0004 ? 'P':'.',
                r->flags & 0x0002 ? 'N':'.',
                r->flags & 0x0001 ? 'C':'.');
            break;
        case CPU_INFO_NAME: return "V30";
        case CPU_INFO_FAMILY: return "NEC V-Series";
        case CPU_INFO_VERSION: return "1.6";
        case CPU_INFO_FILE: return __FILE__;
        case CPU_INFO_CREDITS: return "Real mode NEC emulator v1.3 by Oliver Bergmann\n(initial work based on Fabrice Fabian's i86 core)";
        case CPU_INFO_REG_LAYOUT: return (const char*)nec_reg_layout;
        case CPU_INFO_WIN_LAYOUT: return (const char*)nec_win_layout;
    }
    return buffer[which];
}
unsigned v30_dasm(char *buffer, unsigned pc) { return nec_dasm(buffer,pc); }

void v33_reset(void *param) { nec_reset(param); }
void v33_exit(void) { nec_exit(); }
int v33_execute(int cycles) { return nec_execute(cycles); }
unsigned v33_get_context(void *dst) { return nec_get_context(dst); }
void v33_set_context(void *src) { nec_set_context(src); }
unsigned v33_get_pc(void) { return nec_get_pc(); }
void v33_set_pc(unsigned val) { nec_set_pc(val); }
unsigned v33_get_sp(void) { return nec_get_sp(); }
void v33_set_sp(unsigned val) { nec_set_sp(val); }
unsigned v33_get_reg(int regnum) { return nec_get_reg(regnum); }
void v33_set_reg(int regnum, unsigned val)	{ nec_set_reg(regnum,val); }
void v33_set_nmi_line(int state) { nec_set_nmi_line(state); }
void v33_set_irq_line(int irqline, int state) { nec_set_irq_line(irqline,state); }
void v33_set_irq_callback(int (*callback)(int irqline)) { nec_set_irq_callback(callback); }
const char *v33_info(void *context, int regnum)
{
    static char buffer[32][63+1];
    static int which = 0;
    nec_Regs *r = context;

    which = ++which % 32;
    buffer[which][0] = '\0';
    if( !context )
        r = &I;

    switch( regnum )
    {
        case CPU_INFO_REG+NEC_IP: sprintf(buffer[which], "IP:%04X", r->ip); break;
        case CPU_INFO_REG+NEC_SP: sprintf(buffer[which], "SP:%04X", r->regs.w[SP]); break;
        case CPU_INFO_REG+NEC_FLAGS: sprintf(buffer[which], "F:%04X", r->flags); break;
        case CPU_INFO_REG+NEC_AW: sprintf(buffer[which], "AW:%04X", r->regs.w[AW]); break;
        case CPU_INFO_REG+NEC_CW: sprintf(buffer[which], "CW:%04X", r->regs.w[CW]); break;
        case CPU_INFO_REG+NEC_DW: sprintf(buffer[which], "DW:%04X", r->regs.w[DW]); break;
        case CPU_INFO_REG+NEC_BW: sprintf(buffer[which], "BW:%04X", r->regs.w[BW]); break;
        case CPU_INFO_REG+NEC_BP: sprintf(buffer[which], "BP:%04X", r->regs.w[BP]); break;
        case CPU_INFO_REG+NEC_IX: sprintf(buffer[which], "IX:%04X", r->regs.w[IX]); break;
        case CPU_INFO_REG+NEC_IY: sprintf(buffer[which], "IY:%04X", r->regs.w[IY]); break;
        case CPU_INFO_REG+NEC_ES: sprintf(buffer[which], "ES:%04X", r->sregs[ES]); break;
        case CPU_INFO_REG+NEC_CS: sprintf(buffer[which], "CS:%04X", r->sregs[CS]); break;
        case CPU_INFO_REG+NEC_SS: sprintf(buffer[which], "SS:%04X", r->sregs[SS]); break;
        case CPU_INFO_REG+NEC_DS: sprintf(buffer[which], "DS:%04X", r->sregs[DS]); break;
        case CPU_INFO_REG+NEC_VECTOR: sprintf(buffer[which], "V:%02X", r->int_vector); break;
        case CPU_INFO_REG+NEC_PENDING: sprintf(buffer[which], "P:%X", r->pending_irq); break;
        case CPU_INFO_REG+NEC_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
        case CPU_INFO_REG+NEC_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state); break;
        case CPU_INFO_FLAGS:
            r->flags = CompressFlags();
            sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                r->flags & 0x8000 ? 'M':'.',
                r->flags & 0x4000 ? '?':'.',
                r->flags & 0x2000 ? '?':'.',
                r->flags & 0x1000 ? '?':'.',
                r->flags & 0x0800 ? 'O':'.',
                r->flags & 0x0400 ? 'D':'.',
                r->flags & 0x0200 ? 'I':'.',
                r->flags & 0x0100 ? 'T':'.',
                r->flags & 0x0080 ? 'S':'.',
                r->flags & 0x0040 ? 'Z':'.',
                r->flags & 0x0020 ? '?':'.',
                r->flags & 0x0010 ? 'A':'.',
                r->flags & 0x0008 ? '?':'.',
                r->flags & 0x0004 ? 'P':'.',
                r->flags & 0x0002 ? 'N':'.',
                r->flags & 0x0001 ? 'C':'.');
            break;
        case CPU_INFO_NAME: return "V33";
        case CPU_INFO_FAMILY: return "NEC V-Series";
        case CPU_INFO_VERSION: return "1.6";
        case CPU_INFO_FILE: return __FILE__;
        case CPU_INFO_CREDITS: return "Real mode NEC emulator v1.3 by Oliver Bergmann\n(initial work based on Fabrice Fabian's i86 core)";
        case CPU_INFO_REG_LAYOUT: return (const char*)nec_reg_layout;
        case CPU_INFO_WIN_LAYOUT: return (const char*)nec_win_layout;
    }
    return buffer[which];
}
unsigned v33_dasm(char *buffer, unsigned pc) { return nec_dasm(buffer,pc); }


