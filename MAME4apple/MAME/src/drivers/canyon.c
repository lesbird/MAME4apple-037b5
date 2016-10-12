/***************************************************************************

Atari Canyon Bomber Driver

Memory Map:
        0000-01FF       WRAM
        0400-04FF       W A0=0:MOTOR1, A0=1:MOTOR2
        0500-05FF       W A0=0:EXPLODE, A0=1:TIMER RESET
        0600-067F       W A0=0:WHISTLE1, A0=1:WHISTLE2
        0680-06FF       W A0=0:LED1, A0=1:LED2
        0700-077F       W A0=0:ATTRACT1, A0=1:ATTRACT2
        0800-0FFF       DISPLAY / RAM
        1000-17FF       SWITCHES
        1800-1FFF       OPTIONS
        2000-27FF       ROM1
        2800-2FFF       ROM2
        3000-37FF       ROM3
        3800-3FFF       ROM4 (Program ROM)
       (F800-FFFF)      ROM4 (Program ROM) - only needed for the 6502 vectors

If you have any questions about how this driver works, don't hesitate to
ask.  - Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* vidhrdw/canyon.c */
extern void canyon_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static READ_HANDLER( canyon_options_r )
{
	switch (offset & 0x03)
	{
		case 0x00:
			return ((input_port_0_r(0) >> 6) & 0x03);
		case 0x01:
			return ((input_port_0_r(0) >> 4) & 0x03);
		case 0x02:
			return ((input_port_0_r(0) >> 2) & 0x03);
		case 0x03:
			return ((input_port_0_r(0) >> 0) & 0x03);
	}

	return 0xFF;
}

static READ_HANDLER( canyon_switches_r )
{
	switch (offset & 0x07)
	{
		case 0x00:
			return ((input_port_3_r(0) << 7) & 0x80);
		case 0x01:
			return ((input_port_3_r(0) << 6) & 0x80);
		case 0x02:
			return ((input_port_3_r(0) << 5) & 0x80) | input_port_1_r(0);
		case 0x03:
			return ((input_port_3_r(0) << 4) & 0x80) | input_port_2_r(0);
		case 0x04:
			return ((input_port_3_r(0) << 3) & 0x80);
		case 0x05:
			return ((input_port_3_r(0) << 2) & 0x80);
		case 0x06:
			return ((input_port_3_r(0) << 1) & 0x80) | input_port_1_r(0);
		case 0x07:
			return ((input_port_3_r(0) << 0) & 0x80) | input_port_2_r(0);
	}

	return 0xFF;
}

WRITE_HANDLER( canyon_led_w )
{
	osd_led_w((offset & 0x01), data & 0x01);
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x01ff, MRA_RAM }, /* WRAM */
	{ 0x0800, 0x0bff, MRA_RAM }, /* DISPLAY RAM */
	{ 0x1000, 0x17ff, canyon_switches_r }, /* SWITCHES */
	{ 0x1800, 0x1fff, canyon_options_r }, /* OPTIONS */
	{ 0x2000, 0x27ff, MRA_NOP }, /* PROM1 */
	{ 0x2800, 0x2fff, MRA_NOP }, /* PROM2 */
	{ 0x3000, 0x37ff, MRA_NOP }, /* PROM3 */
	{ 0x3800, 0x3fff, MRA_ROM }, /* PROM4 */
	{ 0xfff0, 0xffff, MRA_ROM }, /* PROM4 for 6502 vectors */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM }, /* WRAM */
