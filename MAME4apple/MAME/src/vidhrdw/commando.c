/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *commando_bgvideoram,*commando_bgcolorram;
size_t commando_bgvideoram_size;
unsigned char *commando_scrollx,*commando_scrolly;
static unsigned char *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Commando has three 256x4 palette PROMs (one per gun), connected to the
  RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void commando_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		palette[3*i] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i+Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i+Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i+Machine->drv->total_colors] >> 3) & 0x01;
		palette[3*i + 1] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i+2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i+2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i+2*Machine->drv->total_colors] >> 3) & 0x01;
		palette[3*i + 2] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int commando_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(commando_bgvideoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,commando_bgvideoram_size);

	/* the background area is twice as tall and twice as large as the screen */
	if ((tmpbitmap2 = bitmap_alloc(2*Machine->drv->screen_width,2*Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void commando_vh_stop(void)
{
	bitmap_free(tmpbitmap2);
	free(dirtybuffer2);
	generic_vh_stop();
}



WRITE_HANDLER( commando_bgvideoram_w )
{
	if (commando_bgvideoram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		commando_bgvideoram[offset] = data;
	}
}



WRITE_HANDLER( commando_bgcolorram_w )
{
	if (commando_bgcolorram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		commando_bgcolorram[offset] = data;
	}
}



WRITE_HANDLER( commando_spriteram_w )
{
	if (data != spriteram[offset] && offset % 4 == 2)
		logerror("%04x: sprite %d X offset (old = %d new = %d) scanline %d\n",
				cpu_get_pc(),offset/4,spriteram[offset],data,255 - (cpu_getfcount() * 256 / cpu_getfperiod()));
	if (data != spriteram[offset] && offset % 4 == 3)
		logerror("%04x: sprite %d Y offset (old = %d new = %d) scanline %d\n",
				cpu_get_pc(),offset/4,spriteram[offset],data,255 - (cpu_getfcount() * 256 / cpu_getfperiod()));
	if (data != spriteram[offset] && offset % 4 == 0)
		logerror("%04x: sprite %d code (old = %d new = %d) scanline %d\n",
				cpu_get_pc(),offset/4,spriteram[offset],data,255 - (cpu_getfcount() * 256 / cpu_getfperiod()));

	spriteram[offset] = data;
}



WRITE_HANDLER( commando_c804_w )
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);

	/* bit 4 resets the sound CPU - we ignore it */

	/* bit 7 flips screen */
	flip_screen_w(offset, ~data & 0x80);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void commando_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	if (full_refresh)
	{
		memset(dirtybuffer2,1,commando_bgvideoram_size);
	}


	for (offs = commando_bgvideoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer2[offs])
		{
			int sx,sy,flipx,flipy;


			dirtybuffer2[offs] = 0;

			sx = offs / 32;
			sy = offs % 32;
			flipx = commando_bgcolorram[offs] & 0x10;
			flipy = commando_bgcolorram[offs] & 0x20;
			if (flip_screen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap2,Machine->gfx[1],
					commando_bgvideoram[offs] + 4*(commando_bgcolorram[offs] & 0xc0),
					commando_bgcolorram[offs] & 0x0f,
					flipx,flipy,
					16 * sx,16 * sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background graphics */
	{
		int scrollx,scrolly;


		scrollx = -(commando_scrolly[0] + 256 * commando_scrolly[1]);
		scrolly = -(commando_scrollx[0] + 256 * commando_scrollx[1]);
		if (flip_screen)
		{
			scrollx = 256 - scrollx;
			scrolly = 256 - scrolly;
		}

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,flipx,flipy,bank;


		/* bit 1 of [offs+1] is not used */

		sx = spriteram[offs + 3] - 0x100 * (spriteram[offs + 1] & 0x01);
		sy = spriteram[offs + 2];
		flipx = spriteram[offs + 1] & 0x04;
		flipy = spriteram[offs + 1] & 0x08;
		bank = (spriteram[offs + 1] & 0xc0) >> 6;

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		if (bank < 3)
			drawgfx(bitmap,Machine->gfx[2],
					spriteram[offs] + 256 * bank,
					(spriteram[offs + 1] & 0x30) >> 4,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy,flipx,flipy;


		sx = offs % 32;
		sy = offs / 32;
		flipx = colorram[offs] & 0x10;
		flipy = colorram[offs] & 0x20;

		if (flip_screen)
		{
			sx = 31 - sx;
			sy = 31 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs] + 4 * (colorram[offs] & 0xc0),
				colorram[offs] & 0x0f,
				flipx,flipy,
				8*sx,8*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,3);
	}
}
