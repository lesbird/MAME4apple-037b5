/***************************************************************************

  memory.c

  Functions which handle the CPU memory and I/O port access.

  Caveats:

  * The install_mem/port_*_handler functions are only intended to be
    called at driver init time. Do not call them after this time.

  * If your driver executes an opcode which crosses a bank-switched
    boundary, it will pull the wrong data out of memory. Although not
    a common case, you may need to revert to memcpy to work around this.
    See machine/tnzs.c for an example.

***************************************************************************/

#include "driver.h"
#include "osd_cpu.h"


#define VERBOSE 0

/* #define MEM_DUMP */

#ifdef MEM_DUMP
static void mem_dump( void );
#endif

/* Convenience macros - not in cpuintrf.h because they shouldn't be used by everyone */
#define ADDRESS_BITS(index) 			(cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].address_bits)
#define ABITS1(index)					(cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].abits1)
#define ABITS2(index)					(cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].abits2)
#define ABITS3(index)					(0)
#define ABITSMIN(index) 				(cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].abitsmin)

#if LSB_FIRST
	#define BYTE_XOR_BE(a) ((a) ^ 1)
	#define BYTE_XOR_LE(a) (a)
#else
	#define BYTE_XOR_BE(a) (a)
	#define BYTE_XOR_LE(a) ((a) ^ 1)
#endif

unsigned char *OP_RAM;
unsigned char *OP_ROM;

/* change bases preserving opcode/data shift for encrypted games */
#define SET_OP_RAMROM(base) 				\
	OP_ROM = (base) + (OP_ROM - OP_RAM);	\
	OP_RAM = (base);


MHELE ophw; 			/* op-code hardware number */

struct ExtMemory ext_memory[MAX_EXT_MEMORY];

static unsigned char *ramptr[MAX_CPU],*romptr[MAX_CPU];

/* element shift bits, mask bits */
int mhshift[MAX_CPU][3], mhmask[MAX_CPU][3];

/* pointers to port structs */
/* ASG: port speedup */
static struct IOReadPort *readport[MAX_CPU];
static struct IOWritePort *writeport[MAX_CPU];
static int portmask[MAX_CPU];
static int readport_size[MAX_CPU];
static int writeport_size[MAX_CPU];
/* HJB 990210: removed 'static' for access by assembly CPU core memory handlers */
const struct IOReadPort *cur_readport;
const struct IOWritePort *cur_writeport;
int cur_portmask;

/* current hardware element map */
static MHELE *cur_mr_element[MAX_CPU];
static MHELE *cur_mw_element[MAX_CPU];

/* sub memory/port hardware element map */
/* HJB 990210: removed 'static' for access by assembly CPU core memory handlers */
MHELE readhardware[MH_ELEMAX << MH_SBITS];	/* mem/port read  */
MHELE writehardware[MH_ELEMAX << MH_SBITS]; /* mem/port write */

/* memory hardware element map */
/* value:					   */
#define HT_RAM	  0 	/* RAM direct		 */
#define HT_BANK1  1 	/* bank memory #1	 */
#define HT_BANK2  2 	/* bank memory #2	 */
#define HT_BANK3  3 	/* bank memory #3	 */
#define HT_BANK4  4 	/* bank memory #4	 */
#define HT_BANK5  5 	/* bank memory #5	 */
#define HT_BANK6  6 	/* bank memory #6	 */
#define HT_BANK7  7 	/* bank memory #7	 */
#define HT_BANK8  8 	/* bank memory #8	 */
#define HT_BANK9  9 	/* bank memory #9	 */
#define HT_BANK10 10	/* bank memory #10	 */
#define HT_BANK11 11	/* bank memory #11	 */
#define HT_BANK12 12	/* bank memory #12	 */
#define HT_BANK13 13	/* bank memory #13	 */
#define HT_BANK14 14	/* bank memory #14	 */
#define HT_BANK15 15	/* bank memory #15	 */
#define HT_BANK16 16	/* bank memory #16	 */
#define HT_NON	  17	/* non mapped memory */
#define HT_NOP	  18	/* NOP memory		 */
#define HT_RAMROM 19	/* RAM ROM memory	 */
#define HT_ROM	  20	/* ROM memory		 */

#define HT_USER   21	/* user functions	 */
/* [MH_HARDMAX]-0xff	  link to sub memory element  */
/*						  (value-MH_HARDMAX)<<MH_SBITS -> element bank */

#define HT_BANKMAX (HT_BANK1 + MAX_BANKS - 1)

/* memory hardware handler */
/* HJB 990210: removed 'static' for access by assembly CPU core memory handlers */
mem_read_handler memoryreadhandler[MH_HARDMAX];
int memoryreadoffset[MH_HARDMAX];
mem_write_handler memorywritehandler[MH_HARDMAX];
int memorywriteoffset[MH_HARDMAX];

/* bank ram base address; RAM is bank 0 */
unsigned char *cpu_bankbase[HT_BANKMAX + 1];
static int bankreadoffset[HT_BANKMAX + 1];
static int bankwriteoffset[HT_BANKMAX + 1];

/* override OP base handler */
static opbase_handler setOPbasefunc[MAX_CPU];
static opbase_handler OPbasefunc;

/* current cpu current hardware element map point */
MHELE *cur_mrhard;
MHELE *cur_mwhard;

/* empty port handler structures */
static struct IOReadPort empty_readport[] =
{
	{ -1 }
};

static struct IOWritePort empty_writeport[] =
{
	{ -1 }
};

static void *install_port_read_handler_common(int cpu, int start, int end, mem_read_handler handler, int install_at_beginning);
static void *install_port_write_handler_common(int cpu, int start, int end, mem_write_handler handler, int install_at_beginning);


/***************************************************************************

  Memory read handling

***************************************************************************/

READ_HANDLER(mrh_ram)		{ return cpu_bankbase[0][offset]; }
READ_HANDLER(mrh_bank1) 	{ return cpu_bankbase[1][offset]; }
READ_HANDLER(mrh_bank2) 	{ return cpu_bankbase[2][offset]; }
READ_HANDLER(mrh_bank3) 	{ return cpu_bankbase[3][offset]; }
READ_HANDLER(mrh_bank4) 	{ return cpu_bankbase[4][offset]; }
READ_HANDLER(mrh_bank5) 	{ return cpu_bankbase[5][offset]; }
READ_HANDLER(mrh_bank6) 	{ return cpu_bankbase[6][offset]; }
READ_HANDLER(mrh_bank7) 	{ return cpu_bankbase[7][offset]; }
READ_HANDLER(mrh_bank8) 	{ return cpu_bankbase[8][offset]; }
READ_HANDLER(mrh_bank9) 	{ return cpu_bankbase[9][offset]; }
READ_HANDLER(mrh_bank10)	{ return cpu_bankbase[10][offset]; }
READ_HANDLER(mrh_bank11)	{ return cpu_bankbase[11][offset]; }
READ_HANDLER(mrh_bank12)	{ return cpu_bankbase[12][offset]; }
READ_HANDLER(mrh_bank13)	{ return cpu_bankbase[13][offset]; }
READ_HANDLER(mrh_bank14)	{ return cpu_bankbase[14][offset]; }
READ_HANDLER(mrh_bank15)	{ return cpu_bankbase[15][offset]; }
READ_HANDLER(mrh_bank16)	{ return cpu_bankbase[16][offset]; }
static mem_read_handler bank_read_handler[] =
{
	mrh_ram,   mrh_bank1,  mrh_bank2,  mrh_bank3,  mrh_bank4,  mrh_bank5,  mrh_bank6,  mrh_bank7,
	mrh_bank8, mrh_bank9,  mrh_bank10, mrh_bank11, mrh_bank12, mrh_bank13, mrh_bank14, mrh_bank15,
	mrh_bank16
};

READ_HANDLER(mrh_error)
{
	logerror("CPU #%d PC %04x: warning - read %02x from unmapped memory address %04x\n",cpu_getactivecpu(),cpu_get_pc(),cpu_bankbase[0][offset],offset);
	return cpu_bankbase[0][offset];
}

READ_HANDLER(mrh_error_sparse)
{
	logerror("CPU #%d PC %08x: warning - read unmapped memory address %08x\n",cpu_getactivecpu(),cpu_get_pc(),offset);
	return 0;
}

