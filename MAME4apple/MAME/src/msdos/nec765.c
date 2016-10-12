/*****************************************************************************
 *
 *	nec765.c
 *
 *	allow direct access to a PCs NEC 765 floppy disc controller
 *
 *	!!!! this is ALPHA - be careful !!!!
 *
 *****************************************************************************/

#include <go32.h>
#include <pc.h>
#include <dpmi.h>
#include <time.h>
#include "driver.h"
#include "mess/machine/wd179x.h"

#define RE_INTERLEAVE	0
#define VERBOSE 		1
/* since most PC floppy disc controllers don't really support FM, */
/* set this to 0x40 to always format/read/write in MFM mode */
#define MFM_OVERRIDE	0x40

#if VERBOSE
#define LOG(msg) if (log) fprintf msg
#else
#define LOG(msg)
#endif

typedef struct
{
	unsigned drive_0:1; 			/* drive 0 in seek mode / busy */
	unsigned drive_1:1; 			/* drive 1 in seek mode / busy */
	unsigned drive_2:1; 			/* drive 2 in seek mode / busy */
	unsigned drive_3:1; 			/* drive 3 in seek mode / busy */
	unsigned io_active:1;			/* read or write command in progress */
	unsigned non_dma_mode:1;		/* fdc is in non DMA mode if set */
	unsigned read_from_fdc:1;		/* request for master (1 fdc->cpu, 0 cpu->fdc) */
	unsigned ready:1;				/* data reg ready for i/o */
}   STM;

typedef struct
{
	unsigned unit_select:2; 		/* unit select */
	unsigned head:1;				/* head */
	unsigned not_ready:1;			/* not ready */
	unsigned equip_check:1; 		/* equipment check (drive fault) */
	unsigned seek_end:1;			/* seek end */
	unsigned int_code:2;			/* interrupt code (0: ok, 1:abort, 2:ill cmd, 3:ready chg) */
}   ST0;

typedef struct
{
	unsigned missing_AM:1;			/* missing address mark */
	unsigned not_writeable:1;		/* not writeable */
	unsigned no_data:1; 			/* no data */
	unsigned x3:1;					/* always 0 */
	unsigned overrun:1; 			/* overrun */
	unsigned data_error:1;			/* data error */
	unsigned x6:1;					/* always 0 */
	unsigned end_of_cylinder:2; 	/* end of cylinder */
}   ST1;

typedef struct
{
	unsigned missing_DAM:1; 		/* missing (deleted) data address mark */
	unsigned bad_cylinder:1;		/* bad cylinder (cylinder == 255) */
	unsigned scan_not_satisfied:1;	/* scan not satisfied */
	unsigned scan_equal_hit:1;		/* scan equal hit */
	unsigned wrong_cylinder:1;		/* wrong cylinder (mismatch) */
	unsigned data_error:1;			/* data error */
	unsigned control_mark:1;		/* control mark (deleted data address mark) */
	unsigned x7:1;					/* always 0 */
}   ST2;

typedef struct
{
	unsigned unit_select:2; 		/* unit select */
	unsigned head:1;				/* head */
	unsigned two_side:1;			/* two side */
	unsigned track_0:1; 			/* track 0 (not reliable!?) */
	unsigned ready:1;				/* ready */
	unsigned write_protect:1;		/* write protect */
	unsigned fault:1;				/* fault */
}   ST3;


typedef struct
{
	ST0 st0;			/* status 0 */
	ST1 st1;			/* status 1 */
	ST2 st2;			/* status 2 */
	UINT8 c;			 /* cylinder */
	UINT8 h;			 /* head */
	UINT8 s;			 /* sector */
	UINT8 n;			 /* sector length */
	ST3 st3;			/* status 3 (after sense drive status cmd) */
	UINT8 pcn;
}   STA;

typedef struct
{
	UINT8 unit; 		 /* unit number 0 = A:, 1 = B: */
	UINT8 type; 		 /* type of drive 0: 360K, 1: 720K, 2: 1.2M, 3: 1.44M */
	UINT8 eot;			 /* end of track */
	UINT8 secl; 		 /* sector length code (0:128, 1:256, 2:512 ... ) */
	UINT8 gap_a;		 /* read/write gap A*/
	UINT8 gap_b;		 /* format gap B */
	UINT8 dtl;			 /* data length (128 if secl == 0) */
	UINT8 filler;		 /* format filler UINT8 (E5) */
	UINT8 mfm;			 /* 0x00 = FM, 0x40 = MFM */
	UINT8 rcmd; 		 /* read command (0x26 normal, 0x2C deleted data address mark) */
	UINT8 wcmd; 		 /* write command (0x05 normal, 0x09 deleted data address mark) */
	UINT8 dstep;		 /* double step shift factor (1 for 40 track disk in 80 track drive) */
	STM stm;			/* main status */
	STA sta;			/* status file */
}   NEC;

typedef struct {
	UINT8 density;
	UINT8 spt;
	UINT8 secl;
	UINT8 gap_a;
	UINT8 gap_b;
}	GAPS;

