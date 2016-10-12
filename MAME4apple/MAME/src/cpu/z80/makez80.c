/*****************************************************************************
 *
 *	 makez80.c
 *	 486++ asm emitter for a Z80 cpu core
 *	 supports GNU assembler and NASM
 *
 *	 Copyright (c) 1999 Juergen Buchmueller, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include "cpuintrf.h"
#include "osd_cpu.h"

#define FAST_MEMORY 	1
#define INLINE_IOPORTS	0		/* inline cpu_readport/cpu_writeport functions */
#define BUSY_LOOP_HACKS 1		/* break out of tight loops waiting for an interrupt */
#define WBYTE_EA4_EDX	1		/* use one writeback function for DD/FD CB <ofs8> xx opcodes */
#define EARLY_CTC		1
#define CALC_PREVPC 	0		/* calculate previouspc */

#define ABITS           4       /* ABITS2_16 + ABITS_MIN_16 */

#define GLOBL	0

#define GAS     0
#define NASM    1
#define TASM	2

typedef unsigned char	byte;
typedef unsigned short  word;
typedef unsigned long   dword;

FILE *output;
int as = GAS;
int verbose = 0, mame_debug = 0, optimize = 0;
int opcodes = 0;
int rbyte = 0, rword = 0, wbyte = 0, wword = 0;
int align_funcs = 2, align_jumps = 2, align_loops = 1;

typedef enum {
    none=0,
    al,  ah,  cl,  ch,  dl,  dh,  bl,  bh,
    ax,  cx,  dx,  bx,  si,  di,  bp,  sp,
	eax, ecx, edx, ebx, esi, edi, ebp, esp
}   x86_regs;

typedef enum {
	dm, 	db, 	dw, 	dd, 	align,	extrn,	globl,	text,
	data,	aaa,	aad,	aam,	aas,	adc,	adcb,	adcl,
	adcw,	add,	addb,	addl,	addw,	and,	andb,	andl,
	andw,	bswap,	bt, 	btc,	btr,	bts,	call,	callf,
	cbw,	cdq,	clc,	cld,	cli,	clts,	cmc,	cmp,
	cmpb,	cmpl,	cmpsb,	cmpsd,	cmpsw,	cmpw,	cpuid,	cwd,
	cwde,	daa,	das,	dec,	decb,	decl,	decw,	div,
	divb,	divd,	divw,	fwait,	hlt,	idiv,	idivb,	idivd,
	idivw,	imul,	imulb,	imuld,	imulw,	in, 	inc,	incb,
	incl,	incw,	insb,	insd,	insw,	intr,	into,	iret,
	iretd,	ja, 	jae,	jael,	jal,	jb, 	jbe,	jbel,
	jbl,	jc, 	jcl,	jcxz,	je, 	jecxz,	jel,	jg,
	jge,	jgel,	jgl,	jl, 	jle,	jlel,	jll,	jmp,
	jmpf,	jmpl,	jna,	jnae,	jnael,	jnal,	jnb,	jnbe,
	jnbel,	jnbl,	jnc,	jncl,	jne,	jnel,	jng,	jnge,
	jngel,	jngl,	jnl,	jnle,	jnlel,	jnll,	jno,	jnol,
	jnp,	jnpl,	jns,	jnsl,	jnz,	jnzl,	jo, 	jol,
	jp, 	jpe,	jpel,	jpl,	jpo,	jpol,	js, 	jsl,
	jz, 	jzl,	lahf,	lds,	lea,	leave,	les,	lfs,
	lgdt,	lgs,	lidt,	lldt,	lmsw,	lock,	lodsb,	lodsd,
	lodsw,	loop,	loope,	loopne, loopnz, loopz,	lss,	ltr,
	mov,	movb,	movl,	movsb,	movsbl, movsbw, movsd,	movsw,
	movswl, movw,	movzbl, movzbw, movzwl, mul,	mulb,	muld,
	mulw,	neg,	negb,	negw,	negl,	nop,	not,	notb,
	notw,	notl,	or, 	orb,	orl,	orw,	out,	outsb,
	outsd,	outsw,	pop,	popa,	popad,	popl,	popf,	popfd,
	popw,	push,	pusha,	pushad, pushl,	pushf,	pushfd, pushw,
	rclb,	rcll,	rclw,	rcrb,	rcrl,	rcrw,	rep,	repe,
	repne,	repnz,	repz,	ret,	retd,	retf,	retfd,	rolb,
	roll,	rolw,	rorb,	rorl,	rorw,	sahf,	salb,	sall,
	salw,	sarb,	sarl,	sarw,	sbbb,	sbbl,	sbbw,	scasb,
	scasd,	scasw,	seta,	setae,	setb,	setbe,	setc,	sete,
	setg,	setge,	setl,	setle,	setna,	setnae, setnb,	setnbe,
	setnc,	setne,	setng,	setnge, setnl,	setnle, setno,	setnp,
	setns,	setnz,	seto,	setp,	setpe,	setpo,	sets,	setz,
	sgdt,	shlb,	shld,	shll,	shlw,	shrb,	shrd,	shrl,
	shrw,	sidt,	sldt,	smsw,	stc,	std,	sti,	stosb,
	stosd,	stosw,	subb,	subl,	subw,	testb,	testl,	testw,
	verr,	verw,	wait,	xchg,	xchgb,	xchgw,	xchgl,	xlat,
	xlatb,	xorb,	xorl,	xorw
}   x86_token;

#define AL	R(al)
#define AH	R(ah)
#define CL	R(cl)
#define CH	R(ch)
#define DL	R(dl)
#define DH	R(dh)
#define BL	R(bl)
#define BH	R(bh)
#define AX	R(ax)
#define CX	R(cx)
#define DX	R(dx)
#define BX	R(bx)
#define SI	R(si)
#define DI	R(di)
#define BP	R(bp)
#define EAX R(eax)
#define ECX R(ecx)
#define EDX R(edx)
#define EBX R(ebx)
#define ESI R(esi)
#define EDI R(edi)
#define EBP R(ebp)
#define ESP R(esp)

void emit(const char *fmt, ...) __attribute__((format(printf,1,2)));

void com(const char *fmt, ...);

const char *O(const char *symbol);

int str(char *dst, const char *src);

const char *x86_rstr[][3] = {
	{"",    "",   ""    },
	{"%al", "al", "al"  },
	{"%ah", "ah", "ah"  },
	{"%cl", "cl", "cl"  },
	{"%ch", "ch", "ch"  },
	{"%dl", "dl", "dl"  },
	{"%dh", "dh", "dh"  },
	{"%bl", "bl", "bl"  },
	{"%bh", "bh", "bh"  },
	{"%ax", "ax", "ax"  },
	{"%cx", "cx", "cx"  },
	{"%dx", "dx", "dx"  },
	{"%bx", "bx", "bx"  },
	{"%si", "si", "si"  },
	{"%di", "di", "di"  },
	{"%bp", "bp", "bp"  },
	{"%sp", "sp", "sp"  },
	{"%eax","eax","eax" },
	{"%ecx","ecx","ecx" },
	{"%edx","edx","edx" },
	{"%ebx","ebx","ebx" },
	{"%esi","esi","esi" },
	{"%edi","edi","edi" },
	{"%ebp","ebp","ebp" },
	{"%esp","esp","esp" },
};

const char *x86_token_str[][3] = {
	{".ascii",          "db",               "db",             },
	{".byte",           "db",               "db",             },
	{".short",          "dw",               "dw",             },
	{".long",           "dd",               "dd",             },
	{".align",          "align",            "align",          },
    {".extern",         "extern",           "extern",         },
    {".global",         "global",           "global",         },
    {".text",           "section\t.text",   ".code",          },
    {".data",           "section\t.data",   ".data",          },
    {"aaa",             "aaa",              "aaa",            },
	{"aad",             "aad",              "aad",            },
	{"aam",             "aam",              "aam",            },
	{"aas",             "aas",              "aas",            },
	{"adc",             "adc",              "adc",            },
	{"adcb",            "adc",              "adc",            },
	{"adcl",            "adc",              "adc",            },
	{"adcw",            "adc",              "adc",            },
	{"add",             "add",              "add",            },
	{"addb",            "add",              "add",            },
	{"addl",            "add",              "add",            },
	{"addw",            "add",              "add",            },
	{"and",             "and",              "and",            },
	{"andb",            "and",              "and",            },
	{"andl",            "and",              "and",            },
	{"andw",            "and",              "and",            },
	{"bswap",           "bswap",            "bswap",          },
	{"bt",              "bt",               "bt",             },
	{"btc",             "btc",              "btc",            },
	{"btr",             "btr",              "btr",            },
	{"bts",             "bts",              "bts",            },
	{"call",            "call",             "call",           },
	{"callf",           "callf",            "callf",          },
	{"cbw",             "cbw",              "cbw",            },
	{"cdq",             "cdq",              "cdq",            },
	{"clc",             "clc",              "clc",            },
	{"cld",             "cld",              "cld",            },
	{"cli",             "cli",              "cli",            },
	{"clts",            "clts",             "clts",           },
	{"cmc",             "cmc",              "cmc",            },
	{"cmp",             "cmp",              "cmp",            },
	{"cmpb",            "cmp",              "cmp",            },
	{"cmpl",            "cmp",              "cmp",            },
	{"cmpsb",           "cmpsb",            "cmpsb",          },
	{"cmpsd",           "cmpsd",            "cmpsd",          },
	{"cmpsw",           "cmpsw",            "cmpsw",          },
	{"cmpw",            "cmp",              "cmp",            },
	{"cpuid",           "cpuid",            "cpuid",          },
	{"cwd",             "cwd",              "cwd",            },
	{"cwde",            "cwde",             "cwde",           },
	{"daa",             "daa",              "daa",            },
	{"das",             "das",              "das",            },
	{"dec",             "dec",              "dec",            },
	{"decb",            "dec",              "dec",            },
	{"decl",            "dec",              "dec",            },
	{"decw",            "dec",              "dec",            },
	{"div",             "div",              "div",            },
	{"divb",            "div",              "div",            },
	{"divd",            "div",              "div",            },
	{"divw",            "div",              "div",            },
	{"fwait",           "fwait",            "fwait",          },
	{"hlt",             "hlt",              "hlt",            },
	{"idiv",            "idiv",             "idiv",           },
	{"idivb",           "idiv",             "idiv",           },
	{"idivd",           "idiv",             "idiv",           },
	{"idivw",           "idiv",             "idiv",           },
	{"imul",            "imul",             "imul",           },
	{"imulb",           "imul",             "imul",           },
	{"imuld",           "imul",             "imul",           },
	{"imulw",           "imul",             "imul",           },
	{"in",              "in",               "in",             },
	{"inc",             "inc",              "inc",            },
	{"incb",            "inc",              "inc",            },
	{"incl",            "inc",              "inc",            },
	{"incw",            "inc",              "inc",            },
	{"insb",            "insb",             "insb",           },
	{"insd",            "insd",             "insd",           },
	{"insw",            "insw",             "insw",           },
	{"int",             "int",              "int",            },
	{"into",            "into",             "into",           },
	{"iret",            "iret",             "iret",           },
	{"iretd",           "iretf",            "iretf",          },
	{"ja",              "ja",               "ja",             },
	{"jae",             "jae",              "jae",            },
	{"jael",            "jae",              "jae",            },
	{"jal",             "ja",               "ja",             },
	{"jb",              "jb",               "jb",             },
	{"jbe",             "jbe",              "jbe",            },
	{"jbel",            "jbe",              "jbe",            },
	{"jbl",             "jb",               "jb",             },
	{"jc",              "jc",               "jc",             },
	{"jcl",             "jc",               "jc",             },
	{"jcxz",            "jcxz",             "jcxz",           },
	{"je",              "je",               "je",             },
	{"jecxz",           "jecxz",            "jecxz",          },
	{"jel",             "je",               "je",             },
	{"jg",              "jg",               "jg",             },
	{"jge",             "jge",              "jge",            },
	{"jgel",            "jge",              "jge",            },
	{"jgl",             "jg",               "jg",             },
	{"jl",              "jl",               "jl",             },
	{"jle",             "jle",              "jle",            },
	{"jlel",            "jle",              "jle",            },
	{"jll",             "jl",               "jl",             },
	{"jmp",             "jmp",              "jmp",            },
	{"jmpf",            "jmpf",             "jmpf",           },
	{"jmpl",            "jmp",              "jmp",            },
	{"jna",             "jna",              "jna",            },
	{"jnae",            "jnae",             "jnae",           },
	{"jnael",           "jnae",             "jnae",           },
	{"jnal",            "jna",              "jna",            },
	{"jnb",             "jnb",              "jnb",            },
	{"jnbe",            "jnbe",             "jnbe",           },
	{"jnbel",           "jnbe",             "jnbe",           },
	{"jnbl",            "jnb",              "jnb",            },
	{"jnc",             "jnc",              "jnc",            },
	{"jncl",            "jnc",              "jnc",            },
	{"jne",             "jne",              "jne",            },
	{"jnel",            "jne",              "jne",            },
	{"jng",             "jng",              "jng",            },
	{"jnge",            "jnge",             "jnge",           },
	{"jngel",           "jnge",             "jnge",           },
	{"jngl",            "jng",              "jng",            },
	{"jnl",             "jnl",              "jnl",            },
	{"jnle",            "jnle",             "jnle",           },
	{"jnlel",           "jnle",             "jnle",           },
	{"jnll",            "jnl",              "jnl",            },
	{"jno",             "jno",              "jno",            },
	{"jnol",            "jno",              "jno",            },
	{"jnp",             "jnp",              "jnp",            },
	{"jnpl",            "jnp",              "jnp",            },
	{"jns",             "jns",              "jns",            },
	{"jnsl",            "jns",              "jns",            },
	{"jnz",             "jnz",              "jnz",            },
	{"jnzl",            "jnz",              "jnz",            },
	{"jo",              "jo",               "jo",             },
	{"jol",             "jo",               "jo",             },
	{"jp",              "jp",               "jp",             },
	{"jpe",             "jpe",              "jpe",            },
	{"jpel",            "jpe",              "jpe",            },
	{"jpl",             "jp",               "jp",             },
	{"jpo",             "jpo",              "jpo",            },
	{"jpol",            "jpo",              "jpo",            },
	{"js",              "js",               "js",             },
	{"jsl",             "js",               "js",             },
	{"jz",              "jz",               "jz",             },
	{"jzl",             "jz",               "jz",             },
	{"lahf",            "lahf",             "lahf",           },
	{"lds",             "lds",              "lds",            },
	{"lea",             "lea",              "lea",            },
	{"leave",           "leave",            "leave",          },
	{"les",             "les",              "les",            },
	{"lfs",             "lfs",              "lfs",            },
	{"lgdt",            "lgdt",             "lgdt",           },
	{"lgs",             "lgs",              "lgs",            },
	{"lidt",            "lidt",             "lidt",           },
	{"lldt",            "lldt",             "lldt",           },
	{"lmsw",            "lmsw",             "lmsw",           },
	{"lock",            "lock",             "lock",           },
	{"lodsb",           "lodsb",            "lodsb",          },
	{"lodsd",           "lodsd",            "lodsd",          },
	{"lodsw",           "lodsw",            "lodsw",          },
	{"loop",            "loop",             "loop",           },
	{"loope",           "loope",            "loope",          },
	{"loopne",          "loopne",           "loopne"          },
	{"loopnz",          "loopnz",           "loopnz"          },
	{"loopz",           "loopz",            "loopz",          },
	{"lss",             "lss",              "lss",            },
	{"ltr",             "ltr",              "ltr",            },
	{"mov",             "mov",              "mov",            },
	{"movb",            "mov",              "mov",            },
	{"movl",            "mov",              "mov",            },
	{"movsb",           "movsb",            "movsb",          },
	{"movsbl",          "movsx",            "movsx",          },
	{"movsbw",          "movsx",            "movsx",          },
	{"movsl",           "movsd",            "movsd",          },
	{"movsw",           "movsw",            "movsw",          },
	{"movswl",          "movsx",            "movsx",          },
	{"movw",            "mov",              "mov",            },
	{"movzbl",          "movzx",            "movzx",          },
	{"movzbw",          "movzx",            "movzx",          },
	{"movzwl",          "movzx",            "movzx",          },
	{"mul",             "mul",              "mul",            },
	{"mulb",            "mul",              "mul",            },
	{"muld",            "mul",              "mul",            },
	{"mulw",            "mul",              "mul",            },
	{"neg",             "neg",              "neg",            },
	{"negb",            "neg",              "neg",            },
	{"negw",            "neg",              "neg",            },
	{"negl",            "neg",              "neg",            },
	{"nop",             "nop",              "nop",            },
	{"not",             "not",              "not",            },
	{"notb",            "not",              "not",            },
	{"notw",            "not",              "not",            },
	{"notl",            "not",              "not",            },
	{"or",              "or",               "or",             },
	{"orb",             "or",               "or",             },
	{"orl",             "or",               "or",             },
	{"orw",             "or",               "or",             },
	{"out",             "out",              "out",            },
	{"outsb",           "outsb",            "outsb",          },
	{"outsd",           "outsd",            "outsd",          },
	{"outsw",           "outsw",            "outsw",          },
	{"pop",             "pop",              "pop",            },
	{"popa",            "popa",             "popa",           },
	{"popal",           "popad",            "popad",          },
	{"popl",            "pop",              "pop",            },
	{"popf",            "popf",             "popf",           },
	{"popfl",           "popfd",            "popfd",          },
	{"popw",            "pop",              "pop",            },
	{"push",            "push",             "push",           },
	{"pusha",           "pusha",            "pusha",          },
	{"pushal",          "pushad",           "pushad"          },
	{"pushl",           "push\tdword",      "push\tdword",    },
	{"pushf",           "pushf",            "pushf",          },
	{"pushfl",          "pushfd",           "pushfd"          },
	{"pushw",           "push\tword",       "push\tword",     },
	{"rclb",            "rcl",              "rcl",            },
	{"rcll",            "rcl",              "rcl",            },
	{"rclw",            "rcl",              "rcl",            },
	{"rcrb",            "rcr",              "rcr",            },
	{"rcrl",            "rcr",              "rcr",            },
	{"rcrw",            "rcr",              "rcr",            },
	{"rep",             "rep",              "rep",            },
	{"repe",            "repe",             "repe",           },
	{"repne",           "repne",            "repne",          },
	{"repnz",           "repnz",            "repnz",          },
	{"repz",            "repz",             "repz",           },
	{"ret",             "ret",              "ret",            },
	{"retd",            "retd",             "retd",           },
	{"retf",            "retf",             "retf",           },
	{"retfd",           "retfd",            "retfd",          },
	{"rolb",            "rol",              "rol",            },
	{"roll",            "rol",              "rol",            },
	{"rolw",            "rol",              "rol",            },
	{"rorb",            "ror",              "ror",            },
	{"rorl",            "ror",              "ror",            },
	{"rorw",            "ror",              "ror",            },
	{"sahf",            "sahf",             "sahf",           },
	{"salb",            "sal",              "sal",            },
	{"sall",            "sal",              "sal",            },
	{"salw",            "sal",              "sal",            },
	{"sarb",            "sar",              "sar",            },
	{"sarl",            "sar",              "sar",            },
	{"sarw",            "sar",              "sar",            },
	{"sbbb",            "sbb ",             "sbb ",           },
	{"sbbl",            "sbb ",             "sbb ",           },
	{"sbbw",            "sbb ",             "sbb ",           },
	{"scasb",           "scasb",            "scasb",          },
	{"scasd",           "scasd",            "scasd",          },
	{"scasw",           "scasw",            "scasw",          },
	{"seta",            "seta",             "seta",           },
	{"setae",           "setae",            "setae",          },
	{"setb",            "setb",             "setb",           },
	{"setbe",           "setbe",            "setbe",          },
	{"setc",            "setc",             "setc",           },
	{"sete",            "sete",             "sete",           },
	{"setg",            "setg",             "setg",           },
	{"setge",           "setge",            "setge",          },
	{"setl",            "setl",             "setl",           },
	{"setle",           "setle",            "setle",          },
	{"setna",           "setna",            "setna",          },
	{"setnae",          "setnae",           "setnae"          },
	{"setnb",           "setnb",            "setnb",          },
	{"setnbe",          "setnbe",           "setnbe"          },
	{"setnc",           "setnc",            "setnc",          },
	{"setne",           "setne",            "setne",          },
	{"setng",           "setng",            "setng",          },
	{"setnge",          "setnge",           "setnge"          },
	{"setnl",           "setnl",            "setnl",          },
	{"setnle",          "setnle",           "setnle"          },
	{"setno",           "setno",            "setno",          },
	{"setnp",           "setnp",            "setnp",          },
	{"setns",           "setns",            "setns",          },
	{"setnz",           "setnz",            "setnz",          },
	{"seto",            "seto",             "seto",           },
	{"setp",            "setp",             "setp",           },
	{"setpe",           "setpe",            "setpe",          },
	{"setpo",           "setpo",            "setpo",          },
	{"sets",            "sets",             "sets",           },
	{"setz",            "setz",             "setz",           },
	{"sgdt",            "sgdt",             "sgdt",           },
	{"shlb",            "shl",              "shl",            },
	{"shld",            "shld",             "shld",           },
	{"shll",            "shl",              "shl",            },
	{"shlw",            "shl",              "shl",            },
	{"shrb",            "shr",              "shr",            },
	{"shrd",            "shrd",             "shrd",           },
	{"shrl",            "shr",              "shr",            },
	{"shrw",            "shr",              "shr",            },
	{"sidt",            "sidt",             "sidt",           },
	{"sldt",            "sldt",             "sldt",           },
	{"smsw",            "smsw",             "smsw",           },
	{"stc",             "stc",              "stc",            },
	{"std",             "std",              "std",            },
	{"sti",             "sti",              "sti",            },
	{"stosb",           "stosb",            "stosb",          },
	{"stosl",           "stosd",            "stosd",          },
	{"stosw",           "stosw",            "stosw",          },
	{"subb",            "sub",              "sub",            },
	{"subl",            "sub",              "sub",            },
	{"subw",            "sub",              "sub",            },
	{"testb",           "test",             "test",           },
	{"testl",           "test",             "test",           },
	{"testw",           "test",             "test",           },
	{"verr",            "verr",             "verr",           },
	{"verw",            "verw",             "verw",           },
	{"wait",            "wait",             "wait",           },
	{"xchg",            "xchg",             "xchg",           },
	{"xchgb",           "xchg",             "xchg",           },
	{"xchgw",           "xchg",             "xchg",           },
	{"xchgl",           "xchg",             "xchg",           },
	{"xlat",            "xlat",             "xlat",           },
	{"xlatb",           "xlatb",            "xlatb",          },
	{"xorb",            "xor",              "xor",            },
	{"xorl",            "xor",              "xor",            },
	{"xorw",            "xor",              "xor",            },
};

#define SF				0x80
#define ZF				0x40
#define YF				0x20
#define HF				0x10
#define XF				0x08
#define PF				0x04
#define VF				PF
#define NF				0x02
#define CF				0x01

#define INT_IRQ         0x01
#define INT_NMI 		0x80

#define zF              M(1,"z80_AF",0,0,0,0)
#define zA				M(1,"z80_AF",0,1,0,0)
#define zAF 			M(2,"z80_AF",0,0,0,0)
#define zAF32			M(4,"z80_AF",0,0,0,0)

#define zC				M(1,"z80_BC",0,0,0,0)
#define zB				M(1,"z80_BC",0,1,0,0)
#define zBC 			M(2,"z80_BC",0,0,0,0)
#define zBC32			M(4,"z80_BC",0,0,0,0)

#define zE				M(1,"z80_DE",0,0,0,0)
#define zD				M(1,"z80_DE",0,1,0,0)
#define zDE 			M(2,"z80_DE",0,0,0,0)
#define zDE32			M(4,"z80_DE",0,0,0,0)

#define zL				M(1,"z80_HL",0,0,0,0)
#define zH				M(1,"z80_HL",0,1,0,0)
#define zHL 			M(2,"z80_HL",0,0,0,0)
#define zHL32			M(4,"z80_HL",0,0,0,0)

#define zLX 			M(1,"z80_IX",0,0,0,0)
#define zHX 			M(1,"z80_IX",0,1,0,0)
#define zIX 			M(2,"z80_IX",0,0,0,0)
#define zIX32			M(4,"z80_IX",0,0,0,0)

#define zLY 			M(1,"z80_IY",0,0,0,0)
#define zHY 			M(1,"z80_IY",0,1,0,0)
#define zIY 			M(2,"z80_IY",0,0,0,0)
#define zIY32			M(4,"z80_IY",0,0,0,0)

#define zPCL			M(1,"z80_PC",0,0,0,0)
#define zPCH			M(1,"z80_PC",0,1,0,0)
#define zPC 			M(2,"z80_PC",0,0,0,0)
#define zPC32			M(4,"z80_PC",0,0,0,0)

#define zSPL			M(1,"z80_SP",0,0,0,0)
#define zSPH			M(1,"z80_SP",0,1,0,0)
#define zSP 			M(2,"z80_SP",0,0,0,0)
#define zSP32			M(4,"z80_SP",0,0,0,0)

#define zF2 			M(1,"z80_AF2",0,0,0,0)
#define zA2 			M(1,"z80_AF2",0,1,0,0)
#define zAF2            M(2,"z80_AF2",0,0,0,0)
#define zBC2			M(2,"z80_BC2",0,0,0,0)
#define zDE2			M(2,"z80_DE2",0,0,0,0)
#define zHL2			M(2,"z80_HL2",0,0,0,0)

#define zR				M(1,"z80_R",0,0,0,0)
#define zR2             M(1,"z80_R2",0,0,0,0)
#define zIFF1           M(1,"z80_IFF1",0,0,0,0)
#define zIFF2           M(1,"z80_IFF2",0,0,0,0)
#define zHALT           M(1,"z80_HALT",0,0,0,0)
#define zIM 			M(1,"z80_IM",0,0,0,0)
#define zI				M(1,"z80_I",0,0,0,0)
#define pending 		M(1,"z80_pending_irq",0,0,0,0)
#define vector_d		M(4,"z80_vector",0,0,0,0)
#define vector_w		M(2,"z80_vector",0,0,0,0)
#define vector_h		M(1,"z80_vector",0,2,0,0)
#define irq_max 		M(4,"z80_irq_max",0,0,0,0)
#define request_irq 	M(4,"z80_request_irq",0,0,0,0)
#define service_irq 	M(4,"z80_service_irq",0,0,0,0)
#define int_state(n)	M(4,"z80_int_state",0,0,n,4)
#define irq(n,o)		M(4,"z80_irq",n,o,0,0)
#define nmi_state		M(4,"z80_nmi_state",0,0,0,0)
#define irq_state		M(4,"z80_irq_state",0,0,0,0)
#define irq_callback	M(4,"z80_irq_callback",0,0,0,0)
#define zEA 			M(4,"z80_EA",0,0,0,0)
#define zMB 			M(1,"z80_MB",0,0,0,0)
#define zDAA(n) 		M(2,"z80_daa",0,0,n,2)

#define zICOUNT 		M(4,"_Z80_ICount",0,0,0,0)
#define zITOTAL 		M(4,"z80_itotal",0,0,0,0)
#define zIEXTRN 		M(4,"z80_iextrn",0,0,0,0)

#define PREVPC			M(4,"_previouspc",0,0,0,0)
#define DEBUG			M(1,"_mame_debug",0,0,0,0)
#define BANKBASE(n) 	M(4,"_cpu_bankbase",0,0,n,4)
#define MRHARD          M(4,"_cur_mrhard",0,0,0,0)
#define READHW(n,a) 	M(1,"_readhardware",n,-0x4000,a,1)
#define READOFFSET(n)	M(4,"_memoryreadoffset",0,0,n,4)
#define READHANDLER(n)	M(4,"_memoryreadhandler",0,0,n,4)

#define MWHARD          M(4,"_cur_mwhard",0,0,0,0)
#define WRITEHW(n,a)	M(1,"_writehardware",n,-0x4000,a,1)
#define WRITEOFFSET(n)	M(4,"_memorywriteoffset",0,0,n,4)
#define WRITEHANDLER(n) M(4,"_memorywritehandler",0,0,n,4)

#define ROM             M(4,"_ROM",0,0,0,0)
#define RAM             M(4,"_RAM",0,0,0,0)
#define OP_ROM			M(4,"_OP_ROM",0,0,0,0)
#define OP_RAM			M(4,"_OP_RAM",0,0,0,0)

/* Z80 accu and flags mapped to x86 (E)AX register */
#define F				AH
#define A				AL
#define AF				AX
#define AF32			EAX

/* Z80 HL mapped to x86 (E)BX register */
#define L				BL
#define H				BH
#define HL				BX
#define HL32			EBX

/* Z80 program counter mapped to x86 (E)SI register */
#define pc32            esi
#define PC              SI
#define PC32            ESI

#include "z80daa.h"

void emit(const char *fmt, ...)
{
va_list arg;
    va_start(arg,fmt);
	vfprintf(output, fmt, arg);
    va_end(arg);
    fprintf(output, "\n");
}

void com(const char *fmt, ...)
{
va_list arg;
int len = 0;
	if (fmt) {
		if (as == GAS)
			len += fprintf(output, "/* ");
		else
			len += fprintf(output, "; ");
		va_start(arg, fmt);
		len += vfprintf(output, fmt, arg);
		va_end(arg);
		if (as == GAS)
		{
			while (len < 72)
			{
				fprintf(output, "\t");
				len += 8 - (len & 7);
			}
			fprintf(output, " */");
		}
    }
	fprintf(output, "\n");
}

