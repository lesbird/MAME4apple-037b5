//
//  TouchControlsView.h
//  MAME4apple
//
//  Safe-area-aware on-screen gamepad overlay: a floating left analog/8-way
//  stick plus action / coin / start / exit buttons. Replaces the old fixed
//  UIButtons and SpriteKit joystick. Feeds the same globals the emulator input
//  mapping reads (touchInputX/Y, onscreenButton[], coin/start/exit flags), all
//  set on the main thread. Auto-hidden when a hardware controller is present.
//

#import <TargetConditionals.h>

#if !TARGET_OS_TV

#import <UIKit/UIKit.h>

#ifdef __cplusplus
extern "C" {
#endif

// Show/hide the shared overlay (dispatches to the main thread). Driven by the
// emulator input layer: shown during a game when no hardware controller is
// connected, hidden otherwise.
void mame_touch_set_visible(int visible);

#ifdef __cplusplus
}
#endif

@interface TouchControlsView : UIView

+ (instancetype)shared;
+ (void)setShared:(TouchControlsView *)view;

@end

#endif /* !TARGET_OS_TV */
