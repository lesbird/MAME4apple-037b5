#ifndef _OSD_DBG_H
#define _OSD_DBG_H

#ifdef MAME_DEBUG

#include <dos.h>
#include <conio.h>
#include <time.h>

#define ARGFMT  __attribute__((format(printf,2,3)))

#ifndef DECL_SPEC
#define DECL_SPEC
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifndef INVALID
#define INVALID 0xffffffff
#endif

#ifndef WIN_EMPTY
#define WIN_EMPTY   '°'
#endif
#ifndef CAPTION_L
#define CAPTION_L   '®'
#endif
#ifndef CAPTION_R
#define CAPTION_R   '¯'
#endif
#ifndef FRAME_TL
#define FRAME_TL    'Ú'
#endif
#ifndef FRAME_BL
#define FRAME_BL    'À'
#endif
#ifndef FRAME_TR
#define FRAME_TR    '¿'
#endif
#ifndef FRAME_BR
#define FRAME_BR    'Ù'
#endif
#ifndef FRAME_V
#define FRAME_V     '³'
#endif
#ifndef FRAME_H
#define FRAME_H     'Ä'
#endif

/***************************************************************************
 *
 * These functions have to be provided by the OS specific code
 *
 ***************************************************************************/

static void osd_screen_update(void);
static void osd_put_screen_char (int ch, int attr, int x, int y);
static void osd_set_screen_curpos (int x, int y);

/***************************************************************************
 * Note: I renamed the set_gfx_mode function to avoid a name clash with
 * DOS' allegro.h. The new function should set any mode that is available
 * on the platform and the get_screen_size function should return the
 * resolution that is actually available.
 * The minimum required size is 80x25 characters, anything higher is ok.
 ***************************************************************************/
static void osd_set_screen_size (unsigned width, unsigned height);
static void osd_get_screen_size (unsigned *width, unsigned *height);

#include <dos.h>
#include <conio.h>

/*
 * Since this file is only to be included from mamedbg.c,
 * I put the following small function bodies in here.
 */

static void osd_screen_update(void)
{
	/* nothing to do */
}

static void osd_put_screen_char(int ch, int attr, int x, int y)
{
	ScreenPutChar(ch,attr,x,y);
}

static void osd_set_screen_curpos(int x, int y)
{
	ScreenSetCursor(y,x);
}

/* DJGPPs conio.h text_info structure */
static struct text_info textinfo;

static void osd_set_screen_size( unsigned width, unsigned height )
{
    union REGS r;

    gppconio_init();
    _set_screen_lines( height );
    r.x.ax = 0x1003;   /* set intensity/blinking */
    r.x.bx = 0;        /* intensity mode */
    int86( 0x10, &r, &r );
    gettextinfo( &textinfo );
}

static void osd_get_screen_size( unsigned *width, unsigned *height )
{
    *width = textinfo.screenwidth;
    *height = textinfo.screenheight;
}

#endif  /* MAME_DEBUG */

#endif	/* _OSD_DBG_H */

