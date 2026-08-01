//
//  GameViewController.m
//  MAME4apple
//
//  Created by Les Bird on 10/3/16.
//  Copyright © 2016 Les Bird. All rights reserved.
//

#import "GameViewController.h"
#import "GameScene.h"
#import <TargetConditionals.h>
#if !TARGET_OS_TV
#import "MameRenderer.h"
#import "TouchControlsView.h"
#endif

@implementation GameViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    SKView *skView = (SKView *)self.view;

#if !TARGET_OS_TV
    // Metal renderer for the in-game frame. Sits above the SpriteKit content
    // (the black background) but below the front-end table and touch controls,
    // hidden until a game starts.
    MameRenderer *renderer = [[MameRenderer alloc] initWithFrame:skView.bounds];
    [MameRenderer setShared:renderer];
    [skView insertSubview:renderer.view atIndex:0];

    // On-screen touch controls, above the Metal view but below the front-end
    // table. Hidden until a game runs without a hardware controller.
    TouchControlsView *touch = [[TouchControlsView alloc] initWithFrame:skView.bounds];
    touch.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    touch.hidden = YES;
    touch.userInteractionEnabled = NO;
    [TouchControlsView setShared:touch];
    [skView insertSubview:touch atIndex:1];
#endif

    // Load the SKScene from 'GameScene.sks'
    GameScene *scene = (GameScene *)[SKScene nodeWithFileNamed:@"GameScene"];

    // Present the scene
    [skView presentScene:scene];

    //skView.showsFPS = YES;
    //skView.showsNodeCount = YES;
}

- (BOOL)shouldAutorotate {
    NSLog(@"shouldAutorotate");
    return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        return UIInterfaceOrientationMaskAllButUpsideDown;
    } else {
        return UIInterfaceOrientationMaskAll;
    }
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Release any cached data, images, etc that aren't in use.
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

@end
