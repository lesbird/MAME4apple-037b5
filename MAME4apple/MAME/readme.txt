
                 M.A.M.E.  -  Multiple Arcade Machine Emulator
          Copyright (C) 1997-2000 by Nicola Salmoria and The MAME Team

Many people have helped with this project--directly, or by releasing the source
code for the drivers they have written. We are not trying to take credit that
isn't ours. See the Acknowledgments section for a list of contributors. Please
note, however, that the list is largely incomplete. Also see the comments in
the source code to see the people who contributed to specific drivers. That
list, too, may be incomplete. We apologize for any omission.

All trademarks cited in this document are property of their respective owners.


Usage and Distribution License
------------------------------

I. Purpose
----------
   MAME is strictly a non-profit project. Its main purpose is to be a reference
   to the inner workings of the emulated arcade machines. This is done for
   educational purposes and to prevent many historical games from sinking into
   oblivion once the hardware they run on stops working. Of course to preserve
   the games, you must also be able to actually play them; you can consider
   that a nice side effect.
   It is not our intention to infringe on any copyrights or patents on the
   original games. All of MAME's source code is either our own or freely
   available. To operate, the emulator requires images of the original ROMs
   from the arcade machines, which must be provided by the user. No portion of
   the original ROM codes are included in the executable.

II. Cost
--------
   MAME is free. Its source code is free. Selling either is not allowed.

III. ROM Images
---------------
   ROM images are copyrighted material. Most of them cannot be distributed
   freely. Distribution of MAME on the same physical medium as illegal copies
   of ROM images is strictly forbidden.
   You are not allowed to distribute MAME in any form if you sell, advertise,
   or publicize illegal CD-ROMs or other media containing ROM images. This
   restriction applies even if you don't make money, directly or indirectly,
   from those activities. You are allowed to make ROMs and MAME available for
   download on the same website, but only if you warn users about the ROMs's
   copyright status, and make it clear that users must not download ROMs unless
   they are legally entitled to do so.

IV. Source Code Distribution
----------------------------
   If you distribute the binary (compiled) version of MAME, you should also
   distribute the source code. If you can't do that, you must provide a link
   to a site where the source can be obtained.

V. Distribution Integrity
-------------------------
   This chapter applies to the official MAME distribution. See below for
   limitations on the distribution of derivative works.
   MAME must be distributed only in the original archives. You are not allowed
   to distribute a modified version, nor to remove and/or add files to the
   archive.

VI. Reuse of Source Code
--------------------------
   This chapter might not apply to specific portions of MAME (e.g. CPU
   emulators) which bear different copyright notices.
   The source code cannot be used in a commercial product without the written
   authorization of the authors. Use in non-commercial products is allowed, and
   indeed encouraged.  If you use portions of the MAME source code in your
   program, however, you must make the full source code freely available as
   well.
   Usage of the _information_ contained in the source code is free for any use.
   However, given the amount of time and energy it took to collect this
   information, if you find new information we would appreciate if you made it
   freely available as well.

VII. Derivative Works
---------------------
   Derivative works are allowed, provided their source code is freely
   available. However, these works are discouraged. MAME is a continuously-
   -evolving project. It is in your best interests to submit your contributions
   to the MAME development team, so they may be integrated into the main
   distribution.
   There are some specific modifications to the source code which go against
   the spirit of the project. They are NOT considered a derivative work, and
   distribution of executables containing them is strictly forbidden. Such
   modifications include, but are not limited to:
   - enabling games that are disabled
   - changing the ROM verification commands so that they report missing games
   - removing the startup information screens
   If you make a derivative work, you are not allowed to call it MAME. You must
   use a different name to make clear that it is a MAME derivative, not an
   official distribution from the MAME team. Simply calling it MAME followed or
   preceded by a punctuation mark (e.g. MAME+) is not sufficient. The name must
   be clearly distinct (e.g. REMAME). The version number must also match the
   number of the official MAME version from which you derived your version.


How to Contact Us
-----------------

The official MAME homepage is http://www.mame.net/  You can always find the
latest release there, including beta versions and information on things being
worked on. Also, a totally legal and free ROM set of Robby Roto is available
on the same page.

If you have bugs to report, check the MAME Testing Project at
http://www.mametesters.com

