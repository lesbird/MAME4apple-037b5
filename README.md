# MAME4apple-037b5

MAME 0.37b5 for iOS devices. 64-bit, rebuilt from the original 0.37b5 source code, SpriteKit renderer.

Uses SKMutableTexture for the render surface.

Supports MFi controllers and iCade.

Based on the original MAME 0.37b5 source code, not the iMAME4All GP2X port. MAME4all rom set will work. The entire project has been designed for iOS and tvOS devices.

64 bit Xcode build project.

Using SpriteKit allows for porting to all iOS devices including the AppleTV and tvOS.

Using AudioQueue for audio streaming.

The project is full 64-bit compliant and has been tested on iPhone 6s, iPad Air 2, iPad Pro 13 and Apple TV 4th Gen.

Front end is minimal at the moment. Scroll through a list of games, select one to play.

What's working:
All "supported" MAME4ALL games are working (although I have not tested all 2000+ games yet).
Framerate for games I tested so far is smooth.
Audio works but is not perfect.
Graphics work for all tested games but some games do not display properly (scaling to fit screen dimensions).
iCade and MFi controllers.
AppleTV running tvOS.

What's not working:
Touch controls experimental so far but they work.
No MAME.CFG file supported but not really needed.
Vector graphics beam width can not be scaled yet.