/* return an immediate decimal byte size value */
const char *B(int value)
{
    static char B_str[32][16];
    static int which = 0;
    char *dst;

    which = ++which % 32;
    dst = B_str[which];
    if (as == GAS)
		sprintf(dst, "$%d", value);
    else
		sprintf(dst, "byte %d", value);
    return B_str[which];
}

/* return an immediate decimal value */
const char *D(int value)
{
    static char D_str[32][16];
    static int which = 0;
    char *dst;

    which = ++which % 32;
    dst = D_str[which];
    if (as == GAS)
        sprintf(dst, "$%d", value);
    else
        sprintf(dst, "%d", value);
    return D_str[which];
}

/* return an immediate hexadecimal value */
const char *X(int value)
{
    static char X_str[32][16];
    static int which = 0;
    char *dst;

    which = ++which % 32;
    dst = X_str[which];
    if (as == GAS)
        sprintf(dst, "$0x%x", value);
    else
        sprintf(dst, "0%xh", value);
    return X_str[which];
}

/* return a symbol with 'short' distance modifier (for NASM) */
char *LS(const char *symbol)
{
	static char buffer[128];
	if (as == NASM)
		sprintf(buffer, "short %s", symbol);
	else
		sprintf(buffer, "%s", symbol);
	return buffer;
}

/* return a symbol with 'near' distance modifier (for NASM) */
char *LN(const char *symbol)
{
	static char buffer[128];
	if (as == NASM)
		sprintf(buffer, "near %s", symbol);
	else
		sprintf(buffer, "%s", symbol);
	return buffer;
}

/* emit a symbol and optional comment */
void S(const char *symbol, const char *comment, ...)
{
	va_list arg;
	char buffer[256];
	char fmt[128];
	int len = 0;

    fmt[0] = '\0';
    if (symbol)
		len += sprintf(fmt, "%s:", symbol);
	if (comment) {
		while (len < 32) {
			strcat(fmt,"\t");
			len += 8 - (len & 7);
		}
		if (as == GAS) {
			strcat(fmt,"/* ");
			len += 3;
		} else {
			strcat(fmt,"; ");
			len += 2;
        }
		len += strlen(comment);
		strcat(fmt, comment);
    }
    va_start(arg, comment);
	len = len - strlen(fmt) + vsprintf(buffer, fmt, arg);
	va_end(arg);

	if (comment && as == GAS) {
		while (len < 72) {
			strcat(buffer,"\t");
			len += 8 - (len & 7);
        }
		strcat(buffer, "*/");
	}
	fprintf(output, "%s\n", buffer);
}

/* emit an symbol, x86 instruction, dst and src operands and comment */
void I(const char *sym, x86_token t, const char *dst, const char *src, const char *comment)
{
	char buffer[32], *p;
	int len = 0, first_tab;

	first_tab = (t < align) ? 24 : 8;
	if (sym)
		len += fprintf(output, "%s:", sym);

	strcpy(buffer, x86_token_str[t][as]);
	if (as == NASM) {
		p = strstr(buffer,"\tdword");       /* remove 'dword' once if it occurs twice */
		if (p && ((src && !strncmp(src,"dword",5)) || (dst && !strncmp(dst,"dword",5))))
			*p = '\0';
		p = strstr(buffer,"\tword");        /* remove 'word' once if it occurs twice */
		if (p && ((src && !strncmp(src,"word",4)) || (dst && !strncmp(dst,"word",4))))
			*p = '\0';
	}

	do {
		len = (len | 7) + 1;
		fprintf(output, "\t");
	} while (len < first_tab);

    len += fprintf(output, "%s", buffer);
    if (dst) {
		do {
			len = (len | 7) + 1;
			fprintf(output, "\t");
		} while (len < first_tab + 8);
        if (src) {
			if (as == GAS)
				len += fprintf(output, "%s, %s", src, dst);
			else
				len += fprintf(output, "%s, %s", dst, src);
		} else {
			len += fprintf(output, "%s", dst);
        }
	} else {
		if (src) {
			do {
				len = (len | 7) + 1;
				fprintf(output, "\t");
			} while (len < first_tab + 8);
			len += fprintf(output, "%s", src);
		}
    }
	if (comment) {
		do {
			fprintf(output, "\t");
			len = (len | 7) + 1;
		} while (len < 48);
		if (as == GAS) {
			len += fprintf(output, "/* %s", comment);
			while (len < 72) {
				fprintf(output, "\t");
				len = (len | 7) + 1;
            }
			len += fprintf(output, " */");
		} else {
			len += fprintf(output, "; %s", comment);
		}
	}
	fprintf(output, "\n");
}

/* return a memory reference */
const char *M(int size, const char *base, x86_regs r1, int offs, x86_regs r2, int mult)
{
	static char mem_str[32][64];
	static int which = 0;
	char *dst;

    which = ++which % 32;
    dst = mem_str[which];
	switch (as) {
		case GAS:
			if (base) {
				dst += sprintf(dst, "%s", base);
				if (offs)
					dst += sprintf(dst, "%+d", offs);
			} else {
				if (offs)
					dst += sprintf(dst, "%d", offs);
			}
			if (r1) {
				dst += sprintf(dst, "(%s", x86_rstr[r1][as]);
				if (r2)
					dst += sprintf(dst, ",%s", x86_rstr[r2][as]);
				if (mult)
					dst += sprintf(dst, ",%d", mult);
				dst += sprintf(dst, ")");
			} else if (r2) {
				dst += sprintf(dst, "(,%s", x86_rstr[r2][as]);
				if (mult)
					dst += sprintf(dst, ",%d", mult);
				dst += sprintf(dst, ")");
			}
			break;

        case NASM:
			switch (size) {
				case 1:
					dst += sprintf(dst, "byte[");
					break;
				case 2:
					dst += sprintf(dst, "word[");
					break;
				case 4:
					dst += sprintf(dst, "dword[");
					break;
				default:	/* no size prefix */
					dst += sprintf(dst, "[");
			}
			if (base)
				dst += sprintf(dst, "%s", base);
			if (r1) {
				if (base)
					dst += sprintf(dst, "+");
				dst += sprintf(dst, "%s", x86_rstr[r1][as]);
				if (offs)
					dst += sprintf(dst, "%+d", offs);
				if (r2) {
					dst += sprintf(dst, "+%s", x86_rstr[r2][as]);
					if (mult && mult > 1)
						dst += sprintf(dst, "*%d", mult);
				}
			} else if (r2) {
				if (base)
					dst += sprintf(dst, "+");
				dst += sprintf(dst, "%s", x86_rstr[r2][as]);
				if (mult && mult > 1)
					dst += sprintf(dst, "*%d", mult);
			} else {
				if (offs) {
					if (base)
						dst += sprintf(dst, "%+d", offs);
					else
						dst += sprintf(dst, "%s", X(offs));
				}
			}
			dst += sprintf(dst, "]");
			break;

        case TASM:
			switch (size) {
				case 1:
					dst += sprintf(dst, "byte ptr [");
					break;
				case 2:
					dst += sprintf(dst, "word ptr [");
					break;
				case 4:
					dst += sprintf(dst, "dword ptr [");
					break;
				default:	/* no size prefix */
					dst += sprintf(dst, "[");
			}
			if (base)
				dst += sprintf(dst, "%s", base);
			if (r1) {
				if (base)
					dst += sprintf(dst, "+");
				dst += sprintf(dst, "%s", x86_rstr[r1][as]);
				if (offs)
					dst += sprintf(dst, "%+d", offs);
				if (r2) {
					dst += sprintf(dst, "+%s", x86_rstr[r2][as]);
					if (mult)
						dst += sprintf(dst, "*%d", mult);
				}
			} else if (r2) {
				if (base)
					dst += sprintf(dst, "+");
				dst += sprintf(dst, "%s", x86_rstr[r2][as]);
				if (mult)
					dst += sprintf(dst, "*%d", mult);
			} else {
				if (offs) {
					if (base)
						dst += sprintf(dst, "%+d", offs);
					else
						dst += sprintf(dst, "%s", X(offs));
				}
			}
			dst += sprintf(dst, "]");
            break;
    }
    return mem_str[which];
}

/* return a x86 register name */
const char *R(x86_regs rg)
{
	if (as == GAS) {
		switch (rg) {
			case none: return "none";
			case al:  return "%al";
			case ah:  return "%ah";
			case cl:  return "%cl";
			case ch:  return "%ch";
			case dl:  return "%dl";
			case dh:  return "%dh";
			case bl:  return "%bl";
			case bh:  return "%bh";
			case ax:  return "%ax";
			case cx:  return "%cx";
			case dx:  return "%dx";
			case bx:  return "%bx";
			case si:  return "%si";
			case di:  return "%di";
			case bp:  return "%bp";
			case sp:  return "%sp";
			case eax: return "%eax";
			case ecx: return "%ecx";
			case edx: return "%edx";
			case ebx: return "%ebx";
			case esi: return "%esi";
			case edi: return "%edi";
			case ebp: return "%ebp";
			case esp: return "%esp";
		}
	} else {
		switch (rg) {
			case none: return "none";
            case al:  return "al";
			case ah:  return "ah";
			case cl:  return "cl";
			case ch:  return "ch";
			case dl:  return "dl";
			case dh:  return "dh";
			case bl:  return "bl";
			case bh:  return "bh";
			case ax:  return "ax";
			case cx:  return "cx";
			case dx:  return "dx";
			case bx:  return "bx";
			case si:  return "si";
			case di:  return "di";
			case bp:  return "bp";
			case sp:  return "sp";
			case eax: return "eax";
			case ecx: return "ecx";
			case edx: return "edx";
			case ebx: return "ebx";
			case esi: return "esi";
			case edi: return "edi";
			case ebp: return "ebp";
			case esp: return "esp";
        }
    }
	return "xx";
}

const char *O(const char *symbol)
{
	static char ofs_str[32][32];
	static int which = 0;
	char *dst;

    which = ++which % 32;
	dst = ofs_str[which];
	switch (as) {
		case GAS:	/* GAS: $ indicates immediate (address of) */
			sprintf(dst, "$%s", symbol);
			break;
		case NASM:	/* NASM: only the name w/o square brackets */
			sprintf(dst, "%s", symbol);
			break;
		case TASM:	/* TASM: offset name */
			sprintf(dst, "offset %s", symbol);
            break;
    }
	return ofs_str[which];
}

int str(char *dst, const char *src)
{
	int argc = 0;
	char strdelim = '"';

	if (as == GAS) {
		*dst++ = strdelim;
		while (*src) {
			switch (*src) {
				case '\b':   /* backspace */
					dst += sprintf(dst, "\\010");
					break;
				case '\t':   /* tab */
					dst += sprintf(dst, "\\011");
					break;
				case '\n':   /* newline */
					dst += sprintf(dst, "\\012");
					break;
				case '\r':   /* carriage return  */
					dst += sprintf(dst, "\\015");
					break;
				default:
					if (src[0] == '%' && src[1] != '%')
						argc++;
					*dst++ = *src;
			}
			src++;
		}
		strcpy(dst, "\\000\"");
	} else {
		char len = 0;
		int instr = 0;

		while (*src) {
			switch (*src) {
				case '\b':   /* backspace */
				case '\t':   /* tab */
				case '\n':   /* line feed */
				case '\r':   /* new line */
					if (instr) {
						*dst++ = strdelim;
						len++;
						instr = 0;
					}
					if (len) {
						*dst++ = ',';
						len++;
					}
					dst += sprintf(dst, "%d", *src);
					break;
				default:
					if (src[0] == '%' && src[1] != '%')
						argc++;
					if (!instr) {
						if (len) {
							*dst++ = ',';
							len++;
						}
                        *dst++ = strdelim;
						len++;
						instr = 1;
					}
                    *dst++ = *src;
					len++;
			}
			src++;
		}
		if (instr) {
			*dst++ = strdelim;
			len++;
		}
		if (len)
			*dst++ = ',';
		strcpy(dst, "0");
    }
    return argc;
}

void debug(int level, char *fmt, ...)
{
	static int dbg_cnt = 0;
	va_list args;

    if (verbose >= level) {
		int i, argc;
		char dbgtxt[16], dbgend[16], format[128];
		char *arg, pusharg[32][64];

        argc = str(format, fmt);
        dbg_cnt++;
		sprintf(dbgtxt, "L%04x", dbg_cnt);
		sprintf(dbgend, "L%04xx", dbg_cnt);
		I(0,		data,	0,					0,		0);
		I(dbgtxt,	dm, 	format, 			0,		0);
		I(0,		text,	0,					0,		0);
        va_start(args, fmt);
		for (i = 0; i < argc; i++) {
			arg = va_arg(args, char *);
			sprintf(pusharg[i],"\t%s\t%s\n",x86_token_str[pushl][as],arg);
		}
		va_end(args);
		I(0,testl,	M(4,"_errorlog",0,0,0,0),   D(-1),  0);
		I(0,je, 	LS(dbgend), 				0,		0);
		I(0,pushad,0,0,0);
		for (i = argc - 1; i >= 0; i--)
			fputs(pusharg[i], output);
		I(0,pushl,	O(dbgtxt),					0,		0);
		I(0,pushl,	M(4,"_errorlog",0,0,0,0),   0,      0);
		I(0,call, "_fprintf",                   0,      0);
		I(0,addl,	ESP,						D((argc + 2) * 4),0);
		I(0,popad,	0,							0,		0);
		S(dbgend,0);
    }
}

static void dumpregs(void)
{
	debug(8,"AF:%04x BC:%04x DE:%04x HL:%04x IX:%04x IY:%04x SP:%04x PC:%04x PC:%04x\n",
		zAF,zBC,zDE,zHL,zIX,zIY,zSP,zPC);
}


static void pad(int alignment)
{
	char buffer[4+1];
	com(0);
	if (alignment) {
		if (as == GAS) {
			sprintf(buffer, "%d", alignment);
			I(0,align,buffer,0,0);
		} else {
			sprintf(buffer, "%d", 2<<alignment);
			I(0,align,buffer,0,0);
		}
	}
}

static void save_af(void)
{
    I(0,movb,   zF,         F,          "save F");
	I(0,movb,	zA, 		A,			"save A");
}

static void restore_af(void)
{
	I(0,movzbl, AF32,		zA, 		"get A");
	I(0,movb,	F,			zF, 		"get F");
}

static void save_z80(int opsize)
{
	I(0,movw,	zPC,		PC, 		"save PC");
	I(0,movw,	zHL,		HL, 		"save HL");
#if CALC_PREVPC
    if (opsize) {
		if ( opsize == +1 )
			I(0,decl,	PC32,		0,			"subtract opcode size from PC");
		else
			I(0,subl,	PC32,		B(opsize),	"subtract opcode size from PC");
		I(0,movl,	PREVPC, 	PC32,		"store previous PC");
	}
#endif
}

static void restore_z80(void)
{
	I(0,movzwl, PC32,		zPC,		"get PC");
	I(0,movzwl, HL32,		zHL,		"get HL");
}

/*************************************************************************
 * MOVB
 * Move register/memory from src to dst
 *************************************************************************/
static void MOVB( const char *dst, const char *src, const char *comment, ... )
{
	char buffer[128+1];
    va_list arg;
	va_start(arg, comment);
	vsprintf(buffer, comment, arg);
    va_end(arg);

    if (!strcmp(dst, src))
		return;
    if (!strcmp(dst, AL) || !strcmp(src, AL) ||
        !strcmp(dst, AH) || !strcmp(src, AH) ||
        !strcmp(dst, CL) || !strcmp(src, CL) ||
        !strcmp(dst, CH) || !strcmp(src, CH) ||
        !strcmp(dst, DL) || !strcmp(src, DL) ||
        !strcmp(dst, DH) || !strcmp(src, DH) ||
        !strcmp(dst, BL) || !strcmp(src, BL) ||
        !strcmp(dst, BH) || !strcmp(src, BH)) {
        if (!strcmp(dst,EAX) || !strcmp(dst,ECX) ||
            !strcmp(dst,EDX) || !strcmp(dst,EBX) ||
            !strcmp(dst,EBP) || !strcmp(dst,ESI) ||
            !strcmp(dst,EDI) || !strcmp(dst,ESP)) {
			/* zero extend byte register */
			I(0,movzbl, dst,	src,	buffer);
        } else {
			/* dst and src are byte register */
			I(0,movb,	dst,	src,	buffer);
        }
    } else {
        if (!strcmp(dst,EAX) || !strcmp(dst,ECX) ||
            !strcmp(dst,EDX) || !strcmp(dst,EBX) ||
            !strcmp(dst,EBP) || !strcmp(dst,ESI) ||
            !strcmp(dst,EDI) || !strcmp(dst,ESP)) {
			/* zero extend memory byte */
			I(0,movzbl, dst,	src,	buffer);
        } else {
			/* memory to memory byte transfer */
			I(0,movb,	CL, 	src,	buffer);
            I(0,movb,   dst,    CL,     0);
        }
    }
}

/*************************************************************************
 * ADJUST_PC
 * increment/decrement or adjust program counter (PC)
 *************************************************************************/
void ADJUST_PC( int offset )
{
	if (offset == +1)
		I(0,incl,	PC32,	0,					"increment PC");
	else
	if (offset == -1)
		I(0,decl,	PC32,	0,					"decrement PC");
	else
		I(0,addl,	PC32,	B(offset),			"adjust PC");
}

/*************************************************************************
 * ROP
 * Read opcode from OP_ROM at PC
 *************************************************************************/
static void ROP( void )
{
	I(0,movl,	EDI,	OP_ROM, 			"get OP_ROM base");
	I(0,movzbl, ECX,	M(1,0,edi,0,pc32,1),"read opcode byte");
	ADJUST_PC(+1);
}

/*************************************************************************
 * ARG
 * dst		where to store the result
 * Get an 8 bit argument from OP_RAM at PC
 *************************************************************************/
static void ARG( const char *dst )
{
	I(0,movl,	EDI,	OP_RAM, 	"get OP_RAM base");
	MOVB( dst, M(1,0,edi,0,pc32,1), "read opcode argument byte");
	ADJUST_PC(+1);
}

/*************************************************************************
 * ARG16
 * dst		where to store the result
 * Get an 16 bit argument from OP_RAM at PC
 *************************************************************************/
static void ARG16(const char *dst)
{
	I(0,movl,	EDI,	OP_RAM, 	"get OP_RAM base");
	if (!strcmp(dst, EAX) || !strcmp(dst, ECX) ||
		!strcmp(dst, EDX) || !strcmp(dst, EBX) ||
		!strcmp(dst, ESI) || !strcmp(dst, EDI) ||
		!strcmp(dst, EBP) || !strcmp(dst, ESP)) {
		I(0,movzwl, dst,	M(2,0,edi,0,pc32,1), "read opcode argument word (zero extend)");
	} else {
		I(0,movzwl, ECX,	M(2,0,edi,0,pc32,1), "read opcode argument word (zero extend)");
        I(0,movl,   dst,    ECX,        "store result");
	}
	if (strcmp(dst, PC32))	/* not loaded PC itself? */
		ADJUST_PC(+2);
}

/*************************************************************************
 * REL
 * Get an sign extended byte argument from OP_RAM at PC into ECX
 *************************************************************************/
static void REL( void )
{
	I(0,movl,	EDI,	OP_RAM, 			"get OP_RAM base");
	I(0,movsbl, ECX,	M(1,0,edi,0,pc32,1),"read 2's complement offset");
	ADJUST_PC(+1);
}

/*************************************************************************
 * OFS
 * base 	IX or IY register (dword[z80_IX] or dword[z80_IY])
 * Build index register plus ofs8 (2's complement offset)
 *************************************************************************/
static void OFS(const char *base)
{
	I(0,movl,	EDI,	OP_RAM, 			"get OP_RAM base");
	I(0,movsbl, ECX,	M(1,0,edi,0,pc32,1),"read opcode argument byte");
	I(0,addl,	ECX,	base,				"add index register");
	I(0,movzwl, EBP,	CX, 				"16 address bits");
    ADJUST_PC(+1);
}

/*************************************************************************
 * CHANGE_PC
 * Check if cur_mrhard for the new PC has changed and call
 * cpu_setOPbase16 if necessary
 *************************************************************************/
static void CHANGE_PC( void )
{
	static int chgpc = 0;
    char symbol[32+1];
	chgpc++;
	sprintf(symbol,"chgpc_%02x", chgpc);
	I(0,movzwl, ECX,	PC, 					"get PC to ECX");
	I(0,movl,	PC32,	ECX,					"make PC 16 bit again");
    I(0,movl,   EDI,    MRHARD,                 0);
    I(0,shrl,   ECX,    D(ABITS),               ">> (ABITS2_16 + ABITS_MIN_16)");
	I(0,movb,	CL, 	M(1,0,edi,0,ecx,1), 	"get cur_mrhard[index]");
	I(0,cmpb,	CL, 	M(1,"_ophw",0,0,0,0),   "changed?");
	I(0,je, 	LS(symbol),0,					"no, skip");
    save_af();
	save_z80( 0 );
	I(0,pushl,	PC32,	0,						0);
	I(0,call,	"_cpu_setOPbase16",0,           0);
	I(0,addl,	ESP,	B(4),					0);
	restore_z80();
	restore_af();
	S(symbol,0);
}

/*************************************************************************
 * RBYTE
 * dst	where to store the result
 * src	memory address
 * Read a byte from memory
 *************************************************************************/
static void RBYTE( int opsize, const char *val, const char *src )
{
	char symbol[15+1];
	rbyte++;
	sprintf(symbol,"rbyte%d",   opsize*CALC_PREVPC);
	if (strcmp(src, EBP))
		I(0,movl,	EBP,	src,		"memory address to EBP");
	I(0,call,	symbol, 0,			"read byte");
	if( strcmp(val, EDX) && strcmp(val, DX)  && strcmp(val, DL) )
		I(0,movl,	val,	EDX,		"store result");
}

/*************************************************************************
 * RWORD
 * val	where to store the result
 * src	memory address
 * Read a word from memory
 *************************************************************************/
static void RWORD( int opsize, const char *val, const char *src )
{
	char symbol[15+1];
	rword++;
	sprintf(symbol,"rword%d", opsize*CALC_PREVPC);
	if (strcmp(src, EBP))
		I(0,movl,	EBP,	src,		"memory address to EBP");
	I(0,call,	symbol, 0,			"read word");
	if( strcmp(val, EDX) && strcmp(val, DX)  && strcmp(val, DL) )
		I(0,movl,	val,	EDX,		"store result");
}

/*************************************************************************
 * WBYTE
 * dst	memory address
 * val	value to store
 * Write a byte to memory
 *************************************************************************/
static void WBYTE( int opsize, const char *dst, const char *val )
{
	char symbol[15+1];
	wbyte++;
	sprintf(symbol,"wbyte%d", opsize*CALC_PREVPC);
	if (strcmp(dst, EBP))
		I(0,movl,	EBP,	dst,	"memory address to EBP");
	if (strcmp(val, EDX))
		I(0,movl,	EDX,	val,	"get value to EDX");
	I(0,call,	symbol, 0,		"write byte");
}

/*************************************************************************
 * WWORD
 * dst	memory address
 * val	value to store
 * Write a word to memory
 *************************************************************************/
static void WWORD( int opsize, const char *dst, const char *val )
{
	char symbol[15+1];
	wword++;
	sprintf(symbol,"wword%d", opsize*CALC_PREVPC);
	if (strcmp(dst, EBP))
		I(0,movl,	EBP,	dst,	"memory address to EBP");
	if (strcmp(val, EDX))
		I(0,movl,	EDX,	val,	"get value to EDX");
	I(0,call,	symbol, 0,		"write word");
}

static void PUSH( int opsize, const char *src )
{
	char reg[32+1];
	strcpy(reg, src);
	I(0,subw,	zSP,	B(2),	0);
	WWORD( opsize, zSP32, reg );
}

static void POP( int opsize, const char *dst )
{
	char reg[32+1];
	strcpy(reg, dst);
	RWORD( opsize, reg, zSP32 );
	I(0,addw,	zSP,	B(2),	0);
}

static char *beg_opcode(int cycles, const char *symbol, const char *comment, ...)
{
	static char endfunc[32];
	char begfunc[32], *dst = begfunc;
	char buffer[256];
	va_list args;

#if GLOBL
	/* put an underscore in front of the symbol */
	sprintf(dst, "_%s", symbol);
	I(0,globl,begfunc,0,0);
#else
	/* extract the symbol */
    strcpy(dst, symbol);
#endif

	/* symbol to put at the end of the function */
	sprintf(endfunc, "L%s", begfunc);

	if (comment) {
		va_start(args, comment);
		vsprintf(buffer, comment, args);
		va_end(args);
		com("***** %s", buffer);
	}
	pad(align_funcs);
    S(begfunc,0);
	if (cycles)
		I(0,subl,	zICOUNT,	B(cycles),	"subtract from ICount");
	if (strncmp(symbol,"xx",2)) /* M1 type opcode fetch? */
		I(0,incb,	zR, 		0,			"increment R register once");

    opcodes++;

    return endfunc;
}

static void end_opcode(const char *op_base)
{
    if (!strcmp(op_base, "z_op")) {
		I(0,orl,	zICOUNT,	B(0),			"all cycles used up?");
		I(0,jle,	LN("z80_execute_done"),0,   0);
#if (!CALC_PREVPC)
		I(0,movl,	PREVPC, 	PC32,			"store previous PC");
#endif
        if (mame_debug) {
			I(0,testb,	DEBUG,			D(1),		0);
			I(0,jne,	LN("z80_mame_debug"),0,     0);
        }
    }
    if (strcmp(op_base, "z_xx")) {
        ROP();
	} else {
		ARG( ECX );  /* byte #4 of [DD/FD CB <ofs8> xx] is read as op_arg! */
	}
	I(0,jmp,	M(4,op_base,0,0,ecx,4),0,		"jump to opcode handler");
    com(0);
}

static void mk_debug( void )
{
	save_af();
	save_z80(0);
#if 0
    I(0,movb,   AL,             zR,         "get R");
    I(0,andb,   AL,             X(0x7f),    "mask lower seven bits");
    I(0,orb,    AL,             zR2,        "get original bit 7");
    I(0,movb,   zR,             AL,         "save it for the debugger");
#endif
    I(0,call,   "_MAME_Debug",  0,          0);
	restore_z80();
	restore_af();
}

