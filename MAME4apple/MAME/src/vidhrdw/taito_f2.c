#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1
#define TC0280GRD_GFX_NUM 2
#define TC0430GRW_GFX_NUM 2

/*
TC0360PRI
---------
Priority manager.
A higher priority value means higher priority. 0 could mean disable but
I'm not sure. If two inputs have the same priority value, I think the first
one has priority, but I'm not sure of that either.
It seems the chip accepts three inputs from three different sources, and
each one of them can declare to have four different priority levels.

000 unknown. Could it be related to highlight/shadow effects in qcrayon2
    and gunfront?
001 in games with a roz layer, this is the roz palette bank (bottom 6 bits
    affect roz color, top 2 bits affect priority)
002 unknown
003 unknown

004 ----xxxx \       priority level 0 (unused? usually 0, pulirula sets it to F in some places)
    xxxx---- | Input priority level 1 (usually FG0)
005 ----xxxx |   #1  priority level 2 (usually BG0)
    xxxx---- /       priority level 3 (usually BG1)

006 ----xxxx \       priority level 0 (usually sprites with top color bits 00)
    xxxx---- | Input priority level 1 (usually sprites with top color bits 01)
007 ----xxxx |   #2  priority level 2 (usually sprites with top color bits 10)
    xxxx---- /       priority level 3 (usually sprites with top color bits 11)

008 ----xxxx \       priority level 0 (e.g. roz layer if top bits of register 001 are 00)
    xxxx---- | Input priority level 1 (e.g. roz layer if top bits of register 001 are 01)
009 ----xxxx |   #3  priority level 2 (e.g. roz layer if top bits of register 001 are 10)
    xxxx---- /       priority level 3 (e.g. roz layer if top bits of register 001 are 11)

00a unused
00b unused
00c unused
00d unused
00e unused
00f unused
*/
static UINT8 TC0360PRI_regs[16];

WRITE_HANDLER( TC0360PRI_w )
{
	TC0360PRI_regs[offset] = data;

if (offset >= 0x0a)
	usrintf_showmessage("write %02x to unused TC0360PRI reg %x",data,offset);
#if 0
#define regs TC0360PRI_regs
	usrintf_showmessage("%02x %02x  %02x %02x  %02x %02x %02x %02x %02x %02x",
		regs[0x00],regs[0x01],regs[0x02],regs[0x03],
		regs[0x04],regs[0x05],regs[0x06],regs[0x07],
		regs[0x08],regs[0x09]);
#endif
}



static UINT16 *spriteram_buffered,*spriteram_delayed;

#define f2_textram_size 0x2000
#define f2_characterram_size 0x2000

static unsigned char *f2_textram;
static unsigned char *taitof2_characterram;

size_t f2_4layerram_size;

static unsigned char *text_dirty;
static unsigned char *f2_4layer_dirty;

static int tile_flipscreen;

static struct osd_bitmap *tmpbitmap5;
static struct osd_bitmap *tmpbitmap6;
static struct osd_bitmap *tmpbitmap7;
static struct osd_bitmap *tmpbitmap8;
static struct osd_bitmap *tmpbitmap9;


unsigned char *taitof2_ram;
unsigned char f2_4layer_priority;
unsigned char *f2_sprite_extension;
size_t f2_spriteext_size;
unsigned int sprites_active_area = 0;

unsigned char *f2_4layerregs;

unsigned char *f2_4layerram;
static unsigned char *char_dirty;	/* 256 text chars */


/****************************************************
Determines what capabilities handled in video driver:
 0 = sprites,
 1 = sprites + layers,
 2 = sprites + layers + rotation
 3 = double lot of standard layers (thundfox)
 4 = 4 bg layers (e.g. deadconx)
****************************************************/
int f2_video_version = 1;

/****************************************************
Determines sprite banking method:
 0 = standard
 1 = use sprite extension area lo bytes for hi 6 bits
 2 = use sprite extension area hi bytes
 3 = use sprite extension area lo bytes as hi bytes
*****************************************************/
int f2_spriteext = 0;

static int f2_pivot_xdisp = 0;
static int f2_pivot_ydisp = 0;
static int spritebank[8];
static int koshien_spritebank;
/******************************************************************************
f2_hide_pixels
==============
Allows us to iron out pixels of rubbish on edge of scr layers.
Value has to be set per game, 0 to +3. Cannot be calculated.
******************************************************************************/
static int f2_hide_pixels;
static int f2_xkludge = 0;   /* Needed by Deadconx, Metalb, Hthero */
static int f2_ykludge = 0;



static int has_two_TC0100SCN(void)
{
	const struct MemoryWriteAddress *mwa;

	/* scan the memory handlers and see if the second TC0100SCN is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (mwa->start != -1)
		{
			if (mwa->handler == TC0100SCN_word_1_w)
				return 1;

			mwa++;
		}
	}

	return 0;
}

static int has_TC0110PCR(void)
{
	const struct MemoryWriteAddress *mwa;

	/* scan the memory handlers and see if the TC0110PCR is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (mwa->start != -1)
		{
			if (mwa->handler == TC0110PCR_word_w)
				return 1;

			mwa++;
		}
	}

	return 0;
}

static int has_TC0280GRD(void)
{
	const struct MemoryWriteAddress *mwa;

	/* scan the memory handlers and see if the TC0280GRD is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (mwa->start != -1)
		{
			if (mwa->handler == TC0280GRD_word_w)
				return 1;

			mwa++;
		}
	}

	return 0;
}

static int has_TC0430GRW(void)
{
	const struct MemoryWriteAddress *mwa;

	/* scan the memory handlers and see if the TC0430GRW is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (mwa->start != -1)
		{
			if (mwa->handler == TC0430GRW_word_w)
				return 1;

			mwa++;
		}
	}

	return 0;
}



//DG comment: the tmpbitmap numbering - currently looking rather illogical -
//may want to be rationalised once sprite layer priority issues resolved.

int taitof2_core_vh_start (void)
{
	spriteram_delayed = malloc(spriteram_size);
	spriteram_buffered = malloc(spriteram_size);
	if (!spriteram_delayed || !spriteram_buffered)
		return 1;

	if (TC0100SCN_vh_start(has_two_TC0100SCN() ? 2 : 1,TC0100SCN_GFX_NUM,f2_hide_pixels))
		return 1;

	if (has_TC0110PCR())
		if (TC0110PCR_vh_start())
			return 1;

	if (has_TC0280GRD())
		if (TC0280GRD_vh_start(TC0280GRD_GFX_NUM))
			return 1;

	if (has_TC0430GRW())
		if (TC0430GRW_vh_start(TC0430GRW_GFX_NUM))
			return 1;


	f2_textram = malloc(f2_textram_size);
	taitof2_characterram = malloc(f2_characterram_size);

	if (f2_video_version == 4)   /* Deadconx, Hthero, MetalBlack */
	{
		char_dirty = malloc (256);   /* always 256 text chars */
		if (!char_dirty) return 1;
		memset (char_dirty,1,256);

		text_dirty = malloc (f2_textram_size/2);
		if (!text_dirty)
		{
			free (char_dirty);
			return 1;
		}
		memset (text_dirty,1,f2_textram_size/2);

		/* create a temporary bitmap slightly larger than the screen for the text layer */
		if ((tmpbitmap5 = bitmap_alloc(64*8, 64*8)) == 0)
		{
			free (char_dirty);
			free (text_dirty);
			return 1;
		}

		f2_4layer_dirty = malloc (f2_4layerram_size/4);
		if (!f2_4layer_dirty)
		{
			free (char_dirty);
			free (text_dirty);
			bitmap_free (tmpbitmap5);
			return 1;
		}
		memset (f2_4layer_dirty,1,f2_4layerram_size/4);

		/* Create temporary bitmaps for the four layers */

		/* create a temporary bitmap slightly larger than the screen for bg 0 */
		if ((tmpbitmap6 = bitmap_alloc(64*8, 64*8)) == 0)
		{
			free (char_dirty);
			free (text_dirty);
			free (f2_4layer_dirty);
			bitmap_free (tmpbitmap5);
			return 1;
		}
		/* create a temporary bitmap slightly larger than the screen for bg 1 */
		if ((tmpbitmap7 = bitmap_alloc(64*8, 64*8)) == 0)
		{
			free (char_dirty);
			free (text_dirty);
			free (f2_4layer_dirty);
			bitmap_free (tmpbitmap5);
			bitmap_free (tmpbitmap6);
			return 1;
		}
		/* create a temporary bitmap slightly larger than the screen for bg 2 */
		if ((tmpbitmap8 = bitmap_alloc(64*8, 64*8)) == 0)
		{
			free (char_dirty);
			free (text_dirty);
			free (f2_4layer_dirty);
			bitmap_free (tmpbitmap5);
			bitmap_free (tmpbitmap6);
			bitmap_free (tmpbitmap7);
			return 1;
		}
		/* create a temporary bitmap slightly larger than the screen for the bg 3 */
		if ((tmpbitmap9 = bitmap_alloc(64*8, 64*8)) == 0)
		{
			free (char_dirty);
			free (text_dirty);
			free (f2_4layer_dirty);
			bitmap_free (tmpbitmap5);
			bitmap_free (tmpbitmap6);
			bitmap_free (tmpbitmap7);
			bitmap_free (tmpbitmap8);
			return 1;
		}
	}

	{
		int i;

		for (i = 0; i < 8; i ++)
			spritebank[i] = 0x400 * i;
	}

	sprites_active_area = 0;

	return 0;
}