READ_HANDLER(mrh_error_sparse_bit)
{
	logerror("CPU #%d PC %08x: warning - read unmapped memory bit addr %08x (byte addr %08x)\n",cpu_getactivecpu(),cpu_get_pc(),offset<<3, offset);
	return 0;
}

READ_HANDLER(mrh_nop)
{
	return 0;
}


/***************************************************************************

  Memory write handling

***************************************************************************/

WRITE_HANDLER(mwh_ram)		{ cpu_bankbase[0][offset] = data;}
WRITE_HANDLER(mwh_bank1)	{ cpu_bankbase[1][offset] = data; }
WRITE_HANDLER(mwh_bank2)	{ cpu_bankbase[2][offset] = data; }
WRITE_HANDLER(mwh_bank3)	{ cpu_bankbase[3][offset] = data; }
WRITE_HANDLER(mwh_bank4)	{ cpu_bankbase[4][offset] = data; }
WRITE_HANDLER(mwh_bank5)	{ cpu_bankbase[5][offset] = data; }
WRITE_HANDLER(mwh_bank6)	{ cpu_bankbase[6][offset] = data; }
WRITE_HANDLER(mwh_bank7)	{ cpu_bankbase[7][offset] = data; }
WRITE_HANDLER(mwh_bank8)	{ cpu_bankbase[8][offset] = data; }
WRITE_HANDLER(mwh_bank9)	{ cpu_bankbase[9][offset] = data; }
WRITE_HANDLER(mwh_bank10)	{ cpu_bankbase[10][offset] = data; }
WRITE_HANDLER(mwh_bank11)	{ cpu_bankbase[11][offset] = data; }
WRITE_HANDLER(mwh_bank12)	{ cpu_bankbase[12][offset] = data; }
WRITE_HANDLER(mwh_bank13)	{ cpu_bankbase[13][offset] = data; }
WRITE_HANDLER(mwh_bank14)	{ cpu_bankbase[14][offset] = data; }
WRITE_HANDLER(mwh_bank15)	{ cpu_bankbase[15][offset] = data; }
WRITE_HANDLER(mwh_bank16)	{ cpu_bankbase[16][offset] = data; }
static mem_write_handler bank_write_handler[] =
{
	mwh_ram,   mwh_bank1,  mwh_bank2,  mwh_bank3,  mwh_bank4,  mwh_bank5,  mwh_bank6,  mwh_bank7,
	mwh_bank8, mwh_bank9,  mwh_bank10, mwh_bank11, mwh_bank12, mwh_bank13, mwh_bank14, mwh_bank15,
	mwh_bank16
};

