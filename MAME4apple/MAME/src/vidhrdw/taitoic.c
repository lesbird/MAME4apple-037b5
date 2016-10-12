/***************************************************************************

TC0100SCN
---------
Tilemap generator, has three 64x64 tilemaps with 8x8 tiles.
The front one fetches gfx data from RAM, the others use ROMs as usual.

0000-3fff BG0
4000-5fff FG0
6000-6fff gfx data for FG0
7000-7fff unknown/unused?
8000-bfff BG1
c000-ffff unknown/unused?

control registers:
000-001 BG0 scroll X
002-003 BG1 scroll X
004-005 FG0 scroll X
006-007 BG0 scroll Y
008-009 BG1 scroll Y
00a-00b FG0 scroll Y
00c-00d ---------------x BG0 disable
        --------------x- BG1 disable
        -------------x-- FG0 disable
        ------------x--- change priority order from BG0-BG1-FG0 to BG1-BG0-FG0
        -----------x---- unknown (cameltru, NOT cameltry)
00e-00f ---------------x flip scren
        --x------------- unknown (thunderfox)


TC0280GRD
TC0430GRW
---------
These generate a zoming/rotating tilemap. The TC0280GRD has to be used in
pairs, while the TC0430GRW is a newer. single-chip solution.
Regardless of the hardware differences, the two are functionally identical
except for incxx and incxy, which need to be multiplied by 2 in the TC0280GRD
to bring them to the same scale of the other parameters.

control registers:
000-003 start x
004-005 incxx
006-007 incyx
008-00b start y
00c-00d incxy
00e-00f incyy


TC0110PCR
---------
Interface to palette RAM, and simple tilemap/sprite priority handler. The
priority order seems to be fixed.
The data bus is 16 bits wide.

000  W selects palette RAM address
002 RW read/write palette RAM
004  W unknown, often written to


TC0220IOC
---------
A simple I/O interface with integrated watchdog.
It has four address inputs, which would suggest 16 bytes of addressing space,
but only the first 8 seem to be used.

000 R  IN00-07 (DSA)
000  W watchdog reset
001 R  IN08-15 (DSB)
002 R  IN16-23 (1P)
002  W unknown. Usually written on startup: initialize?
003 R  IN24-31 (2P)
004 RW coin counters and lockout
005  W unknown
006  W unknown
007 R  INB0-7 (coin)


TC0510NIO
---------
Newer version of the I/O chip

000 R  DSWA
000  W watchdog reset
001 R  DSWB
001  W unknown (ssi)
002 R  1P
003 R  2P
003  W unknown (yuyugogo, qzquest and qzchikyu use it a lot)
004 RW coin counters and lockout
005  W unknown
006  W unknown (koshien and pulirula use it a lot)
007 R  coin

***************************************************************************/

#include "driver.h"
#include "taitoic.h"



static unsigned char taitof2_scrbank;
WRITE_HANDLER( taitof2_scrbank_w )   /* Mjnquest banks its 2 sets of scr tiles */
{
    taitof2_scrbank = (data & 0x1);
}

#define TC0100SCN_RAM_SIZE 0x10000
#define TC0100SCN_TOTAL_CHARS 256
#define TC0100SCN_MAX_CHIPS 2
static int TC0100SCN_chips;
static UINT8 TC0100SCN_ctrl[TC0100SCN_MAX_CHIPS][16];
static UINT8 *TC0100SCN_ram[TC0100SCN_MAX_CHIPS],*TC0100SCN_bg_ram[TC0100SCN_MAX_CHIPS],
		*TC0100SCN_fg_ram[TC0100SCN_MAX_CHIPS],*TC0100SCN_tx_ram[TC0100SCN_MAX_CHIPS],
		*TC0100SCN_char_ram[TC0100SCN_MAX_CHIPS];
static struct tilemap *TC0100SCN_tilemap[TC0100SCN_MAX_CHIPS][3];
static char *TC0100SCN_char_dirty[TC0100SCN_MAX_CHIPS];
static int TC0100SCN_chars_dirty[TC0100SCN_MAX_CHIPS];
static int TC0100SCN_bg_gfx[TC0100SCN_MAX_CHIPS],TC0100SCN_tx_gfx[TC0100SCN_MAX_CHIPS];