static GAPS gaps[] = {
	{DEN_FM_LO,    1, 4, 200, 255},
    {DEN_FM_LO,    2, 3, 200, 255},
    {DEN_FM_LO,    4, 2,  70, 135},
    {DEN_FM_LO,    5, 2,  56,  98},
    {DEN_FM_LO,    5, 1, 200, 255},
    {DEN_FM_LO,    6, 1,  70, 135},
    {DEN_FM_LO,    7, 1,  56,  98},
    {DEN_FM_LO,    8, 1,  25,  48},
    {DEN_FM_LO,    9, 1,   7,   9},
    {DEN_FM_LO,   16, 0,  16,  25},
    {DEN_FM_LO,   17, 0,  11,  17},
    {DEN_FM_LO,   18, 0,   7,   9},

    {DEN_FM_HI,    2, 4, 200, 255},
    {DEN_FM_HI,    3, 4,  42,  80},
    {DEN_FM_HI,    4, 3, 128, 240},
    {DEN_FM_HI,    5, 3,  70, 135},
    {DEN_FM_HI,    8, 2,  42,  80},
    {DEN_FM_HI,    9, 2,  42,  80},
    {DEN_FM_HI,   10, 2,  10,  12},
    {DEN_FM_HI,   16, 1,  32,  50},
    {DEN_FM_HI,   18, 1,  10,  12},
    {DEN_FM_HI,   26, 0,   7,   9},

	{DEN_MFM_LO,   1, 5, 200, 255},
    {DEN_MFM_LO,   2, 4, 200, 255},
    {DEN_MFM_LO,   3, 4,  26,  46},
    {DEN_MFM_LO,   4, 3, 128, 240},
    {DEN_MFM_LO,   5, 3,  21,  31},
    {DEN_MFM_LO,   8, 2,  42,  80},
    {DEN_MFM_LO,   9, 2,  26,  46},
    {DEN_MFM_LO,  10, 2,  10,  12},
    {DEN_MFM_LO,  16, 1,  32,  50},
    {DEN_MFM_LO,  17, 1,  21,  31},
    {DEN_MFM_LO,  18, 1,  10,  12},
    {DEN_MFM_LO,  19, 1,   7,   9},

	{DEN_MFM_HI,  2,  5, 200, 255},
    {DEN_MFM_HI,  4,  4, 200, 255},
    {DEN_MFM_HI,  5,  4,  42,  80},
    {DEN_MFM_HI,  8,  3, 128, 240},
    {DEN_MFM_HI,  9,  3,  76, 148},
    {DEN_MFM_HI, 10,  3,  32,  50},
    {DEN_MFM_HI, 11,  3,  10,  12},
    {DEN_MFM_HI, 16,  2, 128, 240},
    {DEN_MFM_HI, 17,  2,  70, 135},
    {DEN_MFM_HI, 18,  2,  42,  80},
    {DEN_MFM_HI, 19,  2,  32,  50},
    {DEN_MFM_HI, 32,  1, 128, 240},
    {DEN_MFM_HI, 33,  1,  42,  80},
    {DEN_MFM_HI, 34,  1,  32,  50},
    {DEN_MFM_HI, 35,  1,  21,  31},
    {DEN_MFM_HI, 36,  1,  10,  12},
	{0,0,0,0,0}
};

static FILE *log = NULL;
static NEC nec;
static UINT8 unit_type[4] = {0xff, 0xff, 0xff, 0xff};

static __dpmi_paddr old_irq_E;	/* previous interrupt vector 0E */
static __dpmi_paddr new_irq_E;	/* new interrupt vector 0E */
static _go32_dpmi_seginfo nec_dma; /* for allocation of a sector DMA buffer */
static UINT8 irq_flag = 0;		 /* set by IRQ 6 (INT 0E) */
static UINT8 initialized = 0;
static void *timer_motors = NULL;

/*****************************************************************************
 * convert NEC 765 status to WD179X status 1 (restore, seek, step)
 *****************************************************************************/
static UINT8 FDC_STA1(void)
{
	UINT8 sta = 0;

#if 0	/* since the track_0 status seems not to be reliable */
    if (!nec.sta.st3.track_0)       /* track 0 */
		sta |= STA_1_TRACK0;
#else	/* I did it that way */
	if (!nec.sta.pcn == 0)			/* cylinder number 0 */
        sta |= STA_1_TRACK0;
#endif
    if (nec.sta.st3.write_protect)  /* write protect */
		sta |= STA_1_WRITE_PRO;
	if (!nec.sta.st3.ready) 		/* not ready */
		sta |= STA_1_NOT_READY;

	return sta;
}

/*****************************************************************************
 * convert NEC 765 status to WD179X status 2 (read, write, format)
 *****************************************************************************/
