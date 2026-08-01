//
//  TouchControlsView.m
//  MAME4apple
//
//  See TouchControlsView.h.
//

#import "TouchControlsView.h"

#if !TARGET_OS_TV

// Globals the emulator input mapping (iOS_input.m / update_key_array) reads.
// Defined in GameScene.m. Set here, on the main thread.
extern int touchInputX;
extern int touchInputY;
extern unsigned int onscreenButton[];   // ONSCREEN_BUTTON_A..D (see Prefix.pch)
extern BOOL coinButtonPressed;
extern BOOL startButtonPressed;
extern BOOL exitButtonPressed;

typedef NS_ENUM(NSInteger, MameButtonKind)
{
    MameBtn1, MameBtn2, MameBtn3, MameBtn4,   // -> onscreenButton[A..D]
    MameBtnCoin, MameBtnStart, MameBtnExit
};

@interface MameTouchButton : UIView
@property (nonatomic) MameButtonKind kind;
@property (nonatomic) BOOL pressed;
@property (nonatomic, strong) UILabel *label;
@end

@implementation MameTouchButton
- (void)setPressed:(BOOL)pressed
{
    if (_pressed == pressed) return;
    _pressed = pressed;
    self.backgroundColor = pressed ? [UIColor colorWithWhite:1.0 alpha:0.55]
                                   : [UIColor colorWithWhite:0.2 alpha:0.45];
}
@end


static TouchControlsView *sSharedTouch = nil;   // strong (see MameRenderer sShared)

@implementation TouchControlsView
{
    UIView    *_stickBase;
    UIView    *_stickKnob;
    CGPoint    _stickAnchor;
    UITouch   *_stickTouch;
    CGFloat    _stickRadius;

    NSMutableSet<UITouch *> *_touches;
    NSArray<MameTouchButton *> *_buttons;
}

+ (instancetype)shared { return sSharedTouch; }
+ (void)setShared:(TouchControlsView *)view { sSharedTouch = view; }

- (instancetype)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame]))
    {
        self.multipleTouchEnabled = YES;
        self.opaque = NO;
        self.backgroundColor = [UIColor clearColor];
        _touches = [NSMutableSet set];

        BOOL pad = (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad);
        _stickRadius = pad ? 80 : 62;

        // floating stick
        _stickBase = [[UIView alloc] initWithFrame:CGRectMake(0, 0, _stickRadius * 2, _stickRadius * 2)];
        _stickBase.backgroundColor = [UIColor colorWithWhite:0.3 alpha:0.35];
        _stickBase.layer.cornerRadius = _stickRadius;
        _stickBase.userInteractionEnabled = NO;
        _stickBase.hidden = YES;
        [self addSubview:_stickBase];

        CGFloat knobR = _stickRadius * 0.5;
        _stickKnob = [[UIView alloc] initWithFrame:CGRectMake(0, 0, knobR * 2, knobR * 2)];
        _stickKnob.backgroundColor = [UIColor colorWithRed:0.9 green:0.2 blue:0.2 alpha:0.6];
        _stickKnob.layer.cornerRadius = knobR;
        _stickKnob.userInteractionEnabled = NO;
        [_stickBase addSubview:_stickKnob];
        _stickKnob.center = CGPointMake(_stickRadius, _stickRadius);

        // buttons
        NSMutableArray *btns = [NSMutableArray array];
        NSArray *specs = @[ @[@"1", @(MameBtn1)], @[@"2", @(MameBtn2)],
                            @[@"3", @(MameBtn3)], @[@"4", @(MameBtn4)],
                            @[@"COIN", @(MameBtnCoin)], @[@"START", @(MameBtnStart)],
                            @[@"EXIT", @(MameBtnExit)] ];
        for (NSArray *spec in specs)
        {
            MameTouchButton *b = [[MameTouchButton alloc] initWithFrame:CGRectZero];
            b.kind = (MameButtonKind)[spec[1] integerValue];
            b.pressed = NO;
            b.backgroundColor = [UIColor colorWithWhite:0.2 alpha:0.45];
            b.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.6].CGColor;
            b.layer.borderWidth = 2;
            b.userInteractionEnabled = NO;

            UILabel *l = [[UILabel alloc] initWithFrame:CGRectZero];
            l.text = spec[0];
            l.textColor = [UIColor whiteColor];
            l.textAlignment = NSTextAlignmentCenter;
            l.font = [UIFont boldSystemFontOfSize:pad ? 18 : 13];
            b.label = l;
            [b addSubview:l];

            [self addSubview:b];
            [btns addObject:b];
        }
        _buttons = btns;
    }
    return self;
}

- (MameTouchButton *)buttonOfKind:(MameButtonKind)kind
{
    for (MameTouchButton *b in _buttons)
        if (b.kind == kind) return b;
    return nil;
}

