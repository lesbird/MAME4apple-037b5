//
//  iOS_blit.m
//  MAME4apple
//
//  Created by Les Bird on 10/1/16.
//  Rewritten: on iOS the emulated frame is blitted into the Metal renderer's
//  right-sized RGBA8 back buffer using an inlined palette LUT (no per-pixel
//  function call), then published for tear-free display. tvOS keeps the
//  original SpriteKit path.
//

#import <Foundation/Foundation.h>
#import <TargetConditionals.h>
#include "driver.h"
#include "vidhrdw/vector.h"
#include "MameShared.h"

extern bitmap_t *screen;
extern UINT32 gameScreenWidth;
extern UINT32 gameScreenHeight;

extern void objc_flip();

// current_palette lives in iOS_video.m
extern int screen_colors;
void osd_get_pen(int pen, unsigned char *red, unsigned char *green, unsigned char *blue);

#if !TARGET_OS_TV

// Palette lookup table (pen index -> packed RGBA8), rebuilt each frame for
// indexed (8/15/16-bit) games. Sized to screen_colors.
static UINT32 *s_pen_lut = NULL;
static int s_pen_lut_size = 0;

static void rebuild_pen_lut(void)
{
    if (screen_colors > s_pen_lut_size)
    {
        free(s_pen_lut);
        s_pen_lut = (UINT32 *)malloc(sizeof(UINT32) * screen_colors);
        s_pen_lut_size = screen_colors;
    }
    for (int i = 0; i < screen_colors; i++)
    {
        unsigned char r, g, b;
        osd_get_pen(i, &r, &g, &b);
        // packed little-endian => bytes r,g,b,a == MTLPixelFormatRGBA8Unorm
        s_pen_lut[i] = (UINT32)r | ((UINT32)g << 8) | ((UINT32)b << 16) | (0xFFu << 24);
    }
}

void ios_blit(struct osd_bitmap *bitmap)
{
    UINT32 *dst = mame_renderer_backbuffer();
    if (dst == NULL) return;

    const int texW = (int)gameScreenWidth;   // back-buffer row stride (pixels)

    int w = (Machine->visible_area.max_x - Machine->visible_area.min_x) + 1;
    w = (bitmap->width < w) ? bitmap->width : w;
    int h = bitmap->height;
    int x1 = Machine->visible_area.min_x;
    int y1 = 0;

    int aspectY = 1;
    if ((Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) != 0)
    {
        w = bitmap->width;
        h = bitmap->height;
        x1 = 0;
    }
    else if ((Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_1_2) != 0)
    {
        aspectY = 2;
    }

    // clamp to the configured back-buffer size
    if (w > texW) w = texW;
    if (h > (int)gameScreenHeight) h = (int)gameScreenHeight;

    static int last_aspectY = -1;
    if (aspectY != last_aspectY)
    {
        mame_renderer_set_pixel_aspect_y(aspectY);
        last_aspectY = aspectY;
    }

    const int depth = bitmap->depth;
    if (depth != 32)
        rebuild_pen_lut();

    const int x2 = x1 + w;
    const int y2 = y1 + h;

    for (int i = y1; i < y2; i++)
    {
        UINT32 *drow = dst + (size_t)(i - y1) * texW;
        int col = 0;
        if (depth == 8)
        {
            UINT8 *s = (UINT8 *)bitmap->line[i];
            for (int j = x1; j < x2; j++) drow[col++] = s_pen_lut[s[j]];
        }
        else if (depth == 15 || depth == 16)
        {
            UINT16 *s16 = (UINT16 *)bitmap->line[i];
            for (int j = x1; j < x2; j++) drow[col++] = s_pen_lut[s16[j]];
        }
        else // 32-bit direct ARGB -> RGBA
        {
            UINT32 *s32 = (UINT32 *)bitmap->line[i];
            for (int j = x1; j < x2; j++)
            {
                UINT32 c = s32[j];
                UINT32 a = (c >> 24) & 0xFF;
                UINT32 r = (c >> 16) & 0xFF;
                UINT32 g = (c >> 8) & 0xFF;
                UINT32 b = (c) & 0xFF;
                drow[col++] = r | (g << 8) | (b << 16) | (a << 24);
            }
        }
    }

    mame_renderer_publish(w, h);
    objc_flip();
}

#else  // TARGET_OS_TV : original SpriteKit blit into screen->bitmap

// funtion to blit the mame screen to a mutable texture buffer which is a 32 bit osd_bitmap
void ios_blit(struct osd_bitmap *bitmap)
{
    struct osd_bitmap *dstbitmap = screen->bitmap;

    int w = (Machine->visible_area.max_x - Machine->visible_area.min_x) + 1;
    w = (bitmap->width < w) ? bitmap->width : w;
    int h = bitmap->height;
    int x1 = Machine->visible_area.min_x;
    int x2 = x1 + w;
    int y1 = 0;
    int y2 = y1 + h;
    int yadj = Machine->visible_area.min_y / 2;
    if ((Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) != 0)
    {
        w = bitmap->width;
        h = bitmap->height;
        x1 = 0;
        x2 = x1 + w;
        y1 = 0;
        y2 = y1 + h;
        yadj = 0;
    }
    long xoffset = (dstbitmap->width / 2) - (w / 2);
    long yoffset = (dstbitmap->height / 2) - (h / 2) - yadj;

    for (int i = y1; i < y2; i++)
    {
        UINT8 *d = dstbitmap->line[i + yoffset] + (xoffset * 4);
        UINT8 *s = bitmap->line[i];
        UINT16 *s16 = (UINT16 *)bitmap->line[i];
        UINT32 *s32 = (UINT32 *)bitmap->line[i];
        UINT32 n = 0;
        for (int j = x1; j < x2; j++)
        {
            if (bitmap->depth == 8)
            {
                UINT8 r, g, b;
                osd_get_pen(s[j], &r, &g, &b);
                d[n + 0] = r; d[n + 1] = g; d[n + 2] = b; d[n + 3] = 0xFF;
            }
            else if (bitmap->depth == 15 || bitmap->depth == 16)
            {
                UINT8 a, r, g, b;
                UINT16 c = s16[j];
                a = 0xFF;
                osd_get_pen(c, &r, &g, &b);
                d[n + 0] = r; d[n + 1] = g; d[n + 2] = b; d[n + 3] = a;
            }
            else
            {
                UINT32 a, r, g, b;
                UINT32 c = s32[j];
                a = (c & 0xFF000000) >> 24;
                r = (c & 0x00FF0000) >> 16;
                g = (c & 0x0000FF00) >> 8;
                b = (c & 0x000000FF);
                d[n + 0] = (UINT8)r; d[n + 1] = (UINT8)g; d[n + 2] = (UINT8)b; d[n + 3] = (UINT8)a;
            }
            n += 4;
        }
    }
    objc_flip();
}

#endif