static UINT8 FDC_STA2(int read_mode)
{
	UINT8 sta = 0;

	if (nec.sta.st1.overrun)		/* overrun ? */
		sta |= STA_2_LOST_DAT;
	if (nec.sta.st1.data_error ||	/* data error (DAM) ? */
		nec.sta.st2.data_error) 	/* data error (SEC) ? */
		sta |= STA_2_CRC_ERR;
	if (nec.sta.st1.no_data ||		/* no data ? */
		nec.sta.st1.missing_AM) 	/* missing address mark ? */
		sta |= STA_2_REC_N_FND;
	if (nec.sta.st2.control_mark)	/* control mark ? */
		sta |= STA_2_REC_TYPE;
	if (nec.sta.st1.not_writeable)	/* not writeable ? */
		sta |= STA_2_WRITE_PRO;
	if (nec.sta.st0.int_code == 3)	/* interrupt code == ready change ? */
		sta |= STA_2_NOT_READY;

	if (read_mode && !(sta & ~STA_2_REC_TYPE))
		sta |= STA_2_DRQ | STA_2_BUSY;

	return sta;
}

/*****************************************************************************
 * initialize dma controller to read count bytes to ofs
 *****************************************************************************/
static void dma_read(int count)
{
	int ofs = nec_dma.rm_segment * 16 + nec_dma.rm_offset;

	LOG((log,"NEC765  dma_read %08X %04X\n", ofs, count));
	outportb(0x0a, 0x04 | 0x02);		/* mask DMA channel 2 */
	outportb(0x0b, 0x46);				/* DMA read command */
	outportb(0x0c, 0);					/* reset first/last flipflop */
	outportb(0x05, (count - 1) & 0xff); /* transfer size low */
	outportb(0x05, (count - 1) >> 8);	/* transfer size high */
	outportb(0x04, ofs & 0xff); 		/* transfer offset low */
	outportb(0x04, (ofs >> 8) & 0xff);	/* transfer offset high */
	outportb(0x81, (ofs >> 16) & 0xff); /* select 64k page */
	outportb(0x0a, 0x02);				/* start DMA channel 2 */
}

/*****************************************************************************
 * initialize dma controller to read count bytes to ofs
 *****************************************************************************/
static void dma_write(int count)
{
	int ofs = nec_dma.rm_segment * 16 + nec_dma.rm_offset;

	LOG((log,"NEC765  dma_write %08X %04X\n", ofs, count));
	outportb(0x0a, 0x04 | 0x02);		/* mask DMA channel 2 */
	outportb(0x0b, 0x4a);				/* DMA write command */
	outportb(0x0c, 0);					/* reset first/last flipflop */
	outportb(0x05, (count - 1) & 0xff); /* transfer size low */
	outportb(0x05, (count - 1) >> 8);	/* transfer size high */
	outportb(0x04, ofs & 0xff); 		/* transfer offset low */
	outportb(0x04, (ofs >> 8) & 0xff);	/* transfer offset high */
	outportb(0x81, (ofs >> 16) & 0xff); /* select 64k page */
	outportb(0x0a, 0x02);				/* start DMA channel 2 */
}

/*****************************************************************************
 * read NECs main status
 *****************************************************************************/
static int main_status(void)
{
	uclock_t timeout;
	uclock_t utime = uclock();

	timeout = utime + UCLOCKS_PER_SEC;
	while (utime < timeout)
	{
		*(UINT8 *)&nec.stm = inportb(0x3f4);
		if (nec.stm.ready)
			break;
		utime = uclock();
	}

	timeout -= uclock();

	if (timeout <= 0)
	{
		LOG((log,"NEC765  STM: %02X RDY%c DIR%c IO%c NDMA%c %c%c%c%c\n",
			*(UINT8 *) &nec.stm,
			(nec.stm.ready) ? '*' : '-',
			(nec.stm.read_from_fdc) ? '-' : '+',
			(nec.stm.io_active) ? '*' : '-',
			(nec.stm.non_dma_mode) ? '*' : '-',
			(nec.stm.drive_0) ? 'A' : '*',
			(nec.stm.drive_1) ? 'B' : '*',
			(nec.stm.drive_2) ? 'C' : '*',
			(nec.stm.drive_3) ? 'D' : '*'));
	}
	return (timeout > 0);
}

/*****************************************************************************
 * NECs results phase: read everything into the status registers
 *****************************************************************************/