static void mk_take_irq( int opcode )
{
	char symbol[14][32+1];
	sprintf(symbol[ 0],"z80_%02x_check_daisy_1",    opcode);
	sprintf(symbol[ 1],"z80_%02x_check_daisy_2",    opcode);
	sprintf(symbol[ 2],"z80_%02x_check_daisy_3",    opcode);
	sprintf(symbol[ 3],"z80_%02x_check_daisy_4",    opcode);
	sprintf(symbol[ 4],"z80_%02x_check_daisy_5",    opcode);
	sprintf(symbol[ 5],"z80_%02x_take_irq_1",       opcode);
	sprintf(symbol[ 6],"z80_%02x_take_irq_2",       opcode);
	sprintf(symbol[ 7],"z80_%02x_take_irq_3",       opcode);
	sprintf(symbol[ 8],"z80_%02x_take_irq_4",       opcode);
	sprintf(symbol[ 9],"z80_%02x_take_irq_5",       opcode);
	sprintf(symbol[10],"z80_%02x_take_irq_6",       opcode);
	sprintf(symbol[11],"z80_%02x_take_irq_7",       opcode);
	sprintf(symbol[12],"z80_%02x_take_irq_8",       opcode);
	sprintf(symbol[13],"z80_%02x_take_irq_done",    opcode);

    I(0,cmpl,   irq_max,        B(0),               "daisy chain mode?");
	I(0,je, 	LN(symbol[ 5]),0,					"no, single int");

#if (!EARLY_CTC)
	com("===================================================================");
	com("  Z80CTC code                                                      ");
    com("===================================================================");
    save_af();
	I(0,movb,	zIFF1,			D(0),				"clear IFF1");
    I(0,pushl,  D(0),           0,                  "irqline always 0");
	I(0,call,	irq_callback,	0,					"get the vector");
    I(0,addl,   ESP,            B(4),               "clean up stack");
    I(0,movzbl, EDX,            AL,                 "EDX = state");
	I(0,movl,	ECX,			EAX,				0);
	restore_af();
	I(0,shrl,	ECX,			D(8),				"ECX = device");
	I(0,cmpl,	EDX,			int_state(ecx), 	"state change?");
	I(0,je, 	LN(symbol[ 4]),0,					"no change, skip");
	I(0,movl,	int_state(ecx), EDX,				"store new state");
	I(0,movl,	EDX,			D(-1),				0);
	I(0,movl,	ECX,			D(0),				0);
	I(0,movl,	request_irq,	EDX,				"reset request IRQ");
	I(0,movl,	service_irq,	EDX,				"reset service IRQ");
    pad(align_loops);
	S(symbol[1],0);
	I(0,testl,	int_state(ecx), X(Z80_INT_IEO), 	"Z80_INT_IEO set?");
	I(0,je, 	LS(symbol[2]),0,					"no, skip");
	I(0,movl,	request_irq,	EDX,				"mask lower IRQ");
	I(0,movl,	service_irq,	ECX,				"set service device number");
    com(0);
	S(symbol[2],0);
	I(0,testl,	int_state(ecx), X(Z80_INT_REQ), 	"Z80_INT_REQ set?");
	I(0,je, 	LS(symbol[3]),0,					"no, skip");
	I(0,movl,	request_irq,	ECX,				"set request device number");
    com(0);
	S(symbol[3],0);
	I(0,incl,	ECX,			0,					"next device");
	I(0,cmpl,	ECX,			irq_max,			"more entries to check?");
	I(0,jb, 	LS(symbol[1]),0,					"yes, check them");
#endif
	com("===================================================================");
	com("  check Z80CTC request irq                                         ");
    com("===================================================================");
	S(symbol[ 4],0);
    I(0,movl,   ECX,            request_irq,        "get device");
	I(0,cmpl,	ECX,			B(0),				"request?");
	I(0,jl, 	LN(symbol[13]),0,					"should not happen, but...");
    save_af();
	I(0,lea,	ECX,			M(0,0,0,0,ecx,4),	"* sizeof(Z80_DaisyChain)");
    I(0,pushl,  irq(ecx,12),    0,                  "Z80.irq[device].irq_param");
	I(0,call,	irq(ecx, 4),	0,					"call Z80.irq[device].interrupt_entry");
	I(0,addl,	ESP,			B(4),				"one DWORD was pushed");
	I(0,movl,	vector_d,		EAX,				"store vector");
    restore_af();
    I(0,movl,   request_irq,    D(-1),              "reset request IRQ");
	I(0,jmp,	LS(symbol[ 6]), 0,					0);

	com("===================================================================");
	com("  single interrupt code                                            ");
    com("===================================================================");
	S(symbol[ 5],0);
	save_af();
	I(0,movb,	zIFF1,			D(0),				"clear IFF1");
    I(0,pushl,  D(0),           0,                  "irqline always 0");
    I(0,call,   irq_callback,   0,                  "do a irq_callback");
	I(0,movl,	vector_d,		EAX,				"store vector");
    I(0,addl,   ESP,            B(4),               "one DWORD was pushed");
    restore_af();

	com("===================================================================");
	com("  here when a valid vector is set                                  ");
    com("===================================================================");
	S(symbol[ 6],0);
	com(0);
    I(0,cmpb,   zHALT,          D(1),               "CPU was halted?");
	I(0,jne,	LS(symbol[ 7]), 0,					"no, skip");
    I(0,movb,   zHALT,          D(0),               "clear halt flag");
	I(0,incw,	PC, 			0,					0);
    com(0);
	S(symbol[ 7],0);
    I(0,movl,   PREVPC,         D(-1),              "no valid previous PC");
    I(0,cmpb,   zIM,            B(2),               "interrupt mode 2?");
	I(0,jne,	LN(symbol[ 8]), 0,					"no, test IM 1");
	I(0,call,	"z80_push_pc",  0,                  0);
	I(0,movzwl, ECX,			vector_w,			"get vector");
    I(0,movb,   CH,             zI,                 "get MSB from I register");
	RWORD( 0, PC32, ECX );
	I(0,jmp,	symbol[12], 	0,					"change PC and exit");

    com(0);
	S(symbol[ 8],0);
    I(0,cmpb,   zIM,            B(1),               "interrupt mode 1?");
	I(0,jne,	LN(symbol[ 9]),0,					"no, it's IM 0");
	I(0,call,	"z80_push_pc",  0,                  0);
	I(0,addl,	zIEXTRN,		B(11),				"subtract <RST xx> cycles");
	I(0,movl,	PC32,			X(0x38),			"<RST 38> vector");
	I(0,jmp,	symbol[12], 	0,					"change PC and exit");

	com("===================================================================");
	com("  here when interrupt mode is 0                                    ");
    com("===================================================================");
	S(symbol[ 9],0);
	I(0,cmpb,	vector_h,		X(0xcd),			"opcode <CALL imm16>?");
	I(0,jne,	LN(symbol[10]),0,					"no, skip");
	I(0,call,	"z80_push_pc",  0,                  0);
//	The C core doesn't count cycles for <CALL imm16> vector
//  I(0,addl,   zIEXTRN,        B(17),              "subtract cycles for CALL opcode");
	I(0,movzwl, PC32,			vector_w,			"get vector bits 0..15");
	I(0,jmp,	LS(symbol[12]), 0,					"change PC and exit");

    com(0);
	S(symbol[10],0);
	I(0,cmpb,	vector_h,		X(0xc3),			"opcode <JP imm16>?");
	I(0,jne,	LS(symbol[11]), 0,					"no, skip");
//	The C core doesn't count cycles for <JP   imm16> vector
//	I(0,addl,	zIEXTRN,		B(10),				"subtract cycles for JP opcode");
	I(0,movzwl, PC32,			vector_w,			"get vector bits 0..15");
	I(0,jmp,	LS(symbol[12]), 0,					"change PC and exit");

	com(0);
    S(symbol[11],0);
	com("===================================================================");
	com("  here when vector is single byte opcode                           ");
    com("===================================================================");
	I(0,call,	"z80_push_pc",  0,                  0);
	I(0,addl,	zIEXTRN,		B(11),				"assume <RST xx> cycles");
	I(0,movl,	PC32,			vector_d,			"get vector");
	I(0,andl,	PC32,			B(0x38),			"mask RST vector");

	com("===================================================================");
	com("  here with new PC                                                 ");
    com("===================================================================");
	S(symbol[12],0);
	CHANGE_PC();

	com(0);
    pad(align_jumps);
    S(symbol[13],0);
	save_af();
	save_z80(0);
}

static void mk_header(void)
{
	int i;

    com("===========================================================================");
    com("                                                                           ");
    com("    Z80 x86 cpu emulation                                                  ");
    com("    Copyright (c) 1998 by Juergen Buchmueller                              ");
    com("    (pullmoll@t-online.de)                                                 ");
    com("                                                                           ");
if (as == GAS)
    com("    For assembly by GNU assembler only                                     ");
else
if (as == NASM)
    com("    For assembly by NASM only                                              ");
else
    com("    For assembly by TASM 3.2 and later only                                ");
    com("                                                                           ");
    com("    Theory of operation                                                    ");
    com("        eax holds the current accumulator and flags (al = A, ah = F)       ");
    com("        ecx is used as temporary register (address)                        ");
    com("        edx is used as temporary register (data)                           ");
    com("        esi holds the current program counter (PC)                         ");
	com("        ebp is used as memory handler return address                       ");
    com("                                                                           ");
    com("===========================================================================");
    com(0);
    if (as == NASM)
        emit("bits\t32");
    if (as == TASM)
    {
        emit(".486");
        emit(".model\tflat");
    }
    com(0);
    I(0,data,0,0,0);
    S("z_op",0);
    I(0,dd,"op_00,op_01,op_02,op_03,op_04,op_05,op_06,op_07,op_08,op_09,op_0a,op_0b,op_0c,op_0d,op_0e,op_0f",0,0);
    I(0,dd,"op_10,op_11,op_12,op_13,op_14,op_15,op_16,op_17,op_18,op_19,op_1a,op_1b,op_1c,op_1d,op_1e,op_1f",0,0);
    I(0,dd,"op_20,op_21,op_22,op_23,op_24,op_25,op_26,op_27,op_28,op_29,op_2a,op_2b,op_2c,op_2d,op_2e,op_2f",0,0);
    I(0,dd,"op_30,op_31,op_32,op_33,op_34,op_35,op_36,op_37,op_38,op_39,op_3a,op_3b,op_3c,op_3d,op_3e,op_3f",0,0);
    I(0,dd,"op_40,op_41,op_42,op_43,op_44,op_45,op_46,op_47,op_48,op_49,op_4a,op_4b,op_4c,op_4d,op_4e,op_4f",0,0);
    I(0,dd,"op_50,op_51,op_52,op_53,op_54,op_55,op_56,op_57,op_58,op_59,op_5a,op_5b,op_5c,op_5d,op_5e,op_5f",0,0);
    I(0,dd,"op_60,op_61,op_62,op_63,op_64,op_65,op_66,op_67,op_68,op_69,op_6a,op_6b,op_6c,op_6d,op_6e,op_6f",0,0);
    I(0,dd,"op_70,op_71,op_72,op_73,op_74,op_75,op_76,op_77,op_78,op_79,op_7a,op_7b,op_7c,op_7d,op_7e,op_7f",0,0);
    I(0,dd,"op_80,op_81,op_82,op_83,op_84,op_85,op_86,op_87,op_88,op_89,op_8a,op_8b,op_8c,op_8d,op_8e,op_8f",0,0);
    I(0,dd,"op_90,op_91,op_92,op_93,op_94,op_95,op_96,op_97,op_98,op_99,op_9a,op_9b,op_9c,op_9d,op_9e,op_9f",0,0);
    I(0,dd,"op_a0,op_a1,op_a2,op_a3,op_a4,op_a5,op_a6,op_a7,op_a8,op_a9,op_aa,op_ab,op_ac,op_ad,op_ae,op_af",0,0);
    I(0,dd,"op_b0,op_b1,op_b2,op_b3,op_b4,op_b5,op_b6,op_b7,op_b8,op_b9,op_ba,op_bb,op_bc,op_bd,op_be,op_bf",0,0);
    I(0,dd,"op_c0,op_c1,op_c2,op_c3,op_c4,op_c5,op_c6,op_c7,op_c8,op_c9,op_ca,op_cb,op_cc,op_cd,op_ce,op_cf",0,0);
    I(0,dd,"op_d0,op_d1,op_d2,op_d3,op_d4,op_d5,op_d6,op_d7,op_d8,op_d9,op_da,op_db,op_dc,op_dd,op_de,op_df",0,0);
    I(0,dd,"op_e0,op_e1,op_e2,op_e3,op_e4,op_e5,op_e6,op_e7,op_e8,op_e9,op_ea,op_eb,op_ec,op_ed,op_ee,op_ef",0,0);
    I(0,dd,"op_f0,op_f1,op_f2,op_f3,op_f4,op_f5,op_f6,op_f7,op_f8,op_f9,op_fa,op_fb,op_fc,op_fd,op_fe,op_ff",0,0);
    com(0);
    S("z_cb",0);
    I(0,dd,"cb_00,cb_01,cb_02,cb_03,cb_04,cb_05,cb_06,cb_07,cb_08,cb_09,cb_0a,cb_0b,cb_0c,cb_0d,cb_0e,cb_0f",0,0);
    I(0,dd,"cb_10,cb_11,cb_12,cb_13,cb_14,cb_15,cb_16,cb_17,cb_18,cb_19,cb_1a,cb_1b,cb_1c,cb_1d,cb_1e,cb_1f",0,0);
    I(0,dd,"cb_20,cb_21,cb_22,cb_23,cb_24,cb_25,cb_26,cb_27,cb_28,cb_29,cb_2a,cb_2b,cb_2c,cb_2d,cb_2e,cb_2f",0,0);
    I(0,dd,"cb_30,cb_31,cb_32,cb_33,cb_34,cb_35,cb_36,cb_37,cb_38,cb_39,cb_3a,cb_3b,cb_3c,cb_3d,cb_3e,cb_3f",0,0);
    I(0,dd,"cb_40,cb_41,cb_42,cb_43,cb_44,cb_45,cb_46,cb_47,cb_48,cb_49,cb_4a,cb_4b,cb_4c,cb_4d,cb_4e,cb_4f",0,0);
    I(0,dd,"cb_50,cb_51,cb_52,cb_53,cb_54,cb_55,cb_56,cb_57,cb_58,cb_59,cb_5a,cb_5b,cb_5c,cb_5d,cb_5e,cb_5f",0,0);
    I(0,dd,"cb_60,cb_61,cb_62,cb_63,cb_64,cb_65,cb_66,cb_67,cb_68,cb_69,cb_6a,cb_6b,cb_6c,cb_6d,cb_6e,cb_6f",0,0);
    I(0,dd,"cb_70,cb_71,cb_72,cb_73,cb_74,cb_75,cb_76,cb_77,cb_78,cb_79,cb_7a,cb_7b,cb_7c,cb_7d,cb_7e,cb_7f",0,0);
    I(0,dd,"cb_80,cb_81,cb_82,cb_83,cb_84,cb_85,cb_86,cb_87,cb_88,cb_89,cb_8a,cb_8b,cb_8c,cb_8d,cb_8e,cb_8f",0,0);
    I(0,dd,"cb_90,cb_91,cb_92,cb_93,cb_94,cb_95,cb_96,cb_97,cb_98,cb_99,cb_9a,cb_9b,cb_9c,cb_9d,cb_9e,cb_9f",0,0);
    I(0,dd,"cb_a0,cb_a1,cb_a2,cb_a3,cb_a4,cb_a5,cb_a6,cb_a7,cb_a8,cb_a9,cb_aa,cb_ab,cb_ac,cb_ad,cb_ae,cb_af",0,0);
    I(0,dd,"cb_b0,cb_b1,cb_b2,cb_b3,cb_b4,cb_b5,cb_b6,cb_b7,cb_b8,cb_b9,cb_ba,cb_bb,cb_bc,cb_bd,cb_be,cb_bf",0,0);
    I(0,dd,"cb_c0,cb_c1,cb_c2,cb_c3,cb_c4,cb_c5,cb_c6,cb_c7,cb_c8,cb_c9,cb_ca,cb_cb,cb_cc,cb_cd,cb_ce,cb_cf",0,0);
    I(0,dd,"cb_d0,cb_d1,cb_d2,cb_d3,cb_d4,cb_d5,cb_d6,cb_d7,cb_d8,cb_d9,cb_da,cb_db,cb_dc,cb_dd,cb_de,cb_df",0,0);
    I(0,dd,"cb_e0,cb_e1,cb_e2,cb_e3,cb_e4,cb_e5,cb_e6,cb_e7,cb_e8,cb_e9,cb_ea,cb_eb,cb_ec,cb_ed,cb_ee,cb_ef",0,0);
    I(0,dd,"cb_f0,cb_f1,cb_f2,cb_f3,cb_f4,cb_f5,cb_f6,cb_f7,cb_f8,cb_f9,cb_fa,cb_fb,cb_fc,cb_fd,cb_fe,cb_ff",0,0);
    com(0);
    S("z_dd",0);
    I(0,dd,"dd_00,dd_01,dd_02,dd_03,dd_04,dd_05,dd_06,dd_07,dd_08,dd_09,dd_0a,dd_0b,dd_0c,dd_0d,dd_0e,dd_0f",0,0);
    I(0,dd,"dd_10,dd_11,dd_12,dd_13,dd_14,dd_15,dd_16,dd_17,dd_18,dd_19,dd_1a,dd_1b,dd_1c,dd_1d,dd_1e,dd_1f",0,0);
    I(0,dd,"dd_20,dd_21,dd_22,dd_23,dd_24,dd_25,dd_26,dd_27,dd_28,dd_29,dd_2a,dd_2b,dd_2c,dd_2d,dd_2e,dd_2f",0,0);
    I(0,dd,"dd_30,dd_31,dd_32,dd_33,dd_34,dd_35,dd_36,dd_37,dd_38,dd_39,dd_3a,dd_3b,dd_3c,dd_3d,dd_3e,dd_3f",0,0);
    I(0,dd,"dd_40,dd_41,dd_42,dd_43,dd_44,dd_45,dd_46,dd_47,dd_48,dd_49,dd_4a,dd_4b,dd_4c,dd_4d,dd_4e,dd_4f",0,0);
    I(0,dd,"dd_50,dd_51,dd_52,dd_53,dd_54,dd_55,dd_56,dd_57,dd_58,dd_59,dd_5a,dd_5b,dd_5c,dd_5d,dd_5e,dd_5f",0,0);
    I(0,dd,"dd_60,dd_61,dd_62,dd_63,dd_64,dd_65,dd_66,dd_67,dd_68,dd_69,dd_6a,dd_6b,dd_6c,dd_6d,dd_6e,dd_6f",0,0);
    I(0,dd,"dd_70,dd_71,dd_72,dd_73,dd_74,dd_75,dd_76,dd_77,dd_78,dd_79,dd_7a,dd_7b,dd_7c,dd_7d,dd_7e,dd_7f",0,0);
    I(0,dd,"dd_80,dd_81,dd_82,dd_83,dd_84,dd_85,dd_86,dd_87,dd_88,dd_89,dd_8a,dd_8b,dd_8c,dd_8d,dd_8e,dd_8f",0,0);
    I(0,dd,"dd_90,dd_91,dd_92,dd_93,dd_94,dd_95,dd_96,dd_97,dd_98,dd_99,dd_9a,dd_9b,dd_9c,dd_9d,dd_9e,dd_9f",0,0);
    I(0,dd,"dd_a0,dd_a1,dd_a2,dd_a3,dd_a4,dd_a5,dd_a6,dd_a7,dd_a8,dd_a9,dd_aa,dd_ab,dd_ac,dd_ad,dd_ae,dd_af",0,0);
    I(0,dd,"dd_b0,dd_b1,dd_b2,dd_b3,dd_b4,dd_b5,dd_b6,dd_b7,dd_b8,dd_b9,dd_ba,dd_bb,dd_bc,dd_bd,dd_be,dd_bf",0,0);
    I(0,dd,"dd_c0,dd_c1,dd_c2,dd_c3,dd_c4,dd_c5,dd_c6,dd_c7,dd_c8,dd_c9,dd_ca,dd_cb,dd_cc,dd_cd,dd_ce,dd_cf",0,0);
    I(0,dd,"dd_d0,dd_d1,dd_d2,dd_d3,dd_d4,dd_d5,dd_d6,dd_d7,dd_d8,dd_d9,dd_da,dd_db,dd_dc,dd_dd,dd_de,dd_df",0,0);
    I(0,dd,"dd_e0,dd_e1,dd_e2,dd_e3,dd_e4,dd_e5,dd_e6,dd_e7,dd_e8,dd_e9,dd_ea,dd_eb,dd_ec,dd_ed,dd_ee,dd_ef",0,0);
    I(0,dd,"dd_f0,dd_f1,dd_f2,dd_f3,dd_f4,dd_f5,dd_f6,dd_f7,dd_f8,dd_f9,dd_fa,dd_fb,dd_fc,dd_fd,dd_fe,dd_ff",0,0);
    com(0);
    S("z_ed",0);
    I(0,dd,"illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop",0,0);
    I(0,dd,"illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop",0,0);
    I(0,dd,"illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop",0,0);
    I(0,dd,"illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop",0,0);
    I(0,dd,"ed_40,ed_41,ed_42,ed_43,ed_44,ed_45,ed_46,ed_47,ed_48,ed_49,ed_4a,ed_4b,ed_4c,ed_4d,ed_4e,ed_4f",0,0);
    I(0,dd,"ed_50,ed_51,ed_52,ed_53,ed_54,ed_55,ed_56,ed_57,ed_58,ed_59,ed_5a,ed_5b,ed_5c,ed_5d,ed_5e,ed_5f",0,0);
    I(0,dd,"ed_60,ed_61,ed_62,ed_63,ed_64,ed_65,ed_66,ed_67,ed_68,ed_69,ed_6a,ed_6b,ed_6c,ed_6d,ed_6e,ed_6f",0,0);
    I(0,dd,"ed_70,ed_71,ed_72,ed_73,ed_74,ed_75,ed_76,illop,ed_78,ed_79,ed_7a,ed_7b,ed_7c,ed_7d,ed_7e,illop",0,0);
    I(0,dd,"illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop",0,0);
    I(0,dd,"illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop",0,0);
    I(0,dd,"ed_a0,ed_a1,ed_a2,ed_a3,illop,illop,illop,illop,ed_a8,ed_a9,ed_aa,ed_ab,illop,illop,illop,illop",0,0);
    I(0,dd,"ed_b0,ed_b1,ed_b2,ed_b3,illop,illop,illop,illop,ed_b8,ed_b9,ed_ba,ed_bb,illop,illop,illop,illop",0,0);
    I(0,dd,"illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop",0,0);
    I(0,dd,"illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop",0,0);
    I(0,dd,"illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop",0,0);
    I(0,dd,"illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop,illop",0,0);
    com(0);
    S("z_fd",0);
    I(0,dd,"fd_00,fd_01,fd_02,fd_03,fd_04,fd_05,fd_06,fd_07,fd_08,fd_09,fd_0a,fd_0b,fd_0c,fd_0d,fd_0e,fd_0f",0,0);
    I(0,dd,"fd_10,fd_11,fd_12,fd_13,fd_14,fd_15,fd_16,fd_17,fd_18,fd_19,fd_1a,fd_1b,fd_1c,fd_1d,fd_1e,fd_1f",0,0);
    I(0,dd,"fd_20,fd_21,fd_22,fd_23,fd_24,fd_25,fd_26,fd_27,fd_28,fd_29,fd_2a,fd_2b,fd_2c,fd_2d,fd_2e,fd_2f",0,0);
    I(0,dd,"fd_30,fd_31,fd_32,fd_33,fd_34,fd_35,fd_36,fd_37,fd_38,fd_39,fd_3a,fd_3b,fd_3c,fd_3d,fd_3e,fd_3f",0,0);
    I(0,dd,"fd_40,fd_41,fd_42,fd_43,fd_44,fd_45,fd_46,fd_47,fd_48,fd_49,fd_4a,fd_4b,fd_4c,fd_4d,fd_4e,fd_4f",0,0);
    I(0,dd,"fd_50,fd_51,fd_52,fd_53,fd_54,fd_55,fd_56,fd_57,fd_58,fd_59,fd_5a,fd_5b,fd_5c,fd_5d,fd_5e,fd_5f",0,0);
    I(0,dd,"fd_60,fd_61,fd_62,fd_63,fd_64,fd_65,fd_66,fd_67,fd_68,fd_69,fd_6a,fd_6b,fd_6c,fd_6d,fd_6e,fd_6f",0,0);
    I(0,dd,"fd_70,fd_71,fd_72,fd_73,fd_74,fd_75,fd_76,fd_77,fd_78,fd_79,fd_7a,fd_7b,fd_7c,fd_7d,fd_7e,fd_7f",0,0);
    I(0,dd,"fd_80,fd_81,fd_82,fd_83,fd_84,fd_85,fd_86,fd_87,fd_88,fd_89,fd_8a,fd_8b,fd_8c,fd_8d,fd_8e,fd_8f",0,0);
    I(0,dd,"fd_90,fd_91,fd_92,fd_93,fd_94,fd_95,fd_96,fd_97,fd_98,fd_99,fd_9a,fd_9b,fd_9c,fd_9d,fd_9e,fd_9f",0,0);
    I(0,dd,"fd_a0,fd_a1,fd_a2,fd_a3,fd_a4,fd_a5,fd_a6,fd_a7,fd_a8,fd_a9,fd_aa,fd_ab,fd_ac,fd_ad,fd_ae,fd_af",0,0);
    I(0,dd,"fd_b0,fd_b1,fd_b2,fd_b3,fd_b4,fd_b5,fd_b6,fd_b7,fd_b8,fd_b9,fd_ba,fd_bb,fd_bc,fd_bd,fd_be,fd_bf",0,0);
    I(0,dd,"fd_c0,fd_c1,fd_c2,fd_c3,fd_c4,fd_c5,fd_c6,fd_c7,fd_c8,fd_c9,fd_ca,fd_cb,fd_cc,fd_cd,fd_ce,fd_cf",0,0);
    I(0,dd,"fd_d0,fd_d1,fd_d2,fd_d3,fd_d4,fd_d5,fd_d6,fd_d7,fd_d8,fd_d9,fd_da,fd_db,fd_dc,fd_dd,fd_de,fd_df",0,0);
    I(0,dd,"fd_e0,fd_e1,fd_e2,fd_e3,fd_e4,fd_e5,fd_e6,fd_e7,fd_e8,fd_e9,fd_ea,fd_eb,fd_ec,fd_ed,fd_ee,fd_ef",0,0);
    I(0,dd,"fd_f0,fd_f1,fd_f2,fd_f3,fd_f4,fd_f5,fd_f6,fd_f7,fd_f8,fd_f9,fd_fa,fd_fb,fd_fc,fd_fd,fd_fe,fd_ff",0,0);
    com(0);
    S("z_xx",0);
    I(0,dd,"xx_00,xx_01,xx_02,xx_03,xx_04,xx_05,xx_06,xx_07,xx_08,xx_09,xx_0a,xx_0b,xx_0c,xx_0d,xx_0e,xx_0f",0,0);
    I(0,dd,"xx_10,xx_11,xx_12,xx_13,xx_14,xx_15,xx_16,xx_17,xx_18,xx_19,xx_1a,xx_1b,xx_1c,xx_1d,xx_1e,xx_1f",0,0);
    I(0,dd,"xx_20,xx_21,xx_22,xx_23,xx_24,xx_25,xx_26,xx_27,xx_28,xx_29,xx_2a,xx_2b,xx_2c,xx_2d,xx_2e,xx_2f",0,0);
    I(0,dd,"xx_30,xx_31,xx_32,xx_33,xx_34,xx_35,xx_36,xx_37,xx_38,xx_39,xx_3a,xx_3b,xx_3c,xx_3d,xx_3e,xx_3f",0,0);
    I(0,dd,"xx_40,xx_41,xx_42,xx_43,xx_44,xx_45,xx_46,xx_47,xx_48,xx_49,xx_4a,xx_4b,xx_4c,xx_4d,xx_4e,xx_4f",0,0);
    I(0,dd,"xx_50,xx_51,xx_52,xx_53,xx_54,xx_55,xx_56,xx_57,xx_58,xx_59,xx_5a,xx_5b,xx_5c,xx_5d,xx_5e,xx_5f",0,0);
    I(0,dd,"xx_60,xx_61,xx_62,xx_63,xx_64,xx_65,xx_66,xx_67,xx_68,xx_69,xx_6a,xx_6b,xx_6c,xx_6d,xx_6e,xx_6f",0,0);
    I(0,dd,"xx_70,xx_71,xx_72,xx_73,xx_74,xx_75,xx_76,xx_77,xx_78,xx_79,xx_7a,xx_7b,xx_7c,xx_7d,xx_7e,xx_7f",0,0);
    I(0,dd,"xx_80,xx_81,xx_82,xx_83,xx_84,xx_85,xx_86,xx_87,xx_88,xx_89,xx_8a,xx_8b,xx_8c,xx_8d,xx_8e,xx_8f",0,0);
    I(0,dd,"xx_90,xx_91,xx_92,xx_93,xx_94,xx_95,xx_96,xx_97,xx_98,xx_99,xx_9a,xx_9b,xx_9c,xx_9d,xx_9e,xx_9f",0,0);
    I(0,dd,"xx_a0,xx_a1,xx_a2,xx_a3,xx_a4,xx_a5,xx_a6,xx_a7,xx_a8,xx_a9,xx_aa,xx_ab,xx_ac,xx_ad,xx_ae,xx_af",0,0);
    I(0,dd,"xx_b0,xx_b1,xx_b2,xx_b3,xx_b4,xx_b5,xx_b6,xx_b7,xx_b8,xx_b9,xx_ba,xx_bb,xx_bc,xx_bd,xx_be,xx_bf",0,0);
    I(0,dd,"xx_c0,xx_c1,xx_c2,xx_c3,xx_c4,xx_c5,xx_c6,xx_c7,xx_c8,xx_c9,xx_ca,xx_cb,xx_cc,xx_cd,xx_ce,xx_cf",0,0);
    I(0,dd,"xx_d0,xx_d1,xx_d2,xx_d3,xx_d4,xx_d5,xx_d6,xx_d7,xx_d8,xx_d9,xx_da,xx_db,xx_dc,xx_dd,xx_de,xx_df",0,0);
    I(0,dd,"xx_e0,xx_e1,xx_e2,xx_e3,xx_e4,xx_e5,xx_e6,xx_e7,xx_e8,xx_e9,xx_ea,xx_eb,xx_ec,xx_ed,xx_ee,xx_ef",0,0);
    I(0,dd,"xx_f0,xx_f1,xx_f2,xx_f3,xx_f4,xx_f5,xx_f6,xx_f7,xx_f8,xx_f9,xx_fa,xx_fb,xx_fc,xx_fd,xx_fe,xx_ff",0,0);
    com(0);
    com(0);
    S("z80_daa",0);
    for (i = 0; i < sizeof(DAATable)/sizeof(DAATable[0]); i += 4) {
        char buffer[32+1];
        sprintf(buffer,"%5d,%5d,%5d,%5d",
			(word)((DAATable[i+0]<<8)|(DAATable[i+0]>>8)),
			(word)((DAATable[i+1]<<8)|(DAATable[i+1]>>8)),
			(word)((DAATable[i+2]<<8)|(DAATable[i+2]>>8)),
			(word)((DAATable[i+3]<<8)|(DAATable[i+3]>>8)));
        I(0,dw,buffer,0,0);
    }
    I(0,globl,"_Z80_ICount",0,0);
	I("_Z80_ICount",dd,"0",0,"cycle count during run");
	I("z80_itotal",dd,"0",0,"total cycles to run");
	I("z80_iextrn",dd,"0",0,"external cycles to subtract (interrupt)");
    com(0);
    I(0,globl,"_Z80",0,0);
    S("_Z80",0);
    I("z80_AF",         dd,"0",0,"offset _Z80 + 0x00");
    I("z80_BC",         dd,"0",0,"offset _Z80 + 0x04");
    I("z80_DE",         dd,"0",0,"offset _Z80 + 0x08");
    I("z80_HL",         dd,"0",0,"offset _Z80 + 0x0c");
    I("z80_IX",         dd,"0",0,"offset _Z80 + 0x10");
    I("z80_IY",         dd,"0",0,"offset _Z80 + 0x14");
    I("z80_PC",         dd,"0",0,"offset _Z80 + 0x18");
    I("z80_SP",         dd,"0",0,"offset _Z80 + 0x1c");
    I("z80_AF2",        dd,"0",0,"offset _Z80 + 0x20");
    I("z80_BC2",        dd,"0",0,"offset _Z80 + 0x24");
    I("z80_DE2",        dd,"0",0,"offset _Z80 + 0x28");
    I("z80_HL2",        dd,"0",0,"offset _Z80 + 0x2c");
    I("z80_R",          db,"0",0,"offset _Z80 + 0x30");
    I("z80_R2",         db,"0",0,"offset _Z80 + 0x31");
    I("z80_IFF1",       db,"0",0,"offset _Z80 + 0x32");
    I("z80_IFF2",       db,"0",0,"offset _Z80 + 0x33");
    I("z80_HALT",       db,"0",0,"offset _Z80 + 0x34");
    I("z80_IM",         db,"0",0,"offset _Z80 + 0x35");
    I("z80_I",          db,"0",0,"offset _Z80 + 0x36");
    I("z80_pending_irq",db,"0",0,"offset _Z80 + 0x37");
    I("z80_vector",     dd,"0",0,"offset _Z80 + 0x38");
    I("z80_irq_max",    dd,"0",0,"offset _Z80 + 0x3c");
    I("z80_request_irq",dd,"0",0,"offset _Z80 + 0x40");
    I("z80_service_irq",dd,"0",0,"offset _Z80 + 0x44");
    I("z80_int_state",  dd,"0,0,0,0",0,"offset _Z80 + 0x48");
    I("z80_irq",        dd,"0,0,0,0",0,"offset _Z80 + 0x58");
    I(0,                dd,"0,0,0,0",0,"offset _Z80 + 0x68");
    I(0,                dd,"0,0,0,0",0,"offset _Z80 + 0x78");
    I(0,                dd,"0,0,0,0",0,"offset _Z80 + 0x88");
    I("z80_nmi_state",  dd,"0",0,"offset _Z80 + 0x98");
    I("z80_irq_state",  dd,"0",0,"offset _Z80 + 0x9c");
    I("z80_irq_callback",dd,"0",0,"offset _Z80 + 0xa0");
    #define Z80_SIZE    0x00a4

	I("z80_EA",         dd,"0",0,"temporary storage for effective address");
	I("z80_MB",         dd,"0",0,"temporary storage for memory byte (LSB or MSB)");
    com(0);
    if (mame_debug)
        I(0,extrn,"_mame_debug",0,0);
    if (verbose)
        I(0,extrn,"_errorlog",0,0);
    I(0,extrn,"_previouspc",0,0);
    I(0,extrn,"_ROM",0,0);
    I(0,extrn,"_RAM",0,0);
    I(0,extrn,"_OP_ROM",0,0);
    I(0,extrn,"_OP_RAM",0,0);
    com(0);
    I(0,text,0,0,0);
    I(0,extrn,"_malloc",0,0);
    I(0,extrn,"_raise",0,0);
    if (mame_debug)
        I(0,extrn,"_MAME_Debug",0,0);
    if (verbose)
        I(0,extrn,"_fprintf",0,0);
    com(0);
    I(0,extrn,"_ophw",0,0);
    I(0,extrn,"_cpu_bankbase",0,0);
	com(0);
    I(0,extrn,"_mrh_error",0,0);
    I(0,extrn,"_cur_mrhard",0,0);
    I(0,extrn,"_readhardware",0,0);
	I(0,extrn,"_memoryreadoffset",0,0);
	I(0,extrn,"_memoryreadhandler",0,0);
    I(0,extrn,"_cur_mwhard",0,0);
	com(0);
    I(0,extrn,"_mwh_error",0,0);
    I(0,extrn,"_writehardware",0,0);
	I(0,extrn,"_memorywriteoffset",0,0);
	I(0,extrn,"_memorywritehandler",0,0);
    I(0,extrn,"_cpu_setOPbase16",0,0);
    I(0,extrn,"_cpu_getactivecpu",0,0);
    com(0);
    I(0,extrn,"_cpu_readmem16",0,0);
    I(0,extrn,"_cpu_writemem16",0,0);
#if INLINE_IOPORTS
    I(0,extrn,"_cur_portmask",0,0);
    I(0,extrn,"_cur_readport",0,0);
    I(0,extrn,"_cur_writeport",0,0);
#else
    I(0,extrn,"_cpu_readport",0,0);
    I(0,extrn,"_cpu_writeport",0,0);
#endif
    com(0);
}