Here are some of the people contributing to MAME. If you have comments,
suggestions, or bug reports about an existing driver, check the driver's
source code to find who has worked on it, and send comments to that person.
If you are not sure who to contact, write to Nicola. If you have comments
specific to a system other than DOS (e.g. Mac, Win32, Unix), they should be
sent to the respective port maintainer (check the documentation to know who he
is). DON'T SEND THEM TO NICOLA - they will be ignored.

Nicola Salmoria    MC6489@mclink.it

Mike Balfour       mab22@po.cwru.edu
Aaron Giles        agiles@sirius.com
Chris Moore        chris.moore@writeme.com
Brad Oliver        bradman@pobox.com
Andrew Scott       ascott@utkux.utcc.utk.edu
Zsolt Vasvari      vaszs01@banet.net

DON'T SEND BINARY ATTACHMENTS WITHOUT ASKING FIRST, *ESPECIALLY* ROM IMAGES.

THESE ARE NOT SUPPORT ADDRESSES. Support questions sent to these addresses
*will* be ignored. Please understand that this is a *free* project, mostly
targeted at experienced users. We don't have the resources to provide end user
support. Basically, if you can't get the emulator to work, you are on your own.
First of all, read the docs carefully. If you still can't find an answer to
your question, try checking the beginner's sections that many emulation pages
have, or ask on the appropriate Usenet newsgroups (e.g. comp.emulators.misc) or
on the official MAME message board at http://www.mame.net/msg/

For help in compiling MAME, check these pages:
http://www.mame.net/compile.html
http://www.mameworld.net

Also, DO NOT SEND REQUESTS FOR NEW GAMES TO ADD, unless you have some original
info on the game hardware or, even better, own the board and have the technical
expertise needed to help us.
Please don't send us information widely available on the Internet - we are
perfectly capable of finding it ourselves, thank you.



Acknowledgments
---------------

First of all, thanks to Allard van der Bas (avdbas@wi.leidenuniv.nl) for
starting the Arcade Emulation Programming Repository at
http://valhalla.ph.tn.tudelft.nl/emul8
Without the Repository, I would never have even tried to write an emulator.
Unfortunately, the original Repository is now closed, but its spirit lives
on in MAME.

Z80 emulator Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
M6502 emulator Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
Hu6280 Copyright (c) 1999 Bryan McPhail, mish@tendril.force9.net
I86 emulator by David Hedley, modified by Fabrice Frances (frances@ensica.fr)
M6809 emulator by John Butler, based on L.C. Benschop's 6809 Simulator V09.
M6808 based on L.C. Benschop's 6809 Simulator V09.
M68000 emulator Copyright 1999 Karl Stenerud.  All rights reserved.
80x86 M68000 emulator Copyright 1998, Mike Coates, Darren Olafson.
8039 emulator by Mirko Buffoni, based on 8048 emulator by Dan Boris.
T-11 emulator Copyright (C) Aaron Giles 1998
TMS34010 emulator by Alex Pasadyn and Zsolt Vasvari.
TMS9900 emulator by Andy Jones, based on original code by Ton Brouwer.
Cinematronics CPU emulator by Jeff Mitchell, Zonn Moore, Neil Bradley.
Atari AVG/DVG emulation based on VECSIM by Hedley Rainnie, Eric Smith and
Al Kossow.

TMS5220 emulator by Frank Palazzolo.
AY-3-8910 emulation based on various code snippets by Ville Hallik,
  Michael Cuddy, Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.
YM-2203, YM-2151, YM3812 emulation by Tatsuyuki Satoh.
POKEY emulator by Ron Fries (rfries@aol.com).
Many thanks to Eric Smith, Hedley Rainnie and Sean Trowbridge for information
   on the Pokey random number generator.
NES sound hardware info by Jeremy Chadwick and Hedley Rainne.
YM2610 emulation by Hiromitsu Shioya.

Background art by Peter Hirschberg (PeterH@cronuscom.com).

Allegro library by Shawn Hargreaves, 1994/97
SEAL Synthetic Audio Library API Interface Copyright (C) 1995, 1996
   Carlos Hasan. All Rights Reserved.
Video modes created using Tweak 1.6b by Robert Schmidt, who also wrote
   TwkUser.c.
"inflate" code for zip file support by Mark Adler.

DOS executable compressed with UPX by Markus F.X.J. Oberhumer & Laszlo Molnar,
    http://upx.tsx.org/