static void results(int sense_interrupt)
{

	if (sense_interrupt)
	{
		if (main_status() && nec.stm.read_from_fdc)
		{
			*(UINT8*)&nec.sta.st3 = inportb(0x3F5);
			LOG((log,"NEC765  ST3: 0x%02X US%d HD%d TS%c T0%c RY%c WP%c FT%c\n",
				*(UINT8*)&nec.sta.st3,
				nec.sta.st3.unit_select,
				nec.sta.st3.head,
				(nec.sta.st3.two_side) ? '*' : '-',
				(nec.sta.st3.track_0) ? '*' : '-',
				(nec.sta.st3.ready) ? '*' : '-',
				(nec.sta.st3.write_protect) ? '*' : '-',
				(nec.sta.st3.fault) ? '*' : '-'));
		}
		if (main_status() && nec.stm.read_from_fdc)
		{
			*(UINT8*)&nec.sta.pcn = inportb(0x3F5);
			LOG((log,"NEC765  PCN: %02d\n", nec.sta.pcn));
		}
	}
	else
	{
		if (main_status() && nec.stm.read_from_fdc)
		{
			*(UINT8*)&nec.sta.st0 = inportb(0x3F5);
			LOG((log,"NEC765  ST0: 0x%02X US%d HD%d NR%c EC%c SE%c IC%d\n",
				*(UINT8*)&nec.sta.st0,
				nec.sta.st0.unit_select,
				nec.sta.st0.head,
				(nec.sta.st0.not_ready) ? '*' : '-',
				(nec.sta.st0.equip_check) ? '*' : '-',
				(nec.sta.st0.seek_end) ? '*' : '-',
				nec.sta.st0.int_code));
		}
		if (main_status() && nec.stm.read_from_fdc)
		{
			*(UINT8*)&nec.sta.st1 = inportb(0x3F5);
			LOG((log,"NEC765  ST1: 0x%02X MA%c NW%c ND%c OR%c DE%c EC%c\n",
				*(UINT8*)&nec.sta.st1,
				(nec.sta.st1.missing_AM) ? '*' : '-',
				(nec.sta.st1.not_writeable) ? '*' : '-',
				(nec.sta.st1.no_data) ? '*' : '-',
				(nec.sta.st1.overrun) ? '*' : '-',
				(nec.sta.st1.data_error) ? '*' : '-',
				(nec.sta.st1.end_of_cylinder) ? '*' : '-'));
		}
		if (main_status() && nec.stm.read_from_fdc)
		{
			*(UINT8*)&nec.sta.st2 = inportb(0x3F5);
			LOG((log,"NEC765  ST2: 0x%02X MD%c BC%c SN%c SH%c WC%c DD%c CM%c\n",
				*(UINT8*)&nec.sta.st2,
				(nec.sta.st2.missing_DAM) ? '*' : '-',
				(nec.sta.st2.bad_cylinder) ? '*' : '-',
				(nec.sta.st2.scan_not_satisfied) ? '*' : '-',
				(nec.sta.st2.scan_equal_hit) ? '*' : '-',
				(nec.sta.st2.wrong_cylinder) ? '*' : '-',
				(nec.sta.st2.data_error) ? '*' : '-',
				(nec.sta.st2.control_mark) ? '*' : '-'));
		}

		if (main_status() && nec.stm.read_from_fdc)
		{
			*(UINT8*)&nec.sta.c = inportb(0x3F5);
			LOG((log,"NEC765       C:%02d", nec.sta.c));
		}
		if (main_status() && nec.stm.read_from_fdc)
		{
			*(UINT8*)&nec.sta.h = inportb(0x3F5);
			LOG((log," H:%d", nec.sta.h));
		}
		if (main_status() && nec.stm.read_from_fdc)
		{
			*(UINT8*)&nec.sta.s = inportb(0x3F5);
			LOG((log," S:%02d", nec.sta.s));
		}
		if (main_status() && nec.stm.read_from_fdc)
		{
			*(UINT8*)&nec.sta.n = inportb(0x3F5);
			LOG((log," N:%2d", nec.sta.n));
		}
		LOG((log,"\n"));
	}

/* more data? shouldn't happen, but read it away */
	while (main_status() && nec.stm.read_from_fdc)
	{
		UINT8 data = inportb(0x3F5);

		LOG((log,"NEC765  read unexpected %02X\n", data));
	}
}

/*****************************************************************************
 * out_nec: send a UINT8 to the NEC
 *****************************************************************************/
static int out_nec(UINT8 b)
{
	int result = 1;

	if (main_status())
	{
		if (nec.stm.read_from_fdc)
		{
			LOG((log,"NEC765   read_from_fdc!\n"));
			results(0);
		}
		else
		{
			LOG((log,"0x%02X ", b));
			outportb(0x3f5, b);
			result = 0;
		}
	}
	return result;
}

/*****************************************************************************
 * seek_exec: wait until seek is executed and sense interrupt status
 *****************************************************************************/
static void seek_exec(UINT8 * track)
{
	uclock_t timeout;
	uclock_t utime = uclock();

	timeout = utime + UCLOCKS_PER_SEC * 5;

	while (utime < timeout)
	{
		main_status();
		if (irq_flag)
			break;
		utime = uclock();
	}

	timeout -= uclock();

	if (nec.stm.read_from_fdc)
		results(0);

	LOG((log,"NEC765  command "));
    if (out_nec(0x08))      /* sense interrupt status */
		return;
	LOG((log,"\n"));

    results(1);

	if (track)
		*track = nec.sta.pcn;
}

/*****************************************************************************
 * specify drive parameters
 * step rate, head unload timing, head load timing, DMA mode
 *****************************************************************************/
static void specify(void)
{
	uclock_t timeout;
	uclock_t utime = uclock();

	LOG((log,"NEC765 specify\n"));

	timeout = utime + UCLOCKS_PER_SEC;
	irq_flag = 0;
    outportb(0x3f2, 0x08);      /* reset fdc */
    while (utime < timeout)
	{
		if (irq_flag)
			break;
		utime = uclock();
    }
    osd_fdc_motors(nec.unit);   /* take away reset */

	LOG((log,"NEC765  command "));
    if (out_nec(0x03))          /* specify command */
		return;
	if (out_nec(0xdf))			/* steprate, head unload timing */
		return;
	if (out_nec(0x04))			/* head load timing, DMA mode */
		return;
	LOG((log,"\n"));

}