static void mk_interface(void)
{
    I(0,text,0,0,0);
	pad(align_funcs);
    com("===================================================================");
    com("  void Z80_SetRegs(void *regs);                                    ");
    com("===================================================================");
    I(0,globl,"_Z80_SetRegs",0,0);
    S("_Z80_SetRegs",0);
    I(0,pushl,  ESI,    0,                  0);
    I(0,pushl,  EDI,    0,                  0);
	I(0,movl,	ESI,	M(4,0,esp,12,0,0),	"get source");
debug(7,"Z80_SetRegs(0x%08x)\n",ESI);
    I(0,movl,   EDI,    O("_Z80"),          "set destination");
    I(0,movl,   ECX,    X(Z80_SIZE/4),      0);
    I(0,cld,    0,      0,                  0);
	I(0,rep,	0,		0,					0);
	I(0,movsd,	0,		0,					0);
	I(0,movl,	ECX,	zPC32,				0);
	I(0,movl,	EDI,	MRHARD, 			0);
    I(0,shrl,   ECX,    D(ABITS),           0);
	I(0,movb,	CL, 	M(1,0,edi,0,ecx,1), 0);
	I(0,cmpb,	CL, 	M(1,"_ophw",0,0,0,0),0);
	I(0,je, 	"z80_set_context_1",0,          0);
	I(0,pushl,	zPC32,	0,					0);
	I(0,call,	"_cpu_setOPbase16",0,       0);
	I(0,addl,	ESP,	B(4),				0);
	S("z80_set_context_1",0);
    dumpregs();
    I(0,popl,   EDI,    0,                  0);
    I(0,popl,   ESI,    0,                  0);
    I(0,ret,    0,      0,                  0);

	pad(align_funcs);
    com("===================================================================");
    com("  void Z80_GetRegs(void *regs);                                    ");
    com("===================================================================");
    I(0,globl,"_Z80_GetRegs",0,0);
    S("_Z80_GetRegs",0);
    I(0,pushl,  ESI,    0,                  0);
    I(0,pushl,  EDI,    0,                  0);
    I(0,movl,   EDI,    M(4,0,esp,12,0,0),  "get destination");
debug(7,"Z80_GetRegs(0x%08x)\n",EDI);
    I(0,movl,   ESI,    O("_Z80"),          "set source");
    I(0,movl,   ECX,    X(Z80_SIZE/4),      0);
    I(0,cld,    0,      0,                  0);
    I(0,rep,    0,                  0,      0);
    I(0,movsd,  0,                  0,      0);
    dumpregs();
    I(0,popl,   EDI,    0,                  0);
    I(0,popl,   ESI,    0,                  0);
    I(0,ret,    0,      0,                  0);

	pad(align_funcs);
    com("===================================================================");
    com("  unsigned Z80_GetPC(void);                                        ");
    com("===================================================================");
    I(0,globl,"_Z80_GetPC",0,0);
    S("_Z80_GetPC",0);
    I(0,movl,   EAX,    zPC32,                  0);
debug(7,"Z80_GetPC() %04x\n",EAX);
    I(0,ret,    0,      0,                      0);

	pad(align_funcs);
    com("===================================================================");
    com("  void Z80_set_nmi_line(int state);                                ");
    com("===================================================================");
    I(0,globl,"_Z80_set_nmi_line",0,0);
    S("_Z80_set_nmi_line",0);
	I(0,pushl,	EBP,			0,						0);
    I(0,pushl,  EBX,            0,                      0);
    I(0,pushl,  ESI,            0,                      0);
    I(0,pushl,  EDI,            0,                      0);
    I(0,movl,   ECX,            M(4,0,esp,20,0,0),      "get state");
debug(3,"Z80_set_nmi_line(%d)\n",ECX);
    I(0,cmpl,   ECX,            nmi_state,              "changed?");
    I(0,je,     LN("z80_set_nmi_line_done"),0,          "no, leave");
    I(0,movl,   nmi_state,      ECX,                    "store new state");
    I(0,cmpl,   ECX,            B(CLEAR_LINE),          "clear line now?");
    I(0,je,     LN("z80_set_nmi_line_done"),0,          "yes, skip");
    com(0);
	restore_z80();
	restore_af();
    I(0,movl,   PREVPC,         D(-1),                  "no valid previous PC");
    I(0,testb,  zHALT,          D(1),                   "halted?");
    I(0,je,     LS("z80_set_nmi_line_1"),0,             "no, skip");
    I(0,movb,   zHALT,          D(0),                   "clear halt flag");
	I(0,incw,	PC, 			0,						0);
	com(0);
    S("z80_set_nmi_line_1",0);
	I(0,call,	"z80_push_pc",  0,                      0);
	I(0,movb,	CL, 			X(0x00),				0);
	I(0,xchgb,	CL, 			zIFF1,					"clear IFF1 and copy");
	I(0,movb,	zIFF2,			CL, 					"previous state to IFF2");
	I(0,movl,	PC32,			X(0x66),				"PC = NMI vector");
	I(0,movl,	ECX,			PC32,					0);
	I(0,movl,	EDI,			MRHARD, 				0);
	I(0,shrl,	ECX,			D(ABITS),				0);
	I(0,movb,	CL, 			M(1,0,edi,0,ecx,1), 	0);
	I(0,cmpb,	CL, 			M(1,"_ophw",0,0,0,0),   0);
	I(0,je, 	LS("z80_set_nmi_line_2"),0,             0);
	I(0,pushl,	zPC32,			0,						0);
	I(0,call,	"_cpu_setOPbase16",0,                   0);
	I(0,addl,	ESP,			B(4),					0);
	S("z80_set_nmi_line_2",0);
	save_af();
	save_z80(0);
    com(0);
    S("z80_set_nmi_line_done",0);
    I(0,popl,   EDI,            0,                      0);
    I(0,popl,   ESI,            0,                      0);
    I(0,popl,   EBX,            0,                      0);
	I(0,popl,	EBP,			0,						0);
    I(0,ret,    0,              0,                  0);

	pad(align_funcs);
    com("===================================================================");
    com("  void Z80_set_irq_line(int irqline, int state);                   ");
    com("===================================================================");
    I(0,globl,"_Z80_set_irq_line",0,0);
    S("_Z80_set_irq_line",0);
    I(0,pushl,  EBP,            0,                  0);
    I(0,pushl,  EBX,            0,                  0);
    I(0,pushl,  ESI,            0,                  0);
    I(0,pushl,  EDI,            0,                  0);
    restore_af();
	restore_z80();
    I(0,movl,   EDX,            M(4,0,esp,24,0,0),  0);
    I(0,movl,   ECX,            M(4,0,esp,20,0,0),  0);
    I(0,movl,   irq_state,      EDX,                0);
debug(3,"Z80_set_irq_line(%d,%d)\n",ECX,EDX);
    I(0,cmpl,   EDX,            B(CLEAR_LINE),      "line cleared?");
    I(0,je,     LN("z80_set_irq_line_done"),0,      "yes, leave");
#if EARLY_CTC
    I(0,cmpl,   irq_max,        B(0),               "daisy chain mode?");
    I(0,je,     LN("z80_set_irq_line_assert"),0,    "no, it's single int");
    save_af();
    I(0,pushl,  D(0),           0,                  "irqline always 0");
    I(0,call,   irq_callback,   0,                  "get the vector");
    I(0,addl,   ESP,            B(4),               "clean up stack");
	I(0,movl,	ECX,			EAX,				"ECX = (device<<8)");
    I(0,movzbl, EDX,            AL,                 "EDX = state");
    restore_af();
    I(0,shrl,   ECX,            D(8),               "ECX = device");
    I(0,cmpl,   EDX,            int_state(ecx),     "state change?");
    I(0,je,     LN("z80_set_irq_line_assert"),0,    "no change, skip");
    I(0,movl,   int_state(ecx), EDX,                "store new state");
    I(0,movl,   EDX,            D(-1),              0);
    I(0,movl,   ECX,            D(0),               0);
    I(0,movl,   request_irq,    EDX,                "reset request IRQ");
    I(0,movl,   service_irq,    EDX,                "reset service IRQ");
    S("z80_set_irq_line_dc_1",0);
    I(0,testl,  int_state(ecx), X(Z80_INT_IEO),     "Z80_INT_IEO set?");
    I(0,je,     LS("z80_set_irq_line_dc_2"),0,      "no, skip");
    I(0,movl,   request_irq,    EDX,                "mask lower IRQ");
    I(0,movl,   service_irq,    ECX,                "set service device number");
    com(0);
    S("z80_set_irq_line_dc_2",0);
    I(0,testb,  int_state(ecx), X(Z80_INT_REQ),     "Z80_INT_REQ set?");
    I(0,je,     LS("z80_set_irq_line_dc_3"),0,      "no, skip");
    I(0,movl,   request_irq,    ECX,                "set request device number");
    com(0);
    S("z80_set_irq_line_dc_3",0);
    I(0,incl,   ECX,            0,                  "next device");
    I(0,cmpl,   ECX,            irq_max,            "more entries to check?");
    I(0,jb,     LS("z80_set_irq_line_dc_1"),0,      "yes, check them");
    S("z80_set_irq_line_assert",0);
#endif
    I(0,testb,  zIFF1,          X(INT_IRQ),         "interrupts enabled?");
    I(0,je,     LN("z80_set_irq_line_done"),0,      "no, leave");
    mk_take_irq( 0x00 );
    S("z80_set_irq_line_done",0);
    save_af();
	save_z80(0);
    I(0,popl,   EDI,            0,                  0);
    I(0,popl,   ESI,            0,                  0);
    I(0,popl,   EBX,            0,                  0);
    I(0,popl,   EBP,            0,                  0);
    I(0,ret,    0,              0,                  0);
    com(0);

	pad(align_funcs);
    com("===================================================================");
    com("  void Z80_set_irq_callback(int (*callback)(int irqline));         ");
    com("===================================================================");
    I(0,globl,"_Z80_set_irq_callback",0,0);
    S("_Z80_set_irq_callback",0);
    I(0,movl,   EAX, M(4,0,esp,4,0,0),  0);
debug(3,"Z80_set_irq_callback(%x)\n",EAX);
    I(0,movl,   irq_callback,   EAX,    0);
    I(0,ret,0,0,0);
    com(0);

	pad(align_funcs);
    com("===================================================================");
    com("  void Z80_Reset(Z80_DaisyChain *daisy_chain);                     ");
    com("===================================================================");
    I(0,globl,"_Z80_Reset",0,0);
    S("_Z80_Reset",0);
    I(0,pushl,  EBP,        0,                      0);
    I(0,pushl,  EBX,        0,                      0);
    I(0,pushl,  ESI,        0,                      0);
    I(0,pushl,  EDI,        0,                      0);
debug(1,"Z80_reset()\n");
    I(0,movl,   EAX,        D(0),                   0);
    I(0,movl,   EDI,        O("_Z80"),              "wipe out Z80 registers");
    I(0,movl,   ECX,        D(Z80_SIZE/4),          0);
    I(0,cld,    0,          0,                      0);
    I(0,rep,    0,          0,                      0);
    I(0,stosd,  0,          0,                      0);
    I(0,movl,   request_irq,    D(-1),              0);
    I(0,movl,   service_irq,    D(-1),              0);
    I(0,movl,   zSP32,      X(0xf000),              0);
    I(0,movl,   ESI,        M(4,0,esp,20,0,0),      "get daisy_chain");
    I(0,orl,    ESI,        ESI,                    "NULL ?");
    I(0,je,     LS("z80_reset_3"),0,                "yes, skip");
    I(0,movl,   EBX,        D(Z80_MAXDAISY),        0);
    I(0,movl,   EDI,        O("z80_irq"),           "copy to z80_irq++");

    pad(align_loops);
    S("z80_reset_1",0);
    I(0,cmpl,   M(4,0,esi,12,0,0),D(-1),            "more entries?");
    I(0,je,     LS("z80_reset_3"),0,                0);
    I(0,movl,   EAX,        M(4,0,esi,0,0,0),       "get Z80.irq[device].reset");
    I(0,cmp,    EAX,        D(0),                   "handler?");
    I(0,je,     LS("z80_reset_2"),0,                "no, skip");
    I(0,pushl,  M(4,0,esi,12,0,0),0,                "Z80.irq[device].irq_param");
    I(0,call,   EAX,        0,                      0);
    I(0,addl,   ESP,        B(4),                   0);
    S("z80_reset_2",0);
    I(0,cld,    0,          0,                      0);
    I(0,movsd,  0,          0,                      0);
    I(0,movsd,  0,          0,                      0);
    I(0,movsd,  0,          0,                      0);
    I(0,movsd,  0,          0,                      0);
    I(0,incl,   irq_max,    0,                      "count entries");
    I(0,decl,   EBX,        0,                      "more to check?");
    I(0,jne,    LS("z80_reset_1"),0,                0);
    S("z80_reset_3",0);
	I(0,movl,	ECX,	zPC32,				0);
	I(0,movl,	EDI,	MRHARD, 			0);
    I(0,shrl,   ECX,    D(ABITS),           0);
	I(0,movb,	CL, 	M(1,0,edi,0,ecx,1), 0);
	I(0,cmpb,	CL, 	M(1,"_ophw",0,0,0,0),0);
	I(0,je, 	"z80_reset_4",0,            0);
	I(0,pushl,	zPC32,	0,					0);
	I(0,call,	"_cpu_setOPbase16",0,       0);
	I(0,addl,	ESP,	B(4),				0);
	S("z80_reset_4",0);
    dumpregs();
    I(0,popl,   EDI,        0,                      0);
    I(0,popl,   ESI,        0,                      0);
    I(0,popl,   EBX,        0,                      0);
    I(0,popl,   EBP,        0,                      0);
    I(0,ret,    0,          0,                      0);
    com(0);

	pad(align_funcs);
    com("===================================================================");
    com("  int Z80_Execute(int cycles);                                     ");
    com("===================================================================");
    I(0,globl,"_Z80_Execute",0,0);
    S("_Z80_Execute",0);
    I(0,pushl,  EBP,            0,                  0);
    I(0,pushl,  EBX,            0,                  0);
    I(0,pushl,  ESI,            0,                  0);
    I(0,pushl,  EDI,            0,                  0);
    I(0,movl,   EAX,            M(4,0,esp,20,0,0),  "get the ICount");
	I(0,movl,	ECX,			D(0),				0);
	I(0,xchgl,	ECX,			zIEXTRN,			"external cycles (interrupt)");
	I(0,movl,	zITOTAL,		EAX,				"store total cycles");
    I(0,subl,   EAX,            ECX,                "subtract from cycle count");
	I(0,movl,	zICOUNT,		EAX,				"store cycles to run");
debug(6,"Z80_execute(%d)\n",EAX);
    restore_af();
	restore_z80();
    I(0,jmp,    "z80_next_opcode",0,                0);

	pad(align_funcs);
    com("===================================================================");
    com("  illegal opcode                                                   ");
    com("===================================================================");
    S("illop",0);
debug(5,"illegal opcode $%04x: %02x\n", PC32, EAX);

	pad(align_funcs);
    com("===================================================================");
    com("  fetch next opcode                                                ");
    com("===================================================================");
    S("z80_next_opcode",0);
    if (mame_debug) {
        com("===================================================================");
        com("  call MAME debugger                                               ");
        com("===================================================================");
        I(0,testb,  DEBUG,          D(1),           0);
        I(0,je,     LS("z80_no_debug"),0,           0);
        S("z80_mame_debug",0);
        mk_debug();
        S("z80_no_debug",0);
    }
    S("z80_next_opcode_1",0);
    ROP();
	I(0,jmp,	M(4,"z_op",0,0,ecx,4),0,    "jump to opcode handler");

	pad(align_funcs);
    S("z80_execute_done",0);
	save_z80(0);
    save_af();
    I(0,popl,   EDI,            0,          0);
    I(0,popl,   ESI,            0,          0);
    I(0,popl,   EBX,            0,          0);
    I(0,popl,   EBP,            0,          0);
    I(0,movl,   EAX,            zITOTAL,    0);
    I(0,subl,   EAX,            zICOUNT,    0);
debug(6,"Z80_execute() : %d\n", EAX);
    I(0,ret,    0,              0,          0);
    com(0);
}

static void mk_rbyte_handler( int i )
{
#if FAST_MEMORY
	#define rTBL	1	/* jump table */
	#define rRAM	2	/* read RAM */
	#define rNOP	3	/* read NOP */
	#define rBNK	4	/* read bank memory */
	#define rRHW	5	/* readhardware */
	#define rUSR	6	/* read user (handler) */
	#define rNON	7	/* read unassigned memory */

    int j;
    char buffer[128+1], symbol[8][31+1];
    static UINT8 mr[256] = {  /* MHELE (HT_XXX) to symbol mapping */
		rRAM,rBNK,rBNK,rBNK,rBNK,rBNK,rBNK,rBNK,rBNK,rNON,rNOP,rRAM,rRAM,rUSR,rUSR,rUSR,
		rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,
		rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,
		rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,rUSR,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,
		rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW,rRHW
    };

    sprintf(symbol[0], "rbyte%d",     i);
	sprintf(symbol[rTBL], "rbyte%d_tbl", i);
	sprintf(symbol[rRAM], "rbyte0_ram");       /* not different due to opsize */
	sprintf(symbol[rNOP], "rbyte0_nop");       /* not different due to opsize */
	sprintf(symbol[rBNK], "rbyte0_bnk");       /* not different due to opsize */
	sprintf(symbol[rRHW], "rbyte%d_rhw", i);
    sprintf(symbol[rUSR], "rbyte%d_usr", i);
	sprintf(symbol[rNON], "rbyte%d_non", i);

    com("===================================================================");
    com("  read a byte from memory at EBP to EDX for opcodes of size %d      ",i);
    com("===================================================================");
	pad(align_funcs);
    S(symbol[0],0);
    I(0,movl,   ECX,    EBP,                "memory address to ECX");
    I(0,movl,   EDI,    MRHARD,             "read flags");
    I(0,shrl,   ECX,    D(ABITS),           ">> (ABITS2_16 + ABITS_MIN_16)");
    I(0,movzbl, EDX,    M(1,0,edi,0,ecx,1), "get MHELE value");
	I(0,jmp,	M(4,symbol[rTBL],0,0,edx,4),0, "jump to memory read function");

	I(0,data,0,0,0);
	pad(align_funcs);
	S(symbol[rTBL],0);
    for (j = 0; j < 256; j += 4) {
        sprintf(buffer,"%s,%s,%s,%s",symbol[mr[j+0]], symbol[mr[j+1]],symbol[mr[j+2]], symbol[mr[j+3]]);
        I(0,dd,buffer,0,0);
    }
	I(0,text,0,0,0);

	if (i == 0) {	/* functions that are needed only once */
		pad(align_funcs);
		S(symbol[rRAM],"read HT_RAM, HT_RAMROM, HT_ROM");
		I(0,movl,	EDI,	RAM,				"get RAM/ROM base");
		I(0,movb,	DL, 	M(1,0,edi,0,ebp,1), "get byte from RAM/ROM");
		I(0,ret,	0,		0,					0);

		pad(align_funcs);
		S(symbol[rNOP],"read HT_NOP");
        I(0,movb,   DL,     D(0),               "return zero");
        I(0,ret,    0,      0,                  0);

		pad(align_funcs);
		S(symbol[rBNK],"read HT_BANK1 .. HT_BANK8");
		I(0,movl,	ECX,	EBP,				"get address to ECX");
		I(0,movl,	EDI,	BANKBASE(edx),		"get BANKn base");
        I(0,subl,   ECX,    READOFFSET(edx),    "subtract read offset");
		I(0,movb,	DL, 	M(1,0,edi,0,ecx,1), "get byte from BANKn");
		I(0,ret,	0,		0,					0);
	}

	pad(align_funcs);
	S(symbol[rRHW],"read MH_HARDMAX .. 255");
    I(0,movl,   EDI,    EDX,                "get hw to EDI");
    I(0,movl,   ECX,    EBP,                "address to ECX");
    I(0,shll,   EDI,    D(8),               "hw << MH_SBITS");
    I(0,andl,   ECX,    B(15),              "mask bits 0..3");
    I(0,movb,   DL,     READHW(edi,ecx),    "get MHELE");
	I(0,jmp,	M(4,symbol[rTBL],0,0,edx,4),0, "jump to read handler");

	pad(align_funcs);
	S(symbol[rUSR],"read HT_USER .. MH_HARDMAX-1");
    save_af();
    save_z80( i );
    I(0,movl,   ECX,    EBP,                "get address to ECX");
    I(0,subl,   ECX,    READOFFSET(edx),    "subtract read offset");
	I(0,movl,	EDI,	READHANDLER(edx),	"get memoryreadhandler");
    I(0,pushl,  ECX,    0,                  "offset");
	I(0,call,	EDI,	0,					"call handler");
    I(0,movl,   EDX,    EAX,                "return value");
    I(0,addl,   ESP,    B(1*4),             "one DWORD was pushed");
    restore_z80();
    restore_af();
    I(0,ret,    0,      0,                  0);

	pad(align_funcs);
	S(symbol[rNON],"read HT_NON");
    save_af();
    save_z80( i );
    I(0,pushl,  EBP,    0,                  "memory address");
    I(0,call,   "_mrh_error",0,             0);
    I(0,movl,   EDX,    EAX,                "return value");
    I(0,addl,   ESP,    B(1*4),             "one DWORD was pushed");
    restore_z80();
    restore_af();
    I(0,ret,    0,      0,                  0);

#else
	char symbol[2][31+1];
    sprintf(symbol[0], "rbyte%d", i);
    sprintf(symbol[1], "rbyte%d_user", i);

    pad(align_funcs);
    com("===================================================================");
    com("  read a byte from memory at EBP to EDX for opcodes of size %d      ",i);
    com("===================================================================");
    S(symbol[0],0);
    I(0,movl,   ECX,    EBP,                "memory address to ECX");
    I(0,movl,   EDI,    MRHARD,             "read flags");
    I(0,shrl,   ECX,    D(ABITS),           ">> (ABITS2_16 + ABITS_MIN_16)");
    I(0,testb,  M(1,0,edi,0,ecx,1),D(-1),   "ROM/RAM?");
    I(0,jne,    LS(symbol[1]),0,            "nope, need to call handler");
    I(0,movl,   EDI,    RAM,                "get RAM base");
    I(0,movzbl, EDX,    M(1,0,edi,0,ebp,1), "get byte from RAM");
    I(0,ret,    0,      0,                  0);

    pad(align_jumps);
    S(symbol[1],0);
    save_af();
    save_z80( i );
    I(0,pushl,  EBP,    0,                  "memory address");
    I(0,call,   "_cpu_readmem16",0,         0);
    I(0,movl,   EDX,    EAX,                "return value");
    I(0,addl,   ESP,    B(1*4),             "one DWORD was pushed");
    restore_z80();
    restore_af();
    I(0,ret,    0,      0,                  0);
#endif
}

static void mk_rword_handler( int i )
{
	char symbol[3][15+1];
    sprintf(symbol[0], "rword%d",       i);
	sprintf(symbol[1], "rword%d_usr",   i);
	sprintf(symbol[2], "rbyte%d",       i);

	pad(align_funcs);
	com("===================================================================");
	com("  read a word from memory at EBP to EDX for opcodes of size %d      ",i);
    com("===================================================================");
    S(symbol[0],0);
	I(0,movl,	ECX,	EBP,				"memory address to ECX");
	I(0,orl,	ECX,	B(15),				"check if address...");
	I(0,cmpl,	ECX,	EBP,				"...is 0xxxFh");
	I(0,je, 	LS(symbol[1]),0,			"yes, need to read LSB then MSB");
	I(0,shrl,	ECX,	D(4),				">> (ABITS2_16 + ABITS_MIN_16)");
	I(0,movl,	EDI,	MRHARD, 			"read flags");
	I(0,testb,	M(1,0,edi,0,ecx,1),D(-1),	"MHELE == ROM/RAM?");
	I(0,jne,	LS(symbol[1]),0,			"nope, need to call handler");
	pad(align_funcs);
    I(0,movl,   EDI,    RAM,                "get RAM base");
	I(0,movzwl, EDX,	M(2,0,edi,0,ebp,1), "get word from ROM/RAM");
	I(0,ret,	0,		0,					0);

	pad(align_funcs);
	S(symbol[1],0);
	I(0,call,	symbol[2],	0,				"read LSB");
	I(0,incw,	BP, 	0,					"address + 1");
	I(0,movb,	zMB,	DL, 				"save LSB value");
	I(0,call,	symbol[2],	0,				"read MSB");
	I(0,shll,	EDX,	D(8),				"value to MSB");
	I(0,movb,	DL, 	zMB,				"get LSB");
	I(0,ret,	0,		0,					0);
}