Big thanks to Gary Walton (garyw@excels-w.demon.co.uk) for too many things
   to mention.

Thanks to Brian Deuel, Neil Bradley, and the Retrocade dev team for allowing us
to use Retrocade's game history database.

Thanks to Richard Bush for info on several games.

and thanks to everyone else I forgot.


Using the program
-----------------

MAME [name of the game to run] [options]

for example:

   MAME mspacman -soundcard 0

...will run Ms Pac Man without sound


Options:

-tweak/-notweak (default: notweak)
              MAME supports a large number of tweaked VGA modes whose
              resolutions matching those of the emulated games. These modes
              look MUCH better than VESA modes (and are usually faster), but
              they may create compatibility problems with some video cards and
              monitors. Therefore, they are not enabled by default. You should
              by all means use -tweak if your hardware supports it. Note that
              some modes might work and other might not--e.g. your card could
              support 256x256 but not 384x224. In that case, you'll have to
              turn -tweak on or off depending on the game you run. -noscanlines
              can also solve many compatibility problems.

-ntsc         a 288x224 mode with standard NTSC frequencies. You need some
              additional hardware (VGA2TV converter) to use this.

-vesamode vesa1/vesa2b/vesa2l/vesa3
              Forces the VESA mode. The best available one is used by default.
              If your video card crashes during autodetection, however, use
              this option to force a lower standard. (Start with vesa1, and go
              upwards to find the highest one that works.)

-resolution XxY
              where X and Y are width and height (ex: '-resolution 800x600')
              MAME goes to some lengths to autoselect a good resolution. You
              can override MAME's choice with this option. You can omit the
              word "resolution" and simply use -XxY (e.g. '-800x600') as a
              shortcut. Frontend authors are advised to use '-resolution XxY',
              however.

-skiplines N / -skipcolumns N
              If you run a game in a video mode smaller than the visible area,
              you can adjust its position using the PgUp and PgDn keys (alone
              for vertical panning, shifted for horizontal panning). You can
              also use these two parameters to set the initial position: 0 is
              the default, meaning that the screen is centered. You can specify
              both positive and negative offsets.

-scanlines/-noscanlines (default: -scanlines)
              Scanlines are small, evenly-spaced, horizontal blank lines that
              are typical of real arcade monitors. If you don't prefer this
              "authentic" look, turn scanlines off.

-stretch/-nostretch (default: stretch)
              use nostretch to disable pixel doubling in VESA modes (faster,
              but smaller picture).