INLINE void common_get_bg_tile_info(UINT8 *ram,int gfxnum,int tile_index)
{
	int code = (READ_WORD(&ram[4*tile_index + 2]) & 0x7fff) + (taitof2_scrbank << 15);
	int attr = READ_WORD(&ram[4*tile_index]);
	SET_TILE_INFO(gfxnum,code,attr & 0xff);
	tile_info.flags = TILE_FLIPYX((attr & 0xc000) >> 14);
}

INLINE void common_get_tx_tile_info(UINT8 *ram,int gfxnum,int tile_index)
{
	int attr = READ_WORD(&ram[2*tile_index]);
	SET_TILE_INFO(gfxnum,attr & 0xff,(attr & 0x3f00) >> 6);
	tile_info.flags = TILE_FLIPYX((attr & 0xc000) >> 14);
}

static void TC0100SCN_get_bg_tile_info_0(int tile_index)
{
	common_get_bg_tile_info(TC0100SCN_bg_ram[0],TC0100SCN_bg_gfx[0],tile_index);
}

static void TC0100SCN_get_fg_tile_info_0(int tile_index)
{
	common_get_bg_tile_info(TC0100SCN_fg_ram[0],TC0100SCN_bg_gfx[0],tile_index);
}

static void TC0100SCN_get_tx_tile_info_0(int tile_index)
{
	common_get_tx_tile_info(TC0100SCN_tx_ram[0],TC0100SCN_tx_gfx[0],tile_index);
}

static void TC0100SCN_get_bg_tile_info_1(int tile_index)
{
	common_get_bg_tile_info(TC0100SCN_bg_ram[1],TC0100SCN_bg_gfx[1],tile_index);
}

static void TC0100SCN_get_fg_tile_info_1(int tile_index)
{
	common_get_bg_tile_info(TC0100SCN_fg_ram[1],TC0100SCN_bg_gfx[1],tile_index);
}

static void TC0100SCN_get_tx_tile_info_1(int tile_index)
{
	common_get_tx_tile_info(TC0100SCN_tx_ram[1],TC0100SCN_tx_gfx[1],tile_index);
}


void (*get_tile_info[TC0100SCN_MAX_CHIPS][3])(int tile_index) =
{
	{ TC0100SCN_get_bg_tile_info_0, TC0100SCN_get_fg_tile_info_0 ,TC0100SCN_get_tx_tile_info_0 },
	{ TC0100SCN_get_bg_tile_info_1, TC0100SCN_get_fg_tile_info_1 ,TC0100SCN_get_tx_tile_info_1 }
};


static struct GfxLayout TC0100SCN_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};


int TC0100SCN_vh_start(int chips,int gfxnum,int x_offset)
{
	int gfx_index,i;


	if (chips > TC0100SCN_MAX_CHIPS) return 1;

	TC0100SCN_chips = chips;

	for (i = 0;i < chips;i++)
	{
		int xd,yd;

		TC0100SCN_tilemap[i][0] = tilemap_create(get_tile_info[i][0],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
		TC0100SCN_tilemap[i][1] = tilemap_create(get_tile_info[i][1],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
		TC0100SCN_tilemap[i][2] = tilemap_create(get_tile_info[i][2],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

		TC0100SCN_ram[i] = malloc(TC0100SCN_RAM_SIZE);
		TC0100SCN_char_dirty[i] = malloc(TC0100SCN_TOTAL_CHARS);

		if (!TC0100SCN_ram[i] || !TC0100SCN_tilemap[i][0] ||
				!TC0100SCN_tilemap[i][1] || !TC0100SCN_tilemap[i][2])
		{
			TC0100SCN_vh_stop();
			return 1;
		}

		TC0100SCN_bg_ram[i]   = TC0100SCN_ram[i] + 0x0000;
		TC0100SCN_tx_ram[i]   = TC0100SCN_ram[i] + 0x4000;
		TC0100SCN_char_ram[i] = TC0100SCN_ram[i] + 0x6000;
		TC0100SCN_fg_ram[i]   = TC0100SCN_ram[i] + 0x8000;
		memset(TC0100SCN_ram[i],0,TC0100SCN_RAM_SIZE);
		memset(TC0100SCN_char_dirty[i],1,TC0100SCN_TOTAL_CHARS);
		TC0100SCN_chars_dirty[i] = 1;

		/* find first empty slot to decode gfx */
		for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
			if (Machine->gfx[gfx_index] == 0)
				break;
		if (gfx_index == MAX_GFX_ELEMENTS)
		{
			TC0100SCN_vh_stop();
			return 1;
		}

		/* create the char set (gfx will then be updated dynamically from RAM) */
		Machine->gfx[gfx_index] = decodegfx(TC0100SCN_char_ram[i],&TC0100SCN_charlayout);
		if (!Machine->gfx[gfx_index])
			return 1;

		/* set the color information */
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = 64;

		TC0100SCN_tx_gfx[i] = gfx_index;

		/* use the given gfx set for bg tiles */
		TC0100SCN_bg_gfx[i] = gfxnum + i;

		TC0100SCN_tilemap[i][0]->transparent_pen = 0;
		TC0100SCN_tilemap[i][1]->transparent_pen = 0;
		TC0100SCN_tilemap[i][2]->transparent_pen = 0;

		/* I'm setting the optional chip #2 7 bits higher and 2 pixels to the left
		   than chip #1 because that's how thundfox wants it. */
		xd = (i == 0) ? -x_offset : (-x_offset-2);
		yd = (i == 0) ? 8 : 1;
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][0],-16 + xd,-16 - xd);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][0],yd,-yd);
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][1],-16 + xd,-16 - xd);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][1],yd,-yd);
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][2],-16 + xd,-16 - xd - 7);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][2],yd,-yd);
	}

	taitof2_scrbank = 0;

	return 0;
}