/*****************************************************************************
 * read_sector:
 * read a sector number 'sector' from track 'track', side 'side' using head
 * number 'head' of the floppy disk. if the first try fails, change the
 * DDAM mode and try again
 *****************************************************************************/
static UINT8 read_sector(UINT8 track, UINT8 side, UINT8 head, UINT8 sector)
{
	int try;

	LOG((log,"NEC765 read    unit:%d track:%02d side:%d head:%d sector:%d\n",
		nec.unit, track, side, head, sector));

	/* three tries to detect normal / deleted data address mark */
	for (try = 0; try < 3; try++)
	{
		dma_read((nec.secl) ? nec.secl << 8 : nec.dtl);

		irq_flag = 0;

        LOG((log,"NEC765  command "));
		if (out_nec(nec.mfm | nec.rcmd))		/* read sector */
			return STA_2_NOT_READY;
		if (out_nec((side << 2) + nec.unit))	/* side * 4 + unit */
			return STA_2_NOT_READY;
		if (out_nec(track)) 					/* track */
			return STA_2_NOT_READY;
		if (out_nec(head))						/* head (can be 0 for side 1!) */
			return STA_2_NOT_READY;
		if (out_nec(sector))					/* sector */
			return STA_2_NOT_READY;
		if (out_nec(nec.secl))					/* bytes per sector */
			return STA_2_NOT_READY;
		if (out_nec(nec.eot))					/* end of track */
			return STA_2_NOT_READY;
		if (out_nec(nec.gap_a)) 				/* gap A */
			return STA_2_NOT_READY;
		if (out_nec(nec.dtl))					/* data length */
			return STA_2_NOT_READY;
		LOG((log,"\n"));

		for ( ; ; )
        {
			main_status();
			if (irq_flag)
				break;
//			if (!nec.stm.io_active)
//				break;
        }

		results(0);

		if (!nec.sta.st2.control_mark)
			break;

		/* if control mark is set, toggle DDAM mode and try again */
		nec.rcmd ^= 0x0A;	   /* toggle between 0x26 and 0x2C */
	}

	nec.sta.st2.control_mark = 0;
	/* set status 2 control_mark if read deleted was successful */
    if (nec.rcmd & 0x08)
	{
		LOG((log,"NEC765 read    *DDAM*\n"));
		nec.sta.st2.control_mark = 1;
		nec.rcmd ^= 0x0A;	   /* toggle back */
    }

	return FDC_STA2(1);
}

/*****************************************************************************
 * write_sector:
 * write sector number 'sector' to track 'track', side 'side' of
 * the floppy disk using head number 'head'. if ddam in non zero,
 * write the sector with deleted data address mark (DDAM)
 *****************************************************************************/
static UINT8 write_sector(UINT8 track, UINT8 side, UINT8 head, UINT8 sector, UINT8 ddam)
{
	int i;

	LOG((log,"NEC765 write   unit:%d track:%02d side:%d head:%d sector:%d%s\n",
		nec.unit, track, side, head, sector, (ddam) ? " *DDAM*" : ""));

	irq_flag = 0;

	dma_write((nec.secl) ? nec.secl << 8 : nec.dtl);
	nec.wcmd = (ddam) ? 0x09 : 0x05;

	LOG((log,"NEC765  command "));
	if (out_nec(nec.mfm | nec.wcmd))		/* write sector */
		return STA_2_NOT_READY;
	if (out_nec((side << 2) + nec.unit))	/* side * 4 + unit */
		return STA_2_NOT_READY;
	if (out_nec(track)) 					/* track */
		return STA_2_NOT_READY;
	if (out_nec(head))						/* head (can be 0 for side 1!) */
		return STA_2_NOT_READY;
	if (out_nec(sector))					/* sector */
		return STA_2_NOT_READY;
	if (out_nec(nec.secl))					/* bytes per sector */
		return STA_2_NOT_READY;
	if (out_nec(nec.eot))					/* end of track */
		return STA_2_NOT_READY;
	if (out_nec(nec.gap_a)) 				/* gap_a */
		return STA_2_NOT_READY;
	if (out_nec(nec.dtl))					/* data length */
		return STA_2_NOT_READY;
	LOG((log,"\n"));

	for (i = 0; i < 3; i++)
	{
		main_status();
		if (!nec.stm.io_active)
			break;
		if (irq_flag)
			break;
	}

	results(0);

	return FDC_STA2(0);
}

/*****************************************************************************
 * real mode interrupt 0x0E
 *****************************************************************************/
static void irq_E(void)
{
	asm("nop                    \n" /* "nop" to find interrupt entry */
		"sti                    \n" /* enable interrupts */
		"pushl %eax             \n" /* save EAX */
		"pushw %ds              \n" /* save DS */
		"movw  $0x0000, %ax     \n" /* patched with _my_ds() */
		"movw  %ax, %ds         \n" /* transfer to DS */
		"orb   $0x80, _irq_flag \n" /* set IRQ occured flag */
		"popw  %ds              \n" /* get back DS */
		"movb  $0x20, %al       \n" /* end of interrupt */
		"outb  %al, $0x20       \n" /* to PIC */
		"popl  %eax             \n" /* get back EAX */
		"iret                   \n");
}