-depth n      (default: auto)
              Some games need 16-bit color to get accurate graphics. To improve
              speed, you can use '-depth 8', which limits the display to
              standard 256-color mode. (You can also use '-depth 16' to force
              256-color games to use 16-bit color, but this isn't recommended.)

-gamma n      (default: 1.0)
              Sets the initial gamma correction value.

-vgafreq n    where n can be 0 (default) 1, 2 or 3.
              Specifies different frequencies for the custom video modes. This
              can reduce flicker, especially in 224x288 / noscanlines mode.
              WARNING: IT IS POSSIBLE TO SET FREQUENCIES WAY OUTSIDE OF YOUR
              MONITOR'S RANGE, WHICH COULD DAMAGE YOUR MONITOR. BEFORE USING
              THIS OPTION, BE SURE YOU KNOW WHICH FREQUENCIES YOUR MONITOR
              SUPPORTS. USE THIS OPTION AT YOUR OWN RISK.

-vsync/-novsync (default: -novsync)
              Synchronize video display with the video beam instead of using
              the timer. This option can be used only if the selected video
              mode has an appropriate refresh rate. Otherwise, MAME will
              refuse to start, telling you the actual refresh rate of the video
              mode, and the rate it should have.
              If you are using a tweaked mode, MAME will try to automatically
              pick the correct setting for -vgafreq; you can still override it
              using the -vgafreq option.
              Note: the 224x288 / noscanlines mode doesn't work on most cards.
              Many games use this mode, e.g. Pac Man and Galaga. If it doesn't
              work with your card, either turn scanlines on, or don't use
              -vsync.
              If you are using a VESA mode, you should use the program that
              came with your video card to set the appropriate refresh rate.
              Note that when this option is turned on, speed will NOT
              downgrade nicely if your system is not fast enough (i.e.,
              gameplay will be jerky).

-alwayssynced/-noalwayssynced (default: -noalwayssynced)
              For many tweaked VGA modes, MAME has two definitions: One which
              is more compatible, and one which is less compatible but uses
              frequencies compatible with -vsync. By default, the less-
              compatible definition is used only when -vsync is requested;
              using this option, you can force it to be used always.

-triplebuffer/-notriplebuffer (default: -notriplebuffer)
              Enables triple buffering with VESA modes. This is faster than
              -vsync, but doesn't work on all cards. Even if it does remove
              "tearing" during scrolling, it might not be as smooth as -vsync.

-monitor NNNN (default: standard)
              Selects the monitor type:
              standard: standard PC monitor
              ntsc:     NTSC monitor
              pal:      PAL monitor
              arcade:   arcade monitor

-centerx N and -centery N
              Each takes a signed value (-8 to 8 for centerx, -16 to 16 for
              centery) and lets you change the low scanrate modes (monitor=ntsc,
              pal, arcade).

-waitinterlace
              Forces update of both odd and even fields of an interlaced low
              scanrate display (monitor=ntsc,pal,arcade) for each game loop.

-ror          Rotates the display clockwise by 90 degrees.
-rol          Rotates display anticlockwise
-flipx        Flips display horizontally
-flipy        Flips display vertically
              -ror and -rol add authentic *vertical* scanlines, given that you
              have turned your monitor on its side.
              CAUTION:
              Monitors are complicated, high-voltage electronic devices.
              Some monitors are designed to be rotated. If yours is _not_ one
              of them, but you absolutely must turn it on its side, you do so
              at your own risk.

              *******************************************
              PLEASE DO NOT LEAVE YOUR MONITOR UNATTENDED
              IF IT IS PLUGGED IN AND TURNED ON ITS SIDE!
              *******************************************

-norotate     Disable all internal rotation of the image, therefore displaying
              the image in its original orientation (for example, so you need
              a vertical monitor to see vertical games).
              In some cases, the image will be upside down. To correct this,
              use '-norotate -flipx -flipy'

-frameskip n (default: auto)
              Skips frames to speed up the emulation. The argument is the number
              of frames to skip out of 12. For example, if the game normally
              runs at 60 fps, '-frameskip 2' will make it run at 50 fps,
              '-frameskip 6' at 30 fps. Use F11 to display the speed your
              computer is actually reaching. If it is below 100%, increase the
              frameskip value. You can press F9 to change frameskip while
              running the game.
              When set to auto (the default), the frameskip setting is
              dynamically adjusted during run time to display the maximum
              possible frames without dropping below 100% speed.

-antialias/-noantialias (default: -antialias)
              Antialiasing for the vector games.

-beam n       Sets the width in pixels of the vectors. n is a float in the
              range of 1.00 through 16.00.

-flicker n    Makes the vectors flicker. n is an optional argument, a float in
              the range 0.00 - 100.00 (0=none, 100=maximum).

-translucency/-notranslucency (default: -translucency)
              Enables or disables vector translucency.

-soundcard n  Selects sound card. (If this is not specified, MAME will ask you.)

-sr n         Sets the audio sample rate. The default is 22050. Smaller values
              (e.g. 11025) cause lower audio quality but faster emulation speed.
              Higher values (e.g. 44100) cause higher audio quality but slower
              emulation speed.

-stereo/-nostereo (default: -stereo)
              Selects stereo or mono output for games supporting stereo sound.

-volume n     (default: 0)
              Sets the startup volume. It can later be changed with the On
              Screen Display (see Keys section). The volume is an attenuation
              in dB: e.g., "-volume -12" will start with -12dB attenuation.

-ym3812opl/-noym3812opl (default: -noym3812opl)
              Uses the SoundBlaster OPL chip for music emulation of the YM3812
              chip. This is faster, and is reasonably accurate, since the chips
              are 100% compatible. There is no control on the volume, however,
              and you need a real OPL chip for it to work  (If you are using an
              SB-compatible card that emulates the OPL in software, the built-in
              digital emulation will probably sound better).

-joy name (default: none)
              Allows joystick input. 'name' can be:
              none         - no joystick
              auto         - attempts auto detection
              standard     - normal 2 button joystick
              dual         - dual joysticks
              4button      - Stick/Pad with 4 buttons
              6button      - Stick/Pad with 6 buttons
              8button      - Stick/Pad with 8 buttons
              fspro        - CH Flightstick Pro
              wingex       - Wingman Extreme
              wingwarrior  - Wingman Warrior
              sidewinder   - Microsoft Sidewinder (up to 4)
              gamepadpro   - Gravis GamePad Pro
              grip         - Gravis GrIP
              grip4        - Gravis GrIP constrained to move only along the
			                 x and y axes
              sneslpt1     - SNES pad on LPT1 (needs special hardware)
              sneslpt2     - SNES pad on LPT2 (needs special hardware)
              sneslpt3     - SNES pad on LPT3 (needs special hardware)
              psxlpt1      - PSX pad on LPT1 (needs special hardware)
              psxlpt2      - PSX pad on LPT2 (needs special hardware)
              psxlpt3      - PSX pad on LPT3 (needs special hardware)
              n64lpt1      - N64 pad on LPT1 (needs special hardware)
              n64lpt2      - N64 pad on LPT2 (needs special hardware)
              n64lpt3      - N64 pad on LPT3 (needs special hardware)

              Notes:
              1) Use the TAB menu to calibrate analog joysticks. Calibration
              data will be saved in mame.cfg. If you're using different
              joysticks for different games, you may need to recalibrate your
              joystick every time.
              2) Extra buttons of noname joysticks may not work.
              3) The "official" Snespad-Support site is:
              http://snespad.emulationworld.com
              4) http://www.debaser.force9.co.uk/ccmame has info on how to
              connect PSX and N64 pads.