void TC0100SCN_vh_stop(void)
{
	int i;

	for (i = 0;i < TC0100SCN_chips;i++)
	{
		free(TC0100SCN_ram[i]);
		TC0100SCN_ram[i] = 0;
		free(TC0100SCN_char_dirty[i]);
		TC0100SCN_char_dirty[i] = 0;
	}
}


static int TC0100SCN_word_r(int chip,offs_t offset)
{
	int res;

	res = READ_WORD(&TC0100SCN_ram[chip][offset]);

	/* for char gfx data, we have to straighten out the 16-bit word into
	   bytes for gfxdecode() to work */
	#ifdef LSB_FIRST
	if (offset >= 0x6000 && offset < 0x7000)
		res = ((res & 0x00ff) << 8) | ((res & 0xff00) >> 8);
	#endif

	return res;
}

READ_HANDLER( TC0100SCN_word_0_r )
{
	return TC0100SCN_word_r(0,offset);
}

READ_HANDLER( TC0100SCN_word_1_r )
{
	return TC0100SCN_word_r(1,offset);
}


static void TC0100SCN_word_w(int chip,offs_t offset,data_t data)
{
	int oldword = READ_WORD(&TC0100SCN_ram[chip][offset]);
	int newword;

	/* for char gfx data, we have to straighten out the 16-bit word into
	   bytes for gfxdecode() to work */
	#ifdef LSB_FIRST
	if (offset >= 0x6000 && offset < 0x7000)
		data = ((data & 0x00ff00ff) << 8) | ((data & 0xff00ff00) >> 8);
	#endif

	newword = COMBINE_WORD(oldword,data);
	if (oldword != newword)
	{
		WRITE_WORD(&TC0100SCN_ram[chip][offset],newword);

		if (offset < 0x4000)
			tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][0],offset / 4);
		else if (offset < 0x6000)
			tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][2],(offset - 0x4000) / 2);
		else if (offset < 0x7000)
		{
			TC0100SCN_char_dirty[chip][(offset - 0x6000) / 16] = 1;
			TC0100SCN_chars_dirty[chip] = 1;
		}
		else if (offset >= 0x8000 && offset < 0xc000)
			tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][1],(offset - 0x8000) / 4);
	}
}

WRITE_HANDLER( TC0100SCN_word_0_w )
{
	TC0100SCN_word_w(0,offset,data);
}

WRITE_HANDLER( TC0100SCN_word_1_w )
{
	TC0100SCN_word_w(1,offset,data);
}


READ_HANDLER( TC0100SCN_ctrl_word_0_r )
{
	return READ_WORD(&TC0100SCN_ctrl[0][offset]);
}

READ_HANDLER( TC0100SCN_ctrl_word_1_r )
{
	return READ_WORD(&TC0100SCN_ctrl[1][offset]);
}


