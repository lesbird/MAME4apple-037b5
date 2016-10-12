# MAME4apple-037b5

MAME 0.37B5 for iOS devices. 64-bit, rebuilt from the original 0.37B5 source code, SpriteKit renderer.

Uses SKMutableTexture for the render surface.

Supports MFi controllers (as of initial commit the only option - iCade hopefully soon).

Based on the original MAME 0.37B5 source code, not the iMAME4All GP2X port, so the MAME4all rom set will work perfectly. The entire project has been designed for iOS devices, a known platform, so there is no need for a MAME.CFG file to turn on or off features.

64 bit Xcode build project.

Using SpriteKit allows for porting to all iOS devices including the AppleTV and TVos.

Using AudioQueue for audio streaming.

The project is full 64-bit compliant and has been tested on iPhone 6s, iPad Air 2 and iPad Pro 13 inch.

Front end is minimal at the moment. Scroll through a list of games, select one to play.

What's working:
All "supported" MAME4ALL games are working (although I have not tested all 2000+ games yet).
Framerate for games I tested so far is smooth.
Audio works but is not perfect.
Graphics work for all tested games but some games do not display properly (scaling to fit screen dimensions).

What's not working:
Artwork overlays aren't working yet.
iCade not supported yet.
Touch controls not supported yet.
No MAME.CFG file supported.
Vector graphics beam width can not be scaled yet.