-hotrod       Sets a default keyboard configuration suitable for the HotRod
              joystick by HanaHo Games.

-hotrodse     Sets a default keyboard configuration suitable for the HotRod SE
              joystick by HanaHo Games.

-log          Creates a log of illegal memory accesses in ERROR.LOG

-help, -?     Displays current MAME version and copyright notice

 - - -

Note: By default, all the '-list' commands below write info to the screen. If
you wish to write the info to a textfile instead, add this to the end of your
command:

  > filename

...where 'filename' is the textfile's path and name (e.g., c:\mame\list.txt).

 - - -

-list         Displays a list of currently supported games

-listfull     Displays a list of game directory names + descriptions

-listroms     Displays ROMs required by the specified game

-listsamples  Displays samples required by the specified game

-listdetails  Displays a detailed list of drivers and the hardware they use

-listgames    list the supported games, year, manufacturer

-listinfo     list comprehensive details for all of the supported games

-listclones   Lists clones of the specified game. When no game is specified,
              this generates a list of all MAME-supported clones.

-noclones     used together with the list commands, doesn't list alternate
              versions of the same game

-verifyroms   Checks specified game(s) for missing and invalid ROMs.
              Adding * checks all available games.

-verifysets   Checks specified game(s) and reports its status.
              Adding * checks all available games.

-verifysamples check selected game for missing samples.
              Adding * checks all available samples.

-romdir       Specifies an alternate directory (or zipfile name) for loading
              the ROMs for the specified game.
              E.g., 'mame pacman -romdir pachack' will run the Pac Man driver
              but load the ROMs from the "pachack" dir or "pachack.zip" archive.

-mouse/-nomouse (default: -mouse)
              Enables/disables mouse support

-cheat        Cheats, like the speedup in Pac Man or the level-skip in many
              other games, are disabled by default. Use this switch to turn
              them on.

-debug        Activates the integrated debugger.
              During emulation, press the Tilde key (~) to activate the
              debugger. This is available only if the program is compiled with
              MAME_DEBUG defined.

-record name   Records joystick input to file INP/name.inp

-playback name Plays back joystick input from file INP/name.inp

-ignorecfg    ignore mame.cfg and start with the default settings


Keys
----
Tab          Toggles the configuration menu
Tilde        Toggles the On Screen Display
             Use the up and down arrow keys to select the parameter (global
             volume, mixing level, gamma correction etc.), left and right to
             arrow keys to modify it.
P            Pauses the game
Shift+P      While paused, advances to next frame
F3           Resets the game
F4           Shows the game graphics. Use cursor keys to change set/color,
             F4 or Esc to return to the emulation.
F9           Changes frame skip on the fly
F10          Toggles speed throttling
F11          Toggles speed display
Shift+F11    Toggles profiler display
F12          Saves a screen snapshot. The default target directory is SNAP. You
             must create this directory yourself; the program will not create
             it if it doesn't exist.
ESC          Exits emulator