static void TC0100SCN_ctrl_word_w(int chip,offs_t offset,data_t data)
{
	COMBINE_WORD_MEM(&TC0100SCN_ctrl[chip][offset],data);

	data = READ_WORD(&TC0100SCN_ctrl[chip][offset]);

	switch (offset)
	{
		case 0x00:
			tilemap_set_scrollx(TC0100SCN_tilemap[chip][0],0,-data);
			break;

		case 0x02:
			tilemap_set_scrollx(TC0100SCN_tilemap[chip][1],0,-data);
			break;

		case 0x04:
			tilemap_set_scrollx(TC0100SCN_tilemap[chip][2],0,-data);
			break;

		case 0x06:
			tilemap_set_scrolly(TC0100SCN_tilemap[chip][0],0,-data);
			break;

		case 0x08:
			tilemap_set_scrolly(TC0100SCN_tilemap[chip][1],0,-data);
			break;

		case 0x0a:
			tilemap_set_scrolly(TC0100SCN_tilemap[chip][2],0,-data);
			break;

		case 0x0c:
			break;

		case 0x0e:
		{
			int flip = (data & 0x01) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0;

			tilemap_set_flip(TC0100SCN_tilemap[chip][0],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][1],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][2],flip);
			break;
		}
	}
}

WRITE_HANDLER( TC0100SCN_ctrl_word_0_w )
{
	TC0100SCN_ctrl_word_w(0,offset,data);
}

WRITE_HANDLER( TC0100SCN_ctrl_word_1_w )
{
	TC0100SCN_ctrl_word_w(1,offset,data);
}


void TC0100SCN_tilemap_update(void)
{
	int i;

	for (i = 0;i < TC0100SCN_chips;i++)
	{
		/* Decode any characters that have changed */
		if (TC0100SCN_chars_dirty[i])
		{
			int tile_index,j;


			for (tile_index = 0;tile_index < 64*64;tile_index++)
			{
				int attr = READ_WORD(&TC0100SCN_tx_ram[i][2*tile_index]);
				if (TC0100SCN_char_dirty[i][attr & 0xff])
					tilemap_mark_tile_dirty(TC0100SCN_tilemap[i][2],tile_index);
			}

			for (j = 0;j < TC0100SCN_TOTAL_CHARS;j++)
			{
				if (TC0100SCN_char_dirty[i][j])
					decodechar(Machine->gfx[TC0100SCN_tx_gfx[i]],j,TC0100SCN_char_ram[i],&TC0100SCN_charlayout);
				TC0100SCN_char_dirty[i][j] = 0;
			}
			TC0100SCN_chars_dirty[i] = 0;
		}

		tilemap_update(TC0100SCN_tilemap[i][0]);
		tilemap_update(TC0100SCN_tilemap[i][1]);
		tilemap_update(TC0100SCN_tilemap[i][2]);
	}
}

void TC0100SCN_tilemap_draw(struct osd_bitmap *bitmap,int chip,int layer,int flags)
{
	int disable = READ_WORD(&TC0100SCN_ctrl[chip][12]) & 0xf7;

#ifdef MAME_DEBUG
if (disable != 0 && disable != 3 && disable != 7)
	usrintf_showmessage("layer disable = %x",disable);
#endif

	switch (layer)
	{
		case 0:
			if (disable & 0x01) return;
			tilemap_draw(bitmap,TC0100SCN_tilemap[chip][0],flags);
			break;
		case 1:
			if (disable & 0x02) return;
			tilemap_draw(bitmap,TC0100SCN_tilemap[chip][1],flags);
			break;
		case 2:
			if (disable & 0x04) return;
			if (disable & 0x10) return;	/* cameltru */
			tilemap_draw(bitmap,TC0100SCN_tilemap[chip][2],flags);
			break;
	}
}

int TC0100SCN_bottomlayer(int chip)
{
	return (READ_WORD(&TC0100SCN_ctrl[chip][12]) & 0x8) >> 3;
}


/***************************************************************************/

#define TC0280GRD_RAM_SIZE 0x2000
static UINT8 TC0280GRD_ctrl[16];
static UINT8 *TC0280GRD_ram;
static struct tilemap *TC0280GRD_tilemap;
static int TC0280GRD_gfxnum,TC0280GRD_base_color;


