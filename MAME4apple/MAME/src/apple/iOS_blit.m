//
//  iOS_blit.m
//  MAME4apple
//
//  Created by Les Bird on 10/1/16.
//

#import <Foundation/Foundation.h>
#include "driver.h"
#include "vidhrdw/vector.h"

extern bitmap_t *screen;

// funtion to blit the mame screen to a mutable texture buffer which is a 32 bit osd_bitmap
void ios_blit(struct osd_bitmap *bitmap)
{
    struct osd_bitmap *dstbitmap = screen->bitmap;
    
    long xoffset = (dstbitmap->width / 2) - (bitmap->width / 2);
    long yoffset = (dstbitmap->height / 2) - (bitmap->height / 2);
    
    long srcw = bitmap->line[1] - bitmap->line[0];
    long srch = bitmap->height;
    for (int i = 0; i < srch; i++)
    {
        UINT8 *d = dstbitmap->line[i + yoffset] + (xoffset * 4);
        UINT8 *s = bitmap->line[i];
        UINT32 n = 0;
        for (int j = 0; j < srcw; j++)
        {
            if (bitmap->depth == 8)
            {
                UINT8 r, g, b;
                osd_get_pen(s[j], &r, &g, &b);
                d[n + 0] = r;
                d[n + 1] = g;
                d[n + 2] = b;
                d[n + 3] = 0xFF;
            }
            else if (bitmap->depth == 15 || bitmap->depth == 16)
            {
            }
            else
            {
            }
            n += 4;
        }
    }
}

// bitmap size = bitmap->width + (2 * 16) by bitmap->height + (2 * 16)
// bitmap->line[1] - bitmap->line[0] = bitmap->width + (2 * 16)
#if 0 // current
void ios_blit(struct osd_bitmap *bitmap)
{
    struct osd_bitmap *dstbitmap = screen->bitmap;
    UINT8 *src = bitmap->_private;
    long srcstep = (bitmap->line[1] - bitmap->line[0]); // includes safety border
    UINT32 dstbyte = 4;
    UINT8 *dst = (UINT8 *)dstbitmap->_private;
    long dststep = dstbitmap->width * dstbyte;
    long xoffset = (dstbitmap->width / 2) - (srcstep / 2);
    long yoffset = 0; //(dstbitmap->height / 2) - (bitmap->height / 2);
    for (int j = 0; j < dstbitmap->height; j++)
    {
        UINT8 *d = dst;
        int k = 0;
        for (int i = 0; i < dstbitmap->width; i++)
        {
            int n = (i * dstbyte);
            if (i >= xoffset && k < srcstep && j >= yoffset && (j - yoffset) < bitmap->height + 8)
            {
                UINT8 r, g, b;
                osd_get_pen(src[k++], &r, &g, &b);
                d[n + 0] = r;
                d[n + 1] = g;
                d[n + 2] = b;
                d[n + 3] = 0xFF;
            }
            else
            {
                d[n + 0] = 0x00;
                d[n + 1] = 0x00;
                d[n + 2] = 0x00;
                d[n + 3] = 0xFF;
            }
        }
        if (k > 0)
        {
            src += srcstep;
        }
        dst += dststep;
    }
}
#endif

#if 0
void ios_blit(struct osd_bitmap *bitmap)
{
    struct osd_bitmap *dstbitmap = screen->bitmap;
    UINT8 *src = bitmap->_private;
    UINT8 *dst = dstbitmap->_private;
    UINT32 dstbyte = 4; //(dstbitmap->depth == 8) ? 1 : (dstbitmap->depth == 16) ? 2 : 4;
    unsigned long srcw = (bitmap->line[1] - bitmap->line[0]);
    unsigned long dstw = (dstbitmap->width * dstbyte);
    //unsigned long dstw2 = (dstbitmap->line[1] - dstbitmap->line[0]);
    unsigned long xoffset = 0; //(dstbitmap->width / 2) - (bitmap->width / 2);
    //unsigned long yoffset = (dstbitmap->height / 2) - (bitmap->height / 2);
    //dst += (yoffset * dstw); // + (xoffset * dstbyte);
    for (int j = 0; j < bitmap->height; j++)
    {
        UINT8 *d = dst + (xoffset * dstbyte);
        for (int i = 0; i < srcw; i++)
        {
            UINT8 r, g, b;
            osd_get_pen(src[i], &r, &g, &b);
            int n = (i * dstbyte);
            if (dstbyte == 4)
            {
                d[n + 0] = r;
                d[n + 1] = g;
                d[n + 2] = b;
                d[n + 3] = 0xFF;
            }
        }
        dst += dstw;
        src += srcw;
    }
}
#endif
