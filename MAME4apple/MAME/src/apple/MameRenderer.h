//
//  MameRenderer.h
//  MAME4apple
//
//  Metal renderer that presents the emulated frame. Replaces the old
//  SpriteKit SKMutableTexture path on iOS with a vsync-locked MTKView,
//  triple-buffered tear-free handoff, crisp nearest-neighbour scaling, and
//  aspect-correct fit that adapts to orientation and safe areas.
//

#import <TargetConditionals.h>

#if !TARGET_OS_TV

#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>
#import "MameShared.h"

NS_ASSUME_NONNULL_BEGIN

@interface MameRenderer : NSObject <MTKViewDelegate>

// The Metal-backed view. Add this as a subview over the front end; it is
// hidden until a game is running.
@property (nonatomic, readonly) MTKView *view;

// Draw the game pixels crisply with nearest-neighbour sampling (default YES).
@property (nonatomic) BOOL crispScaling;

// Snap the game to an integer multiple when it fits (default YES). Falls back
// to aspect-fit when the screen is too small for 1x.
@property (nonatomic) BOOL integerScaling;

// The shared renderer used by the C handoff functions. Created by the host
// view controller.
+ (instancetype)shared;
+ (void)setShared:(nullable MameRenderer *)renderer;

- (instancetype)initWithFrame:(CGRect)frame;

@end

NS_ASSUME_NONNULL_END

#endif /* !TARGET_OS_TV */