//	{ 0x0680, 0x06ff, canyon_led_w },
	{ 0x0bd0, 0x0bdf, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x0800, 0x0bff, videoram_w, &videoram, &videoram_size }, /* DISPLAY */
	{ 0x2000, 0x27ff, MWA_NOP }, /* PROM1 */
	{ 0x2800, 0x2fff, MWA_NOP }, /* PROM2 */
	{ 0x3000, 0x37ff, MWA_NOP }, /* PROM3 */
	{ 0x3800, 0x3fff, MWA_ROM }, /* PROM4 */
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( canyon )
	PORT_START      /* DSW - fake port, gets mapped to Canyon Bomber ports */
	PORT_DIPNAME( 0x03, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "Spanish" )
	PORT_DIPSETTING(    0x02, "French" )
	PORT_DIPSETTING(    0x03, "German" )
	PORT_DIPNAME( 0x30, 0x00, "Misses Per Play" )
	PORT_DIPSETTING(    0x00, "Three" )
	PORT_DIPSETTING(    0x10, "Four" )
	PORT_DIPSETTING(    0x20, "Five" )
	PORT_DIPSETTING(    0x30, "Six" )
	PORT_DIPNAME( 0xC0, 0x80, "Game Cost" )
	PORT_DIPSETTING(    0x80, "1 coin/player" )
	PORT_DIPSETTING(    0xC0, "2 coins/player" )
	PORT_DIPSETTING(    0x40, "2 players/coin" )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START      /* IN1 - fake port, gets mapped */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT(0xFE, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 - fake port, gets mapped */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(0xFE, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN3 - fake port, gets mapped */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_SERVICE( 0x10, IP_ACTIVE_HIGH )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_TILT ) /* SLAM */

INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
    64,     /* 64 characters */
    1,      /* 1 bit per pixel */
    { 0 },  /* no separation in 1 bpp */
    { 4, 5, 6, 7, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout motionlayout =
{
	32,16,   /* 32*16 characters */
	4,       /* 4 characters? */
	1,       /* 1 bit per pixel */
	{ 0 },   /* no separation in 1 bpp */
	{ 0x100*8 + 7, 0x100*8 + 6, 0x100*8 + 5, 0x100*8 + 4, 7, 6, 5, 4,
	  0x100*8 + 15, 0x100*8 + 14, 0x100*8 + 13, 0x100*8 + 12, 15, 14, 13, 12,
	  0x100*8 + 256+7, 0x100*8 + 256+6, 0x100*8 + 256+5, 0x100*8 + 256+4, 256+7, 256+6, 256+5, 256+4,
	  0x100*8 + 256+15, 0x100*8 + 256+14, 0x100*8 + 256+13, 0x100*8 + 256+12, 256+15, 256+14, 256+13, 256+12 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8     /* every char takes 64 consecutive bytes */
};

static struct GfxLayout bomb_layout =
{
	2,2,	/* 2*2 bomb */
    1,      /* 1 character */
    1,      /* 1 bit per pixel */
    { 0 },  /* no separation in 1 bpp */
    { 4, 4 }, /* I know that this bit is 1 */
	{ 3*16, 3*16 },  /* I know that this bit is 1 */
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 2 },
	{ REGION_GFX2, 0, &motionlayout, 0, 2 },
	{ REGION_GFX1, 0, &bomb_layout,  0, 2 },
	{ -1 } /* end of array */
};


static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
	0x80,0x80,0x80, /* LT GREY */
	0xff,0xff,0xff, /* WHITE */
};
static unsigned short colortable[] =
{
	0x01, 0x00,
	0x01, 0x02,
};
static void init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom)
{
	memcpy(game_palette,palette,sizeof(palette));
	memcpy(game_colortable,colortable,sizeof(colortable));
}


static struct MachineDriver machine_driver_canyon =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
            750000,        /* 0.3 Mhz ???? */
			readmem,writemem,0,0,
            nmi_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
    32*8, 30*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	sizeof(palette) / sizeof(palette[0]) / 3, sizeof(colortable) / sizeof(colortable[0]),
	init_palette,

    VIDEO_TYPE_RASTER,
	0,
    generic_vh_start,
    generic_vh_stop,
    canyon_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0

};


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( canyon )
	ROM_REGION( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "9496-01.d1", 0x3800, 0x0800, 0x8be15080 )
	ROM_RELOAD(             0xF800, 0x0800 )

	ROM_REGION( 0x400, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9492-01.n8", 0x0000, 0x0400, 0x7449f754 )

	ROM_REGION( 0x200, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9505-01.n5", 0x0000, 0x0100, 0x60507c07 )
	ROM_LOAD( "9506-01.m5", 0x0100, 0x0100, 0x0d63396a )
ROM_END


ROM_START( canbprot )
	ROM_REGION( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD_NIB_LOW ( "cbp3000l.j1", 0x3000, 0x0800, 0x49cf29a0 )
	ROM_LOAD_NIB_HIGH( "cbp3000m.p1", 0x3000, 0x0800, 0xb4385c23 )
	ROM_LOAD_NIB_LOW ( "cbp3800l.h1", 0x3800, 0x0800, 0xc7ee4431 )
	ROM_RELOAD_NIB_LOW (              0xf800, 0x0800 ) /* for 6502 vectors */
	ROM_LOAD_NIB_HIGH( "cbp3800m.r1", 0x3800, 0x0800, 0x94246a9a )
	ROM_RELOAD_NIB_HIGH (             0xf800, 0x0800 ) /* for 6502 vectors */

	ROM_REGION( 0x400, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9492-01.n8", 0x0000, 0x0400, 0x7449f754 )

	ROM_REGION( 0x200, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9505-01.n5", 0x0000, 0x0100, 0x60507c07 )
	ROM_LOAD( "9506-01.m5", 0x0100, 0x0100, 0x0d63396a )
ROM_END



GAMEX( 1977, canyon,   0,      canyon, canyon, 0, ROT0, "Atari", "Canyon Bomber", GAME_NO_SOUND )
GAMEX( 1977, canbprot, canyon, canyon, canyon, 0, ROT0, "Atari", "Canyon Bomber (prototype)", GAME_NO_SOUND )