/* Note: some of the vh_starts are identical and look as though they ought to be
   merged. They have been left separate because the games in question
   have special needs not yet met (layer priority or whatever) which are
   likely to require different variable settings. */

int taitof2_default_vh_start (void)
{
	f2_video_version = 1;
	f2_hide_pixels = 0;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_finalb_vh_start (void)
{
	f2_video_version = 1;
	f2_hide_pixels = 1;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_3p_vh_start (void)   /* Megab, Liquidk */
{
	f2_video_version = 1;
	f2_hide_pixels = 3;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_3p_buf_vh_start (void)   /* Solfigtr, Koshien */
{
	f2_video_version = 1;
	f2_hide_pixels = 3;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_driftout_vh_start (void)
{
	f2_video_version = 2;
	f2_hide_pixels = 3;
	f2_spriteext = 0;
	f2_pivot_xdisp = -16;
	f2_pivot_ydisp = 16;
	return (taitof2_core_vh_start());
}

int taitof2_c_vh_start (void)   /* Quiz Crayons, Quiz Jinsei */
{
	f2_video_version = 1;
	f2_hide_pixels = 3;
	f2_spriteext = 3;
	return (taitof2_core_vh_start());
}

int taitof2_ssi_vh_start (void)
{
	f2_video_version = 0;
	f2_hide_pixels = 3;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_growl_vh_start (void)
{
	f2_video_version = 1;
	f2_hide_pixels = 3;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_ninjak_vh_start (void)
{
	f2_video_version = 1;
	f2_hide_pixels = 0;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_gunfront_vh_start (void)
{
	f2_video_version = 1;
	f2_hide_pixels = 3;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_thundfox_vh_start (void)
{
	f2_video_version = 3;
	f2_hide_pixels = 3;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_mjnquest_vh_start (void)
{
	f2_video_version = 1;
	f2_hide_pixels = 0;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_footchmp_vh_start (void)
{
	f2_video_version = 4;
	f2_4layer_priority = 0;
	f2_hide_pixels = 3;
	f2_xkludge = 0x1d;
	f2_ykludge = 0x08;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_hthero_vh_start (void)
{
	f2_video_version = 4;
	f2_4layer_priority = 0;
	f2_hide_pixels = 3;
	f2_xkludge = 0x33;   // needs different kludges from Footchmp
	f2_ykludge = - 0x04;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_deadconx_vh_start (void)
{
	f2_video_version = 4;
	f2_4layer_priority = 0;
	f2_hide_pixels = 3;
	f2_xkludge = 0x1e;
	f2_ykludge = 0x08;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_deadconj_vh_start (void)
{
	f2_video_version = 4;
	f2_4layer_priority = 0;
	f2_hide_pixels = 3;
	f2_xkludge = 0x34;
	f2_ykludge = - 0x05;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_metalb_vh_start (void)
{
	f2_video_version = 4;
	f2_4layer_priority = 1;
	f2_hide_pixels = 3;
	f2_xkludge = 0x34;
	f2_ykludge = - 0x04;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_yuyugogo_vh_start (void)
{
	f2_video_version = 1;
	f2_hide_pixels = 3;
	f2_spriteext = 1;
	return (taitof2_core_vh_start());
}

int taitof2_yesnoj_vh_start (void)
{
	f2_video_version = 1;
	f2_hide_pixels = 3;
	f2_spriteext = 0;
	return (taitof2_core_vh_start());
}

int taitof2_dinorex_vh_start (void)
{
	f2_video_version = 1;
	f2_hide_pixels = 3;
	f2_spriteext = 3;
	return (taitof2_core_vh_start());
}

int taitof2_dondokod_vh_start (void)	/* dondokod, cameltry */
{
	f2_video_version = 2;
	f2_hide_pixels = 3;
	f2_spriteext = 0;
	f2_pivot_xdisp = -16;
	f2_pivot_ydisp = 0;
	return (taitof2_core_vh_start());
}

int taitof2_pulirula_vh_start (void)
{
	f2_video_version = 2;
	f2_hide_pixels = 3;
	f2_spriteext = 2;
	f2_pivot_xdisp = -10;	/* alignment seems correct (see level 2, falling */
	f2_pivot_ydisp = 16;	/* block of ice after armour man) */
	return (taitof2_core_vh_start());
}

void taitof2_vh_stop (void)
{
	free(spriteram_delayed);
	spriteram_delayed = 0;
	free(spriteram_buffered);
	spriteram_buffered = 0;

	TC0100SCN_vh_stop();

	if (has_TC0110PCR())
		TC0110PCR_vh_stop();

	if (has_TC0280GRD())
		TC0280GRD_vh_stop();

	if (has_TC0430GRW())
		TC0430GRW_vh_stop();

	free(f2_textram);
	f2_textram = 0;
	free(taitof2_characterram);
	taitof2_characterram = 0;

	if (f2_video_version == 4)   /* Deadconx, Hthero, Metalblack */
	{
		free (char_dirty);
		free (text_dirty);
		bitmap_free (tmpbitmap5);

		free (f2_4layer_dirty);
		bitmap_free (tmpbitmap6);
		bitmap_free (tmpbitmap7);
		bitmap_free (tmpbitmap8);
		bitmap_free (tmpbitmap9);
	}
}


/********************************************************
          LAYER READ AND WRITE HANDLERS
********************************************************/

READ_HANDLER( taitof2_spriteram_r )
{
	return READ_WORD(&spriteram[offset]);
}

WRITE_HANDLER( taitof2_spriteram_w )
{
	COMBINE_WORD_MEM(&spriteram[offset],data);
}

READ_HANDLER( taitof2_sprite_extension_r )   // for debugging purposes, not used by games
{
	if (offset < 0x1000)
	{
		return READ_WORD(&f2_sprite_extension[offset]);
	}
	else
	{
		return 0x0;
	}
}

WRITE_HANDLER( taitof2_sprite_extension_w )
{
	if (offset < 0x1000)   /* areas above this cleared in some games, but not used */
	{
		COMBINE_WORD_MEM(&f2_sprite_extension[offset],data);
	}
}

READ_HANDLER( taitof2_characterram_r )   /* we have to straighten out the 16-bit word into bytes for gfxdecode() to work */
{
	int res;

	res = READ_WORD (&taitof2_characterram[offset]);

	#ifdef LSB_FIRST
	res = ((res & 0x00ff) << 8) | ((res & 0xff00) >> 8);
	#endif

	return res;
}

WRITE_HANDLER( taitof2_characterram_w )
{
	int oldword = READ_WORD (&taitof2_characterram[offset]);
	int newword;


	#ifdef LSB_FIRST
	data = ((data & 0x00ff00ff) << 8) | ((data & 0xff00ff00) >> 8);
	#endif

	newword = COMBINE_WORD (oldword,data);
	if (oldword != newword)
	{
		WRITE_WORD (&taitof2_characterram[offset],newword);
		char_dirty[offset / (f2_characterram_size/256)] = 1;   /* some games use 32 bytes per text char, not 16 */
	}
}

READ_HANDLER( taitof2_text_r )
{
	return READ_WORD(&f2_textram[offset]);
}

WRITE_HANDLER( taitof2_text_w )
{
	int oldword = READ_WORD (&f2_textram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&f2_textram[offset],newword);
		text_dirty[offset / 2] = 1;
	}
}

READ_HANDLER( taitof2_4layer_r )
{
	return READ_WORD(&f2_4layerram[offset]);
}

WRITE_HANDLER( taitof2_4layer_w )
{
	int oldword = READ_WORD (&f2_4layerram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&f2_4layerram[offset],newword);
		f2_4layer_dirty[offset / 4] = 1;
	}
}



WRITE_HANDLER( taitof2_spritebank_w )
{
	int i;
	int j;

	i=0;
	j = (offset >> 1);
	if (j < 2) return;   /* these are always irrelevant zero writes */

	if ((offset >> 1) < 4)   /* special bank pairs */
	{
		j = (offset & 2);   /* either set pair 0&1 or 2&3 */
		i = data << 11;

		logerror("bank %d, set to: %04x\n", j, i);
		logerror("bank %d, paired so: %04x\n", j + 1, i + 0x400);

		spritebank[j] = i;
		spritebank[j+1] = (i + 0x400);

	}
	else   /* last 4 are individual banks */
	{
		i = data << 10;
		logerror("bank %d, new value: %04x\n", j, i);
		spritebank[j] = i;
	}

}

READ_HANDLER( koshien_spritebank_r )
{
	return koshien_spritebank;
}

WRITE_HANDLER( koshien_spritebank_w )
{
	koshien_spritebank = data;

	spritebank[0]=0x0000;   /* never changes */
	spritebank[1]=0x0400;

	spritebank[2] =  ((data & 0x00f) + 1) * 0x800;
	spritebank[4] = (((data & 0x0f0) >> 4) + 1) * 0x800;
	spritebank[6] = (((data & 0xf00) >> 8) + 1) * 0x800;
	spritebank[3] = spritebank[2] + 0x400;
	spritebank[5] = spritebank[4] + 0x400;
	spritebank[7] = spritebank[6] + 0x400;
}

#define CONVERT_SPRITE_CODE												\
{																		\
	int bank;															\
																		\
	bank = (code & 0x1800) >> 11;										\
	switch (bank)														\
	{																	\
		case 0: code = spritebank[2] * 0x800 + (code & 0x7ff); break;	\
		case 1: code = spritebank[3] * 0x800 + (code & 0x7ff); break;	\
		case 2: code = spritebank[4] * 0x400 + (code & 0x7ff); break;	\
		case 3: code = spritebank[6] * 0x800 + (code & 0x7ff); break;	\
	}																	\
}																		\


/*******************************************
			PALETTE
*******************************************/

void taitof2_update_palette(void)
{
	int i, area;
	int off,extoffs,code,color;
	int spritecont,big_sprite=0,last_continuation_tile=0;
	unsigned short palette_map[256];

	memset (palette_map, 0, sizeof (palette_map));

	if (f2_video_version == 4)   /* Deadconx, Hthero, Metalblack */
	{
		/* text layer */
		for (i = 0;i < f2_textram_size;i += 2)
		{
			int tile;

			tile = READ_WORD (&f2_textram[i]) & 0xff;
			color = (READ_WORD (&f2_textram[i]) & 0x3f00) >> 8;

			palette_map[color] |= Machine->gfx[2]->pen_usage[i];
		}

		/* 4 layers */
		for (i = 0;i < f2_4layerram_size;i += 4)
		{
			int tile;

			tile = READ_WORD (&f2_4layerram[i + 2]);
			color = (READ_WORD (&f2_4layerram[i]) & 0xff);

			palette_map[color] |= Machine->gfx[1]->pen_usage[tile];
		}
	}


// DG: we aren't applying sprite marker tests here, but doesn't seem
// to cause palette overflows, so I don't think we should worry.

	color = 0;
	area = sprites_active_area;

	/* Sprites */
	for (off = 0;off < 0x4000;off += 16)
	{
		/* sprites_active_area may change during processing */
		int offs = off + area;

		if (spriteram_buffered[(offs+6)/2] & 0x8000)
		{
			area = 0x8000 * (spriteram_buffered[(offs+10)/2] & 0x0001);
			continue;
		}

		spritecont = (spriteram_buffered[(offs+8)/2] & 0xff00) >> 8;

		if (spritecont & 0x8)
		{
			big_sprite = 1;
		}
		else if (big_sprite)
		{
			last_continuation_tile = 1;
		}

		code = 0;
		extoffs = offs;
		if (extoffs >= 0x8000) extoffs -= 0x4000;   /* spriteram[0x4000-7fff] has no corresponding extension area */

		if (f2_spriteext == 0)
		{
			code = spriteram_buffered[offs/2] & 0x1fff;
			{
				int bank;

				bank = (code & 0x1c00) >> 10;
				code = spritebank[bank] + (code & 0x3ff);
			}
		}

		if (f2_spriteext == 1)   /* Yuyugogo */
		{
			code = spriteram_buffered[offs/2] & 0x3ff;
			i = (READ_WORD(&f2_sprite_extension[(extoffs >> 3)]) & 0x3f ) << 10;
			code = (i | code);
		}

		if (f2_spriteext == 2)   /* Pulirula */
		{
			code = spriteram_buffered[offs/2] & 0xff;
			i = (READ_WORD(&f2_sprite_extension[(extoffs >> 3)]) & 0xff00 );
			code = (i | code);
		}

		if (f2_spriteext == 3)   /* Dinorex and a few quizzes */
		{
			code = spriteram_buffered[offs/2] & 0xff;
			i = (READ_WORD(&f2_sprite_extension[(extoffs >> 3)]) & 0xff ) << 8;
			code = (i | code);
		}

		if ((spritecont & 0x04) == 0)
			color = spriteram_buffered[(offs+8)/2] & 0x00ff;

		if (last_continuation_tile)
		{
			big_sprite=0;
			last_continuation_tile=0;
		}

		if (!code) continue;   /* tilenum is 0, so ignore it */

		if (Machine->gfx[0]->color_granularity == 64)	/* Final Blow is 6-bit deep */
		{
			color &= ~3;
			palette_map[color+0] |= 0xffff;
			palette_map[color+1] |= 0xffff;
			palette_map[color+2] |= 0xffff;
			palette_map[color+3] |= 0xffff;
		}
		else
			palette_map[color] |= Machine->gfx[0]->pen_usage[code];
	}


	/* Tell MAME about the color usage */
	for (i = 0;i < 256;i++)
	{
		int usage = palette_map[i];
		int j;

		if (usage)
		{
			/* don't use COLOR_TRANSPARENT for games using only tilemap.c */
			if (f2_video_version == 4)
				palette_used_colors[i * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
			else
				if (palette_map[i] & (1 << 0))
					palette_used_colors[i * 16 + 0] = PALETTE_COLOR_USED;
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
		}
	}
}

static void draw_sprites(struct osd_bitmap *bitmap,int *primasks)
{
	/*
		Sprite format:
		0000: ---xxxxxxxxxxxxx tile code (0x0000 - 0x1fff)
		0002: xxxxxxxx-------- sprite y-zoom level
		      --------xxxxxxxx sprite x-zoom level

			  0x00 - non scaled = 100%
			  0x80 - scaled to 50%
			  0xc0 - scaled to 25%
			  0xe0 - scaled to 12.5%
			  0xff - scaled to zero pixels size (off)

		0004: ----xxxxxxxxxxxx x-coordinate (-0x800 to 0x07ff)
		      ---x------------ latch extra scroll
		      --x------------- latch master scroll
		      -x-------------- don't use extra scroll compensation
		      x--------------- absolute screen coordinates (ignore all sprite scrolls)
		      xxxx------------ the typical use of the above is therefore
			                   1010 = set master scroll
		                       0101 = set extra scroll
		0006: ----xxxxxxxxxxxx y-coordinate (-0x800 to 0x07ff)
		      x--------------- marks special control commands (used in conjunction with 00a)
		                       If the special command flag is set:
		      ---------------x related to sprite ram bank
		      ---x------------ unknown (deadconx, maybe others)
		      --x------------- unknown, some games (growl, gunfront) set it to 1 when
		                       screen is flipped
		0008: --------xxxxxxxx color (0x00 - 0xff)
		      -------x-------- flipx
		      ------x--------- flipy
		      -----x---------- if set, use latched color, else use & latch specified one
		      ----x----------- if set, next sprite entry is part of sequence
		      ---x------------ if clear, use latched y coordinate, else use current y
		      --x------------- if set, y += 16
		      -x-------------- if clear, use latched x coordinate, else use current x
		      x--------------- if set, x += 16
		000a: only valid when the special command bit in 006 is set
		      ---------------x related to sprite ram bank. I think this is the one causing
		                       the bank switch, implementing it this way all games seem
		                       to properly bank switch except for footchmp which uses the
		                       bit in byte 006 instead.
		      ------------x--- unknown; some games toggle it before updating sprite ram.
		      ------xx-------- unknown (finalb)
		      -----x---------- unknown (mjnquest)
		      ---x------------ disable the following sprites until another marker with
			                   this bit clear is found
		      --x------------- flip screen

		000b - 000f : unused


		Less aggressive & ops on our coordinates has stopped offscreen
		sprites wrapping back onto screen (Growl, Pulirula) but sprites
		do now seem to "pop up" a bit near the top right in Growl round
		1. Pulirula also seems given to some sprite pop up inside
		screen boundaries. Perhaps this is real behaviour?

	DG comment: the sprite zoom code grafted on from Jarek's TaitoB
	may mean I have pointlessly duplicated x,y latches in the zoom &
	non zoom parts.

	*/
	int x,y,off,extoffs;
	int code,color,spritedata,spritecont,flipx,flipy;
	int xcurrent,ycurrent,big_sprite=0;
	int y_no=0, x_no=0, xlatch=0, ylatch=0, last_continuation_tile=0;   /* for zooms */
	unsigned int zoomword, zoomx, zoomy, zx=0, zy=0, zoomxlatch=0, zoomylatch=0;   /* for zooms */
	int scroll1x, scroll1y;
	int scrollx=0, scrolly=0;
	int curx,cury;
	int f2_x_offset;
	int sprites_flipscreen = 0;

	/* pdrawgfx() needs us to draw sprites front to back, so we have to build a list
	   while processing sprite ram and then draw them all a tthe end */
	struct sprite
	{
		int code,color;
		int flipx,flipy;
		int x,y;
		int zoomx,zoomy;
		int primask;
	};
	struct sprite spritelist[0x400];
	struct sprite *sprite_ptr = spritelist;


	/* must remember enable status from last frame because driftout fails to
	   reactivate them from a certain point onwards. */
	static int sprites_disabled = 1;

	/* must remember master scroll from previous frame because driftout
	   sometimes doesn't set it. */
	static int master_scrollx=0, master_scrolly=0;

	/* must also remember the sprite bank from previous frame. This is a global
	   in order to reset it in vh_start() */


	scroll1x = 0;
	scroll1y = 0;
	x = y = 0;
	xcurrent = ycurrent = 0;
	color = 0;


	f2_x_offset = f2_hide_pixels;   /* Get rid of 0-3 unwanted pixels on edge of screen. */
	if (sprites_flipscreen) f2_x_offset = -f2_x_offset;


	/* safety check to avoid getting stuck in bank 2 for games using only one bank */
	if (sprites_active_area == 0x8000 &&
			spriteram_buffered[(0x8000+6)/2] == 0 &&
			spriteram_buffered[(0x8000+10)/2] == 0)
		sprites_active_area = 0;


	for (off = 0;off < 0x4000;off += 16)
	{
		/* sprites_active_area may change during processing */
		int offs = off + sprites_active_area;

		if (spriteram_buffered[(offs+6)/2] & 0x8000)
		{
			sprites_disabled = spriteram_buffered[(offs+10)/2] & 0x1000;
			sprites_flipscreen = spriteram_buffered[(offs+10)/2] & 0x2000;
			f2_x_offset = f2_hide_pixels;   /* Get rid of 0-3 unwanted pixels on edge of screen. */
			if (sprites_flipscreen) f2_x_offset = -f2_x_offset;
			sprites_active_area = 0x8000 * (spriteram_buffered[(offs+10)/2] & 0x0001);
			continue;
		}

//usrintf_showmessage("%04x",sprites_active_area);

		/* check for extra scroll offset */
		if ((spriteram_buffered[(offs+4)/2] & 0xf000) == 0xa000)
		{
			master_scrollx = spriteram_buffered[(offs+4)/2] & 0xfff;
			if (master_scrollx >= 0x800) master_scrollx -= 0x1000;   /* signed value */
			master_scrolly = spriteram_buffered[(offs+6)/2] & 0xfff;
			if (master_scrolly >= 0x800) master_scrolly -= 0x1000;   /* signed value */
		}

		if ((spriteram_buffered[(offs+4)/2] & 0xf000) == 0x5000)
		{
			scroll1x = spriteram_buffered[(offs+4)/2] & 0xfff;
			if (scroll1x >= 0x800) scroll1x -= 0x1000;   /* signed value */
			scroll1x += master_scrollx;

			scroll1y = spriteram_buffered[(offs+6)/2] & 0xfff;
			if (scroll1y >= 0x800) scroll1y -= 0x1000;   /* signed value */
			scroll1y += master_scrolly;
		}

		if (sprites_disabled)
			continue;

		spritedata = spriteram_buffered[(offs+8)/2];

		spritecont = (spritedata & 0xff00) >> 8;

		if ((spritecont & 0x08) != 0)   /* sprite continuation flag set */
		{
			if (big_sprite == 0)   /* are we starting a big sprite ? */
			{
				xlatch = spriteram_buffered[(offs+4)/2] & 0xfff;
				ylatch = spriteram_buffered[(offs+6)/2] & 0xfff;
				x_no = 0;
				y_no = 0;
				zoomword = spriteram_buffered[(offs+2)/2];
				zoomylatch = (zoomword>>8) & 0xff;
				zoomxlatch = (zoomword) & 0xff;
				big_sprite = 1;   /* we have started a new big sprite */
			}
		}
		else if (big_sprite)
		{
			last_continuation_tile = 1;   /* don't clear big_sprite until last tile done */
		}


		if ((spritecont & 0x04) == 0)
			color = spritedata & 0xff;


// DG: the bigsprite == 0 check fixes "tied-up" little sprites in Thunderfox
// which (mostly?) have spritecont = 0x20 when they are not continuations
// of anything.
		if (big_sprite == 0 || (spritecont & 0xf0) == 0)
		{
			x = spriteram_buffered[(offs+4)/2];

// DG: some absolute x values deduced here are 1 too high (scenes when you get
// home run in Koshien, and may also relate to BG layer woods and stuff as you
// journey in MjnQuest). You will see they are 1 pixel too far to the right.
// Where is this extra pixel offset coming from?? Also Pulirula doors near
// start of round 3 are some games off by a pixel or two; different issue?

			if (x & 0x8000)   /* absolute (koshien) */
			{
				scrollx = - f2_x_offset - 0x60;
				scrolly = 0;
			}
			else if (x & 0x4000)   /* ignore extra scroll */
			{
				scrollx = master_scrollx - f2_x_offset - 0x60;
				scrolly = master_scrolly;
			}
			else   /* all scrolls applied */
			{
				scrollx = scroll1x - f2_x_offset - 0x60;
				scrolly = scroll1y;
			}
			x &= 0xfff;
			y = spriteram_buffered[(offs+6)/2] & 0xfff;

			xcurrent = x;
			ycurrent = y;
		}
		else
		{
			if ((spritecont & 0x10) == 0)
				y = ycurrent;
			else if ((spritecont & 0x20) != 0)
			{
				y += 16;
				y_no++;   /* keep track of y tile for zooms */
			}
			if ((spritecont & 0x40) == 0)
				x = xcurrent;
			else if ((spritecont & 0x80) != 0)
			{
				x += 16;
				y_no=0;
				x_no++;   /* keep track of x tile for zooms */
			}
		}

/* Black lines between flames in Gunfront before zoom finishes
   suggest some flaw in these calculations */

		if (big_sprite)
		{
			zoomx = zoomxlatch;
			zoomy = zoomylatch;

			if (zoomx || zoomy)
			{
				x = xlatch + x_no * (0x100 - zoomx) / 16;
				y = ylatch + y_no * (0x100 - zoomy) / 16;
				zx = xlatch + (x_no+1) * (0x100 - zoomx) / 16 - x;
				zy = ylatch + (y_no+1) * (0x100 - zoomy) / 16 - y;
			}
		}
		else
		{
			zoomword = spriteram_buffered[(offs+2)/2];
			zoomy = (zoomword>>8) & 0xff;
			zoomx = (zoomword) & 0xff;
			zx = (0x100 - zoomx) / 16;
			zy = (0x100 - zoomy) / 16;
		}

		if (last_continuation_tile)
		{
			big_sprite=0;
			last_continuation_tile=0;
		}

		code = 0;
		extoffs = offs;
		if (extoffs >= 0x8000) extoffs -= 0x4000;   /* spriteram[0x4000-7fff] has no corresponding extension area */

		if (f2_spriteext == 0)
		{
			code = spriteram_buffered[(offs)/2] & 0x1fff;
#if 1
			{
				int bank;

				bank = (code & 0x1c00) >> 10;
				code = spritebank[bank] + (code & 0x3ff);
			}
#else
			CONVERT_SPRITE_CODE;
#endif
		}

		if (f2_spriteext == 1)   /* Yuyugogo */
		{
			int i;

			code = spriteram_buffered[(offs)/2] & 0x3ff;
			i = (READ_WORD(&f2_sprite_extension[(extoffs >> 3)]) & 0x3f ) << 10;
			code = (i | code);
		}

		if (f2_spriteext == 2)   /* Pulirula */
		{
			int i;

			code = spriteram_buffered[(offs)/2] & 0xff;
			i = (READ_WORD(&f2_sprite_extension[(extoffs >> 3)]) & 0xff00 );
			code = (i | code);
		}

		if (f2_spriteext == 3)   /* Dinorex and a few quizzes */
		{
			int i;

			code = spriteram_buffered[(offs)/2] & 0xff;
			i = (READ_WORD(&f2_sprite_extension[(extoffs >> 3)]) & 0xff ) << 8;
			code = (i | code);
		}

		if (code == 0) continue;

		flipx = spritecont & 0x01;
		flipy = spritecont & 0x02;

		curx = (x + scrollx) & 0xfff;
		if (curx >= 0x800)	curx -= 0x1000;   /* treat it as signed */

		cury = (y + scrolly) & 0xfff;
		if (cury >= 0x800)	cury -= 0x1000;   /* treat it as signed */

		if (sprites_flipscreen)
		{
			curx = 304 - curx;
			cury = 240 - cury;
			flipx = !flipx;
			flipy = !flipy;
		}

		{
			sprite_ptr->code = code;
			sprite_ptr->color = color;
			if (Machine->gfx[0]->color_granularity == 64)	/* Final Blow is 6-bit deep */
				sprite_ptr->color /= 4;
			sprite_ptr->flipx = flipx;
			sprite_ptr->flipy = flipy;
			sprite_ptr->x = curx;
			sprite_ptr->y = cury;

			if (!(zoomx || zoomy))
			{
				sprite_ptr->zoomx = 0x10000;
				sprite_ptr->zoomy = 0x10000;
			}
			else
			{
				sprite_ptr->zoomx = zx << 12;
				sprite_ptr->zoomy = zy << 12;
			}

			if (primasks)
			{
				sprite_ptr->primask = primasks[(color & 0xc0) >> 6];

				sprite_ptr++;
			}
			else
			{
				drawgfxzoom(bitmap,Machine->gfx[0],
						sprite_ptr->code,
						sprite_ptr->color,
						sprite_ptr->flipx,sprite_ptr->flipy,
						sprite_ptr->x,sprite_ptr->y,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						sprite_ptr->zoomx,sprite_ptr->zoomy);
			}
		}
	}


	/* this happens only if primsks != NULL */
	while (sprite_ptr != spritelist)
	{
		sprite_ptr--;

		pdrawgfxzoom(bitmap,Machine->gfx[0],
				sprite_ptr->code,
				sprite_ptr->color,
				sprite_ptr->flipx,sprite_ptr->flipy,
				sprite_ptr->x,sprite_ptr->y,
				&Machine->visible_area,TRANSPARENCY_PEN,0,
				sprite_ptr->zoomx,sprite_ptr->zoomy,
				sprite_ptr->primask);
	}
}



static void draw_text_layer(struct osd_bitmap *bitmap)
{
	int offs;
	int scrollx,scrolly;
	int f2_x_offset = f2_hide_pixels;   /* Get rid of 0-3 unwanted pixels on edge of screen. */
	if (tile_flipscreen) f2_x_offset = -f2_x_offset;


	/* Do the text layer */
	for (offs = 0;offs < f2_textram_size;offs += 2)
	{
		int tile, color, flipx, flipy;

		tile = READ_WORD (&f2_textram[offs]) & 0xff;
		color = (READ_WORD (&f2_textram[offs]) & 0x3f00) >> 8;
		flipy = (READ_WORD (&f2_textram[offs]) & 0x8000);
		flipx = (READ_WORD (&f2_textram[offs]) & 0x4000);

		if (text_dirty[offs/2] || char_dirty[tile])
		{
			int sx,sy;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;
			if (tile_flipscreen)
			{
				sx = 63 - sx;
				sy = 63 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}
			text_dirty[offs/2] = 0;

			drawgfx(tmpbitmap5,Machine->gfx[2],
				tile,
				color,
				flipx,flipy,
				8*sx,8*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	/* Deadconx, Hthero, Metalb */
	{
		int xkludge = f2_xkludge + f2_x_offset;
		scrollx = READ_WORD (&f2_4layerregs[0x18]) - xkludge + 1;
		scrolly = READ_WORD (&f2_4layerregs[0x1a]) + f2_ykludge;
	}
	scrollx -= f2_x_offset;
	if (tile_flipscreen)
	{
		scrollx = 320+7 - scrollx;
		scrolly = 256+16 - scrolly;
	}
	copyscrollbitmap(bitmap,tmpbitmap5,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
}



static int prepare_sprites;

static void taitof2_handle_sprite_buffering(void)
{
	if (prepare_sprites)	/* no buffering */
		memcpy(spriteram_buffered,spriteram,spriteram_size);
}

void taitof2_no_buffer_eof_callback(void)
{
	prepare_sprites = 1;
}
void taitof2_partial_buffer_delayed_eof_callback(void)
{
	int i;

	prepare_sprites = 0;
	memcpy(spriteram_buffered,spriteram_delayed,spriteram_size);
	for (i = 0;i < spriteram_size/2;i += 4)
		spriteram_buffered[i] = READ_WORD(&spriteram[2*i]);
	memcpy(spriteram_delayed,spriteram,spriteram_size);
}
void taitof2_partial_buffer_delayed_thundfox_eof_callback(void)
{
	int i;

	prepare_sprites = 0;
	memcpy(spriteram_buffered,spriteram_delayed,spriteram_size);
	for (i = 0;i < spriteram_size/2;i += 8)
	{
		spriteram_buffered[i]   = READ_WORD(&spriteram[2*i]);
		spriteram_buffered[i+1] = READ_WORD(&spriteram[2*(i+1)]);
		spriteram_buffered[i+4] = READ_WORD(&spriteram[2*(i+4)]);
	}
	memcpy(spriteram_delayed,spriteram,spriteram_size);
}



/* SSI */
void ssi_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	taitof2_handle_sprite_buffering();

	palette_init_used_colors();
	taitof2_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	palette_recalc();

	/* SSI only uses sprites, the tilemap registers are not even initialized.
	   (they are in Majestic 12, but the tilemaps are not used anyway) */
	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	draw_sprites(bitmap,NULL);
}


void yesnoj_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	taitof2_handle_sprite_buffering();

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	taitof2_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */
	draw_sprites(bitmap,NULL);
	TC0100SCN_tilemap_draw(bitmap,0,TC0100SCN_bottomlayer(0),0);
	TC0100SCN_tilemap_draw(bitmap,0,TC0100SCN_bottomlayer(0)^1,0);
	TC0100SCN_tilemap_draw(bitmap,0,2,0);
}


void taitof2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	taitof2_handle_sprite_buffering();

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	taitof2_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */
	TC0100SCN_tilemap_draw(bitmap,0,TC0100SCN_bottomlayer(0),0);
	TC0100SCN_tilemap_draw(bitmap,0,TC0100SCN_bottomlayer(0)^1,0);
	draw_sprites(bitmap,NULL);
	TC0100SCN_tilemap_draw(bitmap,0,2,0);
}


void taitof2_pri_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int tilepri[3];
	int spritepri[4];
	int layer[3];


	taitof2_handle_sprite_buffering();

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	taitof2_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;
	tilepri[layer[0]] = TC0360PRI_regs[5] & 0x0f;
	tilepri[layer[1]] = TC0360PRI_regs[5] >> 4;
	tilepri[layer[2]] = TC0360PRI_regs[4] >> 4;

	spritepri[0] = TC0360PRI_regs[6] & 0x0f;
	spritepri[1] = TC0360PRI_regs[6] >> 4;
	spritepri[2] = TC0360PRI_regs[7] & 0x0f;
	spritepri[3] = TC0360PRI_regs[7] >> 4;

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],1<<16);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],2<<16);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],4<<16);

	{
		int primasks[4] = {0,0,0,0};
		int i;

		for (i = 0;i < 4;i++)
		{
			if (spritepri[i] < tilepri[0]) primasks[i] |= 0xaa;
			if (spritepri[i] < tilepri[1]) primasks[i] |= 0xcc;
			if (spritepri[i] < tilepri[2]) primasks[i] |= 0xf0;
		}

		draw_sprites(bitmap,primasks);
	}
}



static void draw_roz_layer(struct osd_bitmap *bitmap)
{
	if (has_TC0280GRD())
		TC0280GRD_zoom_draw(bitmap,f2_pivot_xdisp,f2_pivot_ydisp,8);

	if (has_TC0430GRW())
		TC0430GRW_zoom_draw(bitmap,f2_pivot_xdisp,f2_pivot_ydisp,8);
}


void taitof2_pri_roz_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int tilepri[3];
	int spritepri[4];
	int rozpri;
	int layer[3];
	int drawn;
	int lastpri;
	int roz_base_color = (TC0360PRI_regs[1] & 0x3f) << 2;


	taitof2_handle_sprite_buffering();

	if (has_TC0280GRD())
		TC0280GRD_tilemap_update(roz_base_color);

	if (has_TC0430GRW())
		TC0430GRW_tilemap_update(roz_base_color);

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	taitof2_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	{
		int i;

		/* fix roz transparency, but this could compromise the background color */
		for (i = 0;i < 4;i++)
			palette_used_colors[16 * (roz_base_color+i)] = PALETTE_COLOR_TRANSPARENT;
	}
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;
	tilepri[layer[0]] = TC0360PRI_regs[5] & 0x0f;
	tilepri[layer[1]] = TC0360PRI_regs[5] >> 4;
	tilepri[layer[2]] = TC0360PRI_regs[4] >> 4;

	spritepri[0] = TC0360PRI_regs[6] & 0x0f;
	spritepri[1] = TC0360PRI_regs[6] >> 4;
	spritepri[2] = TC0360PRI_regs[7] & 0x0f;
	spritepri[3] = TC0360PRI_regs[7] >> 4;

	rozpri = (TC0360PRI_regs[1] & 0xc0) >> 6;
	rozpri = (TC0360PRI_regs[8 + rozpri/2] >> 4*(rozpri & 1)) & 0x0f;

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */

	drawn = 0;
	lastpri = 0;
	while (drawn < 3)
	{
		if (rozpri > lastpri && rozpri <= tilepri[drawn])
		{
			draw_roz_layer(bitmap);
			lastpri = rozpri;
		}
		TC0100SCN_tilemap_draw(bitmap,0,layer[drawn],(1<<drawn)<<16);
		lastpri = tilepri[drawn];
		drawn++;
	}
	if (rozpri > lastpri)
		draw_roz_layer(bitmap);

	{
		int primasks[4] = {0,0,0,0};
		int i;

		for (i = 0;i < 4;i++)
		{
			if (spritepri[i] < tilepri[0]) primasks[i] |= 0xaaaa;
			if (spritepri[i] < tilepri[1]) primasks[i] |= 0xcccc;
			if (spritepri[i] < tilepri[2]) primasks[i] |= 0xf0f0;
			if (spritepri[i] < rozpri)     primasks[i] |= 0xff00;
		}

		draw_sprites(bitmap,primasks);
	}
}



/* Thunderfox */
void thundfox_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int tilepri[2][3];
	int spritepri[4];
	int layer[2][3];
	int drawn[2];


	taitof2_handle_sprite_buffering();

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	taitof2_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	if (palette_recalc ())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);


	layer[0][0] = TC0100SCN_bottomlayer(0);
	layer[0][1] = layer[0][0]^1;
	layer[0][2] = 2;
	tilepri[0][layer[0][0]] = TC0360PRI_regs[5] & 0x0f;
	tilepri[0][layer[0][1]] = TC0360PRI_regs[5] >> 4;
	tilepri[0][layer[0][2]] = TC0360PRI_regs[4] >> 4;

	layer[1][0] = TC0100SCN_bottomlayer(1);
	layer[1][1] = layer[1][0]^1;
	layer[1][2] = 2;
	tilepri[1][layer[1][0]] = TC0360PRI_regs[9] & 0x0f;
	tilepri[1][layer[1][1]] = TC0360PRI_regs[9] >> 4;
	tilepri[1][layer[1][2]] = TC0360PRI_regs[8] >> 4;

	spritepri[0] = TC0360PRI_regs[6] & 0x0f;
	spritepri[1] = TC0360PRI_regs[6] >> 4;
	spritepri[2] = TC0360PRI_regs[7] & 0x0f;
	spritepri[3] = TC0360PRI_regs[7] >> 4;


	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */


	/*
	TODO: This isn't the correct way to handle the priority. At the moment of
	writing, pdrawgfx() doesn't support 6 layers, so I have to cheat, assuming
	that the two FG layers are always on top of sprites.
	*/

	drawn[0] = drawn[1] = 0;
	while (drawn[0] < 2 && drawn[1] < 2)
	{
		int pick;

		if (tilepri[0][drawn[0]] < tilepri[1][drawn[1]])
			pick = 0;
		else pick = 1;

		TC0100SCN_tilemap_draw(bitmap,pick,layer[pick][drawn[pick]],(1<<(drawn[pick]+2*pick))<<16);
		drawn[pick]++;
	}
	while (drawn[0] < 2)
	{
		TC0100SCN_tilemap_draw(bitmap,0,layer[0][drawn[0]],(1<<drawn[0])<<16);
		drawn[0]++;
	}
	while (drawn[1] < 2)
	{
		TC0100SCN_tilemap_draw(bitmap,1,layer[1][drawn[1]],(1<<(drawn[1]+2))<<16);
		drawn[1]++;
	}

	{
		int primasks[4] = {0,0,0,0};
		int i;

		for (i = 0;i < 4;i++)
		{
			if (spritepri[i] < tilepri[0][0]) primasks[i] |= 0xaaaa;
			if (spritepri[i] < tilepri[0][1]) primasks[i] |= 0xcccc;
			if (spritepri[i] < tilepri[1][0]) primasks[i] |= 0xf0f0;
			if (spritepri[i] < tilepri[1][1]) primasks[i] |= 0xff00;
		}

		draw_sprites(bitmap,primasks);
	}


	/*
	TODO: This isn't the correct way to handle the priority. At the moment of
	writing, pdrawgfx() doesn't support 6 layers, so I have to cheat, assuming
	that the two FG layers are always on top of sprites.
	*/

	if (tilepri[0][2] < tilepri[1][2])
	{
		TC0100SCN_tilemap_draw(bitmap,0,layer[0][2],0);
		TC0100SCN_tilemap_draw(bitmap,1,layer[1][2],0);
	}
	else
	{
		TC0100SCN_tilemap_draw(bitmap,1,layer[1][2],0);
		TC0100SCN_tilemap_draw(bitmap,0,layer[0][2],0);
	}
}



/* Deadconx, Hthero, Metalb */
void deadconx_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,layeroffs=0,layernum=0;
	int xkludge = 0, ykludge = 0;
	int scrollx, scrllx[3], scrlly[3];
	int f2_x_offset;


	taitof2_handle_sprite_buffering();

	tile_flipscreen = 0;   // unsure which register it is

	f2_x_offset = f2_hide_pixels;   /* Get rid of 0-3 unwanted pixels on edge of screen. */
	if (tile_flipscreen) f2_x_offset = -f2_x_offset;

	xkludge = f2_xkludge + f2_x_offset;
	ykludge = f2_ykludge;


/*************************************************
     Update char defs, palette and tilemaps
*************************************************/

	/* Decode any characters that have changed */
	{
		int i;

		for (i = 0; i < 256; i ++)
		{
			if (char_dirty[i])
			{
				decodechar (Machine->gfx[2],i,taitof2_characterram, Machine->drv->gfxdecodeinfo[2].gfxlayout);
			}
		}
	}


	palette_init_used_colors();
	taitof2_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	if (palette_recalc ())
	{
//		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

		memset (text_dirty, 1, f2_textram_size/2);
		memset (f2_4layer_dirty, 1, f2_4layerram_size/4);
	}

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);


/****************************************************************************
            Create bitmaps for the four background layers

We need to run through the basic code four times. The layers seem to be in a
game-definable order; Metalb reverses priority and seems to use other orders.
So we construct bitmaps first and then decide the order to apply them.

The 0x200 is there as a reversor for the y-scroll, it's back to front.
Setting flip DSW messes up offsets for these layers badly.

Xkludge, ykludge seem to vary between games, even between clones (eg.
Hthero vs. Euroch92)

You can see if they're right by checking if objects on sprite layer
match to bg (boxes round managers faces in Hthero, doors in Deadconx).

(layernum << 2) is a guess which gets the relative x positions of layers on
level 1 of Deadconx right. It also fixes the position of text chars put in
these layers in metalb attract. But it may not work in all cases.

TODO: add zooming to each layer, stop kludging offsets.
****************************************************************************/

	layernum = 0;
	layeroffs = (layernum << 12);
	scrollx = READ_WORD (&f2_4layerregs[(layernum << 1)]) - xkludge + (layernum << 2);
	scrlly[layernum] = 0x200 - READ_WORD (&f2_4layerregs[(layernum << 1)+8]) + ykludge;

	for (offs = 0;offs < (f2_4layerram_size/4);offs += 4)
	{
		int tile, color, flipx, flipy;

		tile = READ_WORD (&f2_4layerram[offs + layeroffs + 2]);
		color = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0xff);
		flipy = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0x8000);
		flipx = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0x4000);

		if (f2_4layer_dirty[(offs + layeroffs)/4])
		{
			int sx,sy;

			f2_4layer_dirty[(offs + layeroffs)/4] = 0;

			sy = (offs/4) / 32;
			sx = (offs/4) % 32;
			if (tile_flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap6,Machine->gfx[1],
				tile,
				color,
				flipx,flipy,
				16*sx,16*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	layernum = 1;
	layeroffs = (layernum << 12);
	scrllx[layernum] = READ_WORD (&f2_4layerregs[(layernum << 1)]) - xkludge + (layernum << 2);
	scrlly[layernum] = 0x200 - READ_WORD (&f2_4layerregs[(layernum << 1)+8]) + ykludge;

	for (offs = 0;offs < (f2_4layerram_size/4);offs += 4)
	{
		int tile, color, flipx, flipy;

		tile = READ_WORD (&f2_4layerram[offs + layeroffs + 2]);
		color = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0xff);
		flipy = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0x8000);
		flipx = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0x4000);

		if (f2_4layer_dirty[(offs + layeroffs)/4])
		{
			int sx,sy;

			f2_4layer_dirty[(offs + layeroffs)/4] = 0;

			sy = (offs/4) / 32;
			sx = (offs/4) % 32;
			if (tile_flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap7,Machine->gfx[1],
				tile,
				color,
				flipx,flipy,
				16*sx,16*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	layernum = 2;
	layeroffs = (layernum << 12);
	scrllx[layernum] = READ_WORD (&f2_4layerregs[(layernum << 1)]) - xkludge + (layernum << 2);
	scrlly[layernum] = 0x200 - READ_WORD (&f2_4layerregs[(layernum << 1)+8]) + ykludge;

	for (offs = 0;offs < (f2_4layerram_size/4);offs += 4)
	{
		int tile, color, flipx, flipy;

		tile = READ_WORD (&f2_4layerram[offs + layeroffs + 2]);
		color = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0xff);
		flipy = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0x8000);
		flipx = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0x4000);

		if (f2_4layer_dirty[(offs + layeroffs)/4])
		{
			int sx,sy;

			f2_4layer_dirty[(offs + layeroffs)/4] = 0;

			sy = (offs/4) / 32;
			sx = (offs/4) % 32;
			if (tile_flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap8,Machine->gfx[1],
				tile,
				color,
				flipx,flipy,
				16*sx,16*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	layernum = 3;
	layeroffs = (layernum << 12);
	scrllx[layernum] = READ_WORD (&f2_4layerregs[(layernum << 1)]) - xkludge + (layernum << 2);
	scrlly[layernum] = 0x200 - READ_WORD (&f2_4layerregs[(layernum << 1)+8]) + ykludge;

	for (offs = 0;offs < (f2_4layerram_size/4);offs += 4)
	{
		int tile, color, flipx, flipy;

		tile = READ_WORD (&f2_4layerram[offs + layeroffs + 2]);
		color = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0xff);
		flipy = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0x8000);
		flipx = (READ_WORD (&f2_4layerram[offs + layeroffs]) & 0x4000);

		if (f2_4layer_dirty[(offs + layeroffs)/4])
		{
			int sx,sy;

			f2_4layer_dirty[(offs + layeroffs)/4] = 0;

			sy = (offs/4) / 32;
			sx = (offs/4) % 32;
			if (tile_flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap9,Machine->gfx[1],
				tile,
				color,
				flipx,flipy,
				16*sx,16*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}


/**************************************************************
          Copy 4 bg layer bitmaps and sprites to screen

It seems individual sprites may take priorities at any of FOUR
levels. They are always over the lowest bg layer. But sprite
priority wrt the other three bg layers varies. Code here needs
changing to reflect that (multiple calls to draw_sprites ??)
**************************************************************/

// DG: xscrolling for bg0 stopped working when I used &scrllx[0]
// rather than &scrollx !? So scrllx[0] replaced with scrollx.

	if (tile_flipscreen)
	{
		scrollx = 320 - scrollx;
		scrllx[0] = 320 - scrllx[0];
		scrllx[1] = 320 - scrllx[1];
		scrllx[2] = 320 - scrllx[2];
		scrllx[3] = 320 - scrllx[3];
		scrlly[0] = 256 - scrlly[0];
		scrlly[1] = 256 - scrlly[1];
		scrlly[2] = 256 - scrlly[2];
		scrlly[3] = 256 - scrlly[3];
	}

	if (f2_4layer_priority == 0)
	{
		copyscrollbitmap(bitmap,tmpbitmap6,1,&scrollx,1,&scrlly[0],&Machine->visible_area,TRANSPARENCY_NONE,0);
		copyscrollbitmap(bitmap,tmpbitmap7,1,&scrllx[1],1,&scrlly[1],&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		copyscrollbitmap(bitmap,tmpbitmap8,1,&scrllx[2],1,&scrlly[2],&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		copyscrollbitmap(bitmap,tmpbitmap9,1,&scrllx[3],1,&scrlly[3],&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}
	else   // DG: I bet there are other layer orders in metalb
	{
		copyscrollbitmap(bitmap,tmpbitmap9,1,&scrllx[3],1,&scrlly[3],&Machine->visible_area,TRANSPARENCY_NONE,0);
		copyscrollbitmap(bitmap,tmpbitmap8,1,&scrllx[2],1,&scrlly[2],&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		copyscrollbitmap(bitmap,tmpbitmap7,1,&scrllx[1],1,&scrlly[1],&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		copyscrollbitmap(bitmap,tmpbitmap6,1,&scrollx,1,&scrlly[0],&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}

	draw_sprites(bitmap,NULL);



/************************************************
      Just the text layers left to go now !
************************************************/

	draw_text_layer(bitmap);

	memset (char_dirty,0,256);   /* always 256 text chars */
}
