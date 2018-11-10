# MAME4apple-037b5

MAME 0.37b5 for iOS devices. 64-bit, rebuilt from the original 0.37b5 source code, SpriteKit renderer.

Uses SKMutableTexture for the render surface.

Supports MFi controllers and iCade.

Based on the original MAME 0.37b5 source code as downloaded from the MAMEDev website. Compatible with MAME4all rom set. The entire project has been designed for iOS and tvOS devices.

64 bit Xcode build project.

Using SpriteKit allows for porting to all iOS devices including the AppleTV and tvOS.

The project is 64-bit compatible and has been tested on iPhone 6s, iPad Air 2, iPad Pro 10.5, 11 & 13 and Apple TV 4th Gen.

Front end for iOS devices is based on UITableView. For tvOS it is a scrolling list of games controlled via an MFi controller.

What's working:
Most "supported" MAME4ALL games.
Framerate for games I tested so far is smooth.
Audio works but is not perfect.
Graphics work for all tested games but some games do not display properly (scaling to fit screen dimensions).
iCade and MFi controllers.
AppleTV running tvOS.
