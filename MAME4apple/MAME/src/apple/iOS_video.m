//
//  iOS_video.m
//  MAME4apple
//
//  Created by Les Bird on 9/20/16.
//

#import <Foundation/Foundation.h>
#include "driver.h"
#include "dirty.h"
#include <mach/mach_time.h>

UINT32 gfx_depth;
UINT32 gfx_fps;

static UINT8 *current_palette;

typedef UINT32 TICKER;

extern void apple_init_input();

#define TICKS_PER_SEC 1000

// ticks since up time in milliseconds
UINT32 ticker()
{
    static mach_timebase_info_data_t s_timebase_info;
    
    if (s_timebase_info.denom == 0)
    {
        (void) mach_timebase_info(&s_timebase_info);
    }
    
    uint64_t t = mach_absolute_time();
    uint64_t nano = t * s_timebase_info.numer / s_timebase_info.denom;
    uint64_t milli = nano / 1000000;
    return (UINT32)milli;
}

int osd_init()
{
    apple_init_input();
    return 0;
}

void osd_exit()
{
}

/* Create a bitmap. */
/* VERY IMPORTANT: the function must allocate also a "safety area" 16 pixels wide all */
/* around the bitmap. This is required because, for performance reasons, some graphic */
/* routines don't clip at boundaries of the bitmap. */

const int safety = 16;

// from MSDOS video.c
struct osd_bitmap *osd_alloc_bitmap(int width,int height,int depth)
{
    struct osd_bitmap *bitmap;
    
    if (depth != 8 && depth != 15 && depth != 16 && depth != 32)
    {
        logerror("osd_alloc_bitmap() unknown depth %d\n",depth);
        return NULL;
    }
    
    if ((bitmap = malloc(sizeof(struct osd_bitmap))) != 0)
    {
        int i,rowlen,rdwidth;
        unsigned char *bm;
        
        
        bitmap->depth = depth;
        bitmap->width = width;
        bitmap->height = height;
        
        rdwidth = (width + 7) & ~7;     /* round width to a quadword */
        rowlen = (rdwidth + 2 * safety) * sizeof(unsigned char);
        if (depth == 32)
            rowlen *= 4;
        else if (depth == 15 || depth == 16)
            rowlen *= 2;
        
        if ((bm = malloc((height + 2 * safety) * rowlen)) == 0)
        {
            free(bitmap);
            return 0;
        }
        
        memset(bm,0,(height + 2 * safety) * rowlen);
        
        if ((bitmap->line = malloc((height + 2 * safety) * sizeof(unsigned char *))) == 0)
        {
            free(bm);
            free(bitmap);
            return 0;
        }
        
        for (i = 0;i < height + 2 * safety;i++)
        {
            if (depth == 32)
                bitmap->line[i] = &bm[i * rowlen + 4*safety];
            else if (depth == 15 || depth == 16)
                bitmap->line[i] = &bm[i * rowlen + 2*safety];
            else
                bitmap->line[i] = &bm[i * rowlen + safety];
        }
        bitmap->line += safety;
        
        bitmap->_private = bm;
    }
    
    return bitmap;
}

void osd_free_bitmap(struct osd_bitmap *bitmap)
{
    if (bitmap)
    {
        bitmap->line -= safety;
        free(bitmap->line);
        free(bitmap->_private);
        free(bitmap);
    }
}

extern void objc_alloc_framebuffer(int width, int height, int depth, int attributes, int orientation);

int osd_set_display(int width,int height,int depth,int attributes,int orientation)
{
    return 0;
}

int osd_create_display(int width,int height,int depth,int fps,int attributes,int orientation)
{
    NSLog(@"osd_create_display(%d, %d, %d, %d)", width, height, depth, fps);
    
    gfx_fps = fps;
    gfx_depth = depth;
    objc_alloc_framebuffer(width, height, depth, attributes, orientation);
    return 0;
}

void osd_close_display(void)
{
    free(current_palette);
    current_palette = NULL;
}

void osd_set_visible_area(int min_x,int max_x,int min_y,int max_y)
{
    set_ui_visarea(min_x, min_y, max_x, max_y);
}