int osd_fdc_init(void)
{
	__dpmi_meminfo meminfo;
	unsigned long _my_cs_base;
	unsigned long _my_ds_base;
	UINT8 *p;

	if (initialized)
		return 1;

    log = fopen("nec.log", "w");

	for (p = (UINT8 *) & irq_E; p; p++)
	{
		/* found "nop" ? */
        if (p[0] == 0x90)
			new_irq_E.offset32 = (int) p + 1;
		/* found "movw $0000,%ax" ? */
		if ((p[0] == 0xb8) && (p[1] == 0x00) && (p[2] == 0x00))
		{
			p++;
			*(short *)p = _my_ds();
			break;
		}
	}

    __dpmi_get_segment_base_address(_my_cs(), &_my_cs_base);
	__dpmi_get_segment_base_address(_my_ds(), &_my_ds_base);
	__dpmi_get_protected_mode_interrupt_vector(0x0E, &old_irq_E);

	meminfo.handle = _my_cs();
	meminfo.size = 128;		   /* should be enough */
	meminfo.address = _my_cs_base + (unsigned long) &irq_E;
	if (__dpmi_lock_linear_region(&meminfo))
	{
		LOG((log,"NEC765 init: could not lock code %lx addr:%08lx size:%ld\n",
					meminfo.handle, meminfo.address, meminfo.size));
		return 0;
	}

	meminfo.handle = _my_ds();
	meminfo.size = 128;		   /* should be enough */
	meminfo.address = _my_ds_base + (unsigned long) &old_irq_E;
	if (__dpmi_lock_linear_region(&meminfo))
	{
		LOG((log,"NEC765 init: could not lock data\n"));
		return 0;
	}

	nec_dma.size = 4096;		/* request a 4K buffer for NEC DMA */
	if (_go32_dpmi_allocate_dos_memory(&nec_dma))
	{
		LOG((log,"NEC765 init: could not alloc DOS memory size:%ld\n", nec_dma.size));
		return 0;
	}

    new_irq_E.selector = _my_cs();
	if (__dpmi_set_protected_mode_interrupt_vector(0x0E, &new_irq_E))
	{
		LOG((log,"NEC765 init: could not set vector 0x0E"));
		return 0;
	}

    initialized = 1;

	atexit(osd_fdc_exit);

	specify();

    return 1;
}

void osd_fdc_exit(void)
{
	__dpmi_meminfo meminfo;
	unsigned long _my_cs_base;
	unsigned long _my_ds_base;

	if (!initialized)
		return;

	outportb(0x3f2, 0x0c);

	if (log)
		fclose(log);

	__dpmi_get_segment_base_address(_my_cs(), &_my_cs_base);
	__dpmi_get_segment_base_address(_my_ds(), &_my_ds_base);
	__dpmi_set_protected_mode_interrupt_vector(0x0E, &old_irq_E);

	meminfo.handle = _my_cs();
	meminfo.size = 128;		   /* should be enough */
	meminfo.address = _my_cs_base + (unsigned long) &irq_E;
	__dpmi_unlock_linear_region(&meminfo);

	meminfo.handle = _my_ds();
	meminfo.size = 128;		   /* should be enough */
	meminfo.address = _my_ds_base + (unsigned long) &old_irq_E;
	__dpmi_unlock_linear_region(&meminfo);

	if (nec_dma.rm_segment)
	   _go32_dpmi_free_dos_memory(&nec_dma);

    if (log)
		fclose(log);
	log = NULL;

	initialized = 0;
}

void osd_fdc_motors(UINT8 unit)
{
	nec.unit = unit;

	switch (nec.unit)
	{
		case 0: /* drive 0 */
			if (!timer_motors)
				LOG((log, "FDD A: start\n"));
			outportb(0x3f2, 0x1c);
			break;
		case 1: /* drive 1 */
			if (!timer_motors)
				LOG((log, "FDD B: start\n"));
            outportb(0x3f2, 0x2d);
			break;
	}
	if (timer_motors)
		timer_remove(timer_motors);
	timer_motors = timer_set(10.0, nec.unit, osd_fdc_interrupt);
}

/*****************************************************************************
 * specify density, sectors per track and sector length
 *****************************************************************************/
