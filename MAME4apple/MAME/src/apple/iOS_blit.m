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
extern UINT32 gameScreenWidth;
extern UINT32 gameScreenHeight;

extern void objc_flip();

// funtion to blit the mame screen to a mutable texture buffer which is a 32 bit osd_bitmap
void ios_blit(struct osd_bitmap *bitmap)
{
    struct osd_bitmap *dstbitmap = screen->bitmap;
    
    long depthBytes = (bitmap->depth == 8) ? 1 : (bitmap->depth == 32) ? 4 : 2;
    long xoffset = (dstbitmap->width / 2) - (gameScreenWidth / 2);
    long yoffset = (dstbitmap->height / 2) - (gameScreenHeight / 2);
    
    if ((Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) == 0)
    {
        xoffset -= Machine->drv->default_visible_area.min_x;
        yoffset -= Machine->drv->default_visible_area.min_y;
    }
    
    //NSLog(@"xoffset=%ld yoffset=%ld", xoffset, yoffset);
    
    long srcw = bitmap->width * depthBytes; // bitmap->line[1] - bitmap->line[0];
    long srch = bitmap->height;
    for (int i = 0; i < srch; i++)
    {
        UINT8 *d = dstbitmap->line[i + yoffset] + (xoffset * 4);
        UINT8 *s = bitmap->line[i];
        UINT16 *s16 = (UINT16 *)bitmap->line[i];
        UINT32 *s32 = (UINT32 *)bitmap->line[i];
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
                UINT8 a, r, g, b;
                UINT16 c = s16[j];

                a = 0xFF;
                osd_get_pen(c, &r, &g, &b);
                
                d[n + 0] = r;
                d[n + 1] = g;
                d[n + 2] = b;
                d[n + 3] = a;
            }
            else
            {
                UINT32 a, r, g, b;
                UINT32 c = s32[j];
                a = (c & 0xFF000000) >> 24;
                r = (c & 0x00FF0000) >> 16;
                g = (c & 0x0000FF00) >> 8;
                b = (c & 0x000000FF);
                d[n + 0] = (UINT8)r;
                d[n + 1] = (UINT8)g;
                d[n + 2] = (UINT8)b;
                d[n + 3] = (UINT8)a;
            }
            n += 4;
        }
    }
    objc_flip();
}