UINT32 makecol(int r, int g, int b)
{
    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

UINT8 getr(UINT32 p)
{
    return (UINT8)(p >> 24) & 0xFF;
}

UINT8 getg(UINT32 p)
{
    return (UINT8)(p >> 16) & 0xFF;
}

UINT8 getb(UINT32 p)
{
    return (UINT8)(p >> 8) & 0xFF;
}

int modifiable_palette;
int screen_colors;
int brightness = 100;
float osd_gamma_correction = 1.0f;

// from MSDOS video.c
int osd_allocate_colors(unsigned int totalcolors, const UINT8 *palette, UINT16 *pens, int modifiable)
{
    modifiable_palette = modifiable;
    screen_colors = totalcolors;
    if (gfx_depth != 8)
    {
        screen_colors += 2;
    }
    else {
        screen_colors = 256;
    }
    
    current_palette = malloc(3 * screen_colors * sizeof(unsigned char));
    
    for (int i = 0; i < screen_colors; i++)
    {
        current_palette[3 * i + 0] = current_palette[3 * i + 1] = current_palette[3 * i + 2] = 0;
    }
    
    if (gfx_depth != 8 && modifiable == 0)
    {
        int r,g,b;
        
        for (int i = 0; i < totalcolors; i++)
        {
            r = 255 * brightness * pow(palette[3*i+0] / 255.0, 1 / osd_gamma_correction) / 100;
            g = 255 * brightness * pow(palette[3*i+1] / 255.0, 1 / osd_gamma_correction) / 100;
            b = 255 * brightness * pow(palette[3*i+2] / 255.0, 1 / osd_gamma_correction) / 100;
            *pens++ = makecol(r,g,b);
        }
        
        Machine->uifont->colortable[0] = makecol(0x00,0x00,0x00);
        Machine->uifont->colortable[1] = makecol(0xff,0xff,0xff);
        Machine->uifont->colortable[2] = makecol(0xff,0xff,0xff);
        Machine->uifont->colortable[3] = makecol(0x00,0x00,0x00);
    }
    else
    {
        if (gfx_depth == 8 && totalcolors >= 255)
        {
            int bestblack,bestwhite;
            int bestblackscore,bestwhitescore;
            
            bestblack = bestwhite = 0;
            bestblackscore = 3 * 255 * 255;
            bestwhitescore = 0;
            for (int i = 0; i < totalcolors; i++)
            {
                int r,g,b,score;
                
                r = palette[3 * i + 0];
                g = palette[3 * i + 1];
                b = palette[3 * i + 2];
                score = r*r + g*g + b*b;
                
                if (score < bestblackscore)
                {
                    bestblack = i;
                    bestblackscore = score;
                }
                if (score > bestwhitescore)
                {
                    bestwhite = i;
                    bestwhitescore = score;
                }
            }
            
            for (int i = 0; i < totalcolors; i++)
            {
                pens[i] = i;
            }
            
            /* map black to pen 0, otherwise the screen border will not be black */
            pens[bestblack] = 0;
            pens[0] = bestblack;
            
            Machine->uifont->colortable[0] = pens[bestblack];
            Machine->uifont->colortable[1] = pens[bestwhite];
            Machine->uifont->colortable[2] = pens[bestwhite];
            Machine->uifont->colortable[3] = pens[bestblack];
        }
        else
        {
            /* reserve color 1 for the user interface text */
            current_palette[3 * 1 + 0] = current_palette[3 * 1 + 1] = current_palette[3 * 1 + 2] = 0xff;
            Machine->uifont->colortable[0] = 0;
            Machine->uifont->colortable[1] = 1;
            Machine->uifont->colortable[2] = 1;
            Machine->uifont->colortable[3] = 0;
            
            /* fill the palette starting from the end, so we mess up badly written */
            /* drivers which don't go through Machine->pens[] */
            for (int i = 0; i < totalcolors; i++)
            {
                pens[i] = (screen_colors - 1) - i;
            }
        }
        
        for (int i = 0; i < totalcolors; i++)
        {
            UINT32 p = pens[i];
            UINT8 r = palette[3 * i + 0];
            UINT8 g = palette[3 * i + 1];
            UINT8 b = palette[3 * i + 2];
            current_palette[3 * p + 0] = r;
            current_palette[3 * p + 1] = g;
            current_palette[3 * p + 2] = b;
            //NSLog(@"palette[%d] current_palette[%d] r=%02X g=%02X b=%02X", i, p, r, g, b);
        }
    }

    return 0;
}

void osd_modify_pen(int pen,unsigned char red, unsigned char green, unsigned char blue)
{
    //NSLog(@"osd_modify_pen(%d)", pen);
    if (current_palette[3 * pen + 0] != red || current_palette[3 * pen + 1] != green || current_palette[3 * pen + 2] != blue)
    {
        //NSLog(@"osd_modify_pen(%d, red=%02X, green=%02X, blue=%02X", pen, red, green, blue);
        current_palette[3 * pen + 0] = red;
        current_palette[3 * pen + 1] = green;
        current_palette[3 * pen + 2] = blue;
    }
}

void osd_get_pen(int pen,unsigned char *red, unsigned char *green, unsigned char *blue)
{
    *red =	 current_palette[3 * pen + 0];
    *green = current_palette[3 * pen + 1];
    *blue =  current_palette[3 * pen + 2];
}

int osd_skip_this_frame(void)
{
    return 0;
}

extern int game_bitmap_update;
extern void ios_blit(struct osd_bitmap *bitmap);
TICKER curr;

void vsync()
{
    //while (game_bitmap_update != 0)
    //{
    //}
}

void osd_update_video_and_audio(struct osd_bitmap *game_bitmap)
{
    static TICKER prev;
    
    struct osd_bitmap *bitmap = game_bitmap;

    curr = ticker();
    TICKER target = prev + (TICKS_PER_SEC / gfx_fps);
    
    do
    {
        curr = ticker();
    } while (curr < target);
    
    prev = curr;
    
    profiler_mark(PROFILER_BLIT);

    ios_blit(bitmap);
    
    profiler_mark(PROFILER_END);
    
    game_bitmap_update = 1;
}

dirtygrid grid1;
char *dirty_new=grid1;

void osd_mark_dirty(int _x1,int _y1,int _x2,int _y2, int o)
{
    //NSLog(@"osd_mark_dirty(%d, %d, %d, %d, %d)", _x1, _y1, _x2, _y2, o);
}

void osd_clearbitmap(struct osd_bitmap *bitmap)
{
    //*
    NSLog(@"osd_clearbitmap()");

    UINT32 bytes_per_pixel = 0;
    if (bitmap->depth == 8)
    {
        bytes_per_pixel = 1;
    }
    else if (bitmap->depth == 15 || bitmap->depth == 16)
    {
        bytes_per_pixel = 2;
    }
    else
    {
        bytes_per_pixel = 4;
    }
    UINT8 *ptr = bitmap->_private;
    UINT16 line_width = bitmap->line[1] - bitmap->line[0];
    for (int j = 0; j < bitmap->height; j++)
    {
        memset(ptr, 0, line_width * bytes_per_pixel);
        ptr += line_width;
    }
    extern int bitmap_dirty;        /* in mame.c */
    bitmap_dirty = 1;
    //*/
#if 0
    // from MSDOS video.c
    int i;
    
    for (i = 0; i < bitmap->height; i++)
    {
        if (bitmap->depth == 16)
        {
            memset(bitmap->line[i], 0, 2 * bitmap->width);
        }
        else
        {
            memset(bitmap->line[i], 0, bitmap->width);
        }
    }
    
    if (bitmap == Machine->scrbitmap)
    {
        extern int bitmap_dirty;        /* in mame.c */
        
        osd_mark_dirty (0,0,bitmap->width-1,bitmap->height-1,1);
        bitmap_dirty = 1;
    }
#endif
}

void osd_set_gamma(float _gamma)
{
}

float osd_get_gamma(void)
{
    return 1;
}

/* brightess = percentage 0-100% */
void osd_set_brightness(int _brightness)
{
}

int osd_get_brightness(void)
{
    return 100;
}

void osd_pause(int paused)
{
}

void osd_save_snapshot(struct osd_bitmap *bitmap)
{
}

// MAME could potentially spam the log resulting in massive delays and slow frame rate
void CLIB_DECL logerror(const char *text,...)
{
    /*
    va_list arg;
    va_start(arg,text);
    NSLogv([NSString stringWithUTF8String:text], arg);
    va_end(arg);
     */
}