void osd_fdc_density(UINT8 unit, UINT8 density, UINT8 tracks, UINT8 spt, UINT8 eot, UINT8 secl)
{
__dpmi_regs r = {{0,}, };
int i;

	nec.unit = unit;
    nec.eot = eot;
	nec.secl = secl;
	nec.dstep = 0;

	/* BIOS get current drive parameters */
    r.x.ax = 0x0800;
	r.h.dl = nec.unit;
	__dpmi_int(0x13, &r);
	nec.type = r.h.bl;

	if (nec.type != unit_type[nec.unit])
    {
		switch (nec.type)
		{
			case 1: /* 5.25" 360K */
				LOG((log,"NEC765 FDD %c: type 5.25\" 360K, sides %d, DOS spt %d%s\n",
						'A' + nec.unit, r.h.dh + 1, r.h.cl & 0x1f, (nec.dstep) ? " (double steps)":""));
				r.x.ax = 0x1701;
				break;
			case 2: /* 5.25" 1.2M */
				if (tracks < 50)
                    nec.dstep = 1;
				LOG((log,"NEC765 FDD %c: type 5.25\" 1.2M, sides %d, DOS spt %d%s\n",
						'A' + nec.unit, r.h.dh + 1, r.h.cl & 0x1f, (nec.dstep) ? " (double steps)":""));
				r.x.ax = 0x1703;
				break;
			case 3: /* 3.5"  720K */
				if (tracks < 50)
                    nec.dstep = 1;
				LOG((log,"NEC765 FDD %c: type 3.5\" 720K, sides %d, DOS spt %d%s\n",
						'A' + nec.unit, r.h.dh + 1, r.h.cl & 0x1f, (nec.dstep) ? " (double steps)":""));
				r.x.ax = 0x1704;
				break;
			case 4: /* 3.5"  1.44M */
				if (tracks < 50)
                    nec.dstep = 1;
				LOG((log,"NEC765 FDD %c: type 3.5\" 1.44M, sides %d, DOS spt %d%s\n",
						'A' + nec.unit, r.h.dh + 1, r.h.cl & 0x1f, (nec.dstep) ? " (double steps)":""));
				r.x.ax = 0x1703;
				break;
			default: /* unknown */
				LOG((log,"NEC765 FDD %c: unknown type! sides %d, DOS spt %d\n",
						'A' + nec.unit, r.h.dh + 1, r.h.cl & 0x1f));
				r.x.ax = 0x1703;
                break;
        }

		unit_type[nec.unit] = nec.type;
		__dpmi_int(0x13, &r);

		for (i = 0; gaps[i].spt; i++)
		{
			if (gaps[i].density == density && gaps[i].secl == secl && spt <= gaps[i].spt)
			{
				nec.gap_a = gaps[i].gap_a;
				nec.gap_b = gaps[i].gap_b;
				break;
			}
		}
		if (gaps[i].spt)
		{
			LOG((log,"NEC765  using gaps entry #%d {%d,%d,%d,%d,%d} for %d,%d,%d\n",
                i,gaps[i].density,gaps[i].spt,gaps[i].secl,gaps[i].gap_a,gaps[i].gap_b,density,spt,secl));
        }
		else
		{
			LOG((log,"NEC765  ERR den:%d spt:%d secl:%d not found!\n",density,spt,secl));
            nec.gap_a = 10;
			nec.gap_b = 12;
		}
		nec.dtl = (secl)?0xff:0x80; /* data length */
		nec.filler = 0xe5;			/* format filler UINT8 */

		LOG((log,"NEC765  FDD %c: den:%d trks:%d spt:%d eot:%d secl:%d gap_a:%d gap_b:%d\n",
			'A' + nec.unit, density, tracks, spt, eot, secl, nec.gap_a, nec.gap_b));

        nec.rcmd = 0x26;    /* read command (0x26 normal, 0x2C deleted data address mark) */
		nec.wcmd = 0x05;	/* write command (0x05 normal, 0x09 deleted data address mark) */
    }

	switch (density)
	{
		case DEN_FM_LO:
			nec.mfm = MFM_OVERRIDE; /* FM commands */
			outportb(0x3f7, 0x02);	/* how do we set 125 kbps ?? */
			break;
		case DEN_FM_HI:
			nec.mfm = MFM_OVERRIDE; /* FM commands */
			outportb(0x3f7, 0x02);	/* set 250/300 kbps */
            break;
		case DEN_MFM_LO:
			nec.mfm = 0x40; 		/* MFM commands */
			outportb(0x3f7, 0x02);	/* set 250/300 kbps */
			break;
		case DEN_MFM_HI:
			nec.mfm = 0x40; 		/* MFM commands */
			outportb(0x3f7, 0x01);	/* set 500 kbps */
            break;
    }
}

void osd_fdc_interrupt(int param)
{
    timer_motors = NULL;
	/* stop the motor(s) */
	LOG((log,"NEC765 FDD %c: stop", 'A' + param));
    outportb(0x3f2, 0x0c);
}

UINT8 osd_fdc_recal(UINT8 * track)
{
	LOG((log,"NEC765 recal\n"));

    irq_flag = 0;
	LOG((log,"NEC765  command "));
    if (out_nec(0x07))                  /* 1st recal command */
		return STA_1_NOT_READY;
	if (out_nec(nec.unit))
		return STA_1_NOT_READY;
	LOG((log,"\n"));

	seek_exec(track);
	if (nec.sta.pcn)					/* not yet on track 0 ? */
	{
		irq_flag = 0;
		LOG((log,"NEC765  command "));
		if (out_nec(0x07))				/* 2nd recal command */
			return STA_1_NOT_READY;
		if (out_nec(nec.unit))
			return STA_1_NOT_READY;
		LOG((log,"\n"));
		seek_exec(track);
	}

	/* reset unit type */
	unit_type[nec.unit] = 0xff;

	return FDC_STA1();
}