static void mk_wbyte_handler( int i )
{
#if FAST_MEMORY
	#define wTBL   1	/* jump table */
    #define wRAM   2    /* write RAM */
	#define wNOP   3	/* don't write (NOP or ROM) */
	#define wRAR   4	/* write RAM and ROM */
	#define wBNK   5	/* write bank memory */
	#define wWHW   6	/* writehardware */
	#define wUSR   7	/* write user (handler) */
	#define wNON   8	/* write unassigned memory */
    int j;
	char buffer[128+1], symbol[9][31+1];
    static UINT8 mw[256] = {  /* MHELE (HT_XXX) to symbol mapping */
		wRAM,wBNK,wBNK,wBNK,wBNK,wBNK,wBNK,wBNK,wBNK,wNON,wNOP,wRAR,wNOP,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,
		wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,wUSR,
		wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,
		wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,
		wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,
		wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,
		wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,
		wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW,wWHW
    };

    sprintf(symbol[0], "wbyte%d",     i);
	sprintf(symbol[wTBL], "wbyte%d_tbl", i);
	sprintf(symbol[wRAM], "wbyte0_ram");       /* not different due to opsize */
    sprintf(symbol[wNOP], "wbyte0_nop");       /* not different due to opsize */
	sprintf(symbol[wRAR], "wbyte0_rar");       /* not different due to opsize */
	sprintf(symbol[wBNK], "wbyte0_bnk");       /* not different due to opsize */
    sprintf(symbol[wWHW], "wbyte%d_whw", i);
    sprintf(symbol[wUSR], "wbyte%d_usr", i);
	sprintf(symbol[wNON], "wbyte%d_non", i);

    com("===================================================================");
    com("  write a byte from EDX to memory at EBP for opcodes of size %d     ",i);
    com("===================================================================");
	pad(align_funcs);
    S(symbol[0],0);
    I(0,movl,   ECX,    EBP,                "memory address to ECX");
    I(0,movl,   EDI,    MWHARD,             "write flags");
    I(0,shrl,   ECX,    D(ABITS),           ">> (ABITS2_16 + ABITS_MIN_16)");
    I(0,movzbl, EDI,    M(1,0,edi,0,ecx,1), "get MHELE value");
	I(0,jmp,	M(4,symbol[wTBL],0,0,edi,4),0, "jump to memory write function");

	I(0,data,0,0,0);
	pad(align_funcs);
	S(symbol[wTBL],0);
    for (j = 0; j < 256; j += 4) {
        sprintf(buffer,"%s,%s,%s,%s",symbol[mw[j+0]], symbol[mw[j+1]],symbol[mw[j+2]], symbol[mw[j+3]]);
        I(0,dd,buffer,0,0);
    }
	I(0,text,0,0,0);

	if (i == 0) {	/* functions that are needed only once */
		pad(align_funcs);
		S(symbol[wRAM],"HT_RAM");
		I(0,movl,	EDI,	RAM,				"get RAM base");
		I(0,movb,	M(1,0,edi,0,ebp,1), DL, 	"put byte into RAM");
		I(0,ret,	0,		0,					0);

		pad(align_funcs);
		S(symbol[wNOP],"HT_NOP");
        I(0,ret,    0,      0,                  "do nothing");

		pad(align_funcs);
		S(symbol[wRAR],"HT_RAMROM");
		I(0,movl,	EDI,	RAM,				"get RAM base");
		I(0,movl,	ECX,	ROM,				"get ROM base");
		I(0,movb,	M(1,0,edi,0,ebp,1), DL, 	"put byte into RAM");
		I(0,movb,	M(1,0,ecx,0,ebp,1), DL, 	"put byte into ROM");
        I(0,ret,    0,      0,                  0);

		pad(align_funcs);
		S(symbol[wBNK],"HT_BANK1 - HT_BANK8");
		I(0,movl,	ECX,	EBP,				"get address int0 ECX");
		I(0,subl,	ECX,	WRITEOFFSET(edi),	"subtract write offset");
		I(0,movl,	EDI,	BANKBASE(edi),		"get BANKn base");
		I(0,movb,	M(1,0,edi,0,ecx,1), DL, 	"put byte to BANKn + offset");
		I(0,ret,	0,		0,					0);
	}

	pad(align_funcs);
	S(symbol[wWHW],"MH_HARDMAX..255");
    I(0,movl,   ECX,    EBP,                "get address to ECX");
    I(0,shll,   EDI,    D(8),               "hw << MH_SBITS");
    I(0,andl,   ECX,    B(15),              "mask bits 0..3");
    I(0,movzbl, EDI,    WRITEHW(edi,ecx),   "get MHELE");
	I(0,jmp,	M(4,symbol[wTBL],0,0,edi,4),0, "jump to write handler");

	pad(align_funcs);
	S(symbol[wUSR],"HT_USER .. MH_HARDMAX-1");
    save_af();
    save_z80( i );
    I(0,movl,   ECX,    EBP,                "get address to ECX");
    I(0,pushl,  EDX,    0,                  "data byte");
    I(0,subl,   ECX,    WRITEOFFSET(edi),   "subtract write offset");
    I(0,movl,   EDI,    WRITEHANDLER(edi),  "get memorywritehandler");
    I(0,pushl,  ECX,    0,                  "offset");
	I(0,call,	EDI,	0,					"call handler");
    I(0,addl,   ESP,    B(2*4),             "two DWORDs were pushed");
    restore_z80();
    restore_af();
    I(0,ret,    0,      0,                  0);

	pad(align_funcs);
    S(symbol[wNON],"HT_NON");
    save_af();
    save_z80( i );
    I(0,pushl,  EDX,    0,                  "data byte");
    I(0,pushl,  EBP,    0,                  "memory address");
    I(0,call,   "_mwh_error",0,             0);
    I(0,addl,   ESP,    B(2*4),             "two DWORDs were pushed");
    restore_z80();
    restore_af();
    I(0,ret,    0,      0,                  0);

#else
	char symbol[2][31+1];
    sprintf(symbol[0], "wbyte%d", i);
    sprintf(symbol[1], "wbyte%d_user", i);
    pad(align_funcs);
    com("===================================================================");
    com("  write a byte from EDX to memory at EBP for opcodes of size %d     ",i);
    com("===================================================================");
    S(symbol[0],0);
    I(0,movl,   ECX,    EBP,                "get memory address to ECX");
    I(0,movl,   EDI,    MWHARD,             "write flags");
    I(0,shrl,   ECX,    D(ABITS),           ">> (ABITS2_16 + ABITS_MIN_16)");
    I(0,movzbl, ECX,    M(1,0,edi,0,ecx,1), "get MHELE");
    I(0,orb,    CL,     CL,                 "write to RAM?");
    I(0,jne,    LS(symbol[1]),0,            "nope, need to call writemem16");
    I(0,movl,   EDI,    RAM,                "get RAM base");
    I(0,movb,   M(1,0,edi,0,ebp,1),DL,      "store byte to RAM");
    I(0,ret,    0,      0,                  "return to caller");

    pad(align_jumps);
    S(symbol[1],0);
    save_af();
    save_z80( i );
    I(0,pushl,  EDX,    0,                  "value");
    I(0,pushl,  EBP,    0,                  "memory address");
    I(0,call,   "_cpu_writemem16",0,        0);
    I(0,addl,   ESP,    B(2*4),             "two DWORDs were pushed");
    restore_z80();
    restore_af();
    I(0,ret,    0,      0,                  "return to caller");
#endif
}

static void mk_wword_handler( int i )
{
	char symbol[3][15+1];

    sprintf(symbol[0], "wword%d",       i);
	sprintf(symbol[1], "wword%d_usr",   i);
	sprintf(symbol[2], "wbyte%d",       i);
	pad(align_funcs);
	com("===================================================================");
    com("  write a word from EDX to memory at ECX for opcodes of size %d     ",i);
    com("===================================================================");
    S(symbol[0],0);
	I(0,movl,	ECX,	EBP,				"memory address to ECX");
	I(0,orb,	CL, 	X(0x0f),			"check if address...");
	I(0,cmpl,	ECX,	EBP,				"...is 0xxxFh");
	I(0,je, 	LS(symbol[1]),0,			"yes, need to write LSB then MSB");
	I(0,movl,	EDI,	MWHARD, 			"write flags");
    I(0,shrl,   ECX,    D(ABITS),           ">> (ABITS2_16 + ABITS_MIN_16)");
	I(0,testb,	M(1,0,edi,0,ecx,1),D(-1),	"MHELE == RAM?");
	I(0,jne,	LS(symbol[1]),0,			"nope, need to call handler");
	pad(align_funcs);
	I(0,movl,	EDI,	RAM,				"get RAM base");
	I(0,movw,	M(2,0,edi,0,ebp,1),DX,		"store word to RAM");
	I(0,ret,	0,		0,					0);

	pad(align_funcs);
	S(symbol[1],0);
	I(0,movb,	zMB,	DH, 				"save MSB of data");
	I(0,movzbl, EDX,	DL, 				"EDX = LSB only");
	I(0,call,	symbol[2],	0,				"write LSB");
	I(0,incw,	BP, 	0,					"address + 1");
	I(0,movzbl, EDX,	zMB,				"EDX = MSB only");
	I(0,call,	symbol[2],	0,				"write MSB");
	I(0,ret,	0,		0,					0);
}

static void mk_memory_handlers( void )
{
	int i;

	for (i = 0; i <= 4*CALC_PREVPC; i++) {
		mk_rbyte_handler(i);
        mk_rword_handler(i);
        mk_wbyte_handler(i);
		mk_wword_handler(i);
    }

	com(0);
    com("===================================================================");
	com("  push PC to stack (called from interrupt handlers)                ");
	com("===================================================================");
	pad(align_funcs);
	S("z80_push_pc",0);
	I(0,subw,	zSP,	B(2),				0);
	I(0,movzwl, EDX,	PC, 				"get PC to EDX");
	I(0,movzwl, EBP,	zSP,				"get SP to EBP");
	I(0,jmp,	"wword0",  0,               "write word");

    com(0);
    com("===================================================================");
	com("  write back EDX to address EBP (DD/FD CB <ofs> xx opcodes)        ");
	com("===================================================================");
	pad(align_funcs);
	S("wb_ea4_edx",0);
#if CALC_PREVPC
    I(0,call,   "wbyte4",0,                 "write the value in EDX");
#else
	I(0,call,	"wbyte0",0,                 "write the value in EDX");
#endif
    end_opcode("z_op");
}

void M_INC( const char *dst )
{
	I(0,shrb,	F,		D(1),			"save carry");
	I(0,incb,	dst,	0,				0);
    I(0,lahf,   0,      0,              0);
	I(0,seto,	CL, 	0,				0);
    I(0,shlb,   CL,     D(2),           "get overflow to P/V bit position");
	I(0,andb,	F,		X(SF|ZF|HF|CF), "mask S,Z,H and C");
	I(0,orb,	F,		CL, 			"combine the flags");
}

void M_DEC( const char *dst )
{
	I(0,shrb,	F,		D(1),			"save carry");
	I(0,decb,	dst,	0,				0);
    I(0,lahf,   0,      0,              0);
	I(0,seto,	CL, 	0,				0);
    I(0,shlb,   CL,     D(2),           "get overflow to P/V bit position");
	I(0,andb,	F,		X(SF|ZF|HF|CF), "mask S,Z,H and C");
	I(0,orb,	F,		X(NF),			"set N");
	I(0,orb,	F,		CL, 			"combine the flags");
}

void M_ADD16( const char *dst, const char *src )
{
	I(0,movw,	DX, 	dst,			"get destination");
	I(0,movw,	CX, 	src,			"get source");
	I(0,addb,	DL, 	CL, 			0);
	I(0,adcb,	DH, 	CH, 			0);
	I(0,movb,	CL, 	F,				"get old flags");
    I(0,lahf,   0,      0,              0);
	I(0,andb,	CL, 	X(SF|ZF|PF),	"mask S,Z and P/V");
    I(0,movw,   dst,    DX,             "store result");
	I(0,andb,	F,		X(HF|CF),		"mask H and C");
	I(0,orb,	F,		CL, 			"set old flag bits");
}

void M_ADD( const char *src )
{
	I(0,addb,	A,		src,			"add source");
    I(0,lahf,   0,      0,              0);
	I(0,seto,	CL, 	0,				0);
    I(0,shlb,   CL,     D(2),           "get overflow to P/V bit position");
	I(0,andb,	F,		X(SF|ZF|HF|CF), "mask S,Z,H and C");
	I(0,orb,	F,		CL, 			"combine the flags");
}

void M_ADC( const char *src )
{
	I(0,sahf,	0,		0,				"set flags");
	I(0,adcb,	A,		src,			"add source with carry");
    I(0,lahf,   0,      0,              0);
	I(0,seto,	CL, 	0,				0);
    I(0,shlb,   CL,     D(2),           "get overflow to P/V bit position");
	I(0,andb,	F,		X(SF|ZF|HF|CF), "mask S,Z,H and C");
	I(0,orb,	F,		CL, 			"combine the flags");
}

void M_SUB( const char *src )
{
	I(0,subb,	A,		src,			"subtract source");
    I(0,lahf,   0,      0,              0);
	I(0,seto,	CL, 	0,				0);
    I(0,shlb,   CL,     D(2),           "get overflow to P/V bit position");
	I(0,andb,	F,		X(SF|ZF|HF|CF), "mask S,Z,H and C");
	I(0,orb,	F,		X(NF),			"set N");
	I(0,orb,	F,		CL, 			"combine the flags");
}

void M_SBC( const char *src )
{
	I(0,sahf,	0,		0,				"set the flags");
	I(0,sbbb,	A,		src,			"subtract source with carry");
    I(0,lahf,   0,      0,              0);
	I(0,seto,	CL, 	0,				0);
    I(0,shlb,   CL,     D(2),           "get overflow to P/V bit position");
	I(0,andb,	F,		X(SF|ZF|HF|CF), "mask S,Z and C");
	I(0,orb,	F,		X(NF),			"set N");
	I(0,orb,	F,		CL, 			"combine the flags");
}

void M_AND( const char *src )
{
	I(0,andb,	A,		src,			"logical and with source");
	I(0,lahf,	0,		0,				0);
	I(0,andb,	F,		X(SF|ZF|PF),	"mask S,Z and P/V");
	I(0,orb,	F,		X(HF),			"set H");
}

void M_XOR( const char *src )
{
	I(0,xorb,	A,		src,			"logical exclusive or with source");
    I(0,lahf,   0,      0,              0);
	I(0,andb,	F,		X(SF|ZF|PF),	"mask S,Z and P/V");
}

void M_OR( const char *src )
{
	I(0,orb,	A,		src,			"logical or with source");
	I(0,lahf,	0,		0,				0);
	I(0,andb,	F,		X(SF|ZF|PF),	"mask S,Z and P/V flags");
}

void M_CP( const char *src )
{
	I(0,cmpb,	A,		src,			"compare with source");
	I(0,lahf,	0,		0,				0);
	I(0,seto,	CL, 	0,				0);
    I(0,shlb,   CL,     D(2),           "get overflow to P/V bit position");
	I(0,andb,	F,		X(SF|ZF|HF|CF), "mask S,Z,H and C flags");
	I(0,orb,	F,		X(NF),			"set N");
	I(0,orb,	F,		CL, 			"combine the flags");
}

#if BUSY_LOOP_HACKS
void M_JR( int opcode )
{
    char symbol[4][32+1];
	int i;
	for (i = 0; i < 4; i++)
		sprintf(symbol[i],"op_%02x_%d", opcode, i);
	I(0,movl,	EDX,		PC32,		"ECX = old PC");
	I(0,decl,	EDX,		0,			"- 1");
	REL();
	I(0,addl,	PC32,		ECX,		"change PC");
	I(0,cmpl,	EDX,		PC32,		"busy loop (tight) ?");
	I(0,jne,	LS(symbol[0]),0,		"no, skip");
	I(0,orl,	zICOUNT,	B(0),		"more cycles left?");
	I(0,jle,	LN(symbol[3]),0,		"no, skip");
	I(0,movl,	zICOUNT,	D(0),		"bail out");
	I(0,jmp,	symbol[3],	0,			0);

    pad(align_jumps);
	S(symbol[0],0);
	I(0,decl,	EDX,		0,			"EDX = oldpc - 1");
	I(0,movl,	EDI,		OP_ROM, 	"get OP_ROM base");
	I(0,movb,	CL, 		M(1,0,edi,0,pc32,1), "read opcode byte");
	I(0,cmpl,	EDX,		PC32,		"busy loop (1 byte opcode) ?");
	I(0,jne,	LS(symbol[2]),0,		"no, skip");
	I(0,cmpb,	CL, 		X(0x00),	"NOP - JR xxxx ?");
	I(0,jne,	LS(symbol[1]),0,		"no, skip");
	I(0,orl,	zICOUNT,	B(0),		"more cycles left?");
	I(0,jle,	LS(symbol[3]),0,		"nope, leave");
	I(0,movl,	zICOUNT,	D(0),		"bail out");
	I(0,jmp,	symbol[3],	0,			0);

    pad(align_jumps);
	S(symbol[1],0);
	I(0,cmpb,	CL, 		X(0xfb),	"EI - JR xxxx ?");
	I(0,jne,	LS(symbol[2]),0,		"no, skip");
	I(0,cmpl,	zICOUNT,	B(4),		"more than four cycles left?");
	I(0,jle,	LS(symbol[3]),0,		"nope, leave");
	I(0,movl,	zICOUNT,	D(4),		"bail out after EI");
	I(0,jmp,	symbol[3],	0,			0);

    pad(align_jumps);
	S(symbol[2],0);
	I(0,cmpb,	CL, 		X(0x31),	"LD SP,xxxx - JR xxxx ?");
	I(0,jne,	LS(symbol[3]),0,		"no, skip");
    I(0,subl,   EDX,        B(2),       "EDX = oldpc - 3");
	I(0,cmpl,	EDX,		PC32,		"busy loop (3 byte opcode) ?");
	I(0,jne,	LS(symbol[3]),0,		"no, skip");
	I(0,cmpl,	zICOUNT,	B(10),		"more than ten cycles left?");
	I(0,jle,	LS(symbol[3]),0,		"nope, leave");
	I(0,movl,	zICOUNT,	D(10),		"bail out after LD SP,xxxx");
	I(0,jmp,	LS(symbol[3]),0,		0);

    pad(align_jumps);
	S(symbol[3],0);
	CHANGE_PC();
}
#else
void M_JR( int opcode )
{
	REL();
	I(0,addl,	PC32,		ECX,		"change PC");
	CHANGE_PC();
}
#endif

void M_JR_CC( int opcode, int flag )
{
	char symbol[32+1];
	sprintf(symbol,"op_%02x_take", opcode);
	if (flag) {
		I(0,sahf,	0,		0,				"set flags");
		switch (opcode & 0x38) {
			case 0x20: I(0,jnz, LS(symbol),0,"Z == 0: take jump");  break;
			case 0x28: I(0,jz,	LS(symbol),0,"Z == 1: take jump");  break;
			case 0x30: I(0,jnc, LS(symbol),0,"C == 0: take jump");  break;
			case 0x38: I(0,jc,	LS(symbol),0,"C == 1: take jump");  break;
		}
		ADJUST_PC(+1);
		end_opcode("z_op");
	}

    pad(align_jumps);
	S(symbol,"take the relative jump");
	REL();
	I(0,addl,	PC32,		ECX,	"change PC");
    I(0,subl,   zICOUNT,B(5),       "five additional cycles");
	CHANGE_PC();
}

void M_DJNZ( int opcode )
{
	char symbol[32+1];
	sprintf(symbol,"op_%02x_take    ", opcode);
	I(0,decb,	zB, 	0,			"decrement B");
	I(0,jne,	LS(symbol),0,		"not zero, take the jump");
	ADJUST_PC(+1);
	end_opcode("z_op");

    pad(align_jumps);
	S(symbol,"take the relative jump");
	REL();
	I(0,addl,	PC32,		ECX,	"change PC");
    I(0,subl,   zICOUNT,B(5),       "five additional cycles");
	CHANGE_PC();
}

void M_HALT( int opcode )
{
	char symbol[32+1];
    sprintf(symbol,"op_%02x_done", opcode);
	I(0,movb,	zHALT,		B(1),		"set HALT flag");
	ADJUST_PC(-1);
	I(0,orl,	zICOUNT,	B(0),		"cycle count <= zero already?");
	I(0,jle,	LS(symbol), 0,			0);
	I(0,movl,	zICOUNT,	D(0),		0);
	S(symbol,0);
}

void M_RET( int opcode )
{
	POP( 1, PC32 );
	CHANGE_PC();
}

void M_RET_CC( int opcode )
{
	char symbol[32+1];
	sprintf(symbol,"op_%02x_done", opcode);
	I(0,sahf,	0,		0,			"set flags");
	switch (opcode & 0x38) {
		case 0x00: I(0,jne,    LS(symbol),0,"Z == 0: return");  break;
		case 0x08: I(0,je,	   LS(symbol),0,"Z == 1: return");  break;
		case 0x10: I(0,jnc,    LS(symbol),0,"C == 0: return");  break;
		case 0x18: I(0,jc,	   LS(symbol),0,"C == 1: return");  break;
		case 0x20: I(0,jpo,    LS(symbol),0,"P == 0: return");  break;
		case 0x28: I(0,jpe,    LS(symbol),0,"P == 1: return");  break;
		case 0x30: I(0,jns,    LS(symbol),0,"S == 0: return");  break;
		case 0x38: I(0,js,	   LS(symbol),0,"S == 1: return");  break;
	}
	end_opcode("z_op");

    pad(align_jumps);
	S(symbol,"execute the return");
	I(0,subl,	zICOUNT,B(6),		"six additional cycles");
	M_RET( opcode );
}

#if BUSY_LOOP_HACKS
void M_JP( int opcode )
{
	char symbol[4][32+1];
	int i;
	for (i = 0; i < 4; i++)
		sprintf(symbol[i],"op_%02x_%d", opcode, i);
	I(0,movl,	ECX,		PC32,		"ECX = PC");
	I(0,decl,	ECX,		0,			"- 1");
	ARG16( PC32 );
	I(0,cmpl,	ECX,		PC32,		"busy loop (tight) ?");
	I(0,jne,	LS(symbol[0]),0,		"no, skip");
	I(0,orl,	zICOUNT,	B(0),		"more cycles left?");
	I(0,jle,	LN(symbol[3]),0,		"no, skip");
	I(0,movl,	zICOUNT,	D(0),		"bail out");
	I(0,jmp,	symbol[3],	0,			0);
	pad(align_jumps);
	S(symbol[0],0);
	I(0,decl,	ECX,		0,			"ECX = oldpc - 1");
	I(0,movl,	EDI,		OP_ROM, 	"get OP_ROM base");
	I(0,movb,	DL, 		M(1,0,edi,0,pc32,1), "read opcode byte");
	I(0,cmpl,	ECX,		PC32,		"busy loop (1 byte opcode) ?");
	I(0,jne,	LS(symbol[2]),0,		"no, skip");
	I(0,cmpb,	DL, 		X(0x00),	"NOP - JP xxxx ?");
	I(0,jne,	LS(symbol[1]),0,		"no, skip");
	I(0,orl,	zICOUNT,	B(0),		"more cycles left?");
	I(0,jle,	LS(symbol[3]),0,		"nope, leave");
	I(0,movl,	zICOUNT,	D(0),		"bail out");
	I(0,jmp,	symbol[3],	0,			0);
	pad(align_jumps);
	S(symbol[1],0);
	I(0,cmpb,	DL, 		X(0xfb),	"EI - JP xxxx ?");
	I(0,jne,	LS(symbol[2]),0,		"no, skip");
	I(0,cmpl,	zICOUNT,	B(4),		"more than four cycles left?");
	I(0,jle,	LS(symbol[3]),0,		"nope, leave");
	I(0,movl,	zICOUNT,	D(4),		"bail out after EI");
	I(0,jmp,	symbol[3],	0,			0);
	pad(align_jumps);
	S(symbol[2],0);
	I(0,cmpb,	DL, 		X(0x31),	"LD SP,xxxx - JP xxxx ?");
	I(0,jne,	LS(symbol[3]),0,		"no, skip");
	I(0,subl,	ECX,		B(2),		"ECX = oldpc - 3");
	I(0,cmpl,	ECX,		PC32,		"busy loop (3 byte opcode) ?");
	I(0,jne,	LS(symbol[3]),0,		"no, skip");
	I(0,cmpl,	zICOUNT,	B(10),		"more than ten cycles left?");
	I(0,jle,	LS(symbol[3]),0,		"nope, leave");
	I(0,movl,	zICOUNT,	D(10),		"bail out after LD SP,xxxx");
	I(0,jmp,	LS(symbol[3]),0,		0);
	pad(align_jumps);
	S(symbol[3],0);
	CHANGE_PC();
}
#else
void M_JP( int opcode )
{
	ARG16( PC32 );
	CHANGE_PC();
}
#endif

void M_JP_CC( int opcode )
{
	char symbol[32+1];
	sprintf(symbol,"op_%02x_take", opcode);
	I(0,sahf,	0,		0,			"set flags");
	switch (opcode & 0x38) {
		case 0x00: I(0,jne,    LS(symbol),0,"Z == 0: jump");    break;
		case 0x08: I(0,je,	   LS(symbol),0,"Z == 1: jump");    break;
		case 0x10: I(0,jnc,    LS(symbol),0,"C == 0: jump");    break;
		case 0x18: I(0,jc,	   LS(symbol),0,"C == 1: jump");    break;
		case 0x20: I(0,jpo,    LS(symbol),0,"P == 0: jump");    break;
		case 0x28: I(0,jpe,    LS(symbol),0,"P == 1: jump");    break;
		case 0x30: I(0,jns,    LS(symbol),0,"S == 0: jump");    break;
		case 0x38: I(0,js,	   LS(symbol),0,"S == 1: jump");    break;
	}
	ADJUST_PC(+2);
	end_opcode("z_op");

    pad(align_jumps);
	S(symbol,"take the jump");
	ARG16( PC32 );
	CHANGE_PC();
}

void M_JP_REG( int opcode, const char *reg )
{
	I(0,movl,	PC32,	reg,  0);
	CHANGE_PC();
}

void M_CALL( int opcode )
{
	ARG16( zEA );
	PUSH( 3, PC32 );
	I(0,movl,	PC32,	zEA,		"get the new PC");
	CHANGE_PC();
}

void M_CALL_CC( int opcode )
{
	char symbol[32+1];
	sprintf(symbol,"op_%02x_take", opcode);
	I(0,sahf,	0,		0,			"set flags");
	switch (opcode & 0x38) {
		case 0x00: I(0,jne,    LS(symbol),0,"Z == 0: call");    break;
		case 0x08: I(0,je,	   LS(symbol),0,"Z == 1: call");    break;
		case 0x10: I(0,jnc,    LS(symbol),0,"C == 0: call");    break;
		case 0x18: I(0,jc,	   LS(symbol),0,"C == 1: call");    break;
		case 0x20: I(0,jpo,    LS(symbol),0,"P == 0: call");    break;
		case 0x28: I(0,jpe,    LS(symbol),0,"P == 1: call");    break;
		case 0x30: I(0,jns,    LS(symbol),0,"S == 0: call");    break;
		case 0x38: I(0,js,	   LS(symbol),0,"S == 1: call");    break;
    }
	ADJUST_PC(+2);
    end_opcode("z_op");

    pad(align_jumps);
	S(symbol,"make the call");
	ARG16( zEA );
	PUSH( 3, PC32 );
	I(0,movl,	PC32,	zEA,		"get new PC");
    I(0,subl,   zICOUNT,B(7),       "seven additional cycles");
	CHANGE_PC();
}

void M_RST( int opcode )
{
	PUSH( 1, PC32 );
	I(0,movl,	PC32,	X(opcode & 0x38),  0);
	CHANGE_PC();
}

void M_EX_AF_AF2( void )
{
	I(0,xchgb,	F,		zF2,	0);
	I(0,xchgb,	A,		zA2,	0);
}

void M_EXX( void )
{
	I(0,movzwl, ECX,	zBC,	0);
	I(0,xchgw,	CX, 	zBC2,	0);
	I(0,movw,	zBC,	CX, 	0);
	I(0,movzwl, ECX,	zDE,	0);
	I(0,xchgw,	CX, 	zDE2,	0);
	I(0,movw,	zDE,	CX, 	0);
	I(0,xchgw,	HL, 	zHL2,	0);
}

void M_EX_DE_HL( void )
{
	I(0,xchgw,	HL, 	zDE,	0);
}

void M_RLCA( void )
{
	I(0,shrb,	F,		D(1),			"knock out C");
	I(0,rolb,	A,		D(1),			0);
	I(0,adcb,	F,		F,				"get C");
	I(0,andb,	F,		X(~(HF|NF)&255),"remove H and N flags");
}

void M_RRCA( void )
{
	I(0,shrb,	F,		D(1),			"knock out C");
	I(0,rorb,	A,		D(1),			0);
	I(0,adcb,	F,		F,				"get C");
	I(0,andb,	F,		X(~(HF|NF)&255),"remove H and N flags");
}

void M_RLA( void )
{
	I(0,shrb,	F,		D(1),			"C into carry");
	I(0,rclb,	A,		D(1),			0);
	I(0,adcb,	F,		F,				"get C");
	I(0,andb,	F,		X(~(HF|NF)&255),"remove H and N flags");
}