static void TC0280GRD_get_tile_info(int tile_index)
{
	int attr = READ_WORD(&TC0280GRD_ram[2*tile_index]);
	SET_TILE_INFO(TC0280GRD_gfxnum,attr & 0x3fff,((attr & 0xc000) >> 14) + TC0280GRD_base_color);
}


int TC0280GRD_vh_start(int gfxnum)
{
	TC0280GRD_ram = malloc(TC0280GRD_RAM_SIZE);
	TC0280GRD_tilemap = tilemap_create(TC0280GRD_get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,64);

	if (!TC0280GRD_ram || !TC0280GRD_tilemap)
	{
		TC0280GRD_vh_stop();
		return 1;
	}

	tilemap_set_clip(TC0280GRD_tilemap,0);

	TC0280GRD_gfxnum = gfxnum;

	return 0;
}

int TC0430GRW_vh_start(int gfxnum)
{
	return TC0280GRD_vh_start(gfxnum);
}

void TC0280GRD_vh_stop(void)
{
	free(TC0280GRD_ram);
	TC0280GRD_ram = 0;
}

void TC0430GRW_vh_stop(void)
{
	TC0280GRD_vh_stop();
}

READ_HANDLER( TC0280GRD_word_r )
{
	return READ_WORD(&TC0280GRD_ram[offset]);
}

READ_HANDLER( TC0430GRW_word_r )
{
	return TC0280GRD_word_r(offset);
}

WRITE_HANDLER( TC0280GRD_word_w )
{
	int oldword = READ_WORD(&TC0280GRD_ram[offset]);
	int newword = COMBINE_WORD(oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&TC0280GRD_ram[offset],newword);
		tilemap_mark_tile_dirty(TC0280GRD_tilemap,offset / 2);
	}
}

WRITE_HANDLER( TC0430GRW_word_w )
{
	TC0280GRD_word_w(offset,data);
}

WRITE_HANDLER( TC0280GRD_ctrl_word_w )
{
	COMBINE_WORD_MEM(&TC0280GRD_ctrl[offset],data);
}

WRITE_HANDLER( TC0430GRW_ctrl_word_w )
{
	TC0280GRD_ctrl_word_w(offset,data);
}

void TC0280GRD_tilemap_update(int base_color)
{
	if (TC0280GRD_base_color != base_color)
	{
		TC0280GRD_base_color = base_color;
		tilemap_mark_all_tiles_dirty(TC0280GRD_tilemap);
	}

	tilemap_update(TC0280GRD_tilemap);
}

void TC0430GRW_tilemap_update(int base_color)
{
	TC0280GRD_tilemap_update(base_color);
}

static void zoom_draw(struct osd_bitmap *bitmap,int xoffset,int yoffset,UINT32 priority,int xmultiply)
{
	UINT32 startx,starty;
	int incxx,incxy,incyx,incyy;
	struct osd_bitmap *srcbitmap = TC0280GRD_tilemap->pixmap;

	/* 24-bit signed */
	startx = ((READ_WORD(&TC0280GRD_ctrl[0]) & 0xff) << 16) + READ_WORD(&TC0280GRD_ctrl[2]);
	if (startx & 0x800000) startx -= 0x1000000;
	incxx = (INT16)READ_WORD(&TC0280GRD_ctrl[4]);
	incxx *= xmultiply;
	incyx = (INT16)READ_WORD(&TC0280GRD_ctrl[6]);
	/* 24-bit signed */
	starty = ((READ_WORD(&TC0280GRD_ctrl[8]) & 0xff) << 16) + READ_WORD(&TC0280GRD_ctrl[10]);
	if (starty & 0x800000) starty -= 0x1000000;
	incxy = (INT16)READ_WORD(&TC0280GRD_ctrl[12]);
	incxy *= xmultiply;
	incyy = (INT16)READ_WORD(&TC0280GRD_ctrl[14]);

	startx -= xoffset * incxx + yoffset * incyx;
	starty -= xoffset * incxy + yoffset * incyy;

	copyrozbitmap(bitmap,srcbitmap,startx << 4,starty << 4,
			incxx << 4,incxy << 4,incyx << 4,incyy << 4,
			1,	/* copy with wraparound */
			&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen,priority);
}

void TC0280GRD_zoom_draw(struct osd_bitmap *bitmap,int xoffset,int yoffset,UINT32 priority)
{
	zoom_draw(bitmap,xoffset,yoffset,priority,2);
}

