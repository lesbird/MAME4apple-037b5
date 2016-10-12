#ifndef _MAMEDBG_H
#define _MAMEDBG_H

#include "mame.h"

#ifndef DECL_SPEC
#define DECL_SPEC
#endif

#ifdef MAME_DEBUG

#define COLOR_TITLE         YELLOW
#define COLOR_FRAME 		LIGHTCYAN
#define COLOR_REGS			WHITE
#define COLOR_DASM          WHITE
#define COLOR_MEM1          WHITE
#define COLOR_MEM2          WHITE
#define COLOR_CMDS			WHITE
#define COLOR_BRK_EXEC		YELLOW
#define COLOR_BRK_DATA		(YELLOW+BLUE*16)
#define COLOR_BRK_REGS		(YELLOW+BLUE*16)
#define COLOR_ERROR         (YELLOW+RED*16)
#define COLOR_HELP			(WHITE+BLUE*16)
#define COLOR_PROMPT        CYAN
#define COLOR_CHANGES       LIGHTCYAN
#define COLOR_PC            (WHITE+BLUE*16) /* MB 980103 */
#define COLOR_CURSOR		(WHITE+RED*16)	/* MB 980103 */

/***************************************************************************
 *
 * The following functions are defined in mamedbg.c
 *
 ***************************************************************************/
/* What EA address to set with debug_ea_info (origin) */
enum {
    EA_DST,
    EA_SRC
};

/* Size of the data element accessed (or the immediate value) */
enum {
    EA_DEFAULT,
    EA_INT8,
    EA_UINT8,
    EA_INT16,
    EA_UINT16,
    EA_INT32,
    EA_UINT32,
    EA_SIZE
};

/* Access modes for effective addresses to debug_ea_info */
enum {
    EA_NONE,        /* no EA mode */
    EA_VALUE,       /* immediate value */
    EA_ABS_PC,      /* change PC absolute (JMP or CALL type opcodes) */
    EA_REL_PC,      /* change PC relative (BRA or JR type opcodes) */
	EA_ZPG_RD,		/* read zero page memory */
	EA_ZPG_WR,		/* write zero page memory */
	EA_ZPG_RDWR,	/* read then write zero page memory */
    EA_MEM_RD,      /* read memory */
    EA_MEM_WR,      /* write memory */
    EA_MEM_RDWR,    /* read then write memory */
    EA_PORT_RD,     /* read i/o port */
    EA_PORT_WR,     /* write i/o port */
    EA_COUNT
};

/***************************************************************************
 * This function can (should) be called by a disassembler to set
 * information for the debugger. It sets the address, size and type
 * of a memory or port access, an absolute or relative branch or
 * an immediate value and at the same time returns a string that
 * contains a literal hex string for that address.
 * Later it could also return a symbol for that address and access.
 ***************************************************************************/
extern const char *set_ea_info( int what, unsigned address, int size, int access );

/* Startup and shutdown functions; called from cpu_run */
extern void mame_debug_init(void);
extern void mame_debug_exit(void);

/* If this flag is set, a CPU core should call MAME_Debug from it's execution loop */
extern int mame_debug;

/* This is the main entry into the mame debugger */
extern void MAME_Debug(void);

/***************************************************************************
 * Convenience macro for the CPU cores, this is defined to empty
 * if MAME_DEBUG is not specified, so a CPU core can simply add
 * CALL_MAME_DEBUG; before executing an instruction
 ***************************************************************************/
#define CALL_MAME_DEBUG if( mame_debug ) MAME_Debug()

#else   /* MAME_DEBUG */

#define CALL_MAME_DEBUG

#endif  /* !MAME_DEBUG */

#endif /* _MAMEDBG_H */
