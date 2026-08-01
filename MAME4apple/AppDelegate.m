//
//  AppDelegate.m
//  MAME4apple
//
//  Created by Les Bird on 10/3/16.
//  Copyright © 2016 Les Bird. All rights reserved.
//

#import "AppDelegate.h"
#include "MameShared.h"

@interface AppDelegate ()

@end

@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application {
    // Park the emulator thread at a frame boundary before we lose the
    // foreground, so it isn't running (or touching GPU/audio) while suspended.
    mame_pause_set(1);
}


- (void)applicationDidEnterBackground:(UIApplication *)application {
    mame_pause_set(1);
}


- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Undo changes made on entering the background; actual resume happens in
    // applicationDidBecomeActive.
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Resume emulation once we're active again.
    mame_pause_set(0);
}


- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}


@end