void M_RRA( void )
{
	I(0,shrb,	F,		D(1),			"C into carry");
	I(0,rcrb,	A,		D(1),			0);
	I(0,adcb,	F,		F,				"get C");
	I(0,andb,	F,		X(~(HF|NF)&255),"remove H and N flags");
}

void M_CPL( void )
{
	I(0,xorb,	A,	X(0xff),			"one's complement accumulator");
	I(0,orb,	F,	X(HF|NF),			"set H and N");
}

void M_DAA( int opcode )
{
	I(0,movzbl, ECX,	A,				"get accu");
	I(0,rorb,	F,		D(2),			"check N flag");
	I(0,adcb,	CH, 	CH, 			0);
	I(0,rorb,	F,		D(3),			"check H flag");
	I(0,adcb,	CH, 	CH, 			0);
	I(0,rorb,	F,		D(5),			"check C flag");
	I(0,adcb,	CH, 	CH, 			0);
	I(0,movzwl, AF32,	zDAA(ecx),		"get DAA table entry");
}

void M_SCF( void )
{
	I(0,andb,	F,	X(~(HF|NF)&255),"clear H and N");
    I(0,orb,    F,  X(CF),          "set C");
}

void M_CCF( void )
{
	I(0,movb,	CL, 	F,			0);
	I(0,andb,	F,		X(~(HF|NF)),"clear H and N");
	I(0,andb,	CL, 	X(CF),		"mask C");
	I(0,shlb,	CL, 	D(4),		"move C to H");
	I(0,orb,	CL, 	X(CF),		"set C");
	I(0,xorb,	F,		CL, 		"toggle C, set/leave H");
}

void M_DI( void )
{
	I(0,movb,	zIFF1,	D(0),	"clear both interrupt flip-flops");
	I(0,movb,	zIFF2,	D(0),	0);
}

void M_EI( int opcode )
{
	char symbol[32+1];
	sprintf(symbol, "op_%02x_skip", opcode);
	I(0,movb,	CL, 			X(0xff),		0);
	I(0,movb,	zIFF2,			CL, 			"set interrupt flip-flop 2");
	I(0,cmpb,	CL, 			zIFF1,			"interrupt flip-flop 1 already set?");
	I(0,je, 	LN(symbol), 	0,				"yes, skip");
	I(0,movb,	zIFF1,			CL, 			"set IFF1");
	I(0,cmpl,	irq_state,		B(CLEAR_LINE),	"irq line clear?");
	I(0,je, 	LN(symbol), 	0,				"yes, skip");
	save_af();
	save_z80(2);
	I(0,movl,	ESI,			zICOUNT,		"save remaining cycle count");
	I(0,movl,	EDI,			zITOTAL,		"and total cycle count");
	I(0,pushl,	D(1),			0,				"execute one opcode");
	I(0,call,	"_Z80_Execute", 0,              "recursive.. eww!");
	I(0,movl,	zITOTAL,		EDI,			"restore total cycle count");
	I(0,movl,	zICOUNT,		ESI,			"restore remaining cycle count");
    I(0,addl,   ESP,            B(4),           "clean up stack");
	I(0,subl,	zICOUNT,		EAX,			"subtract cycles used");
	restore_z80();
	restore_af();
	mk_take_irq( opcode );
	I(0,movl,	ECX,			D(0),			0);
	I(0,xchgl,	ECX,			zIEXTRN,		"interrupt used cycles");
	I(0,subl,	zICOUNT,		ECX,			"subtract from cycle count");
	com(0);
    pad(align_jumps);
	S(symbol,0);
}

void M_RLC( const char *reg )
{
	I(0,rolb,	reg,	D(1),			"rotate left circular");
	I(0,setc,	CL, 	0,				"CL = 1 if carry set");
	I(0,orb,	reg,	D(0),			"check sign, zero and parity");
    I(0,lahf,   0,      0,              0);
	I(0,andb,	F,		X(SF|ZF|PF),	"mask S, Z and P/V flags");
	I(0,orb,	F,		CL, 			"combine with C");
}

void M_RRC( const char *reg )
{
	I(0,rorb,	reg,	D(1),			"rotate right circular");
	I(0,setc,	CL, 	0,				"CL = 1 if carry set");
	I(0,orb,	reg,	D(0),			"check sign, zero and parity");
    I(0,lahf,   0,      0,              0);
	I(0,andb,	F,		X(SF|ZF|PF),	"mask S, Z and P/V flags");
	I(0,orb,	F,		CL, 			"combine with C");
}

void M_RL( const char *reg )
{
	I(0,shrb,	F,		D(1),			"get C into carry");
	I(0,rclb,	reg,	D(1),			"rotate left through carry");
	I(0,setc,	CL, 	0,				"CL = 1 if carry set");
    I(0,orb,    reg,    D(0),           0);
    I(0,lahf,   0,      0,              0);
	I(0,andb,	F,		X(SF|ZF|PF),	"mask S,Z and P/V flags");
	I(0,orb,	F,		CL, 			"combine with C");
}

void M_RR( const char *reg )
{
	I(0,shrb,	F,		D(1),			"get C into carry");
	I(0,rcrb,	reg,	D(1),			"rotate right through carry");
	I(0,setc,	CL, 	0,				"CL = 1 if carry set");
	I(0,orb,	reg,	D(0),			"check sign, zero and parity");
    I(0,lahf,   0,      0,              0);
	I(0,andb,	F,		X(SF|ZF|PF),	"mask S,Z and P/V flags");
	I(0,orb,	F,		CL, 			"combine with C");
}

void M_SLL( const char *reg )
{
	I(0,shlb,	reg,	D(1),			"shift left logical");
    I(0,lahf,   0,      0,              0);
	I(0,andb,	F,		X(SF|PF|CF),	"mask S, P/V and C flags");
	I(0,orb,	reg,	D(1),			"set bit 0 of destination");
}

void M_SRL( const char *reg )
{
	I(0,shrb,	reg,	D(1),			"shift right logical");
    I(0,lahf,   0,      0,              0);
	I(0,andb,	F,		X(SF|ZF|PF|CF), "mask S, Z, P/V and C flags");
}

void M_SLA( const char *reg )
{
	I(0,salb,	reg,	D(1),			"shift left arithmetical (well..)");
    I(0,lahf,   0,      0,              0);
	I(0,andb,	F,		X(SF|ZF|PF|CF), "mask S, Z, P/V and C flags");
}

void M_SRA( const char *reg )
{
	I(0,sarb,	reg,	D(1),			"shift right arithmetical");
    I(0,lahf,   0,      0,              0);
	I(0,andb,	F,		X(SF|ZF|PF|CF), "mask S, Z, P/V and C flags");
}

void M_BIT( int n, const char *reg )
{
	I(0,movb,	CL, 	F,				"save flags");
    I(0,movb,   CH,     X(1<<n),        "bit mask");
	I(0,andb,	CL, 	X(CF),			"keep C flag");
	I(0,orb,	CL, 	X(HF),			"set H flag");
    I(0,andb,   CH,     reg,            "check sign and zero");
    I(0,lahf,   0,      0,              0);
	I(0,andb,	F,		X(SF|ZF),		"mask S and Z flags");
	I(0,orb,	F,		CL, 			"combine with C");
}

void M_RES( int n, const char *reg )
{
	I(0,andb,	reg,	X(~(1<<n)&255), "reset bit mask");
}

void M_SET( int n, const char *reg )
{
	I(0,orb,	reg,	X(1<<n),		"set bit mask");
}

void M_OUT( int opsize, const char *port, const char *reg )
{
#if INLINE_IOPORTS
	static int wport = 0;
	char symbol[3][7+1];
	int i;
	wport++;
	for (i = 0; i < 3; i++)
		sprintf(symbol[i], "WP%03x_%d", wport, i);
	if (strcmp(reg, EDX))
		I(0,movzbl, EDX,	reg,			"output register");
	if (strcmp(port, ECX))
		I(0,movl,	ECX,	port,			"output port");
	save_af();
    save_z80(opsize);
    I(0,movl,   EDI,    M(4,"_cur_writeport",0,0,0,0),0);
	I(0,orl,	EDI,	EDI,				0);
	I(0,je, 	LS(symbol[2]),0,			0);
    I(0,andl,   ECX,    M(4,"_cur_portmask",0,0,0,0),0);
	pad(align_jumps);
	S(symbol[0],0);
	I(0,cmpl,	M(4,0,edi,0,0,0),	D(-1),	"end of table?");
	I(0,je, 	LS(symbol[2]),0,			0);
	I(0,cmpl,	ECX,	M(4,0,edi,0,0,0),	"port < start?");
	I(0,jb, 	LS(symbol[1]),0,			0);
	I(0,cmpl,	ECX,	M(4,0,edi,4,0,0),	"port > end?");
	I(0,ja, 	LS(symbol[1]),0,			0);
	I(0,movl,	EBX,	M(4,0,edi,8,0,0),	"get handler");
	I(0,orl,	EBX,	EBX,				"IOWP_NOP ?");
	I(0,je, 	LS(symbol[2]),0,			0);
	I(0,subl,	ECX,	M(4,0,edi,0,0,0),	"subtract start");
	I(0,pushl,	EDX,	0,					"value");
	I(0,pushl,	ECX,	0,					"offset");
	I(0,call,	EBX,	0,					"call handler");
	I(0,addl,	ESP,	B(8),				"two DWORDs were pushed");
    I(0,jmp,    LS(symbol[2]),0,            0);
	pad(align_jumps);
	S(symbol[1],0);
	I(0,addl,	EDI,	B(12),				"skip sizeof(IOWritePort)");
	I(0,jmp,	LS(symbol[0]),0,			"test next entry");
	pad(align_jumps);
    S(symbol[2],0);
	restore_z80();
	restore_af();
#else
	if (strcmp(reg,EDX))
		I(0,movzbl, EDX,	reg,				"output register");
	if (strcmp(port,ECX))
		I(0,movl,	ECX,	port,				"port");
	save_af();
    save_z80(opsize);
    I(0,pushl,  EDX,    0,                  "value");
	I(0,pushl,	ECX,	0,					"port");
	I(0,call,	"_cpu_writeport",0,         0);
	I(0,addl,	ESP,	B(8),				"two DWORDs were pushed");
	restore_z80();
	restore_af();
#endif
}

void M_IN( int opsize, const char *reg, const char *port )
{
#if INLINE_IOPORTS
	static int wport = 0;
	char symbol[3][7+1];
	int i;
	wport++;
	for (i = 0; i < 3; i++)
		sprintf(symbol[i], "RP%03x_%d", wport, i);
	if (strcmp(port,ECX))
		I(0,movl,	ECX,	port,			"output port");
	save_af();
    save_z80(opsize);
    I(0,movl,   EDI,    M(4,"_cur_readport",0,0,0,0),0);
	I(0,orl,	EDI,	EDI,				0);
	I(0,je, 	LS(symbol[2]),0,			0);
    I(0,andl,   ECX,    M(4,"_cur_portmask",0,0,0,0),0);
	pad(align_jumps);
	S(symbol[0],0);
	I(0,cmpl,	M(4,0,edi,0,0,0),	D(-1),	"end of table?");
	I(0,je, 	LS(symbol[2]),0,			0);
	I(0,cmpl,	ECX,	M(4,0,edi,0,0,0),	"port < start?");
	I(0,jb, 	LS(symbol[1]),0,			0);
	I(0,cmpl,	ECX,	M(4,0,edi,4,0,0),	"port > end?");
	I(0,ja, 	LS(symbol[1]),0,			0);
	I(0,movl,	EAX,	M(4,0,edi,8,0,0),	"get handler");
	I(0,orl,	EAX,	EAX,				"IORP_NOP ?");
	I(0,je, 	LS(symbol[2]),0,			0);
	I(0,subl,	ECX,	M(4,0,edi,0,0,0),	"subtract start");
	I(0,pushl,	ECX,	0,					"offset");
	I(0,call,	EAX,	0,					"call handler");
	I(0,addl,	ESP,	B(4),				0);
	I(0,jmp,	LS(symbol[2]),0,			0);
	pad(align_jumps);
	S(symbol[1],0);
	I(0,addl,	EDI,	B(12),				"skip sizeof(IOReadPort)");
	I(0,jmp,	LS(symbol[0]),0,			"test next entry");
	pad(align_jumps);
    S(symbol[2],0);
	I(0,movzbl, EDX,	AL, 				0);
	restore_z80();
	restor_af();
#else
	if (strcmp(port,ECX))
        I(0,movl,   ECX,    port,           "output port");
	save_af();
    save_z80(opsize);
    I(0,pushl,  ECX,    0,              0);
	I(0,call,	"_cpu_readport",0,      0);
	I(0,addl,	ESP,	B(4),			0);
	I(0,movzbl, EDX,	AL, 				0);
    restore_z80();
	restore_af();
#endif
	if (strcmp(reg, EDX) && strcmp(reg, DL))
        I(0,movb,   reg,    DL,             "input register");
}

void M_OUT_BC( const char *reg )
{
	I(0,movzbl, EDX,	reg,			"get value to EDX");
	M_OUT( 2, zBC32, EDX );
}

void M_IN_BC( const char *reg )
{
	M_IN( 2, EDX, zBC32 );
	I(0,movb,	CL, 	F,				"save flags");
	I(0,andb,	CL, 	X(CF),			"mask C flag");
	I(0,orb,	DL, 	DL, 			"check sign, zero and parity");
	I(0,lahf,	0,		0,				0);
    I(0,andb,   F,      X(SF|ZF|PF),    "mask S, Z and P/V flags");
	if (strcmp(reg, DL))
		I(0,movb,	reg,	DL, 			0);
	I(0,orb,	F,		CL, 			"combine with carry");
}

void M_ADC16( const char *src )
{
	I(0,movw,	DX, 	src,			"get source");
	I(0,sahf,	0,		0,				"set flags");
	I(0,adcb,	L,		DL, 			"add LSB");
	I(0,adcb,	H,		DH, 			"add with carry MSB");
	I(0,seto,	CL, 	0,				"CL = 1 if overflow");
    I(0,lahf,   0,      0,              0);
    I(0,shlb,   CL,     D(2),           "shift overflow to P/V bit position");
	I(0,andb,	F,		X(SF|HF|CF),	"mask S, H and C flags");
	I(0,orb,	F,		CL, 			"combine with P/V");
	I(0,cmpl,	HL32,	B(0),			"HL = zero?");
	I(0,setz,	CL, 	0,				"CL = 1 if zero");
	I(0,shlb,	CL, 	D(6),			"shift zero to Z bit position");
	I(0,orb,	F,		CL, 			"combine with Z");
}

void M_SBC16( const char *src )
{
	I(0,movw,	DX, 	src,			"get source");
	I(0,sahf,	0,		0,				"set flags");
	I(0,sbbb,	L,		DL, 			0);
	I(0,sbbb,	H,		DH, 			0);
    I(0,seto,   CL,     0,              "F = 1 if overflow");
	I(0,lahf,	0,		0,				0);
    I(0,shlb,   CL,     D(2),           "shift overflow to P/V bit position");
	I(0,andb,	F,		X(SF|HF|CF),	"mask S, H and C flags");
	I(0,orb,	F,		CL, 			"combine with P/V");
	I(0,cmpl,	HL32,	B(0),			"HL = zero?");
	I(0,setz,	CL, 	0,				"CL = 1 if zero");
    I(0,orb,    F,      X(NF),          "set N");
	I(0,shlb,	CL, 	D(6),			"shift zero to Z bit position");
	I(0,orb,	F,		CL, 			"combine with Z");
}

void M_NEG( void )
{
	I(0,movb,	CL, 	A,				"get accumulator");
	I(0,movb,	A,		D(0),			"subtract from zero");
	I(0,subb,	A,		CL, 			0);
	I(0,lahf,	0,		0,				0);
	I(0,seto,	CL, 	0,				"F = 1 if overflow");
    I(0,shlb,   CL,     D(2),           "get overflow to P/V bit position");
	I(0,andb,	F,		X(SF|ZF|HF|CF), "mask S, Z, H and C flags");
	I(0,orb,	F,		X(NF),			"set N");
	I(0,orb,	F,		CL, 			"combine with P/V");
}

void M_RETI( int opcode )
{
	char symbol[32+1];
	sprintf(symbol,"ed_%02x_skip", opcode);

    POP( 2, PC32 );
	CHANGE_PC();
    I(0,movl,   ECX,    service_irq,    0);
	I(0,cmpl,	ECX,	B(0),			0);
	I(0,jl, 	LS(symbol),0,			0);
	save_af();
	save_z80(2);
	I(0,lea,	ECX,	M(0,0,0,0,ecx,4),"* sizeof(Z80_DaisyChain)");
	I(0,pushl,	irq(ecx,12),0,			"Z80.irq[device].irq_param");
	I(0,call,	irq(ecx, 8),0,			"Z80.irq[device].interrupt_reti");
    I(0,addl,   ESP,    B(4),           0);
	restore_z80();
    restore_af();
	pad(align_jumps);
	S(symbol,0);
}

void M_RETN( int opcode )
{
	char symbol[32+1];
	sprintf(symbol,"ed_%02x_skip", opcode);

	POP( 2, PC32 );
	CHANGE_PC();
    I(0,movb,   CL,             zIFF2,      "get interrupt flip-flop 2");
	I(0,orb,	CL, 			CL, 		"will RETN enable interrupts?");
	I(0,je, 	LN(symbol),0,				"no, skip");
	I(0,movb,	zIFF1,			CL, 		"copy to interrupt flip-flop 1");
	I(0,cmpl,	irq_state,		B(CLEAR_LINE),"irq line cleared?");
	I(0,je, 	LN(symbol),0,				"yes, skip");
	mk_take_irq( opcode );
	I(0,movl,	ECX,			D(0),			0);
	I(0,xchgl,	ECX,			zIEXTRN,		"interrupt used cycles");
    I(0,subl,   zICOUNT,        ECX,            "subtract from cycle count");
    pad(align_jumps),
	S(symbol,0);
}

void M_LD_I_A( void )
{
	I(0,movb,	zI, 	A,	   0);
}

void M_LD_A_I( void )
{
	I(0,movb,	CL, 	F,				"save flags");
    I(0,movb,   A,      zI,             0);
	I(0,andb,	CL, 	X(CF),			"mask C flag");
	I(0,orb,	A,		A,				"check sign and zero");
	I(0,lahf,	0,		0,				0);
	I(0,andb,	F,		X(SF|ZF),		"mask S and Z flags");
	I(0,orb,	F,		CL, 			"combine with C");
    I(0,movb,   CL,     zIFF2,          "get interrupt flip-flop 2");
	I(0,andb,	CL, 	X(PF),			"shift to P/V bit position");
	I(0,orb,	F,		CL, 			"get IFF2 into P/V");
}

void M_LD_R_A( void )
{
	I(0,movb,	CL, 	A,				0);
	I(0,movb,	zR, 	CL, 			"store accu into R");
	I(0,andb,	CL, 	X(0x80),		"mask bit 7");
	I(0,movb,	zR2,	CL, 			"save in R2");
}

void M_LD_A_R( void )
{
	I(0,movb,	CL, 	F,				"save flags");
	I(0,movb,	A,		zR, 			0);
	I(0,andb,	A,		X(0x7f),		"mask lower 7 bits");
    I(0,andb,   CL,     X(CF),          "mask C flag");
	I(0,orb,	A,		zR2,			"get bit 7 from original R");
	I(0,lahf,	0,		0,				0);
	I(0,andb,	F,		X(SF|ZF),		"mask S and Z flags");
	I(0,orb,	F,		CL, 			"combine with C");
    I(0,movb,   CL,     zIFF2,          "get interrupt flip-flop 2");
	I(0,andb,	CL, 	X(PF),			"shift to P/V bit position");
    I(0,orb,    F,      CL,             "get IFF2 into P/V");
}

void M_RLD( void )
{
	RBYTE( 2, EDX, HL32 );
	I(0,movb,	CH, 	A,				"get accu into CH");
	I(0,movb,	CL, 	F,				"save flags");
	I(0,andl,	ECX,	X(0x0f00|CF),	"mask lower nibble and keep C flag");
	I(0,shll,	EDX,	D(4),			"shift memory byte left one nibble");
	I(0,xorb,	A,		CH, 			"remove lower nibble from accu");
	I(0,orb,	DL, 	CH, 			"put lower nibble of A into lower nibble of result");
	I(0,orb,	A,		DH, 			"put upper nibble of result into lower nibble of A");
	I(0,lahf,	0,		0,				0);
	I(0,andb,	F,		X(SF|ZF|PF),	"mask S, Z and P/V flags");
	I(0,orb,	F,		CL, 			"combine with C");
	I(0,movzbl, EDX,	DL, 			"EDX = result");
	WBYTE( 2, HL32, EDX );
}

void M_RRD( void )
{
	RBYTE( 2, EDX, HL32 );
	I(0,movb,	DH, 	A,				"put A into MSB of result");
	I(0,rorw,	DX, 	D(4),			"DL = result, DH 4..7 = lower nibble of memory");
	I(0,movb,	CL, 	F,				"save flags");
	I(0,shrb,	DH, 	D(4),			"DH 0..3 = lower nibble of memory");
	I(0,andb,	CL, 	X(CF),			"keep C flag");
	I(0,andb,	A,		X(0xf0),		"keep upper nibble of A");
	I(0,orb,	A,		DH, 			"set lower nibble from memory");
	I(0,lahf,	0,		0,				0);
	I(0,andb,	F,		X(SF|ZF|PF),	"mask S, Z and P/V flags");
	I(0,orb,	F,		CL, 			"combine flags");
	I(0,movzbl, EDX,	DL, 			"EDX = result");
	WBYTE( 2, HL32, EDX );
}

const char *s(char *fmt, ...)
{
	va_list arg;
    static char buffer[128];
	va_start(arg, fmt);
	vsprintf(buffer, fmt, arg);
	va_end(arg);
	return buffer;
}

void mk_opcodes(int op_base)
{
	char op[32];
	char hl[2+1], h[2+1], l[2+1], m[8+1];
	char zhl[32+1], zh[32+1], zl[32+1], zhl32[32+1];
	int mc = 0, mi = 0, mr = 0;

	if (op_base == 0xdd00) {
		strcpy(op,"dd_%02x");   /* opcode label mask */
		strcpy(hl,"IX");        /* word register name */
		strcpy(h,"HX");         /* MSB byte register name */
		strcpy(l,"LX");         /* LSB byte register name */
		strcpy(m,"(IX+ofs8)");  /* memory register name */
		strcpy(zh,zHX); 		/* MSB byte register */
		strcpy(zl,zLX); 		/* LSB byte register */
		strcpy(zhl,zIX);		/* word register */
		strcpy(zhl32,zIX32);	/* dword register */
		mc = 8; 				/* additional cycles for r/w (IX+ofs8) */
        mi = 5;                 /* additional cycles for w (IX+ofs8),imm8 */
        mr = 1;                 /* additional cycles for accessing IXL/IXH */
    } else
	if (op_base == 0xfd00) {
		strcpy(op,"fd_%02x");   /* opcode label mask */
		strcpy(hl,"IY");        /* word register name */
		strcpy(h,"HY");         /* MSB byte register name */
		strcpy(l,"LY");         /* LSB byte register name */
		strcpy(m,"(IY+ofs8)");  /* memory register name */
		strcpy(zh,zHY); 		/* MSB byte register */
		strcpy(zl,zLY); 		/* LSB byte register */
		strcpy(zhl,zIY);		/* word register */
		strcpy(zhl32,zIY32);	/* dword register */
		mc = 8; 				/* additional cycles for r/w (IY+ofs8) */
		mi = 5; 				/* additional cycles for w (IY+ofs8),imm8 */
		mr = 1; 				/* additional cycles for accessing IXL/IXH */
    } else {
		strcpy(op,"op_%02x");   /* opcode label mask */
		strcpy(hl,"HL");        /* word register name */
		strcpy(h,"H");          /* MSB byte register name */
		strcpy(l,"L");          /* LSB byte register name */
		strcpy(m,"(HL)");       /* memory register name */
		strcpy(zh,H);			/* MSB byte register */
		strcpy(zl,L);			/* LSB byte register */
		strcpy(zhl,HL); 		/* word register */
		strcpy(zhl32,HL32); 	/* dword register */
    }

	beg_opcode( 4,s(op,0x00),"NOP");
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x08),"EX   AF,AF'");
    M_EX_AF_AF2();
	end_opcode("z_op");

	beg_opcode( 8,s(op,0x10),"DJNZ ofs8");
    M_DJNZ( op_base|0x10 );
	end_opcode("z_op");

	beg_opcode(12,s(op,0x18),"JR   ofs8");
    M_JR( op_base|0x18 );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0x20),"JR   NZ,ofs8");
    M_JR_CC( op_base|0x20, ZF );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0x28),"JR   Z,ofs8");
    M_JR_CC( op_base|0x28, ZF );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0x30),"JR   NC,ofs8");
    M_JR_CC( op_base|0x30, CF );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0x38),"JR   C,ofs8");
    M_JR_CC( op_base|0x38, CF );
	end_opcode("z_op");

/*********/

    beg_opcode(10,s(op,0x01),"LD   BC,imm16");
	ARG16( zBC32 );
	end_opcode("z_op");

	beg_opcode(10,s(op,0x11),"LD   DE,imm16");
    ARG16( zDE32 );
	end_opcode("z_op");

	beg_opcode(10,s(op,0x21),"LD   %s,imm16",hl);
    ARG16( zhl32 );
	end_opcode("z_op");

	beg_opcode(10,s(op,0x31),"LD   SP,imm16");
	ARG16( zSP32 );
	end_opcode("z_op");

/*********/

    beg_opcode(11,s(op,0x09),"ADD  %s,BC",hl);
	M_ADD16( zhl, zBC );
	end_opcode("z_op");

	beg_opcode(11,s(op,0x19),"ADD  %s,DE",hl);
	M_ADD16( zhl, zDE );
	end_opcode("z_op");

	beg_opcode(11,s(op,0x29),"ADD  %s,%s",hl,hl);
	M_ADD16( zhl, zhl );
	end_opcode("z_op");

	beg_opcode(11,s(op,0x39),"ADD  %s,SP",hl);
	M_ADD16( zhl, zSP );
	end_opcode("z_op");

/*********/

    beg_opcode( 7,s(op,0x02),"LD   (BC),A");
	I(0,movzbl, EDX,	A,		0);
	WBYTE( 1, zBC32, EDX );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0x0a),"LD   A,(BC)");
	RBYTE( 1, EDX, zBC32 );
	I(0,movb,	A,		DL, 	0);
	end_opcode("z_op");

    beg_opcode( 7,s(op,0x12),"LD   (DE),A");
	I(0,movzbl, EDX,	A,		0);
	WBYTE( 1, zDE32, EDX );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0x1a),"LD   A,(DE)");
	RBYTE( 1, EDX, zDE32 );
	I(0,movb,	A,		DL, 	0);
	end_opcode("z_op");

    beg_opcode(16,s(op,0x22),"LD   (imm16),%s",hl);
	ARG16( EBP );
	WWORD( (op_base)?4:3, EBP, zhl32 );
	end_opcode("z_op");

	beg_opcode(16,s(op,0x2a),"LD   %s,(imm16)",hl);
	ARG16( EBP );
	RWORD( (op_base)?4:3, zhl32, EBP );
	end_opcode("z_op");

	beg_opcode(13,s(op,0x32),"LD   (imm16),A");
	ARG16( EBP );
	I(0,movzbl, EDX,	A,		0);
	WBYTE( 3, EBP, EDX );
	end_opcode("z_op");

    beg_opcode(13,s(op,0x3a),"LD   A,(imm16)");
	ARG16( EBP );
	RBYTE( 3, EDX, EBP );
	I(0,movb,	A,		DL, 	0);
	end_opcode("z_op");

/*********/

    beg_opcode( 6,s(op,0x03),"INC  BC");
	I(0,incw,	zBC,	0,		0);
	end_opcode("z_op");

    beg_opcode( 6,s(op,0x0b),"DEC  BC");
    I(0,decw,   zBC,    0,      0);
	end_opcode("z_op");

	beg_opcode( 6,s(op,0x13),"INC  DE");
    I(0,incw,   zDE,    0,      0);
	end_opcode("z_op");

    beg_opcode( 6,s(op,0x1b),"DEC  DE");
    I(0,decw,   zDE,    0,      0);
	end_opcode("z_op");

	beg_opcode( 6,s(op,0x23),"INC  %s",hl);
    I(0,incw,   zhl,    0,      0);
	end_opcode("z_op");

    beg_opcode( 6,s(op,0x2b),"DEC  %s",hl);
    I(0,decw,   zhl,    0,      0);
	end_opcode("z_op");

	beg_opcode( 6,s(op,0x33),"INC  SP");
	I(0,incw,	zSP,	0,		0);
	end_opcode("z_op");

	beg_opcode( 6, s(op,0x3b),"DEC  SP");
	I(0,decw,	zSP,	0,		0);
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0x04),"INC  B");
	M_INC( zB );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x0c),"INC  C");
	M_INC( zC );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x14),"INC  D");
	M_INC( zD );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x1c),"INC  E");
	M_INC( zE );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x24),"INC  %s",h);
    M_INC( zh );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x2c),"INC  %s",l);
    M_INC( zl );
	end_opcode("z_op");

	beg_opcode(11+mc,s(op,0x34),"INC  %s",m);
    if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
        M_INC( DL );
		WBYTE( 3, EBP, EDX );
    } else {
		RBYTE( 1, EDX, HL32 );
        M_INC( DL );
		WBYTE( 1, HL32, EDX );
    }
	end_opcode("z_op");

    beg_opcode( 4,s(op,0x3c),"INC  A");
    M_INC( A );
	end_opcode("z_op");

