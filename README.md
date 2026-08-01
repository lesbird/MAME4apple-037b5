# MAME4apple-037b5

MAME 0.37b5 for iOS devices. 64-bit, rebuilt from the original 0.37b5 source
code. Compatible with the MAME4all rom set.

Based on the original MAME 0.37b5 source code as downloaded from the MAMEDev
website. The emulator core is unmodified; only the OS-dependent ("apple") layer
is specific to this port. Designed for iOS and tvOS.

## iOS "apple" layer (rewritten)

The iOS OS-dependent layer was rewritten for solid, low-latency behaviour on
modern iPhone and iPad hardware:

- **Graphics — Metal.** The in-game frame is presented by a dedicated Metal
  renderer (`MTKView`) with crisp nearest-neighbour, aspect-correct / integer
  scaling. A triple-buffered, lock-guarded handoff between the emulator thread
  and the display eliminates tearing without stalling emulation. Frame pacing
  uses a precise `mach_wait_until` deadline (no busy-wait), so timing is smooth
  and the CPU/battery aren't burned spinning.
- **Audio — AVAudioEngine.** Sound is played through `AVAudioEngine` /
  `AVAudioSourceNode` fed by a lock-free single-producer/single-consumer ring
  buffer, with under/over-run handling and audio-session interruption and
  route-change support (phone calls, unplugging headphones).
- **Input.** Game controllers are read on the main thread into an atomic
  snapshot that the emulator thread consumes, so controller state is never
  touched cross-thread. Supports MFi controllers and iCade.
- **On-screen touch controls.** A safe-area-aware overlay with a floating
  analog/8-way stick plus action / coin / start / exit buttons that adapt to
  iPhone and iPad and auto-hide when a hardware controller is connected.
- **Lifecycle.** The emulation parks cleanly at a frame boundary when the app
  is backgrounded and resumes when it returns to the foreground.
- **File I/O.** ROM/sample/config/nvram/highscore paths use bounded string
  handling suited to iOS's long Documents directory paths.

Minimum deployment target: **iOS 15**. 64-bit Xcode project.

## Front end

The game browser is a `UITableView` listing games for which ROMs are present in
the app's `Documents/roms/` directory.

## tvOS

The tvOS target still uses the original SpriteKit renderer and AudioQueue audio
path (guarded by `TARGET_OS_TV`); the rewrite above applies to iOS.

## What's working

- Most "supported" MAME4ALL games.
- Smooth, tear-free, vsync-paced video via Metal.
- Clean audio via AVAudioEngine.
- iCade and MFi controllers, plus the on-screen touch controls.
- AppleTV / tvOS (original SpriteKit path).

Tested against the MAME4all rom set on iPhone and iPad.
