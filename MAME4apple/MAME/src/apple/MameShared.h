//
//  MameShared.h
//  MAME4apple
//
//  Shared C API for the rewritten iOS "apple" layer: frame-buffer handoff,
//  emulation pause primitive, and lifecycle hooks. Kept dependency-light so it
//  can be included from both plain C/Obj-C OSD files and Obj-C++ renderer code.
//

#ifndef MameShared_h
#define MameShared_h

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Frame-buffer handoff (implemented in MameRenderer.mm, iOS only)
//
// The emulator thread renders one frame into the current back buffer (obtained
// from mame_renderer_backbuffer) and then calls mame_renderer_publish to hand
// it to the Metal view. A triple buffer guarantees the display never reads a
// buffer the emulator is still writing, so frames are always tear-free without
// blocking the emulator thread.
// ---------------------------------------------------------------------------

// (Re)allocate buffers/texture for a game whose full bitmap is width x height.
// Safe to call again for a new game. Shows the Metal view.
void mame_renderer_configure(int width, int height);

// Pointer to the RGBA8 back buffer (row stride == configured width * 4 bytes).
// Returns NULL before configure. The emulator writes the visible sub-rect into
// the top-left of this buffer.
uint32_t *mame_renderer_backbuffer(void);

// Publish the back buffer as the newest frame. active_width/active_height give
// the valid (visible) sub-rect that was written this frame.
void mame_renderer_publish(int active_width, int active_height);

// Hide the Metal view and stop its display loop (returning to the front end).
void mame_renderer_hide(void);

// Pixel aspect hint: 1 == normal, 2 == double-height pixels (VIDEO_PIXEL_ASPECT
// _RATIO_1_2 games such as Blasteroids). Set from the blit when the game starts.
void mame_renderer_set_pixel_aspect_y(int scale);

// ---------------------------------------------------------------------------
// Emulation pause primitive (implemented in iOS_video.m)
//
// Used for app lifecycle (background/foreground) and the MAME UI pause. When
// paused, the emulator thread blocks on a condition variable at the next frame
// boundary instead of spinning, so no CPU/battery is burned and audio can be
// safely stopped.
// ---------------------------------------------------------------------------

void mame_pause_set(int paused);      // 1 = pause, 0 = resume
int  mame_pause_is_paused(void);
void mame_pause_wait_if_needed(void); // called by the emu thread each frame

// ---------------------------------------------------------------------------
// Input (implemented in iOS_input.m)
//
// GameController must be read on the main thread. This is called once per
// frame from the main run loop (SKScene update:) to snapshot controller state;
// the emulator thread then reads only the snapshot (osd_poll_joysticks), never
// GameController directly.
// ---------------------------------------------------------------------------
void mame_input_poll_controllers_main(void);

// Number of connected hardware gamepads seen at the last main-thread poll.
// Used to auto-hide the on-screen touch controls.
int  mame_input_gamepad_count(void);

// On-screen touch controls overlay (implemented in TouchControlsView.m).
void mame_touch_set_visible(int visible);

#ifdef __cplusplus
}
#endif

#endif /* MameShared_h */