/*********/

	beg_opcode( 4,s(op,0x05),"DEC  B");
    M_DEC( zB );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x0d),"DEC  C");
    M_DEC( zC );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x15),"DEC  D");
    M_DEC( zD );
	end_opcode("z_op");

    beg_opcode( 4,s(op,0x1d),"DEC  E");
    M_DEC( zE );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x25),"DEC  %s",h);
    M_DEC( zh );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x2d),"DEC  %s",l);
    M_DEC( zl );
	end_opcode("z_op");

    beg_opcode(11+mc,s(op,0x35),"DEC  %s",m);
    if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
        M_DEC( DL );
		WBYTE( 3, EBP, EDX );
    } else {
		RBYTE( 1, EDX, HL32 );
        M_DEC( DL );
		WBYTE( 1, HL32, EDX );
    }
	end_opcode("z_op");

    beg_opcode( 4,s(op,0x3d),"DEC  A");
    M_DEC( A );
	end_opcode("z_op");

/*********/

	beg_opcode( 7,s(op,0x06),"LD   B,imm8");
	ARG( CL );
	I(0,movb,	zB, 	CL, 	0);
	end_opcode("z_op");

	beg_opcode( 7,s(op,0x0e),"LD   C,imm8");
	ARG( CL );
	I(0,movb,	zC, 	CL, 	0);
	end_opcode("z_op");

	beg_opcode( 7,s(op,0x16),"LD   D,imm8");
	ARG( CL );
	I(0,movb,	zD, 	CL, 	0);
	end_opcode("z_op");

    beg_opcode( 7,s(op,0x1e),"LD   E,imm8");
	ARG( CL );
	I(0,movb,	zE, 	CL, 	0);
	end_opcode("z_op");

	if ( op_base ) {
		beg_opcode( 5,s(op,0x26),"LD   %s,imm8",h);
		ARG( CL );
		I(0,movb,	zh, 	CL, 	0);
	} else {
		beg_opcode( 7,s(op,0x26),"LD   %s,imm8",h);
		ARG( zh );
    }
    end_opcode("z_op");

	if ( op_base ) {
		beg_opcode( 5,s(op,0x2e),"LD   %s,imm8",l);
		ARG( CL );
		I(0,movb,	zl, 	CL, 	0);
	} else {
        beg_opcode( 7,s(op,0x2e),"LD   %s,imm8",l);
		ARG( zl );
	}
	end_opcode("z_op");

    beg_opcode(10+mi,s(op,0x36),"LD   %s,imm8",m);
    if (op_base) {
		OFS( zhl32 );
		ARG( EDX );
#if WB_EA4_EDX
		I(0,jmp,	"wb_ea4_edx",0,    0);
#else
		WBYTE( 4, EBP, EDX );
		end_opcode("z_op");
#endif
    } else {
		ARG( EDX );
		WBYTE( 2, HL32, EDX );
		end_opcode("z_op");
    }

    beg_opcode( 7,s(op,0x3e),"LD   A,imm8");
	ARG( CL );
	I(0,movb,	A,		CL, 	0);
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0x1f),"RRA");
	M_RRA();
	end_opcode("z_op");

    beg_opcode( 4,s(op,0x07),"RLCA");
    M_RLCA();
	end_opcode("z_op");

    beg_opcode( 4,s(op,0x0f),"RRCA");
    M_RRCA();
	end_opcode("z_op");

    beg_opcode( 4,s(op,0x17),"RLA");
    M_RLA();
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x27),"DAA");
	M_DAA( op_base|0x27 );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x2f),"CPL");
	M_CPL();
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x37),"SCF");
	M_SCF();
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x3f),"CCF");
	M_CCF();
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0x40),"LD   B,B");
	MOVB( zB, zB, "B = B" );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x41),"LD   B,C");
	MOVB( zB, zC, "B = C" );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x42),"LD   B,D");
	MOVB( zB, zD, "B = D" );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x43),"LD   B,E");
	MOVB( zB, zE, "B = E" );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x44),"LD   B,%s",h);
	MOVB( zB, zh, "B = %s", h );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x45),"LD   B,%s",l);
	MOVB( zB, zl, "B = %s", l );
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x46),"LD   B,%s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
	}
	MOVB( zB, DL, "B = %s", m );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x47),"LD   B,A");
	MOVB( zB, A, "B = A" );
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0x48),"LD   C,B");
	MOVB( zC, zB, "C = B"  );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x49),"LD   C,C");
	MOVB( zC, zC, "C = C"  );
    end_opcode("z_op");

	beg_opcode( 4,s(op,0x4a),"LD   C,D");
	MOVB( zC, zD, "C = D"  );
    end_opcode("z_op");

	beg_opcode( 4,s(op,0x4b),"LD   C,E");
	MOVB( zC, zE, "C = E"  );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x4c),"LD   C,%s",h);
	MOVB( zC, zh, "C = %s", h );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x4d),"LD   C,%s",l);
	MOVB( zC, zl, "C = %s", l );
    end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x4e),"LD   C,%s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	MOVB( zC, DL, "C = %s", m );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x4f),"LD   C,A");
	MOVB( zC, A, "C = A" );
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0x50),"LD   D,B");
	MOVB( zD, zB, "D = B" );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x51),"LD   D,C");
	MOVB( zD, zC, "D = C" );
    end_opcode("z_op");

	beg_opcode( 4,s(op,0x52),"LD   D,D");
	MOVB( zD, zD, "D = D" );
    end_opcode("z_op");

	beg_opcode( 4,s(op,0x53),"LD   D,E");
	MOVB( zD, zE, "D = E" );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x54),"LD   D,%s",h);
	MOVB( zD, zh, "D = %s", h );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x55),"LD   D,%s",l);
	MOVB( zD, zl, "D = %s", l );
    end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x56),"LD   D,%s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	MOVB( zD, DL, "D = %s", m );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x57),"LD   D,A");
	MOVB( zD, A, "D = A" );
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0x58),"LD   E,B");
	MOVB( zE, zB, "E = B" );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x59),"LD   E,C");
	MOVB( zE, zC, "E = C" );
    end_opcode("z_op");

	beg_opcode( 4,s(op,0x5a),"LD   E,D");
	MOVB( zE, zD, "E = D" );
    end_opcode("z_op");

	beg_opcode( 4,s(op,0x5b),"LD   E,E");
	MOVB( zE, zE, "E = E" );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x5c),"LD   E,%s",h);
	MOVB( zE, zh, "E = %s", h );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x5d),"LD   E,%s",l);
	MOVB( zE, zl, "E = %s", l );
    end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x5e),"LD   E,%s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
	} else {
		RBYTE( 1, EDX, HL32 );
    }
	MOVB( zE, DL, "E = %s", m );
    end_opcode("z_op");

	beg_opcode( 4,s(op,0x5f),"LD   E,A");
	MOVB( zE, A, "E = A" );
    end_opcode("z_op");

/*********/

	beg_opcode( 4+mr,s(op,0x60),"LD   %s,B",h);
	MOVB( zh, zB, "%s = B", h );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x61),"LD   %s,C",h);
	MOVB( zh, zC, "%s = C", h );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x62),"LD   %s,D",h);
	MOVB( zh, zD, "%s = D", h );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x63),"LD   %s,E",h);
	MOVB( zh, zE, "%s = E", h );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x64),"LD   %s,%s",h,h);
	MOVB( zh, zh, "%s = %s", h, h );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x65),"LD   %s,%s",h,l);
	MOVB( zh, zl, "%s = %s", h, l );
    end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x66),"LD   H,%s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	MOVB( H, DL, "H = %s", m );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x67),"LD   %s,A",h);
	MOVB( zh, A, "%s = A", h );
    end_opcode("z_op");

/*********/

	beg_opcode( 4+mr,s(op,0x68),"LD   %s,B",l);
	MOVB( zl, zB, "%s = B", l );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x69),"LD   %s,C",l);
	MOVB( zl, zC, "%s = C", l );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x6a),"LD   %s,D",l);
	MOVB( zl, zD, "%s = D", l );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x6b),"LD   %s,E",l);
	MOVB( zl, zE, "%s = E", l );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x6c),"LD   %s,%s",l,h);
	MOVB( zl, zh, "%s = %s", l, h );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x6d),"LD   %s,%s",l,l);
	MOVB( zl, zl, "%s = %s", l, l );
    end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x6e),"LD   L,%s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	MOVB( L, DL, "L = %s", m );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x6f),"LD   %s,A",l);
	MOVB( zl, A, "%s = A", l );
	end_opcode("z_op");

/*********/

    beg_opcode( 7+mc,s(op,0x70),"LD   %s,B",m);
	MOVB( EDX, zB, "%s = B", m  );
	if (op_base) {
		OFS( zhl32 );
		WBYTE( 3, EBP, EDX );
	} else {
		WBYTE( 1, HL32, EDX );
	}
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x71),"LD   %s,C",m);
	MOVB( EDX, zC, "%s = C", m  );
	if (op_base) {
		OFS( zhl32 );
		WBYTE( 3, EBP, EDX );
	} else {
		WBYTE( 1, HL32, EDX );
    }
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x72),"LD   %s,D",m);
	MOVB( EDX, zD, "%s = D", m  );
	if (op_base) {
		OFS( zhl32 );
		WBYTE( 3, EBP, EDX );
	} else {
		WBYTE( 1, HL32, EDX );
	}
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x73),"LD   %s,E",m);
	MOVB( EDX, zE, "%s = E", m );
    if (op_base) {
		OFS( zhl32 );
		WBYTE( 3, EBP, EDX );
	} else {
		WBYTE( 1, HL32, EDX );
    }
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x74),"LD   %s,H",m);
	MOVB( EDX, H, "%s = H", m );
	if (op_base) {
		OFS( zhl32 );
		WBYTE( 3, EBP, EDX );
	} else {
		WBYTE( 1, HL32, EDX );
    }
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x75),"LD   %s,L",m);
	MOVB( EDX, L, "%s = L", m );
    if (op_base) {
		OFS( zhl32 );
		WBYTE( 3, EBP, EDX );
	} else {
		WBYTE( 1, HL32, EDX );
    }
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x76),"HALT");
	M_HALT( op_base|0x76 );
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x77),"LD   %s,A",m);
	MOVB( EDX, A, "%s = A", m );
	if (op_base) {
		OFS( zhl32 );
		WBYTE( 3, EBP, EDX );
	} else {
		WBYTE( 1, HL32, EDX );
    }
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0x78),"LD   A,B");
	MOVB( A, zB, "A = B" );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x79),"LD   A,C");
	MOVB( A, zC, "A = C" );
    end_opcode("z_op");

	beg_opcode( 4,s(op,0x7a),"LD   A,D");
	MOVB( A, zD, "A = D" );
    end_opcode("z_op");

	beg_opcode( 4,s(op,0x7b),"LD   A,E");
	MOVB( A, zE, "A = E" );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x7c),"LD   A,%s",h);
	MOVB( A, zh, "A = %s", h );
    end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x7d),"LD   A,%s",l);
	MOVB( A, zl, "A = %s", l );
    end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x7e),"LD   A,%s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	MOVB( A, DL, "A = %s", m );
	end_opcode("z_op");

    beg_opcode( 4,s(op,0x7f),"LD   A,A");
	MOVB( A, A, "A = A" );
    end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0x80),"ADD  A,B");
	M_ADD( zB );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x81),"ADD  A,C");
	M_ADD( zC );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x82),"ADD  A,D");
	M_ADD( zD );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x83),"ADD  A,E");
	M_ADD( zE );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x84),"ADD  A,%s",h);
	M_ADD( zh );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x85),"ADD  A,%s",l);
	M_ADD( zl );
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x86),"ADD  A,%s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	M_ADD( DL );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x87),"ADD  A,A");
	M_ADD( A );
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0x88),"ADC  A,B");
	M_ADC( zB );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x89),"ADC  A,C");
	M_ADC( zC );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x8a),"ADC  A,D");
	M_ADC( zD );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x8b),"ADC  A,E");
	M_ADC( zE );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x8c),"ADC  A,%s",h);
	M_ADC( zh );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x8d),"ADC  A,%s",l);
	M_ADC( zl );
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x8e),"ADC  A,%s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	M_ADC( DL );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x8f),"ADC  A,A");
	M_ADC( A );
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0x90),"SUB  B");
	M_SUB( zB );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x91),"SUB  C");
	M_SUB( zC );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x92),"SUB  D");
	M_SUB( zD );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x93),"SUB  E");
	M_SUB( zE );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x94),"SUB  %s",h);
	M_SUB( zh );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x95),"SUB  %s",l);
	M_SUB( zl );
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x96),"SUB  %s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	M_SUB( DL );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x97),"SUB  A");
	M_SUB( A );
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0x98),"SBC  A,B");
	M_SBC( zB );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x99),"SBC  A,C");
	M_SBC( zC );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x9a),"SBC  A,D");
	M_SBC( zD );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x9b),"SBC  A,E");
	M_SBC( zE );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x9c),"SBC  A,%s",h);
	M_SBC( zh );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0x9d),"SBC  A,%s",l);
	M_SBC( zl );
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0x9e),"SBC  A,%s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	M_SBC( DL );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0x9f),"SBC  A,A");
	M_SBC( A );
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0xa0),"AND  B");
	M_AND( zB );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xa1),"AND  C");
	M_AND( zC );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xa2),"AND  D");
	M_AND( zD );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xa3),"AND  E");
	M_AND( zE );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0xa4),"AND  %s",h);
	M_AND( zh );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0xa5),"AND  %s",l);
	M_AND( zl );
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0xa6),"AND  %s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	M_AND( DL );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xa7),"AND  A");
	M_AND( A );
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0xa8),"XOR  B");
	M_XOR( zB );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xa9),"XOR  C");
	M_XOR( zC );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xaa),"XOR  D");
	M_XOR( zD );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xab),"XOR  E");
	M_XOR( zE );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0xac),"XOR  %s",h);
	M_XOR( zh );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0xad),"XOR  %s",l);
	M_XOR( zl );
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0xae),"XOR  %s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	M_XOR( DL );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xaf),"XOR  A");
	M_XOR( A );
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0xb0),"OR   B");
	M_OR( zB );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xb1),"OR   C");
	M_OR( zC );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xb2),"OR   D");
	M_OR( zD );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xb3),"OR   E");
	M_OR( zE );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0xb4),"OR   %s",h);
	M_OR( zh );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0xb5),"OR   %s",l);
	M_OR( zl );
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0xb6),"OR   %s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	M_OR( DL );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xb7),"OR   A");
	M_OR( A );
	end_opcode("z_op");

/*********/

    beg_opcode( 4,s(op,0xb8),"CP   B");
	M_CP( zB );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xb9),"CP   C");
	M_CP( zC );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xba),"CP   D");
	M_CP( zD );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xbb),"CP   E");
	M_CP( zE );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0xbc),"CP   %s",h);
	M_CP( zh );
	end_opcode("z_op");

	beg_opcode( 4+mr,s(op,0xbd),"CP   %s",l);
	M_CP( zl );
	end_opcode("z_op");

	beg_opcode( 7+mc,s(op,0xbe),"CP   %s",m);
	if (op_base) {
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
    } else {
		RBYTE( 1, EDX, HL32 );
    }
	M_CP( DL );
	end_opcode("z_op");

	beg_opcode( 4,s(op,0xbf),"CP   A");
	M_CP( A );
	end_opcode("z_op");

/*********/

    beg_opcode( 5,s(op,0xc0),"RET  NZ");
	M_RET_CC( op_base|0xc0 );
	end_opcode("z_op");

	beg_opcode( 5,s(op,0xc8),"RET  Z");
	M_RET_CC( op_base|0xc8 );
	end_opcode("z_op");

	beg_opcode( 5,s(op,0xd0),"RET  NC");
	M_RET_CC( op_base|0xd0 );
	end_opcode("z_op");

    beg_opcode( 5,s(op,0xd8),"RET  C");
	M_RET_CC( op_base|0xd8 );
	end_opcode("z_op");

    beg_opcode( 5,s(op,0xe0),"RET  PE");
	M_RET_CC( op_base|0xe0 );
	end_opcode("z_op");

    beg_opcode( 5,s(op,0xe8),"RET  PO");
	M_RET_CC( op_base|0xe8 );
	end_opcode("z_op");

    beg_opcode( 5,s(op,0xf0),"RET  P");
	M_RET_CC( op_base|0xf0 );
	end_opcode("z_op");

    beg_opcode( 5,s(op,0xf8),"RET  M");
	M_RET_CC( op_base|0xf8 );
	end_opcode("z_op");

/*********/

    beg_opcode(10,s(op,0xc1),"POP  BC");
	POP( 1, zBC32 );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xd1),"POP  DE");
	POP( 1, zDE32 );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xe1),"POP  %s",hl);
	POP( (op_base)?2:1, zhl32 );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xf1),"POP  AF");
	POP( 1, zAF32 );
	restore_af();
	end_opcode("z_op");

	beg_opcode(11,s(op,0xc5),"PUSH BC");
	PUSH( 1, zBC32 );
	end_opcode("z_op");

    beg_opcode(11,s(op,0xd5),"PUSH DE");
	PUSH( 1, zDE32 );
	end_opcode("z_op");

    beg_opcode(11,s(op,0xe5),"PUSH %s",hl);
	PUSH( (op_base)?2:1, zhl32 );
	end_opcode("z_op");

    beg_opcode(11,s(op,0xf5),"PUSH AF");
	save_af();
	PUSH( 1, zAF32 );
	end_opcode("z_op");

/*********/

	beg_opcode(10,s(op,0xc2),"JP   NZ,imm16");
	M_JP_CC( op_base|0xc2 );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xca),"JP   Z,imm16");
	M_JP_CC( op_base|0xca );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xd2),"JP   NC,imm16");
	M_JP_CC( op_base|0xd2 );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xda),"JP   C,imm16");
	M_JP_CC( op_base|0xda );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xe2),"JP   PE,imm16");
	M_JP_CC( op_base|0xe2 );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xea),"JP   PO,imm16");
	M_JP_CC( op_base|0xea );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xf2),"JP   P,imm16");
	M_JP_CC( op_base|0xf2 );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xfa),"JP   M,imm16");
	M_JP_CC( op_base|0xfa );
	end_opcode("z_op");

/*********/

    beg_opcode(10,s(op,0xc3),"JP   imm16");
	M_JP( op_base|0xc3 );
	end_opcode("z_op");

	if (op_base) {
        beg_opcode( 0,s(op,0xcb),"%02x CB <ofs8> xx", op_base >> 8);
		OFS( zhl32 );
		RBYTE( 3, EDX, EBP );
		end_opcode("z_xx");
    } else {
        beg_opcode( 0,s(op,0xcb),"CB xx");
		end_opcode("z_cb");
    }

	beg_opcode(11,s(op,0xd3),"OUT  (imm8),A");
    ARG( ECX );
	I(0,movzbl, EDX,	A,		"value");
    I(0,movb,   CH,     A,      "16 bit port A on upper 8 address lines");
	M_OUT( 2, ECX, EDX );
	end_opcode("z_op");

    beg_opcode(11,s(op,0xdb),"IN   A,(imm8)");
    ARG( ECX );
    I(0,movb,   CH,     A,      "16 bit port A on upper 8 address lines");
	M_IN( 2, EDX, ECX );
	I(0,movb,	A,		DL, 	"get result");
    end_opcode("z_op");

    beg_opcode(19,s(op,0xe3),"EX   (SP),%s",hl);
	RWORD( (op_base)?2:1, EDX, zSP32 );
	I(0,xchgw,	DX,    zhl, 	0);
	WWORD( (op_base)?2:1, zSP32, EDX );
	end_opcode("z_op");

    beg_opcode( 4,s(op,0xeb),"EX   DE,HL");
	M_EX_DE_HL();
	end_opcode("z_op");

    beg_opcode( 4,s(op,0xf3),"DI");
    M_DI();
	end_opcode("z_op");

    beg_opcode( 4,s(op,0xfb),"EI");
    M_EI( op_base|0xfb );
	end_opcode("z_op");

/*********/

    beg_opcode(10,s(op,0xc4),"CALL NZ,imm16");
	M_CALL_CC( op_base|0xc4 );
	end_opcode("z_op");

	beg_opcode(10,s(op,0xcc),"CALL Z,imm16");
	M_CALL_CC( op_base|0xcc );
	end_opcode("z_op");

	beg_opcode(10,s(op,0xd4),"CALL NC,imm16");
	M_CALL_CC( op_base|0xd4 );
	end_opcode("z_op");

	beg_opcode(10,s(op,0xdc),"CALL C,imm16");
	M_CALL_CC( op_base|0xdc );
	end_opcode("z_op");

	beg_opcode(10,s(op,0xe4),"CALL PE,imm16");
	M_CALL_CC( op_base|0xe4 );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xec),"CALL PO,imm16");
	M_CALL_CC( op_base|0xec );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xf4),"CALL P,imm16");
	M_CALL_CC( op_base|0xf4 );
	end_opcode("z_op");

    beg_opcode(10,s(op,0xfc),"CALL M,imm16");
	M_CALL_CC( op_base|0xfc );
	end_opcode("z_op");

/*********/

	beg_opcode(10,s(op,0xc9),"RET");
	M_RET( op_base|0xc9 );
	end_opcode("z_op");

    beg_opcode( 4,s(op,0xd9),"EXX");
    M_EXX();
	end_opcode("z_op");

    beg_opcode( 4,s(op,0xe9),"JP   (%s)",hl);
    M_JP_REG( op_base|0xe9, zhl32 );
	end_opcode("z_op");

    beg_opcode( 6,s(op,0xf9),"LD   SP,%s",hl);
	I(0,movl,	ECX,	zhl32,		0);
	I(0,movl,	zSP32,	ECX,		0);
	end_opcode("z_op");

	beg_opcode(17,s(op,0xcd),"CALL imm16");
    M_CALL( op_base|0xcd );
	end_opcode("z_op");

    beg_opcode( 4,s(op,0xdd),"DD xx");
	end_opcode("z_dd");

    beg_opcode( 0,s(op,0xed),"ED xx");
	end_opcode("z_ed");

    beg_opcode( 4,s(op,0xfd),"FD xx");
    end_opcode("z_fd");

/*********/

    beg_opcode( 7,s(op,0xc6),"ADD  A,imm8");
	ARG( DL );
	M_ADD( DL );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0xce),"ADC  A,imm8");
	ARG( DL );
	M_ADC( DL );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0xd6),"SUB  imm8");
	ARG( DL );
	M_SUB( DL );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0xde),"SBC  A,imm8");
	ARG( DL );
	M_SBC( DL );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0xe6),"AND  imm8");
	ARG( DL );
	M_AND( DL );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0xee),"XOR  imm8");
	ARG( DL );
	M_XOR( DL );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0xf6),"OR   imm8");
	ARG( DL );
	M_OR( DL );
	end_opcode("z_op");

	beg_opcode( 7,s(op,0xfe),"CP   imm8");
	ARG( DL );
	M_CP( DL );
	end_opcode("z_op");

/*********/

    beg_opcode(11,s(op,0xc7),"RST  $00");
    M_RST( op_base|0xc7 );
	end_opcode("z_op");

    beg_opcode(11,s(op,0xcf),"RST  $08");
    M_RST( op_base|0xcf );
	end_opcode("z_op");

    beg_opcode(11,s(op,0xd7),"RST  $10");
    M_RST( op_base|0xd7 );
	end_opcode("z_op");

    beg_opcode(11,s(op,0xdf),"RST  $18");
    M_RST( op_base|0xdf );
	end_opcode("z_op");

    beg_opcode(11,s(op,0xe7),"RST  $20");
    M_RST( op_base|0xe7 );
	end_opcode("z_op");

    beg_opcode(11,s(op,0xef),"RST  $28");
    M_RST( op_base|0xef );
	end_opcode("z_op");

    beg_opcode(11,s(op,0xf7),"RST  $30");
    M_RST( op_base|0xf7 );
	end_opcode("z_op");

    beg_opcode(11,s(op,0xff),"RST  $38");
	M_RST( op_base|0xff );
	end_opcode("z_op");
}

void mk_opcodes_cb(void)
{
	int i, j, b;
    char r[4+1], zr[32+1];
    for (i = 0; i < 0x100; i += 0x08) {
		for (j = 0; j < 8; j++) {
			switch (j) {
				case 0: strcpy(r,"B");    strcpy(zr,zB); break;
				case 1: strcpy(r,"C");    strcpy(zr,zC); break;
				case 2: strcpy(r,"D");    strcpy(zr,zD); break;
				case 3: strcpy(r,"E");    strcpy(zr,zE); break;
				case 4: strcpy(r,"H");    strcpy(zr,H);  break;
				case 5: strcpy(r,"L");    strcpy(zr,L);  break;
				case 6: strcpy(r,"(HL)"); strcpy(zr,DL); break;
				case 7: strcpy(r,"A");    strcpy(zr,A);  break;
			}
			switch (i) {
				case 0x00:
					if (j == 6) {
						beg_opcode(15,s("cb_%02x",i+j),"RLC  %s",r);
						RBYTE( 2, EDX, HL32 );
                    } else {
						beg_opcode( 8,s("cb_%02x",i+j),"RLC  %s",r);
					}
					M_RLC( zr );
					if (j == 6) WBYTE( 2, HL32, EDX );
					end_opcode("z_op");
                    break;
				case 0x08:
					if (j == 6) {
						beg_opcode(15,s("cb_%02x",i+j),"RRC  %s",r);
						RBYTE( 2, EDX, HL32 );
                    } else {
						beg_opcode( 8,s("cb_%02x",i+j),"RRC  %s",r);
					}
					M_RRC( zr );
					if (j == 6) WBYTE( 2, HL32, EDX );
					end_opcode("z_op");
                    break;
                case 0x10:
					if (j == 6) {
						beg_opcode(15,s("cb_%02x",i+j),"RL   %s",r);
						RBYTE( 2, EDX, HL32 );
                    } else {
						beg_opcode( 8,s("cb_%02x",i+j),"RL   %s",r);
					}
					M_RL ( zr );
					if (j == 6) WBYTE( 2, HL32, EDX );
					end_opcode("z_op");
                    break;
                case 0x18:
					if (j == 6) {
						beg_opcode(15,s("cb_%02x",i+j),"RR   %s",r);
						RBYTE( 2, EDX, HL32 );
                    } else {
						beg_opcode( 8,s("cb_%02x",i+j),"RR   %s",r);
					}
					M_RR ( zr );
					if (j == 6) WBYTE( 2, HL32, EDX );
					end_opcode("z_op");
                    break;
                case 0x20:
					if (j == 6) {
						beg_opcode(15,s("cb_%02x",i+j),"SLA  %s",r);
						RBYTE( 2, EDX, HL32 );
                    } else {
						beg_opcode( 8,s("cb_%02x",i+j),"SLA  %s",r);
					}
					M_SLA( zr );
					if (j == 6) WBYTE( 2, HL32, EDX );
					end_opcode("z_op");
                    break;
                case 0x28:
					if (j == 6) {
						beg_opcode(15,s("cb_%02x",i+j),"SRA  %s",r);
						RBYTE( 2, EDX, HL32 );
                    } else {
						beg_opcode( 8,s("cb_%02x",i+j),"SRA  %s",r);
					}
					M_SRA( zr );
					if (j == 6) WBYTE( 2, HL32, EDX );
					end_opcode("z_op");
                    break;
                case 0x30:
					if (j == 6) {
						beg_opcode(15,s("cb_%02x",i+j),"SLL  %s",r);
						RBYTE( 2, EDX, HL32 );
                    } else {
						beg_opcode( 8,s("cb_%02x",i+j),"SLL  %s",r);
					}
					M_SLL( zr );
					if (j == 6) WBYTE( 2, HL32, EDX );
					end_opcode("z_op");
                    break;
                case 0x38:
					if (j == 6) {
						beg_opcode(15,s("cb_%02x",i+j),"SRL  %s",r);
						RBYTE( 2, EDX, HL32 );
                    } else {
						beg_opcode( 8,s("cb_%02x",i+j),"SRL  %s",r);
					}
					M_SRL( zr );
					if (j == 6) WBYTE( 2, HL32, EDX );
					end_opcode("z_op");
                    break;
                case 0x40: case 0x48: case 0x50: case 0x58:
				case 0x60: case 0x68: case 0x70: case 0x78:
					b = (i >> 3) & 7;
					if (j == 6) {
                        beg_opcode(12,s("cb_%02x",i+j),"BIT  %d,%s",b,r);
						RBYTE( 2, EDX, HL32 );
					} else {
						beg_opcode( 8,s("cb_%02x",i+j),"BIT  %d,%s",b,r);
                    }
					M_BIT( b, zr );
					end_opcode("z_op");
                    break;
				case 0x80: case 0x88: case 0x90: case 0x98:
				case 0xa0: case 0xa8: case 0xb0: case 0xb8:
					b = (i >> 3) & 7;
					if (j == 6) {
                        beg_opcode(15,s("cb_%02x",i+j),"RES  %d,%s",b,r);
						RBYTE( 2, EDX, HL32 );
					} else {
                        beg_opcode( 8,s("cb_%02x",i+j),"RES  %d,%s",b,r);
					}
					M_RES( b, zr );
					if (j == 6) WBYTE( 2, HL32, EDX );
					end_opcode("z_op");
                    break;
				case 0xc0: case 0xc8: case 0xd0: case 0xd8:
				case 0xe0: case 0xe8: case 0xf0: case 0xf8:
					b = (i >> 3) & 7;
					if (j == 6) {
                        beg_opcode(15,s("cb_%02x",i+j),"SET  %d,%s",b,r);
						RBYTE( 2, EDX, HL32 );
					} else {
						beg_opcode( 8,s("cb_%02x",i+j),"SET  %d,%s",b,r);
					}
					M_SET( b, zr );
					if (j == 6) WBYTE( 2, HL32, EDX );
					end_opcode("z_op");
                    break;
            }
		}
	}
}