WRITE_HANDLER(mwh_error)
{
	logerror("CPU #%d PC %04x: warning - write %02x to unmapped memory address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset);
	cpu_bankbase[0][offset] = data;
}

WRITE_HANDLER(mwh_error_sparse)
{
	logerror("CPU #%d PC %08x: warning - write %02x to unmapped memory address %08x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset);
}

WRITE_HANDLER(mwh_error_sparse_bit)
{
	logerror("CPU #%d PC %08x: warning - write %02x to unmapped memory bit addr %08x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset<<3);
}

WRITE_HANDLER(mwh_rom)
{
	logerror("CPU #%d PC %04x: warning - write %02x to ROM address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset);
}

WRITE_HANDLER(mwh_ramrom)
{
	cpu_bankbase[0][offset] = cpu_bankbase[0][offset + (OP_ROM - OP_RAM)] = data;
}

WRITE_HANDLER(mwh_nop)
{
}


/***************************************************************************

  Memory structure building

***************************************************************************/

/* return element offset */
static MHELE *get_element( MHELE *element , int ad , int elemask ,
						MHELE *subelement , int *ele_max )
{
	MHELE hw = element[ad];
	int i,ele;
	int banks = ( elemask / (1<<MH_SBITS) ) + 1;

	if( hw >= MH_HARDMAX ) return &subelement[(hw-MH_HARDMAX)<<MH_SBITS];

	/* create new element block */
	if( (*ele_max)+banks > MH_ELEMAX )
	{
		logerror("memory element size overflow\n");
		return 0;
	}
	/* get new element nunber */
	ele = *ele_max;
	(*ele_max)+=banks;
#ifdef MEM_DUMP
	logerror("create element %2d(%2d)\n",ele,banks);
#endif
	/* set link mark to current element */
	element[ad] = ele + MH_HARDMAX;
	/* get next subelement top */
	subelement	= &subelement[ele<<MH_SBITS];
	/* initialize new block */
	for( i = 0 ; i < (1<<MH_SBITS) ; i++ )
		subelement[i] = hw;

	return subelement;
}

static void set_element( int cpu , MHELE *celement , int sp , int ep , MHELE type , MHELE *subelement , int *ele_max )
{
	int i;
	int edepth = 0;
	int shift,mask;
	MHELE *eele = celement;
	MHELE *sele = celement;
	MHELE *ele;
	int ss,sb,eb,ee;

#ifdef MEM_DUMP
	logerror("set_element %8X-%8X = %2X\n",sp,ep,type);
#endif
	if( (unsigned int) sp > (unsigned int) ep ) return;
	do{
		mask  = mhmask[cpu][edepth];
		shift = mhshift[cpu][edepth];

		/* center element */
		ss = (unsigned int) sp >> shift;
		sb = (unsigned int) sp ? ((unsigned int) (sp-1) >> shift) + 1 : 0;
		eb = ((unsigned int) (ep+1) >> shift) - 1;
		ee = (unsigned int) ep >> shift;

		if( sb <= eb )
		{
			if( (sb|mask)==(eb|mask) )
			{
				/* same reason */
				ele = (sele ? sele : eele);
				for( i = sb ; i <= eb ; i++ ){
					ele[i & mask] = type;
				}
			}
			else
			{
				if( sele ) for( i = sb ; i <= (sb|mask) ; i++ )
					sele[i & mask] = type;
				if( eele ) for( i = eb&(~mask) ; i <= eb ; i++ )
					eele[i & mask] = type;
			}
		}

		edepth++;

		if( ss == sb ) sele = 0;
		else sele = get_element( sele , ss & mask , mhmask[cpu][edepth] ,
									subelement , ele_max );
		if( ee == eb ) eele = 0;
		else eele = get_element( eele , ee & mask , mhmask[cpu][edepth] ,
									subelement , ele_max );

	}while( sele || eele );
}


/* ASG 980121 -- allocate all the external memory */
static int memory_allocate_ext (void)
{
	struct ExtMemory *ext = ext_memory;
	int cpu;

	/* a change for MESS */
	if (Machine->gamedrv->rom == 0)  return 1;

	/* loop over all CPUs */
	for (cpu = 0; cpu < cpu_gettotalcpu (); cpu++)
	{
		const struct MemoryReadAddress *mra;
		const struct MemoryWriteAddress *mwa;

		int region = REGION_CPU1+cpu;
		int size = memory_region_length(region);

		/* now it's time to loop */
		while (1)
		{
			int lowest = 0x7fffffff, end, lastend;

			/* find the base of the lowest memory region that extends past the end */
			for (mra = Machine->drv->cpu[cpu].memory_read; mra->start != -1; mra++)
				if (mra->end >= size && mra->start < lowest) lowest = mra->start;
			for (mwa = Machine->drv->cpu[cpu].memory_write; mwa->start != -1; mwa++)
				if (mwa->end >= size && mwa->start < lowest) lowest = mwa->start;

			/* done if nothing found */
			if (lowest == 0x7fffffff)
				break;

			/* now loop until we find the end of this contiguous block of memory */
			lastend = -1;
			end = lowest;
			while (end != lastend)
			{
				lastend = end;

				/* find the base of the lowest memory region that extends past the end */
				for (mra = Machine->drv->cpu[cpu].memory_read; mra->start != -1; mra++)
					if (mra->start <= end && mra->end > end) end = mra->end + 1;
				for (mwa = Machine->drv->cpu[cpu].memory_write; mwa->start != -1; mwa++)
					if (mwa->start <= end && mwa->end > end) end = mwa->end + 1;
			}

			/* time to allocate */
			ext->start = lowest;
			ext->end = end - 1;
			ext->region = region;
			ext->data = malloc (end - lowest);

			/* if that fails, we're through */
			if (!ext->data)
				return 0;

			/* reset the memory */
			memset (ext->data, 0, end - lowest);
			size = ext->end + 1;
			ext++;
		}
	}

	return 1;
}


unsigned char *findmemorychunk(int cpu, int offset, int *chunkstart, int *chunkend)
{
	int region = REGION_CPU1+cpu;
	struct ExtMemory *ext;

	/* look in external memory first */
	for (ext = ext_memory; ext->data; ext++)
		if (ext->region == region && ext->start <= offset && ext->end >= offset)
		{
			*chunkstart = ext->start;
			*chunkend = ext->end;
			return ext->data;
		}

	/* return RAM */
	*chunkstart = 0;
	*chunkend = memory_region_length(region) - 1;
	return ramptr[cpu];
}


unsigned char *memory_find_base (int cpu, int offset)
{
	int region = REGION_CPU1+cpu;
	struct ExtMemory *ext;

	/* look in external memory first */
	for (ext = ext_memory; ext->data; ext++)
		if (ext->region == region && ext->start <= offset && ext->end >= offset)
			return ext->data + (offset - ext->start);

	return ramptr[cpu] + offset;
}

/* make these static so they can be used in a callback by game drivers */

static int rdelement_max = 0;
static int wrelement_max = 0;
static int rdhard_max = HT_USER;
static int wrhard_max = HT_USER;

/* return = FALSE:can't allocate element memory */
int memory_init(void)
{
	int i, cpu;
	const struct MemoryReadAddress *memoryread;
	const struct MemoryWriteAddress *memorywrite;
	const struct MemoryReadAddress *mra;
	const struct MemoryWriteAddress *mwa;
	const struct IOReadPort *ioread;
	const struct IOWritePort *iowrite;
	int abits1,abits2,abits3,abitsmin;
	rdelement_max = 0;
	wrelement_max = 0;
	rdhard_max = HT_USER;
	wrhard_max = HT_USER;

	for( cpu = 0 ; cpu < MAX_CPU ; cpu++ )
		cur_mr_element[cpu] = cur_mw_element[cpu] = 0;

	ophw = 0xff;

	/* ASG 980121 -- allocate external memory */
	if (!memory_allocate_ext ())
		return 0;

	for( cpu = 0 ; cpu < cpu_gettotalcpu() ; cpu++ )
	{
		const struct MemoryReadAddress *_mra;
		const struct MemoryWriteAddress *_mwa;

		setOPbasefunc[cpu] = NULL;

		ramptr[cpu] = romptr[cpu] = memory_region(REGION_CPU1+cpu);

		/* initialize the memory base pointers for memory hooks */
		_mra = Machine->drv->cpu[cpu].memory_read;
		if (_mra)
		{
			while (_mra->start != -1)
			{
//				if (_mra->base) *_mra->base = memory_find_base (cpu, _mra->start);
//				if (_mra->size) *_mra->size = _mra->end - _mra->start + 1;
				_mra++;
			}
		}
		_mwa = Machine->drv->cpu[cpu].memory_write;
		if (_mwa)
		{
			while (_mwa->start != -1)
			{
				if (_mwa->base) *_mwa->base = memory_find_base (cpu, _mwa->start);
				if (_mwa->size) *_mwa->size = _mwa->end - _mwa->start + 1;
				_mwa++;
			}
		}

		/* initialize port structures */
		readport_size[cpu] = 0;
		writeport_size[cpu] = 0;
		readport[cpu] = 0;
		writeport[cpu] = 0;

		/* install port handlers - at least an empty one */
		ioread = Machine->drv->cpu[cpu].port_read;
		if (ioread == 0)  ioread = empty_readport;

		while (1)
		{
			if (install_port_read_handler_common(cpu, ioread->start, ioread->end, ioread->handler, 0) == 0)
			{
				memory_shutdown();
				return 0;
			}

			if (ioread->start == -1)  break;

			ioread++;
		}


		iowrite = Machine->drv->cpu[cpu].port_write;
		if (iowrite == 0)  iowrite = empty_writeport;

		while (1)
		{
			if (install_port_write_handler_common(cpu, iowrite->start, iowrite->end, iowrite->handler, 0) == 0)
			{
				memory_shutdown();
				return 0;
			}

			if (iowrite->start == -1)  break;

			iowrite++;
		}

		portmask[cpu] = 0xffff;
#if (HAS_Z80)
		if ((Machine->drv->cpu[cpu].cpu_type & ~CPU_FLAGS_MASK) == CPU_Z80 &&
			(Machine->drv->cpu[cpu].cpu_type & CPU_16BIT_PORT) == 0)
			portmask[cpu] = 0xff;
#endif
	}

	/* initialize global handler */
	for( i = 0 ; i < MH_HARDMAX ; i++ ){
		memoryreadoffset[i] = 0;
		memorywriteoffset[i] = 0;
	}
	/* bank memory */
	for (i = 1; i <= MAX_BANKS; i++)
	{
		memoryreadhandler[i] = bank_read_handler[i];
		memorywritehandler[i] = bank_write_handler[i];
	}
	/* non map memory */
	memoryreadhandler[HT_NON] = mrh_error;
	memorywritehandler[HT_NON] = mwh_error;
	/* NOP memory */
	memoryreadhandler[HT_NOP] = mrh_nop;
	memorywritehandler[HT_NOP] = mwh_nop;
	/* RAMROM memory */
	memorywritehandler[HT_RAMROM] = mwh_ramrom;
	/* ROM memory */
	memorywritehandler[HT_ROM] = mwh_rom;

	/* if any CPU is 21-bit or more, we change the error handlers to be more benign */
	for (cpu = 0; cpu < cpu_gettotalcpu(); cpu++)
		if (ADDRESS_BITS (cpu) >= 21)
		{
			memoryreadhandler[HT_NON] = mrh_error_sparse;
			memorywritehandler[HT_NON] = mwh_error_sparse;
#if (HAS_TMS34010)
			if ((Machine->drv->cpu[cpu].cpu_type & ~CPU_FLAGS_MASK)==CPU_TMS34010)
			{
				memoryreadhandler[HT_NON] = mrh_error_sparse_bit;
				memorywritehandler[HT_NON] = mwh_error_sparse_bit;
			}
#endif
		}

	for( cpu = 0 ; cpu < cpu_gettotalcpu() ; cpu++ )
	{
		/* cpu selection */
		abits1 = ABITS1 (cpu);
		abits2 = ABITS2 (cpu);
		abits3 = ABITS3 (cpu);
		abitsmin = ABITSMIN (cpu);

		/* element shifter , mask set */
		mhshift[cpu][0] = (abits2+abits3);
		mhshift[cpu][1] = abits3;			/* 2nd */
		mhshift[cpu][2] = 0;				/* 3rd (used by set_element)*/
		mhmask[cpu][0]	= MHMASK(abits1);		/*1st(used by set_element)*/
		mhmask[cpu][1]	= MHMASK(abits2);		/*2nd*/
		mhmask[cpu][2]	= MHMASK(abits3);		/*3rd*/

		/* allocate current element */
		if( (cur_mr_element[cpu] = (MHELE *)malloc(sizeof(MHELE)<<abits1)) == 0 )
		{
			memory_shutdown();
			return 0;
		}
		if( (cur_mw_element[cpu] = (MHELE *)malloc(sizeof(MHELE)<<abits1)) == 0 )
		{
			memory_shutdown();
			return 0;
		}

		/* initialize current element table */
		for( i = 0 ; i < (1<<abits1) ; i++ )
		{
			cur_mr_element[cpu][i] = HT_NON;	/* no map memory */
			cur_mw_element[cpu][i] = HT_NON;	/* no map memory */
		}

		memoryread = Machine->drv->cpu[cpu].memory_read;
		memorywrite = Machine->drv->cpu[cpu].memory_write;

		/* memory read handler build */
		if (memoryread)
		{
			mra = memoryread;
			while (mra->start != -1) mra++;
			mra--;

			while (mra >= memoryread)
			{
				install_mem_read_handler (cpu, mra->start, mra->end, mra->handler);
				mra--;
			}
		}

		/* memory write handler build */
		if (memorywrite)
		{
			mwa = memorywrite;
			while (mwa->start != -1) mwa++;
			mwa--;

			while (mwa >= memorywrite)
			{
				install_mem_write_handler (cpu, mwa->start, mwa->end, mwa->handler);
				mwa--;
			}
		}
	}

	logerror("used read  elements %d/%d , functions %d/%d\n"
			,rdelement_max,MH_ELEMAX , rdhard_max,MH_HARDMAX );
	logerror("used write elements %d/%d , functions %d/%d\n"
			,wrelement_max,MH_ELEMAX , wrhard_max,MH_HARDMAX );

#ifdef MEM_DUMP
	mem_dump();
#endif
	return 1;	/* ok */
}

void memory_set_opcode_base(int cpu,unsigned char *base)
{
	romptr[cpu] = base;
}


void memorycontextswap(int activecpu)
{
	cpu_bankbase[0] = ramptr[activecpu];

	cur_mrhard = cur_mr_element[activecpu];
	cur_mwhard = cur_mw_element[activecpu];

	/* ASG: port speedup */
	cur_readport = readport[activecpu];
	cur_writeport = writeport[activecpu];
	cur_portmask = portmask[activecpu];

	OPbasefunc = setOPbasefunc[activecpu];

	/* op code memory pointer */
	ophw = HT_RAM;
	OP_RAM = cpu_bankbase[0];
	OP_ROM = romptr[activecpu];
}

void memory_shutdown(void)
{
	struct ExtMemory *ext;
	int cpu;

	for( cpu = 0 ; cpu < MAX_CPU ; cpu++ )
	{
		if( cur_mr_element[cpu] != 0 )
		{
			free( cur_mr_element[cpu] );
			cur_mr_element[cpu] = 0;
		}
		if( cur_mw_element[cpu] != 0 )
		{
			free( cur_mw_element[cpu] );
			cur_mw_element[cpu] = 0;
		}

		if (readport[cpu] != 0)
		{
			free(readport[cpu]);
			readport[cpu] = 0;
		}

		if (writeport[cpu] != 0)
		{
			free(writeport[cpu]);
			writeport[cpu] = 0;
		}
	}

	/* ASG 980121 -- free all the external memory */
	for (ext = ext_memory; ext->data; ext++)
		free (ext->data);
	memset (ext_memory, 0, sizeof (ext_memory));
}



/***************************************************************************

  Perform a memory read. This function is called by the CPU emulation.

***************************************************************************/

/* use these constants to define which type of memory handler to build */
#define TYPE_8BIT					0		/* 8-bit aligned */
#define TYPE_16BIT_BE				1		/* 16-bit aligned, big-endian */
#define TYPE_16BIT_LE				2		/* 16-bit aligned, little-endian */

#define CAN_BE_MISALIGNED			0		/* word/dwords can be read on non-16-bit boundaries */
#define ALWAYS_ALIGNED				1		/* word/dwords are always read on 16-bit boundaries */

/* stupid workarounds so that we can generate an address mask that works even for 32 bits */
#define ADDRESS_TOPBIT(abits)		(1UL << (ABITS1_##abits + ABITS2_##abits + ABITS_MIN_##abits - 1))
#define ADDRESS_MASK(abits) 		(ADDRESS_TOPBIT(abits) | (ADDRESS_TOPBIT(abits) - 1))


/* generic byte-sized read handler */
#define READBYTE(name,type,abits)														\
data_t name(offs_t address) 															\
{																						\
	MHELE hw;																			\
																						\
	/* first-level lookup */															\
	hw = cur_mrhard[(UINT32)address >> (ABITS2_##abits + ABITS_MIN_##abits)];			\
																						\
	/* for compatibility with setbankhandler, 8-bit systems must call handlers */		\
	/* for banked memory reads/writes */												\
	if (type == TYPE_8BIT && hw == HT_RAM)												\
		return cpu_bankbase[HT_RAM][address];											\
	else if (type != TYPE_8BIT && hw <= HT_BANKMAX) 									\
	{																					\
		if (type == TYPE_16BIT_BE)														\
			return cpu_bankbase[hw][BYTE_XOR_BE(address) - memoryreadoffset[hw]];		\
		else if (type == TYPE_16BIT_LE) 												\
			return cpu_bankbase[hw][BYTE_XOR_LE(address) - memoryreadoffset[hw]];		\
	}																					\
																						\
	/* second-level lookup */															\
	if (hw >= MH_HARDMAX)																\
	{																					\
		hw -= MH_HARDMAX;																\
		hw = readhardware[(hw << MH_SBITS) + (((UINT32)address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))];	\
																						\
		/* for compatibility with setbankhandler, 8-bit systems must call handlers */	\
		/* for banked memory reads/writes */											\
		if (type == TYPE_8BIT && hw == HT_RAM)											\
			return cpu_bankbase[HT_RAM][address];										\
		else if (type != TYPE_8BIT && hw <= HT_BANKMAX) 								\
		{																				\
			if (type == TYPE_16BIT_BE)													\
				return cpu_bankbase[hw][BYTE_XOR_BE(address) - memoryreadoffset[hw]];	\
			else if (type == TYPE_16BIT_LE) 											\
				return cpu_bankbase[hw][BYTE_XOR_LE(address) - memoryreadoffset[hw]];	\
		}																				\
	}																					\
																						\
	/* fall back to handler */															\
	if (type == TYPE_8BIT)																\
		return (*memoryreadhandler[hw])(address - memoryreadoffset[hw]);				\
	else																				\
	{																					\
		int shift = (address & 1) << 3; 												\
		int data = (*memoryreadhandler[hw])((address & ~1) - memoryreadoffset[hw]); 	\
		if (type == TYPE_16BIT_BE)														\
			return (data >> (shift ^ 8)) & 0xff;										\
		else if (type == TYPE_16BIT_LE) 												\
			return (data >> shift) & 0xff;												\
	}																					\
}

/* generic word-sized read handler (16-bit aligned only!) */
#define READWORD(name,type,abits,align) 												\
data_t name##_word(offs_t address)														\
{																						\
	MHELE hw;																			\
																						\
	/* only supports 16-bit memory systems */											\
	if (type == TYPE_8BIT)																\
		printf("Unsupported type for READWORD macro!\n");                               \
																						\
	/* handle aligned case first */ 													\
	if (align == ALWAYS_ALIGNED || !(address & 1))										\
	{																					\
		/* first-level lookup */														\
		hw = cur_mrhard[(UINT32)address >> (ABITS2_##abits + ABITS_MIN_##abits)];		\
		if (hw <= HT_BANKMAX)															\
			return READ_WORD(&cpu_bankbase[hw][address - memoryreadoffset[hw]]);		\
																						\
		/* second-level lookup */														\
		if (hw >= MH_HARDMAX)															\
		{																				\
			hw -= MH_HARDMAX;															\
			hw = readhardware[(hw << MH_SBITS) + (((UINT32)address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))];	\
			if (hw <= HT_BANKMAX)														\
				return READ_WORD(&cpu_bankbase[hw][address - memoryreadoffset[hw]]);	\
		}																				\
																						\
		/* fall back to handler */														\
		return (*memoryreadhandler[hw])(address - memoryreadoffset[hw]);				\
	}																					\
																						\
	/* unaligned case */																\
	else if (type == TYPE_16BIT_BE) 													\
	{																					\
		int data = name(address) << 8;													\
		return data | (name(address + 1) & 0xff);										\
	}																					\
	else if (type == TYPE_16BIT_LE) 													\
	{																					\
		int data = name(address) & 0xff;												\
		return data | (name(address + 1) << 8); 										\
	}																					\
}

/* generic dword-sized read handler (16-bit aligned only!) */
#define READLONG(name,type,abits,align) 												\
data_t name##_dword(offs_t address) 													\
{																						\
	UINT16 word1, word2;																\
	MHELE hw1, hw2; 																	\
																						\
	/* only supports 16-bit memory systems */											\
	if (type == TYPE_8BIT)																\
		printf("Unsupported type for READWORD macro!\n");                               \
																						\
	/* handle aligned case first */ 													\
	if (align == ALWAYS_ALIGNED || !(address & 1))										\
	{																					\
		int address2 = (address + 2) & ADDRESS_MASK(abits); 							\
																						\
		/* first-level lookup */														\
		hw1 = cur_mrhard[(UINT32)address >> (ABITS2_##abits + ABITS_MIN_##abits)];		\
		hw2 = cur_mrhard[(UINT32)address2 >> (ABITS2_##abits + ABITS_MIN_##abits)]; 	\
																						\
		/* second-level lookup */														\
		if (hw1 >= MH_HARDMAX)															\
		{																				\
			hw1 -= MH_HARDMAX;															\
			hw1 = readhardware[(hw1 << MH_SBITS) + (((UINT32)address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))];	\
		}																				\
		if (hw2 >= MH_HARDMAX)															\
		{																				\
			hw2 -= MH_HARDMAX;															\
			hw2 = readhardware[(hw2 << MH_SBITS) + (((UINT32)address2 >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))]; \
		}																				\
																						\
		/* process each word */ 														\
		if (hw1 <= HT_BANKMAX)															\
			word1 = READ_WORD(&cpu_bankbase[hw1][address - memoryreadoffset[hw1]]); 	\
		else																			\
			word1 = (*memoryreadhandler[hw1])(address - memoryreadoffset[hw1]); 		\
		if (hw2 <= HT_BANKMAX)															\
			word2 = READ_WORD(&cpu_bankbase[hw2][address2 - memoryreadoffset[hw2]]);	\
		else																			\
			word2 = (*memoryreadhandler[hw2])(address2 - memoryreadoffset[hw2]);		\
																						\
		/* fall back to handler */														\
		if (type == TYPE_16BIT_BE)														\
			return (word1 << 16) | (word2 & 0xffff);									\
		else if (type == TYPE_16BIT_LE) 												\
			return (word1 & 0xffff) | (word2 << 16);									\
	}																					\
																						\
	/* unaligned case */																\
	else if (type == TYPE_16BIT_BE) 													\
	{																					\
		int data = name(address) << 24; 												\
		data |= name##_word(address + 1) << 8;											\
		return data | (name(address + 3) & 0xff);										\
	}																					\
	else if (type == TYPE_16BIT_LE) 													\
	{																					\
		int data = name(address) & 0xff;												\
		data |= name##_word(address + 1) << 8;											\
		return data | (name(address + 3) << 24);										\
	}																					\
}


/* the handlers we need to generate */
READBYTE(cpu_readmem16,    TYPE_8BIT,	  16)
READBYTE(cpu_readmem20,    TYPE_8BIT,	  20)
READBYTE(cpu_readmem21,    TYPE_8BIT,	  21)

READBYTE(cpu_readmem16bew, TYPE_16BIT_BE, 16BEW)
READWORD(cpu_readmem16bew, TYPE_16BIT_BE, 16BEW, ALWAYS_ALIGNED)

READBYTE(cpu_readmem16lew, TYPE_16BIT_LE, 16LEW)
READWORD(cpu_readmem16lew, TYPE_16BIT_LE, 16LEW, ALWAYS_ALIGNED)

READBYTE(cpu_readmem24,    TYPE_8BIT,	  24)

READBYTE(cpu_readmem24bew, TYPE_16BIT_BE, 24BEW)
READWORD(cpu_readmem24bew, TYPE_16BIT_BE, 24BEW, CAN_BE_MISALIGNED)
READLONG(cpu_readmem24bew, TYPE_16BIT_BE, 24BEW, CAN_BE_MISALIGNED)

READBYTE(cpu_readmem26lew, TYPE_16BIT_LE, 26LEW)
READWORD(cpu_readmem26lew, TYPE_16BIT_LE, 26LEW, ALWAYS_ALIGNED)
READLONG(cpu_readmem26lew, TYPE_16BIT_LE, 26LEW, ALWAYS_ALIGNED)

READBYTE(cpu_readmem29,    TYPE_16BIT_LE, 29)
READWORD(cpu_readmem29,    TYPE_16BIT_LE, 29,	 CAN_BE_MISALIGNED)
READLONG(cpu_readmem29,    TYPE_16BIT_LE, 29,	 CAN_BE_MISALIGNED)

READBYTE(cpu_readmem32,    TYPE_16BIT_BE, 32)
READWORD(cpu_readmem32,    TYPE_16BIT_BE, 32,	 CAN_BE_MISALIGNED)
READLONG(cpu_readmem32,    TYPE_16BIT_BE, 32,	 CAN_BE_MISALIGNED)

READBYTE(cpu_readmem32lew, TYPE_16BIT_LE, 32LEW)
READWORD(cpu_readmem32lew, TYPE_16BIT_LE, 32LEW, CAN_BE_MISALIGNED)
READLONG(cpu_readmem32lew, TYPE_16BIT_LE, 32LEW, CAN_BE_MISALIGNED)


/***************************************************************************

  Perform a memory write. This function is called by the CPU emulation.

***************************************************************************/

/* generic byte-sized write handler */
#define WRITEBYTE(name,type,abits)														\
void name(offs_t address,data_t data)													\
{																						\
	MHELE hw;																			\
																						\
	/* first-level lookup */															\
	hw = cur_mwhard[(UINT32)address >> (ABITS2_##abits + ABITS_MIN_##abits)];			\
																						\
	/* for compatibility with setbankhandler, 8-bit systems must call handlers */		\
	/* for banked memory reads/writes */												\
	if (type == TYPE_8BIT && hw == HT_RAM)												\
	{																					\
		cpu_bankbase[HT_RAM][address] = data;											\
		return; 																		\
	}																					\
	else if (type != TYPE_8BIT && hw <= HT_BANKMAX) 									\
	{																					\
		if (type == TYPE_16BIT_BE)														\
			cpu_bankbase[hw][BYTE_XOR_BE(address) - memorywriteoffset[hw]] = data;		\
		else if (type == TYPE_16BIT_LE) 												\
			cpu_bankbase[hw][BYTE_XOR_LE(address) - memorywriteoffset[hw]] = data;		\
		return; 																		\
	}																					\
																						\
	/* second-level lookup */															\
	if (hw >= MH_HARDMAX)																\
	{																					\
		hw -= MH_HARDMAX;																\
		hw = writehardware[(hw << MH_SBITS) + (((UINT32)address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))];	\
																						\
		/* for compatibility with setbankhandler, 8-bit systems must call handlers */	\
		/* for banked memory reads/writes */											\
		if (type == TYPE_8BIT && hw == HT_RAM)											\
		{																				\
			cpu_bankbase[HT_RAM][address] = data;										\
			return; 																	\
		}																				\
		else if (type != TYPE_8BIT && hw <= HT_BANKMAX) 								\
		{																				\
			if (type == TYPE_16BIT_BE)													\
				cpu_bankbase[hw][BYTE_XOR_BE(address) - memorywriteoffset[hw]] = data;	\
			else if (type == TYPE_16BIT_LE) 											\
				cpu_bankbase[hw][BYTE_XOR_LE(address) - memorywriteoffset[hw]] = data;	\
			return; 																	\
		}																				\
	}																					\
																						\
	/* fall back to handler */															\
	if (type != TYPE_8BIT)																\
	{																					\
		int shift = (address & 1) << 3; 												\
		if (type == TYPE_16BIT_BE)														\
			shift ^= 8; 																\
		data = (0xff000000 >> shift) | ((data & 0xff) << shift);						\
		address &= ~1;																	\
	}																					\
	(*memorywritehandler[hw])(address - memorywriteoffset[hw], data);					\
}

/* generic word-sized write handler (16-bit aligned only!) */
#define WRITEWORD(name,type,abits,align)												\
void name##_word(offs_t address,data_t data)											\
{																						\
	MHELE hw;																			\
																						\
	/* only supports 16-bit memory systems */											\
	if (type == TYPE_8BIT)																\
		printf("Unsupported type for WRITEWORD macro!\n");                              \
																						\
	/* handle aligned case first */ 													\
	if (align == ALWAYS_ALIGNED || !(address & 1))										\
	{																					\
		/* first-level lookup */														\
		hw = cur_mwhard[(UINT32)address >> (ABITS2_##abits + ABITS_MIN_##abits)];		\
		if (hw <= HT_BANKMAX)															\
		{																				\
			WRITE_WORD(&cpu_bankbase[hw][address - memorywriteoffset[hw]], data);		\
			return; 																	\
		}																				\
																						\
		/* second-level lookup */														\
		if (hw >= MH_HARDMAX)															\
		{																				\
			hw -= MH_HARDMAX;															\
			hw = writehardware[(hw << MH_SBITS) + (((UINT32)address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))]; \
			if (hw <= HT_BANKMAX)														\
			{																			\
				WRITE_WORD(&cpu_bankbase[hw][address - memorywriteoffset[hw]], data);	\
				return; 																\
			}																			\
		}																				\
																						\
		/* fall back to handler */														\
		(*memorywritehandler[hw])(address - memorywriteoffset[hw], data & 0xffff);		\
	}																					\
																						\
	/* unaligned case */																\
	else if (type == TYPE_16BIT_BE) 													\
	{																					\
		name(address, data >> 8);														\
		name(address + 1, data & 0xff); 												\
	}																					\
	else if (type == TYPE_16BIT_LE) 													\
	{																					\
		name(address, data & 0xff); 													\
		name(address + 1, data >> 8);													\
	}																					\
}

/* generic dword-sized write handler (16-bit aligned only!) */
#define WRITELONG(name,type,abits,align)												\
void name##_dword(offs_t address,data_t data)											\
{																						\
	UINT16 word1, word2;																\
	MHELE hw1, hw2; 																	\
																						\
	/* only supports 16-bit memory systems */											\
	if (type == TYPE_8BIT)																\
		printf("Unsupported type for WRITEWORD macro!\n");                              \
																						\
	/* handle aligned case first */ 													\
	if (align == ALWAYS_ALIGNED || !(address & 1))										\
	{																					\
		int address2 = (address + 2) & ADDRESS_MASK(abits); 							\
																						\
		/* first-level lookup */														\
		hw1 = cur_mwhard[(UINT32)address >> (ABITS2_##abits + ABITS_MIN_##abits)];		\
		hw2 = cur_mwhard[(UINT32)address2 >> (ABITS2_##abits + ABITS_MIN_##abits)]; 	\
																						\
		/* second-level lookup */														\
		if (hw1 >= MH_HARDMAX)															\
		{																				\
			hw1 -= MH_HARDMAX;															\
			hw1 = writehardware[(hw1 << MH_SBITS) + (((UINT32)address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))]; \
		}																				\
		if (hw2 >= MH_HARDMAX)															\
		{																				\
			hw2 -= MH_HARDMAX;															\
			hw2 = writehardware[(hw2 << MH_SBITS) + (((UINT32)address2 >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))];	\
		}																				\
																						\
		/* extract words */ 															\
		if (type == TYPE_16BIT_BE)														\
		{																				\
			word1 = data >> 16; 														\
			word2 = data & 0xffff;														\
		}																				\
		else if (type == TYPE_16BIT_LE) 												\
		{																				\
			word1 = data & 0xffff;														\
			word2 = data >> 16; 														\
		}																				\
																						\
		/* process each word */ 														\
		if (hw1 <= HT_BANKMAX)															\
			WRITE_WORD(&cpu_bankbase[hw1][address - memorywriteoffset[hw1]], word1);	\
		else																			\
			(*memorywritehandler[hw1])(address - memorywriteoffset[hw1], word1);		\
		if (hw2 <= HT_BANKMAX)															\
			WRITE_WORD(&cpu_bankbase[hw2][address2 - memorywriteoffset[hw2]], word2);	\
		else																			\
			(*memorywritehandler[hw2])(address2 - memorywriteoffset[hw2], word2);		\
	}																					\
																						\
	/* unaligned case */																\
	else if (type == TYPE_16BIT_BE) 													\
	{																					\
		name(address, data >> 24);														\
		name##_word(address + 1, (data >> 8) & 0xffff); 								\
		name(address + 3, data & 0xff); 												\
	}																					\
	else if (type == TYPE_16BIT_LE) 													\
	{																					\
		name(address, data & 0xff); 													\
		name##_word(address + 1, (data >> 8) & 0xffff); 								\
		name(address + 3, data >> 24);													\
	}																					\
}


/* the handlers we need to generate */
WRITEBYTE(cpu_writemem16,	 TYPE_8BIT, 	16)
WRITEBYTE(cpu_writemem20,	 TYPE_8BIT, 	20)
WRITEBYTE(cpu_writemem21,	 TYPE_8BIT, 	21)

WRITEBYTE(cpu_writemem16bew, TYPE_16BIT_BE, 16BEW)
WRITEWORD(cpu_writemem16bew, TYPE_16BIT_BE, 16BEW, ALWAYS_ALIGNED)

WRITEBYTE(cpu_writemem16lew, TYPE_16BIT_LE, 16LEW)
WRITEWORD(cpu_writemem16lew, TYPE_16BIT_LE, 16LEW, ALWAYS_ALIGNED)

WRITEBYTE(cpu_writemem24,	 TYPE_8BIT, 	24)

WRITEBYTE(cpu_writemem24bew, TYPE_16BIT_BE, 24BEW)
WRITEWORD(cpu_writemem24bew, TYPE_16BIT_BE, 24BEW, CAN_BE_MISALIGNED)
WRITELONG(cpu_writemem24bew, TYPE_16BIT_BE, 24BEW, CAN_BE_MISALIGNED)

WRITEBYTE(cpu_writemem26lew, TYPE_16BIT_LE, 26LEW)
WRITEWORD(cpu_writemem26lew, TYPE_16BIT_LE, 26LEW, ALWAYS_ALIGNED)
WRITELONG(cpu_writemem26lew, TYPE_16BIT_LE, 26LEW, ALWAYS_ALIGNED)

WRITEBYTE(cpu_writemem29,	 TYPE_16BIT_LE, 29)
WRITEWORD(cpu_writemem29,	 TYPE_16BIT_LE, 29,    CAN_BE_MISALIGNED)
WRITELONG(cpu_writemem29,	 TYPE_16BIT_LE, 29,    CAN_BE_MISALIGNED)

WRITEBYTE(cpu_writemem32,	 TYPE_16BIT_BE, 32)
WRITEWORD(cpu_writemem32,	 TYPE_16BIT_BE, 32,    CAN_BE_MISALIGNED)
WRITELONG(cpu_writemem32,	 TYPE_16BIT_BE, 32,    CAN_BE_MISALIGNED)

WRITEBYTE(cpu_writemem32lew, TYPE_16BIT_LE, 32LEW)
WRITEWORD(cpu_writemem32lew, TYPE_16BIT_LE, 32LEW, CAN_BE_MISALIGNED)
WRITELONG(cpu_writemem32lew, TYPE_16BIT_LE, 32LEW, CAN_BE_MISALIGNED)


/***************************************************************************

  Opcode base changers. This function is called by the CPU emulation.

***************************************************************************/

/* generic opcode base changer */
#define SETOPBASE(name,abits,shift) 													\
void name(int pc)																		\
{																						\
	MHELE hw;																			\
																						\
	pc = (UINT32)pc >> shift;															\
																						\
	/* allow overrides */																\
	if (OPbasefunc) 																	\
	{																					\
		pc = OPbasefunc(pc);															\
		if (pc == -1)																	\
			return; 																	\
	}																					\
																						\
	/* perform the lookup */															\
	hw = cur_mrhard[(UINT32)pc >> (ABITS2_##abits + ABITS_MIN_##abits)];				\
	if (hw >= MH_HARDMAX)																\
	{																					\
		hw -= MH_HARDMAX;																\
		hw = readhardware[(hw << MH_SBITS) + (((UINT32)pc >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))]; \
	}																					\
	ophw = hw;																			\
																						\
	/* RAM or banked memory */															\
	if (hw <= HT_BANKMAX)																\
	{																					\
		SET_OP_RAMROM(cpu_bankbase[hw] - memoryreadoffset[hw])							\
		return; 																		\
	}																					\
																						\
	/* do not support on callback memory region */										\
	logerror("CPU #%d PC %04x: warning - op-code execute on mapped i/o\n",              \
				cpu_getactivecpu(),cpu_get_pc());										\
}


/* the handlers we need to generate */
SETOPBASE(cpu_setOPbase16,	  16,	 0)
SETOPBASE(cpu_setOPbase16bew, 16BEW, 0)
SETOPBASE(cpu_setOPbase16lew, 16LEW, 0)
SETOPBASE(cpu_setOPbase20,	  20,	 0)
SETOPBASE(cpu_setOPbase21,	  21,	 0)
SETOPBASE(cpu_setOPbase24,	  24,	 0)
SETOPBASE(cpu_setOPbase24bew, 24BEW, 0)
SETOPBASE(cpu_setOPbase29,	  29,	 3)
SETOPBASE(cpu_setOPbase32,	  32,	 0)
SETOPBASE(cpu_setOPbase32lew, 32LEW, 0)


/***************************************************************************

  Perform an I/O port read. This function is called by the CPU emulation.

***************************************************************************/
int cpu_readport(int port)
{
	const struct IOReadPort *iorp = cur_readport;

	port &= cur_portmask;

	/* search the handlers. The order is as follows: first the dynamically installed
	   handlers are searched, followed by the static ones in whatever order they were
	   specified in the driver */
	while (iorp->start != -1)
	{
		if (port >= iorp->start && port <= iorp->end)
		{
			mem_read_handler handler = iorp->handler;


			if (handler == IORP_NOP) return 0;
			else return (*handler)(port - iorp->start);
		}

		iorp++;
	}

	logerror("CPU #%d PC %04x: warning - read unmapped I/O port %02x\n",cpu_getactivecpu(),cpu_get_pc(),port);
	return 0;
}


/***************************************************************************

  Perform an I/O port write. This function is called by the CPU emulation.

***************************************************************************/
void cpu_writeport(int port, int value)
{
	const struct IOWritePort *iowp = cur_writeport;

	port &= cur_portmask;

	/* search the handlers. The order is as follows: first the dynamically installed
	   handlers are searched, followed by the static ones in whatever order they were
	   specified in the driver */
	while (iowp->start != -1)
	{
		if (port >= iowp->start && port <= iowp->end)
		{
			mem_write_handler handler = iowp->handler;


			if (handler == IOWP_NOP) return;
			else (*handler)(port - iowp->start,value);

			return;
		}

		iowp++;
	}

	logerror("CPU #%d PC %04x: warning - write %02x to unmapped I/O port %02x\n",cpu_getactivecpu(),cpu_get_pc(),value,port);
}


/* set readmemory handler for bank memory  */
void cpu_setbankhandler_r(int bank, mem_read_handler handler)
{
	int offset = 0;
	MHELE hardware;

	switch( (FPTR)handler )
	{
	case (FPTR)MRA_RAM:
	case (FPTR)MRA_ROM:
		handler = mrh_ram;
		break;
	case (FPTR)MRA_BANK1:
	case (FPTR)MRA_BANK2:
	case (FPTR)MRA_BANK3:
	case (FPTR)MRA_BANK4:
	case (FPTR)MRA_BANK5:
	case (FPTR)MRA_BANK6:
	case (FPTR)MRA_BANK7:
	case (FPTR)MRA_BANK8:
	case (FPTR)MRA_BANK9:
	case (FPTR)MRA_BANK10:
	case (FPTR)MRA_BANK11:
	case (FPTR)MRA_BANK12:
	case (FPTR)MRA_BANK13:
	case (FPTR)MRA_BANK14:
	case (FPTR)MRA_BANK15:
	case (FPTR)MRA_BANK16:
		hardware = (int)MWA_BANK1 - (int)handler + 1;
		handler = bank_read_handler[hardware];
		offset = bankreadoffset[hardware];
		break;
	case (FPTR)MRA_NOP:
		handler = mrh_nop;
		break;
	default:
		offset = bankreadoffset[bank];
		break;
	}
	memoryreadoffset[bank] = offset;
	memoryreadhandler[bank] = handler;
}

/* set writememory handler for bank memory	*/
void cpu_setbankhandler_w(int bank, mem_write_handler handler)
{
	int offset = 0;
	MHELE hardware;

	switch( (FPTR)handler )
	{
	case (FPTR)MWA_RAM:
		handler = mwh_ram;
		break;
	case (FPTR)MWA_BANK1:
	case (FPTR)MWA_BANK2:
	case (FPTR)MWA_BANK3:
	case (FPTR)MWA_BANK4:
	case (FPTR)MWA_BANK5:
	case (FPTR)MWA_BANK6:
	case (FPTR)MWA_BANK7:
	case (FPTR)MWA_BANK8:
	case (FPTR)MWA_BANK9:
	case (FPTR)MWA_BANK10:
	case (FPTR)MWA_BANK11:
	case (FPTR)MWA_BANK12:
	case (FPTR)MWA_BANK13:
	case (FPTR)MWA_BANK14:
	case (FPTR)MWA_BANK15:
	case (FPTR)MWA_BANK16:
		hardware = (int)MWA_BANK1 - (int)handler + 1;
		handler = bank_write_handler[hardware];
		offset = bankwriteoffset[hardware];
		break;
	case (FPTR)MWA_NOP:
		handler = mwh_nop;
		break;
	case (FPTR)MWA_RAMROM:
		handler = mwh_ramrom;
		break;
	case (FPTR)MWA_ROM:
		handler = mwh_rom;
		break;
	default:
		offset = bankwriteoffset[bank];
		break;
	}
	memorywriteoffset[bank] = offset;
	memorywritehandler[bank] = handler;
}

/* cpu change op-code memory base */
void cpu_setOPbaseoverride (int cpu,opbase_handler function)
{
	setOPbasefunc[cpu] = function;
	if (cpu == cpu_getactivecpu())
		OPbasefunc = function;
}


void *install_mem_read_handler(int cpu, int start, int end, mem_read_handler handler)
{
	MHELE hardware = 0;
	int abitsmin;
	int i, hw_set;
#if VERBOSE
	logerror("Install new memory read handler:\n");
	logerror("             cpu: %d\n", cpu);
	logerror("           start: 0x%08x\n", start);
	logerror("             end: 0x%08x\n", end);
#ifdef __LP64__
	logerror(" handler address: 0x%016lx\n", (unsigned long) handler);
#else
	logerror(" handler address: 0x%08x\n", (unsigned int) handler);
#endif
#endif
	abitsmin = ABITSMIN (cpu);

	/* see if this function is already registered */
	hw_set = 0;
	for ( i = 0 ; i < MH_HARDMAX ; i++)
	{
		/* record it if it matches */
		if (( memoryreadhandler[i] == handler ) &&
			(  memoryreadoffset[i] == start))
		{
#if VERBOSE
			logerror("handler match - use old one\n");
#endif
			hardware = i;
			hw_set = 1;
		}
	}
	switch ((FPTR)handler)
	{
		case (FPTR)MRA_RAM:
		case (FPTR)MRA_ROM:
			hardware = HT_RAM;	/* special case ram read */
			hw_set = 1;
			break;
		case (FPTR)MRA_BANK1:
		case (FPTR)MRA_BANK2:
		case (FPTR)MRA_BANK3:
		case (FPTR)MRA_BANK4:
		case (FPTR)MRA_BANK5:
		case (FPTR)MRA_BANK6:
		case (FPTR)MRA_BANK7:
		case (FPTR)MRA_BANK8:
		case (FPTR)MRA_BANK9:
		case (FPTR)MRA_BANK10:
		case (FPTR)MRA_BANK11:
		case (FPTR)MRA_BANK12:
		case (FPTR)MRA_BANK13:
		case (FPTR)MRA_BANK14:
		case (FPTR)MRA_BANK15:
		case (FPTR)MRA_BANK16:
		{
			hardware = (int)MRA_BANK1 - (int)handler + 1;
			memoryreadoffset[hardware] = bankreadoffset[hardware] = start;
			cpu_bankbase[hardware] = memory_find_base(cpu, start);
			hw_set = 1;
			break;
		}
		case (FPTR)MRA_NOP:
			hardware = HT_NOP;
			hw_set = 1;
			break;
	}
	if (!hw_set)  /* no match */
	{
		/* create newer hardware handler */
		if( rdhard_max == MH_HARDMAX )
		{
			logerror("read memory hardware pattern over !\n");
			logerror("Failed to install new memory handler.\n");
			return memory_find_base(cpu, start);
		}
		else
		{
			/* register hardware function */
			hardware = rdhard_max++;
			memoryreadhandler[hardware] = handler;
			memoryreadoffset[hardware] = start;
		}
	}
	/* set hardware element table entry */
	set_element( cpu , cur_mr_element[cpu] ,
		(((unsigned int) start) >> abitsmin) ,
		(((unsigned int) end) >> abitsmin) ,
		hardware , readhardware , &rdelement_max );
#if VERBOSE
	logerror("Done installing new memory handler.\n");
	logerror("used read  elements %d/%d , functions %d/%d\n"
			,rdelement_max,MH_ELEMAX , rdhard_max,MH_HARDMAX );
#endif
	return memory_find_base(cpu, start);
}

void *install_mem_write_handler(int cpu, int start, int end, mem_write_handler handler)
{
	MHELE hardware = 0;
	int abitsmin;
	int i, hw_set;
#if VERBOSE
	logerror("Install new memory write handler:\n");
	logerror("             cpu: %d\n", cpu);
	logerror("           start: 0x%08x\n", start);
	logerror("             end: 0x%08x\n", end);
#ifdef __LP64__
	logerror(" handler address: 0x%016lx\n", (unsigned long) handler);
#else
	logerror(" handler address: 0x%08x\n", (unsigned int) handler);
#endif
#endif
	abitsmin = ABITSMIN (cpu);

	/* see if this function is already registered */
	hw_set = 0;
	for ( i = 0 ; i < MH_HARDMAX ; i++)
	{
		/* record it if it matches */
		if (( memorywritehandler[i] == handler ) &&
			(  memorywriteoffset[i] == start))
		{
#if VERBOSE
			logerror("handler match - use old one\n");
#endif
			hardware = i;
			hw_set = 1;
		}
	}

	switch( (FPTR)handler )
	{
		case (FPTR)MWA_RAM:
			hardware = HT_RAM;	/* special case ram write */
			hw_set = 1;
			break;
		case (FPTR)MWA_BANK1:
		case (FPTR)MWA_BANK2:
		case (FPTR)MWA_BANK3:
		case (FPTR)MWA_BANK4:
		case (FPTR)MWA_BANK5:
		case (FPTR)MWA_BANK6:
		case (FPTR)MWA_BANK7:
		case (FPTR)MWA_BANK8:
		case (FPTR)MWA_BANK9:
		case (FPTR)MWA_BANK10:
		case (FPTR)MWA_BANK11:
		case (FPTR)MWA_BANK12:
		case (FPTR)MWA_BANK13:
		case (FPTR)MWA_BANK14:
		case (FPTR)MWA_BANK15:
		case (FPTR)MWA_BANK16:
		{
			hardware = (int)MWA_BANK1 - (int)handler + 1;
			memorywriteoffset[hardware] = bankwriteoffset[hardware] = start;
			cpu_bankbase[hardware] = memory_find_base(cpu, start);
			hw_set = 1;
			break;
		}
		case (FPTR)MWA_NOP:
			hardware = HT_NOP;
			hw_set = 1;
			break;
		case (FPTR)MWA_RAMROM:
			hardware = HT_RAMROM;
			hw_set = 1;
			break;
		case (FPTR)MWA_ROM:
			hardware = HT_ROM;
			hw_set = 1;
			break;
	}
	if (!hw_set)  /* no match */
	{
		/* create newer hardware handler */
		if( wrhard_max == MH_HARDMAX )
		{
			logerror("write memory hardware pattern over !\n");
			logerror("Failed to install new memory handler.\n");

			return memory_find_base(cpu, start);
		}
		else
		{
			/* register hardware function */
			hardware = wrhard_max++;
			memorywritehandler[hardware] = handler;
			memorywriteoffset[hardware] = start;
		}
	}
	/* set hardware element table entry */
	set_element( cpu , cur_mw_element[cpu] ,
		(((unsigned int) start) >> abitsmin) ,
		(((unsigned int) end) >> abitsmin) ,
		hardware , writehardware , &wrelement_max );
#if VERBOSE
	logerror("Done installing new memory handler.\n");
	logerror("used write elements %d/%d , functions %d/%d\n"
			,wrelement_max,MH_ELEMAX , wrhard_max,MH_HARDMAX );
#endif
	return memory_find_base(cpu, start);
}

void *install_port_read_handler(int cpu, int start, int end, mem_read_handler handler)
{
	return install_port_read_handler_common(cpu, start, end, handler, 1);
}

void *install_port_write_handler(int cpu, int start, int end, mem_write_handler handler)
{
	return install_port_write_handler_common(cpu, start, end, handler, 1);
}

static void *install_port_read_handler_common(int cpu, int start, int end,
											  mem_read_handler handler, int install_at_beginning)
{
	int i, oldsize;

	oldsize = readport_size[cpu];
	readport_size[cpu] += sizeof(struct IOReadPort);

	if (readport[cpu] == 0)
	{
		readport[cpu] = malloc(readport_size[cpu]);
	}
	else
	{
		readport[cpu] = realloc(readport[cpu], readport_size[cpu]);
	}

	if (readport[cpu] == 0)  return 0;

	if (install_at_beginning)
	{
		/* can't do a single memcpy because it doesn't handle overlapping regions correctly??? */
		for (i = oldsize / sizeof(struct IOReadPort); i >= 1; i--)
		{
			memcpy(&readport[cpu][i], &readport[cpu][i - 1], sizeof(struct IOReadPort));
		}

		i = 0;
	}
	else
	{
		i = oldsize / sizeof(struct IOReadPort);
	}

#ifdef MEM_DUMP
	logerror("Installing port read handler: cpu %d  slot %X  start %X  end %X\n", cpu, i, start, end);
#endif

	readport[cpu][i].start = start;
	readport[cpu][i].end = end;
	readport[cpu][i].handler = handler;

	return readport[cpu];
}

static void *install_port_write_handler_common(int cpu, int start, int end,
											   mem_write_handler handler, int install_at_beginning)
{
	int i, oldsize;

	oldsize = writeport_size[cpu];
	writeport_size[cpu] += sizeof(struct IOWritePort);

	if (writeport[cpu] == 0)
	{
		writeport[cpu] = malloc(writeport_size[cpu]);
	}
	else
	{
		writeport[cpu] = realloc(writeport[cpu], writeport_size[cpu]);
	}

	if (writeport[cpu] == 0)  return 0;

	if (install_at_beginning)
	{
		/* can't do a single memcpy because it doesn't handle overlapping regions correctly??? */
		for (i = oldsize / sizeof(struct IOWritePort); i >= 1; i--)
		{
			memcpy(&writeport[cpu][i], &writeport[cpu][i - 1], sizeof(struct IOWritePort));
		}

		i = 0;
	}
	else
	{
		i = oldsize / sizeof(struct IOWritePort);
	}

#ifdef MEM_DUMP
	logerror("Installing port write handler: cpu %d  slot %X  start %X  end %X\n", cpu, i, start, end);
#endif

	writeport[cpu][i].start = start;
	writeport[cpu][i].end = end;
	writeport[cpu][i].handler = handler;

	return writeport[cpu];
}

#ifdef MEM_DUMP
static void mem_dump( void )
{
	extern int totalcpu;
	int cpu;
	int naddr,addr;
	MHELE nhw,hw;

	FILE *temp = fopen ("memdump.log", "w");

	if (!temp) return;

	for( cpu = 0 ; cpu < 1 ; cpu++ )
	{
		fprintf(temp,"cpu %d read memory \n",cpu);
		addr = 0;
		naddr = 0;
		nhw = 0xff;
		while( (addr >> mhshift[cpu][0]) <= mhmask[cpu][0] ){
			hw = cur_mr_element[cpu][addr >> mhshift[cpu][0]];
			if( hw >= MH_HARDMAX )
			{	/* 2nd element link */
				hw = readhardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((addr>>mhshift[cpu][1]) & mhmask[cpu][1])];
				if( hw >= MH_HARDMAX )
					hw = readhardware[((hw-MH_HARDMAX)<<MH_SBITS) + (addr & mhmask[cpu][2])];
			}
			if( nhw != hw )
			{
				if( addr )
	fprintf(temp,"  %08x(%08x) - %08x = %02x\n",naddr,memoryreadoffset[nhw],addr-1,nhw);
				nhw = hw;
				naddr = addr;
			}
			addr++;
		}
		fprintf(temp,"  %08x(%08x) - %08x = %02x\n",naddr,memoryreadoffset[nhw],addr-1,nhw);

		fprintf(temp,"cpu %d write memory \n",cpu);
		naddr = 0;
		addr = 0;
		nhw = 0xff;
		while( (addr >> mhshift[cpu][0]) <= mhmask[cpu][0] ){
			hw = cur_mw_element[cpu][addr >> mhshift[cpu][0]];
			if( hw >= MH_HARDMAX )
			{	/* 2nd element link */
				hw = writehardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((addr>>mhshift[cpu][1]) & mhmask[cpu][1])];
				if( hw >= MH_HARDMAX )
					hw = writehardware[((hw-MH_HARDMAX)<<MH_SBITS) + (addr & mhmask[cpu][2])];
			}
			if( nhw != hw )
			{
				if( addr )
	fprintf(temp,"  %08x(%08x) - %08x = %02x\n",naddr,memorywriteoffset[nhw],addr-1,nhw);
				nhw = hw;
				naddr = addr;
			}
			addr++;
		}
	fprintf(temp,"  %08x(%08x) - %08x = %02x\n",naddr,memorywriteoffset[nhw],addr-1,nhw);
	}
	fclose(temp);
}
#endif

