//
//  RomDownloadManager.h
//  MAME4apple
//
//  Created by WhiteTiger "sgama la rete" on 04/12/2016.
//  Copyright © 2016 Les Bird. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class RomDownload;

@interface RomDownloadManager : NSObject

@property (strong, readonly) NSMutableArray *activeDownloads;

+ (id)sharedManager;
- (void)addRequest:(NSURLRequest *)request;
- (void)removeDownload:(RomDownload *)download;

@end

@interface RomDownload : NSObject {
    NSURLConnection *_connection;
    NSURL *escapedPath;
    NSString *savePath;
    NSFileHandle *fileHandle;
    
    long long expectedBytes;
    float progress;
    
    __weak RomDownloadManager *_delegate;
}

@property (nonatomic, weak) UILabel *progressLabel;
@property (nonatomic, weak) CALayer *progressLayer;

- (instancetype)initWithRequest:(NSURLRequest *)request delegate:(RomDownloadManager *)delegate;
- (float)progress;
- (NSString *)name;

@end