void TC0430GRW_zoom_draw(struct osd_bitmap *bitmap,int xoffset,int yoffset,UINT32 priority)
{
	zoom_draw(bitmap,xoffset,yoffset,priority,1);
}


/***************************************************************************/


static int TC0110PCR_addr;
static UINT16 *TC0110PCR_ram;
#define TC0110PCR_RAM_SIZE 0x2000

int TC0110PCR_vh_start(void)
{
	TC0110PCR_ram = malloc(TC0110PCR_RAM_SIZE * sizeof(*TC0110PCR_ram));

	if (!TC0110PCR_ram) return 1;

	return 0;
}

void TC0110PCR_vh_stop(void)
{
	free(TC0110PCR_ram);
	TC0110PCR_ram = 0;
}

READ_HANDLER( TC0110PCR_word_r )
{
	switch (offset)
	{
		case 2:
			return TC0110PCR_ram[TC0110PCR_addr];

		default:
logerror("PC %06x: warning - read TC0110PCR address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

WRITE_HANDLER( TC0110PCR_word_w )
{
	switch (offset)
	{
		case 0:
			TC0110PCR_addr = (data >> 1) & 0xfff;   /* In test mode game writes to odd register number so it is (data>>1) */
			if (data>0x1fff) logerror ("Write to palette index > 0x1fff\n");
			break;

		case 2:
		{
			int r,g,b;   /* data = palette BGR value */


			TC0110PCR_ram[TC0110PCR_addr] = data & 0xffff;

			r = (data >>  0) & 0x1f;
			g = (data >>  5) & 0x1f;
			b = (data >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			palette_change_color(TC0110PCR_addr,r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to TC0110PCR address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}


/***************************************************************************/


static unsigned char TC0220IOC_regs[16];

READ_HANDLER( TC0220IOC_r )
{
	switch (offset)
	{
		case 0x00:	/* IN00-07 (DSA) */
			return readinputport(0);

		case 0x01:	/* IN08-15 (DSB) */
			return readinputport(1);

		case 0x02:	/* IN16-23 (1P) */
			return readinputport(2);

		case 0x03:	/* IN24-31 (2P) */
			return readinputport(3);

		case 0x04:	/* coin counters and lockout */
			return TC0220IOC_regs[4];

		case 0x07:	/* INB0-7 (coin) */
			return readinputport(4);

		default:
logerror("PC %06x: warning - read TC0220IOC address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

WRITE_HANDLER( TC0220IOC_w )
{
	TC0220IOC_regs[offset] = data;

	switch (offset)
	{
		case 0x00:
			watchdog_reset_w(offset,data);
			break;

		case 0x04:	/* coin counters and lockout */
			coin_lockout_w(0,~data & 0x01);
			coin_lockout_w(1,~data & 0x02);
			coin_counter_w(0,data & 0x04);
			coin_counter_w(1,data & 0x08);
			break;

		default:
logerror("PC %06x: warning - write to TC0220IOC address %02x\n",cpu_get_pc(),offset);
			break;
	}
}


/***************************************************************************/


static unsigned char TC0510NIO_regs[16];

READ_HANDLER( TC0510NIO_r )
{
	switch (offset)
	{
		case 0x00:	/* DSA */
			return readinputport(0);

		case 0x01:	/* DSB */
			return readinputport(1);

		case 0x02:	/* 1P */
			return readinputport(2);

		case 0x03:	/* 2P */
			return readinputport(3);

		case 0x04:	/* coin counters and lockout */
			return TC0510NIO_regs[4];

		case 0x07:	/* coin */
			return readinputport(4);

		default:
logerror("PC %06x: warning - read TC0510NIO address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

WRITE_HANDLER( TC0510NIO_w )
{
	TC0510NIO_regs[offset] = data;

	switch (offset)
	{
		case 0x00:
			watchdog_reset_w(offset,data);
			break;

		case 0x04:	/* coin counters and lockout */
			coin_lockout_w(0,~data & 0x01);
			coin_lockout_w(1,~data & 0x02);
			coin_counter_w(0,data & 0x04);
			coin_counter_w(1,data & 0x08);
			break;

		default:
logerror("PC %06x: warning - write %02x to TC0510NIO address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}