void mk_opcodes_xx(void)
{
	int i, j, b;
	char r[32+1], zr[32+1];
    for (i = 0; i < 0x100; i += 0x08) {
		for (j = 0; j < 8; j++) {
			switch (j) {
				case 0: strcpy(r,"B=(IX/IY+ofs8)"); strcpy(zr,zB); break;
				case 1: strcpy(r,"C=(IX/IY+ofs8)"); strcpy(zr,zC); break;
				case 2: strcpy(r,"D=(IX/IY+ofs8)"); strcpy(zr,zD); break;
				case 3: strcpy(r,"E=(IX/IY+ofs8)"); strcpy(zr,zE); break;
				case 4: strcpy(r,"H=(IX/IY+ofs8)"); strcpy(zr,H);  break;
				case 5: strcpy(r,"L=(IX/IY+ofs8)"); strcpy(zr,L);  break;
				case 6: strcpy(r,"(IX/IY+ofs8)");   strcpy(zr,DL); break;
				case 7: strcpy(r,"A=(IX/IY+ofs8)"); strcpy(zr,A);  break;
			}
			switch (i) {
				case 0x00:
					beg_opcode(19,s("xx_%02x",i+j),"RLC  %s",r);
					M_RLC( DL );
					if (j != 6) I(0,movb, zr, DL, 0);
#if WB_EA4_EDX
					I(0,jmp,	"wb_ea4_edx",0,    0);
#else
					WBYTE( 4, EBP, EDX );
					end_opcode("z_op");
#endif
					break;
				case 0x08:
					beg_opcode(19,s("xx_%02x",i+j),"RRC  %s",r);
					M_RRC( DL );
					if (j != 6) I(0,movb, zr, DL, 0);
#if WB_EA4_EDX
					I(0,jmp,	"wb_ea4_edx",0,    0);
#else
					WBYTE( 4, EBP, EDX );
					end_opcode("z_op");
#endif
                    break;
				case 0x10:
					beg_opcode(19,s("xx_%02x",i+j),"RL   %s",r);
					M_RL ( DL );
					if (j != 6) I(0,movb, zr, DL, 0);
#if WB_EA4_EDX
					I(0,jmp,	"wb_ea4_edx",0,    0);
#else
					WBYTE( 4, EBP, EDX );
					end_opcode("z_op");
#endif
                    break;
				case 0x18:
					beg_opcode(19,s("xx_%02x",i+j),"RR   %s",r);
					M_RR ( DL );
					if (j != 6) I(0,movb, zr, DL, 0);
#if WB_EA4_EDX
					I(0,jmp,	"wb_ea4_edx",0,    0);
#else
					WBYTE( 4, EBP, EDX );
					end_opcode("z_op");
#endif
                    break;
				case 0x20:
					beg_opcode(19,s("xx_%02x",i+j),"SLA  %s",r);
					M_SLA( DL );
					if (j != 6) I(0,movb, zr, DL, 0);
#if WB_EA4_EDX
					I(0,jmp,	"wb_ea4_edx",0,    0);
#else
					WBYTE( 4, EBP, EDX );
					end_opcode("z_op");
#endif
                    break;
				case 0x28:
					beg_opcode(19,s("xx_%02x",i+j),"SRA  %s",r);
					M_SRA( DL );
					if (j != 6) I(0,movb, zr, DL, 0);
#if WB_EA4_EDX
					I(0,jmp,	"wb_ea4_edx",0,    0);
#else
					WBYTE( 4, EBP, EDX );
					end_opcode("z_op");
#endif
                    break;
				case 0x30:
					beg_opcode(19,s("xx_%02x",i+j),"SLL  %s",r);
					M_SLL( DL );
					if (j != 6) I(0,movb, zr, DL, 0);
#if WB_EA4_EDX
					I(0,jmp,	"wb_ea4_edx",0,    0);
#else
					WBYTE( 4, EBP, EDX );
					end_opcode("z_op");
#endif
                    break;
				case 0x38:
					beg_opcode(19,s("xx_%02x",i+j),"SRL  %s",r);
					M_SRL( DL );
					if (j != 6) I(0,movb, zr, DL, 0);
#if WB_EA4_EDX
					I(0,jmp,	"wb_ea4_edx",0,    0);
#else
					WBYTE( 4, EBP, EDX );
					end_opcode("z_op");
#endif
                    break;
				case 0x40: case 0x48: case 0x50: case 0x58:
				case 0x60: case 0x68: case 0x70: case 0x78:
					b = (i >> 3) & 7;
					beg_opcode(16,s("xx_%02x",i+j),"BIT  %d,%s",b,r);
					M_BIT( b, DL );
					end_opcode("z_op");
                    break;
				case 0x80: case 0x88: case 0x90: case 0x98:
				case 0xa0: case 0xa8: case 0xb0: case 0xb8:
					b = (i >> 3) & 7;
					beg_opcode(19,s("xx_%02x",i+j),"RES  %d,%s",b,r);
					M_RES( b, DL );
					if (j != 6) I(0,movb, zr, DL, 0);
#if WB_EA4_EDX
					I(0,jmp,	"wb_ea4_edx",0,    0);
#else
					WBYTE( 4, EBP, EDX );
					end_opcode("z_op");
#endif
                    break;
				case 0xc0: case 0xc8: case 0xd0: case 0xd8:
				case 0xe0: case 0xe8: case 0xf0: case 0xf8:
					b = (i >> 3) & 7;
					beg_opcode(19,s("xx_%02x",i+j),"SET  %d,%s",b,r);
					M_SET( b, DL );
					if (j != 6) I(0,movb, zr, DL, 0);
#if WB_EA4_EDX
					I(0,jmp,	"wb_ea4_edx",0,    0);
#else
					WBYTE( 4, EBP, EDX );
					end_opcode("z_op");
#endif
                    break;
            }
		}
	}
}

void mk_opcodes_ed(void)
{
	beg_opcode(12,"ed_40","IN   B,(C)");
	M_IN_BC( zB );
	end_opcode("z_op");

	beg_opcode(12,"ed_41","OUT  (C),B");
	M_OUT_BC( zB );
	end_opcode("z_op");

	beg_opcode(15,"ed_42","SBC  HL,BC");
	M_SBC16( zBC );
	end_opcode("z_op");

	beg_opcode(20,"ed_43","LD   (imm16),BC");
	ARG16( ECX );
	WWORD( 4, ECX, zBC32 );
	end_opcode("z_op");

	beg_opcode( 8,"ed_44","NEG");
	M_NEG();
	end_opcode("z_op");

	beg_opcode(14,"ed_45","RETN");
	M_RETN( 0x45 );
	end_opcode("z_op");

	beg_opcode( 8,"ed_46","IM   0");
	I(0,movb,	zIM,	D(0),	"set IM = 0");
	end_opcode("z_op");

	beg_opcode( 9,"ed_47","LD   I,A");
	M_LD_I_A();
	end_opcode("z_op");

	beg_opcode(12,"ed_48","IN   C,(C)");
	M_IN_BC( zC );
	end_opcode("z_op");

	beg_opcode(12,"ed_49","OUT  (C),C");
	M_OUT_BC( zC );
	end_opcode("z_op");

	beg_opcode(15,"ed_4a","ADC  HL,BC");
	M_ADC16( zBC );
	end_opcode("z_op");

	beg_opcode(20,"ed_4b","LD   BC,(imm16)");
	ARG16( ECX );
	RWORD( 4, EDX, ECX );
	I(0,movw,	zBC,	DX, 	0);
	end_opcode("z_op");

	beg_opcode( 8,"ed_4c","NEG");
	M_NEG();
	end_opcode("z_op");

	beg_opcode(14,"ed_4d","RETI");
	M_RETI(0x4d);
	end_opcode("z_op");

	beg_opcode( 8,"ed_4e","IM   0");
	I(0,movb,	zIM,	D(0),	"set IM = 0");
	end_opcode("z_op");

	beg_opcode( 9,"ed_4f","LD   R,A");
	M_LD_R_A();
	end_opcode("z_op");

	beg_opcode(12,"ed_50","IN   D,(C)");
	M_IN_BC( zD );
	end_opcode("z_op");

	beg_opcode(12,"ed_51","OUT  (C),D");
	M_OUT_BC( zD );
	end_opcode("z_op");

	beg_opcode(15,"ed_52","SBC  HL,DE");
	M_SBC16( zDE );
	end_opcode("z_op");

	beg_opcode(20,"ed_53","LD   (imm16),DE");
	ARG16( ECX );
	WWORD( 4, ECX, zDE32 );
	end_opcode("z_op");

	beg_opcode( 8,"ed_54","NEG");
	M_NEG();
	end_opcode("z_op");

	beg_opcode(14,"ed_55","RETN");
	M_RETN( 0x55 );
	end_opcode("z_op");

	beg_opcode( 8,"ed_56","IM   1");
	I(0,movb,	zIM,	D(1),	"set IM = 1");
	end_opcode("z_op");

	beg_opcode( 9,"ed_57","LD   A,I");
	M_LD_A_I();
	end_opcode("z_op");

	beg_opcode(12,"ed_58","IN   E,(C)");
	M_IN_BC( zE );
	end_opcode("z_op");

	beg_opcode(12,"ed_59","OUT  (C),E");
	M_OUT_BC( zE );
	end_opcode("z_op");

	beg_opcode(15,"ed_5a","ADC  HL,DE");
	M_ADC16( zDE );
	end_opcode("z_op");

	beg_opcode(20,"ed_5b","LD   DE,(imm16)");
	ARG16( ECX );
	RWORD( 4, EDX, ECX );
	I(0,movw,	zDE,	DX, 	0);
	end_opcode("z_op");

	beg_opcode( 8,"ed_5c","NEG");
	M_NEG();
	end_opcode("z_op");

	beg_opcode(14,"ed_5d","RETI");
	M_RETI( 0x5d );
	end_opcode("z_op");

	beg_opcode( 8,"ed_5e","IM   2");
	I(0,movb,	zIM,	D(2),	"set IM = 2");
	end_opcode("z_op");

	beg_opcode( 9,"ed_5f","LD   A,R");
	M_LD_A_R();
	end_opcode("z_op");

	beg_opcode(12,"ed_60","IN   H,(C)");
	M_IN_BC( H );
	end_opcode("z_op");

	beg_opcode(12,"ed_61","OUT  (C),H");
	M_OUT_BC( H );
	end_opcode("z_op");

	beg_opcode(15,"ed_62","SBC  HL,HL");
	M_SBC16( HL );
	end_opcode("z_op");

	beg_opcode(20,"ed_63","LD   (imm16),HL");
	ARG16( ECX );
	WWORD( 4, ECX, HL32 );
	end_opcode("z_op");

	beg_opcode( 8,"ed_64","NEG");
	M_NEG();
	end_opcode("z_op");

	beg_opcode(14,"ed_65","RETN");
	M_RETN( 0x65 );
	end_opcode("z_op");

	beg_opcode( 8,"ed_66","IM   0");
	I(0,movb,	zIM,	D(0),	"set IM = 0");
	end_opcode("z_op");

	beg_opcode(18,"ed_67","RRD  (HL)");
	M_RRD();
	end_opcode("z_op");

	beg_opcode(12,"ed_68","IN   L,(C)");
	M_IN_BC( L );
	end_opcode("z_op");

	beg_opcode(12,"ed_69","OUT  (C),L");
	M_OUT_BC( L );
	end_opcode("z_op");

	beg_opcode(15,"ed_6a","ADC  HL,HL");
	M_ADC16( HL );
	end_opcode("z_op");

	beg_opcode(20,"ed_6b","LD   HL,(imm16)");
	ARG16( ECX );
	RWORD( 4, EDX, ECX );
	I(0,movw,	HL, 	DX, 	0);
	end_opcode("z_op");

	beg_opcode( 8,"ed_6c","NEG");
	M_NEG();
	end_opcode("z_op");

	beg_opcode(14,"ed_6d","RETI");
	M_RETI( 0x6d );
	end_opcode("z_op");

	beg_opcode( 8,"ed_6e","IM   0");
	I(0,movb,	zIM,	D(0),	"set IM = 0");
	end_opcode("z_op");

	beg_opcode(18,"ed_6f","RLD  (HL)");
	M_RLD();
	end_opcode("z_op");

	beg_opcode(12,"ed_70","IN   F,(C)");
	M_IN_BC( DL );
	end_opcode("z_op");

	beg_opcode(12,"ed_71","OUT  (C),0");
	I(0,movb,	DL, 	D(0),	0);
    M_OUT_BC( DL );
	end_opcode("z_op");

	beg_opcode(15,"ed_72","SBC  HL,SP");
	M_SBC16( zSP );
	end_opcode("z_op");

	beg_opcode(20,"ed_73","LD   (imm16),SP");
	ARG16( ECX );
	WWORD( 4, ECX, zSP32 );
	end_opcode("z_op");

	beg_opcode( 8,"ed_74","NEG");
	M_NEG();
	end_opcode("z_op");

	beg_opcode(14,"ed_75","RETN");
	M_RETN( 0x75 );
	end_opcode("z_op");

	beg_opcode( 8,"ed_76","IM   1");
	I(0,movb,	zIM,	D(1),	"set IM = 1");
	end_opcode("z_op");

	beg_opcode(12,"ed_78","IN   A,(C)");
	M_IN_BC( A );
	end_opcode("z_op");

	beg_opcode(12,"ed_79","OUT  (C),A");
	M_OUT_BC( A );
	end_opcode("z_op");

	beg_opcode(15,"ed_7a","ADC  HL,SP");
	M_ADC16( zSP );
	end_opcode("z_op");

	beg_opcode(20,"ed_7b","LD   SP,(imm16)");
	ARG16( ECX );
	RWORD( 4, EDX, ECX );
	I(0,movw,	zSP,	DX, 	0);
	end_opcode("z_op");

	beg_opcode( 8,"ed_7c","NEG");
	M_NEG();
	end_opcode("z_op");

	beg_opcode(14,"ed_7d","RETI");
	M_RETI( 0x7d );
	end_opcode("z_op");

	beg_opcode( 8,"ed_7e","IM   2");
	I(0,movb,	zIM,	D(2),	"set IM = 2");
	end_opcode("z_op");

	beg_opcode(16,"ed_a0","LDI");
	RBYTE( 2, EDX, HL32 );
	WBYTE( 2, zDE32, EDX );
    I(0,andb,   F,      X(SF|ZF|CF),    "keep S,Z and C flags");
	I(0,incw,	HL, 	0,				"HL++");
	I(0,incw,	zDE,	0,				"DE++");
	I(0,decw,	zBC,	0,				"BC--");
	I(0,je, 	LS("ed_a0_skip"),0,     0);
	I(0,orb,	F,		X(PF),			"BC not zero, set P/V");
	S("ed_a0_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_a1","CPI");
	RBYTE( 2, EDX, HL32 );
	I(0,movb,	CL, 	F,				"save flags");
    I(0,andb,   CL,     X(CF),          "keep C flag");
	I(0,cmpb,	A,		DL, 			0);
	I(0,lahf,	0,		0,				0);
	I(0,andb,	F,		X(SF|ZF|HF),	"mask S,Z and H flags");
	I(0,orb,	F,		X(NF),			"set N flag");
	I(0,orb,	F,		CL, 			"combine with C");
	I(0,incw,	HL, 	0,				"HL++");
	I(0,decw,	zBC,	0,				"BC--");
	I(0,je, 	LS("ed_a1_skip"),0,     0);
	I(0,orb,	F,		X(PF),			"BC not zero, set P/V");
	S("ed_a1_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_a2","INI");
	I(0,decb,	zB, 	0,				"--B");
	M_IN( 2, EDX, zBC32 );
	WBYTE( 2, HL32, EDX );
	I(0,movb,	F,		X(ZF|NF),		"set Z and N flags");
	I(0,incw,	HL, 	0,				"HL++");
	I(0,testb,	zB, 	B(-1),			"B counted down?");
	I(0,je, 	LS("ed_a2_skip"),0,     0);
	I(0,movb,	F,		X(NF),			"set N only");
	S("ed_a2_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_a3","OUTI");
	RBYTE( 2, EDX, HL32 );
	M_OUT( 2, zBC32, EDX );
    I(0,movb,   F,      X(ZF|NF),       "set Z and N");
	I(0,incw,	HL, 	0,				"HL++");
	I(0,decb,	zB, 	0,				"B--");
	I(0,je, 	LS("ed_a3_skip"),0,     0);
	I(0,movb,	F,		X(NF),			"set N only");
	S("ed_a3_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_a8","LDD");
	RBYTE( 2, EDX, HL32 );
	WBYTE( 2, zDE32, EDX );
    I(0,andb,   F,      X(SF|ZF|CF),    "keep S,Z and C flags");
	I(0,decw,	HL, 	0,				"HL--");
	I(0,decw,	zDE,	0,				"DE--");
	I(0,decw,	zBC,	0,				"BC--");
	I(0,je, 	LS("ed_a8_skip"),0,     0);
	I(0,orb,	F,		X(PF),			"BC not zero, set P/V");
	S("ed_a8_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_a9","CPD");
	RBYTE( 2, EDX, HL32 );
	I(0,movb,	CL, 	F,				"save flags");
    I(0,andb,   CL,     X(CF),          "keep C flag");
	I(0,cmpb,	A,		DL, 			0);
	I(0,lahf,	0,		0,				0);
	I(0,andb,	F,		X(SF|ZF|HF),	"mask S,Z and H flags");
	I(0,orb,	F,		X(NF),			"set N flag");
    I(0,orb,    F,      CL,             "combine with C");
	I(0,decw,	HL, 	0,				"HL--");
	I(0,decw,	zBC,	0,				"BC--");
	I(0,je, 	LS("ed_a9_skip"),0,     0);
	I(0,orb,	F,		X(PF),			"BC not zero, set P/V");
	S("ed_a9_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_aa","IND");
	I(0,decb,	zB, 	0,				"--B");
	M_IN( 2, EDX,  zBC32 );
	WBYTE( 2, HL32, EDX );
    I(0,movb,   F,      X(ZF|NF),       "set Z and N flags");
	I(0,decw,	HL, 	0,				"HL--");
	I(0,testb,	zB, 	B(-1),			"B counted down?");
	I(0,je, 	LS("ed_aa_skip"),0,     0);
	I(0,andb,	F,		X(NF),			"leave N only");
	S("ed_aa_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_ab","OUTD");
	RBYTE( 2, EDX, HL32 );
	M_OUT( 2, zBC32, EDX );
	I(0,movb,	F,		X(ZF|NF),		"set Z and N flags");
	I(0,decw,	HL, 	0,				"HL--");
	I(0,decb,	zB, 	0,				"B--");
	I(0,je, 	LS("ed_ab_skip"),0,     0);
	I(0,andb,	F,		X(NF),			"set N only");
	S("ed_ab_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_b0","LDIR");
    RBYTE( 2, EDX, HL32 );
	WBYTE( 2, zDE32, EDX );
	I(0,andb,	F,		X(SF|ZF|CF),	"keep S,Z and C flags");
	I(0,incw,	HL, 	0,				"HL++");
	I(0,incw,	zDE,	0,				"DE++");
	I(0,decw,	zBC,	0,				"BC--");
	I(0,je, 	LS("ed_b0_skip"),0,     0);
	I(0,orb,	F,		X(PF),			"BC not zero, set P/V");
	ADJUST_PC(-2);
    I(0,subl,   zICOUNT,B(5),           "five more cycles");
    S("ed_b0_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_b1","CPIR");
	RBYTE( 2, EDX, HL32 );
	I(0,movb,	CL, 	F,				"save flags");
	I(0,andb,	CL, 	X(CF),			"keep C flag");
	I(0,cmpb,	A,		DL, 			0);
	I(0,lahf,	0,		0,				0);
	I(0,andb,	F,		X(SF|ZF|HF),	"mask S,Z and H flags");
	I(0,orb,	F,		X(NF),			"set N flag");
	I(0,orb,	F,		CL, 			"combine with C");
	I(0,incw,	HL, 	0,				"HL++");
	I(0,decw,	zBC,	0,				"BC--");
	I(0,je, 	LS("ed_b1_skip"),0,     0);
	I(0,orb,	F,		X(PF),			"BC not zero, set P/V");
	I(0,testb,	F,		X(ZF),			"Z flag set?");
	I(0,jne,	LS("ed_b1_skip"),0,     "yes, don't repeat");
	ADJUST_PC(-2);
    I(0,subl,   zICOUNT,B(5),           "five more cycles");
    S("ed_b1_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_b2","INIR");
	I(0,decb,	zB, 	0,				"--B");
	M_IN( 2, EDX,  zBC32 );
	WBYTE( 2, HL32, EDX );
	I(0,incw,	HL, 	0,				"HL++");
	I(0,movb,	F,		X(ZF|NF),		"set Z and N");
	I(0,testb,	zB, 	B(-1),			"B counted down?");
	I(0,je, 	LS("ed_b2_skip"),0,     0);
	I(0,movb,	F,		X(NF),			"set N only");
	ADJUST_PC(-2);
    I(0,subl,   zICOUNT,B(5),           "five more cycles");
    S("ed_b2_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_b3","OTIR");
	RBYTE( 2, EDX, HL32 );
	M_OUT( 2, zBC32, EDX );
    I(0,movb,   F,      X(ZF|NF),       "set Z and N");
	I(0,incw,	HL, 	0,				"HL++");
	I(0,decb,	zB, 	0,				"B--");
	I(0,je, 	LS("ed_b3_skip"),0,     0);
	I(0,movb,	F,		X(NF),			"set N only");
	ADJUST_PC(-2);
    I(0,subl,   zICOUNT,B(5),           "five more cycles");
    S("ed_b3_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_b8","LDDR");
	RBYTE( 2, EDX, HL32 );
	WBYTE( 2, zDE32, EDX );
    I(0,andb,   F,      X(SF|ZF|CF),    "keep S,Z and C flags");
	I(0,decw,	HL, 	0,				"HL--");
	I(0,decw,	zDE,	0,				"DE--");
	I(0,decw,	zBC,	0,				"BC--");
	I(0,je, 	LS("ed_b8_skip"),0,     0);
	I(0,orb,	F,		X(PF),			"BC not zero, set P/V");
	ADJUST_PC(-2);
    I(0,subl,   zICOUNT,B(5),           "five more cycles");
    S("ed_b8_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_b9","CPDR");
	RBYTE( 2, EDX, HL32 );
	I(0,movb,	CL, 	F,				"save flags");
	I(0,andb,	CL, 	X(CF),			"keep C flag");
	I(0,cmpb,	A,		DL, 			0);
	I(0,lahf,	0,		0,				0);
	I(0,andb,	F,		X(SF|ZF|HF),	"mask S,Z and H flags");
	I(0,orb,	F,		X(NF),			"set N flag");
	I(0,orb,	F,		CL, 			"combine with C");
	I(0,decw,	HL, 	0,				"HL--");
	I(0,decw,	zBC,	0,				"BC--");
	I(0,je, 	LS("ed_b9_skip"),0,     0);
	I(0,orb,	F,		X(PF),			"BC not zero, set P/V");
	I(0,testb,	F,		X(ZF),			"Z flag set?");
	I(0,jne,	LS("ed_b9_skip"),0,     "yes, don't repeat");
	ADJUST_PC(-2);
    I(0,subl,   zICOUNT,B(5),           "five more cycles");
    S("ed_b9_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_ba","INDR");
	I(0,decb,	zB, 	0,				"--B");
	M_IN( 2, EDX,  zBC32 );
	WBYTE( 2, HL32, EDX );
    I(0,movb,   F,      X(ZF|NF),       "set Z and N");
	I(0,decw,	HL, 	0,				"HL--");
	I(0,testb,	zB, 	B(-1),			0);
	I(0,je, 	LS("ed_ba_skip"),0,     0);
	I(0,movb,	F,		X(NF),			"set N only");
	ADJUST_PC(-2);
    I(0,subl,   zICOUNT,B(5),           "five more cycles");
    S("ed_ba_skip",0);
	end_opcode("z_op");

	beg_opcode(16,"ed_bb","OTDR");
	RBYTE( 2, EDX, HL32 );
	M_OUT( 2, zBC32, EDX );
    I(0,movb,   F,      X(ZF|NF),       "set Z and N");
	I(0,decw,	HL, 	0,				"HL--");
	I(0,decb,	zB, 	0,				"B--");
	I(0,je, 	LS("ed_bb_skip"),0,     0);
	I(0,movb,	F,		X(NF),			"set N only");
	ADJUST_PC(-2);
    I(0,subl,   zICOUNT,B(5),           "five more cycles");
    S("ed_bb_skip",0);
	end_opcode("z_op");
}

int main(int ac, char **av)
{
int i;

	if (ac < 2) {
		fprintf(stderr, "usage: %s [options] output.asm\n", av[0]);
		fprintf(stderr, "options can be one of the following\n");
		fprintf(stderr, "-nasm\tgenerate NASM output%s\n",(as==NASM)?" (default)":"");
		fprintf(stderr, "-gas\tgenerate GNU assembler output%s\n",(as==GAS)?" (default)":"");
		fprintf(stderr, "-verbose n\toutput error.log (n= 0 none - 9 everything)\n");
		fprintf(stderr, "-On\temit self optimizing code (n=0 no, n=1 yes)\n");
		fprintf(stderr, "-Ddefine\tdefines for conditional output\n");
		fprintf(stderr, "\t-DMAME_DEBUG output code to call MAMEs debugger\n");
		return 1;
	}
	printf("Z80 x86 asm emitter V0.6 for MAME\n");
	printf("Copyright (c) 1999 by Juergen Buchmueller (pullmoll@t-online.de)\n");

	for (i = 1; av[i][0] == '-'; i++) {
		if (!strcmp(av[i], "-gas")) {
            as = GAS;
		} else if (!strcmp(av[i], "-nasm") || !strcmp(av[i], "-nasmw")) {
            as = NASM;
		} else if (!strcmp(av[i], "-tasm")) {
			as = TASM;
		} else if (!strcmp(av[i],"-verbose")) {
			if (++i < ac - 1) {
				sscanf(av[i], "%d", &verbose);
            }
		} else if (!strncmp(av[i], "-D", 2)) {
			if (!strcmp(&av[i][2],"MAME_DEBUG")) {
				mame_debug = 1;
			} else {
				/* fprintf(stderr, "ignored define '%s'\n", &av[i][2]); */
			}
		} else if (!strncmp(av[i], "-m", 2)) {
			if (!strncmp(&av[i][2],"386",3)) {
				align_funcs = 1;
				align_jumps = 1;
                align_loops = 1;
			} else
			if (!strncmp(&av[i][2],"486",3)) {
				align_funcs = 2;
				align_jumps = 2;
                align_loops = 1;
            } else
			if (!strncmp(&av[i][2],"pentium",3)) {
				align_funcs = 2;
				align_jumps = 2;
                align_loops = 1;
            } else
            if (!strncmp(&av[i][2],"align-functions=",16)) {
				sscanf(&av[i][18], "%d", &align_funcs);
			} else
			if (!strncmp(&av[i][2],"align-jumps=",12)) {
				sscanf(&av[i][14], "%d", &align_jumps);
            } else
			if (!strncmp(&av[i][2],"align-loops=",12)) {
                sscanf(&av[i][14], "%d", &align_loops);
            } else {
                /* fprintf(stderr, "ignored optimizing option '%s'\n", &av[i][2]); */
            }
        } else {
/*
 *			fprintf(stderr, "unsupported option '%s'\n", av[i]);
 *			return 1;
 */
        }
    }
	if (as == GAS)	printf("- GNU assembler output\n");
	if (as == NASM) printf("- NASM output\n");
	if (as == TASM) printf("- TASM output\n");
	if (mame_debug) printf("- with MAME_Debug code\n");
    printf("- verbose level   %d\n", verbose);
	printf("- align functions %d (%d bytes)\n", align_funcs, 2<<align_funcs);
	printf("- align jumps     %d (%d bytes)\n", align_jumps, 2<<align_jumps);
	printf("- align loops     %d (%d bytes)\n", align_loops, 2<<align_loops);

	output = fopen(av[i], "w");

	mk_header();
	mk_interface();
	mk_memory_handlers();
    mk_opcodes(0x0000);
	mk_opcodes_ed();
	mk_opcodes_cb();
    mk_opcodes(0xdd00);
	mk_opcodes(0xfd00);
	mk_opcodes_xx();

	if (as == TASM)
		emit("\tend");

	printf("generated %d opcodes\n", opcodes);
	printf("functions rbyte:%d wbyte:%d rword:%d wword:%d\n",
		rbyte, wbyte, rword, wword);

	fclose(output);
	return 0;
}