- (void)layoutSubviews
{
    [super layoutSubviews];

    BOOL pad = (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad);
    CGRect b = self.bounds;
    UIEdgeInsets s = self.safeAreaInsets;
    CGFloat pd = 16;

    CGFloat W = CGRectGetWidth(b), H = CGRectGetHeight(b);
    CGFloat abd = pad ? 92 : 62;      // action button diameter
    CGFloat gap = pad ? 16 : 10;
    CGFloat smw = pad ? 120 : 84;     // small button width
    CGFloat smh = pad ? 44 : 34;

    // top row: COIN (left), EXIT (center), START (right)
    [self buttonOfKind:MameBtnCoin].frame  = CGRectMake(s.left + pd, s.top + pd, smw, smh);
    [self buttonOfKind:MameBtnExit].frame  = CGRectMake((W - smw) / 2.0, s.top + pd, smw, smh);
    [self buttonOfKind:MameBtnStart].frame = CGRectMake(W - s.right - pd - smw, s.top + pd, smw, smh);

    // bottom-right cluster: 1,2,3,4 in a row (rightmost = 1)
    CGFloat by = H - s.bottom - pd - abd;
    CGFloat rightEdge = W - s.right - pd;
    MameButtonKind order[4] = { MameBtn1, MameBtn2, MameBtn3, MameBtn4 };
    for (int i = 0; i < 4; i++)
    {
        CGFloat x = rightEdge - (i + 1) * abd - i * gap;
        MameTouchButton *btn = [self buttonOfKind:order[i]];
        btn.frame = CGRectMake(x, by, abd, abd);
        btn.layer.cornerRadius = abd / 2.0;
    }

    for (MameTouchButton *btn in _buttons)
        btn.label.frame = btn.bounds;
}

#pragma mark - hit testing

- (MameTouchButton *)buttonAtPoint:(CGPoint)p
{
    for (MameTouchButton *btn in _buttons)
        if (CGRectContainsPoint(btn.frame, p)) return btn;
    return nil;
}

- (void)updateState
{
    // stick
    if (_stickTouch)
    {
        CGPoint loc = [_stickTouch locationInView:self];
        CGFloat dx = loc.x - _stickAnchor.x;
        CGFloat dy = loc.y - _stickAnchor.y;
        CGFloat dist = sqrt(dx * dx + dy * dy);
        if (dist > _stickRadius)
        {
            dx = dx / dist * _stickRadius;
            dy = dy / dist * _stickRadius;
        }
        _stickKnob.center = CGPointMake(_stickRadius + dx, _stickRadius + dy);

        const CGFloat dead = _stickRadius * 0.28;   // deadzone
        touchInputX = (fabs(dx) > dead) ? (int)(dx / 8.0) : 0;
        // UIKit y grows downward; emulator expects +Y == up, so invert.
        touchInputY = (fabs(dy) > dead) ? (int)(-dy / 8.0) : 0;
    }

    // buttons: pressed if any active (non-stick) touch is inside
    for (MameTouchButton *btn in _buttons)
    {
        BOOL pressed = NO;
        for (UITouch *t in _touches)
        {
            if (t == _stickTouch) continue;
            if (CGRectContainsPoint(btn.frame, [t locationInView:self])) { pressed = YES; break; }
        }
        btn.pressed = pressed;

        switch (btn.kind)
        {
            case MameBtn1: onscreenButton[0] = pressed ? 1 : 0; break;
            case MameBtn2: onscreenButton[1] = pressed ? 1 : 0; break;
            case MameBtn3: onscreenButton[2] = pressed ? 1 : 0; break;
            case MameBtn4: onscreenButton[3] = pressed ? 1 : 0; break;
            case MameBtnCoin:  coinButtonPressed  = pressed; break;
            case MameBtnStart: startButtonPressed = pressed; break;
            case MameBtnExit:  exitButtonPressed  = pressed; break;
        }
    }
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches)
    {
        [_touches addObject:t];
        CGPoint loc = [t locationInView:self];
        BOOL onButton = ([self buttonAtPoint:loc] != nil);
        if (!_stickTouch && !onButton && loc.x < CGRectGetWidth(self.bounds) / 2.0)
        {
            _stickTouch = t;
            _stickAnchor = loc;
            _stickBase.center = loc;
            _stickKnob.center = CGPointMake(_stickRadius, _stickRadius);
            _stickBase.hidden = NO;
        }
    }
    [self updateState];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [self updateState];
}

- (void)endTouches:(NSSet<UITouch *> *)touches
{
    for (UITouch *t in touches)
    {
        [_touches removeObject:t];
        if (t == _stickTouch)
        {
            _stickTouch = nil;
            _stickBase.hidden = YES;
            touchInputX = 0;
            touchInputY = 0;
        }
    }
    [self updateState];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self endTouches:touches]; }
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self endTouches:touches]; }

- (void)clearAll
{
    [_touches removeAllObjects];
    _stickTouch = nil;
    _stickBase.hidden = YES;
    touchInputX = touchInputY = 0;
    for (MameTouchButton *btn in _buttons) btn.pressed = NO;
    onscreenButton[0] = onscreenButton[1] = onscreenButton[2] = onscreenButton[3] = 0;
    coinButtonPressed = startButtonPressed = exitButtonPressed = NO;
}

@end

void mame_touch_set_visible(int visible)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        TouchControlsView *v = [TouchControlsView shared];
        if (!v) return;
        if (!visible) [v clearAll];
        v.hidden = visible ? NO : YES;
        v.userInteractionEnabled = visible ? YES : NO;
    });
}

#endif /* !TARGET_OS_TV */