UINT8 osd_fdc_seek(UINT8 t, UINT8 * track)
{
	if (nec.sta.pcn == (t << nec.dstep))
	{
		*track = nec.sta.pcn;
		return FDC_STA1();
	}

	LOG((log,"NEC765 seek    unit:%d track:%02d\n", nec.unit, t));
	irq_flag = 0;

	LOG((log,"NEC765  command "));
    if (out_nec(0x0f))                  /* seek command */
		return STA_1_NOT_READY;
	if (out_nec(nec.unit))				/* unit number */
		return STA_1_NOT_READY;
	if (out_nec(t << nec.dstep))		/* track number */
		return STA_1_NOT_READY;
	LOG((log,"\n"));

	seek_exec(track);

	return FDC_STA1();
}

UINT8 osd_fdc_step(int dir, UINT8 * track)
{
int new_track;
	LOG((log,"NEC765 step    unit:%d direction:%+d (PCN:%02d)\n", nec.unit, dir, nec.sta.pcn));
	irq_flag = 0;

	LOG((log,"NEC765  command "));
    if (out_nec(0x0f))                  /* seek command */
		return STA_1_NOT_READY;
	if (out_nec(nec.unit))
		return STA_1_NOT_READY;
	new_track = nec.sta.pcn + (dir << nec.dstep);
	if (new_track < 0)
		new_track = 0;
	if (new_track > 82)
		new_track = 82;
	if (out_nec(new_track))
			return STA_1_NOT_READY;
	LOG((log,"\n"));

	seek_exec(track);

	if (track)
		*track = nec.sta.pcn;

	return FDC_STA1();
}

UINT8 osd_fdc_format(UINT8 t, UINT8 h, UINT8 spt, UINT8 * fmt)
{
int i;

	/* let the motors run */
    osd_fdc_motors(nec.unit);

	LOG((log,"NEC765 format  unit:%d track:%02d head:%d sec/track:%d\n", nec.unit, t, h, spt));

#if RE_INTERLEAVE
	/* reorder the sector sequence for PC optimized reading (interleave) */
	{
UINT8 sec;
UINT8 sec_min = 255;
UINT8 sec_max = 0;
		for (i = 0; i < spt; i++)
		{
			sec = fmt[i * 4 + 2];
			if (sec < sec_min)
				sec_min = sec;
			if (sec > sec_max)
				sec_max = sec;
		}
		/* only reorder if all sectors from min to max are in the track! */
		if (sec_max - sec_min + 1 == spt)
		{
			LOG((log,"NEC765 reordered sectors:"));
            sec = sec_min;
			for (i = 0; i < spt; i++)
			{
				LOG((log," %3d", sec));
                fmt[i * 4 + 2] = sec;
				sec += spt / RE_INTERLEAVE;
				if (sec > sec_max)
					sec = sec - spt + 1 - spt % RE_INTERLEAVE;
			}
			LOG((log,"\n"));
        }
	}
#endif

    nec.eot = 0;
	for (i = 0; i < spt; i++)
	{
		if (fmt[i * 4 + 2] > nec.eot)
			nec.eot = fmt[i * 4 + 2];
	}

	movedata(_my_ds(), (unsigned) fmt, nec_dma.pm_selector, nec_dma.pm_offset, spt * 4);

	irq_flag = 0;

	dma_write(spt * 4);

	LOG((log,"NEC765  command "));
	if (out_nec(nec.mfm | 0x0d))	   /* format track */
		return STA_2_NOT_READY;
	if (out_nec((h << 2) + nec.unit))	/* head * 4 + unit select */
		return STA_2_NOT_READY;
	if (out_nec(nec.secl))				/* bytes per sector */
		return STA_2_NOT_READY;
	if (out_nec(spt))					/* sectors / track */
		return STA_2_NOT_READY;
	if (out_nec(nec.gap_b)) 			 /* gap_b */
		return STA_2_NOT_READY;
	if (out_nec(nec.filler))			/* format filler UINT8 */
		return STA_2_NOT_READY;
	LOG((log,"\n"));

	for (i = 0; i < 3; i++)
	{
		main_status();
		if (!nec.stm.io_active)
			break;
		if (irq_flag)
			break;
	}

	results(0);

	return FDC_STA2(0);
}

UINT8 osd_fdc_put_sector(UINT8 track, UINT8 side, UINT8 head, UINT8 sector, UINT8 *buff, UINT8 ddam)
{
UINT8 result;

	movedata(
		_my_ds(), (unsigned) buff,
		nec_dma.pm_selector, nec_dma.pm_offset,
		((nec.secl) ? nec.secl << 8 : nec.dtl));
	result = write_sector(track, side, head, sector, ddam);

	return result;
}

UINT8 osd_fdc_get_sector(UINT8 track, UINT8 side, UINT8 head, UINT8 sector, UINT8 *buff)
{
UINT8 result;

	result = read_sector(track, side, head, sector);
	movedata(
		nec_dma.pm_selector, nec_dma.pm_offset,
		_my_ds(), (unsigned) buff,
		((nec.secl) ? nec.secl << 8 : nec.dtl));

	return result;
}
