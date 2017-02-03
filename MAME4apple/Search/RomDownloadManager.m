//
//  RomDownloadManager.m
//  MAME4apple
//
//  Created by WhiteTiger "sgama la rete" on 04/12/2016.
//  Copyright © 2016 Les Bird. All rights reserved.
//

#import "RomDownloadManager.h"
#import "AppDelegate.h"

@implementation RomDownload

- (instancetype)initWithRequest:(NSURLRequest *)request delegate:(RomDownloadManager *)delegate {
    if (self = [super init]) {
        _connection = [[NSURLConnection alloc] initWithRequest:request delegate:self startImmediately:YES];
        _delegate = delegate;
        
        savePath = [NSString stringWithFormat:@"file://%@%@", NSTemporaryDirectory(), [_connection.originalRequest.URL lastPathComponent]];
        escapedPath = [NSURL URLWithString:[savePath stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]]];
        
        [[NSFileManager defaultManager] createFileAtPath:escapedPath.path contents:nil attributes:nil];
        fileHandle = [NSFileHandle fileHandleForWritingAtPath:escapedPath.path];
        
        if (!fileHandle) {
            NSLog(@"Error opening handle");
        }
    }
    
    return self;
}

- (float)progress {
    return progress;
}

- (NSString *)name {
    return _connection.originalRequest.URL.path.lastPathComponent;
}

- (void)stop {
    [_connection cancel];
    _connection = nil;
}

#pragma mark - Private Methods

- (void)addGame:(NSString *)filename {
    NSString *romsPath = [NSHomeDirectory() stringByAppendingPathComponent:@"Documents/roms"];
    
    NSFileManager *fm = [NSFileManager defaultManager];
    
    if (![fm fileExistsAtPath:romsPath]) {
        [fm createDirectoryAtPath:romsPath withIntermediateDirectories:NO attributes:nil error:nil];
    }
    
    if ([fm moveItemAtPath:filename toPath:[romsPath stringByAppendingPathComponent:[filename lastPathComponent]] error:nil]) {
        [[NSNotificationCenter defaultCenter] postNotificationName:@"UPDATE_ROMSET" object:nil];
    
    } else {
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Error!" message:@"The game is already present" preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"Close" style:UIAlertActionStyleCancel handler:nil]];
        
        [[[[UIApplication sharedApplication] keyWindow] rootViewController] presentViewController:alert animated:YES completion:nil];
    }
}

#pragma mark - NSURLConnectionDelegate

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
    [UIApplication sharedApplication].networkActivityIndicatorVisible = YES;
    
    expectedBytes = [response expectedContentLength];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
    [fileHandle seekToEndOfFile];
    [fileHandle writeData:data];
    
    progress = [fileHandle offsetInFile] / (float)expectedBytes;
    
    NSLog(@"Downloading: %i%%", (int)(progress * 100));
    
    if (self.progressLabel) {
        int roundedProgress = (int)(progress * 100);
        self.progressLabel.text = [NSString stringWithFormat:@"Downloading: %i%%", roundedProgress];
    }
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    //[ZAActivityBar showErrorWithStatus:@"Failed to download ROM, Connection Error" duration:5];
    [_delegate removeDownload:self];
}

- (NSCachedURLResponse *)connection:(NSURLConnection *)connection willCacheResponse:    (NSCachedURLResponse *)cachedResponse {
    return nil;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        if (self.progressLabel)
            self.progressLabel.text = [NSString stringWithFormat:@"Opening"];
    });
    
    //NSError * error = nil;
    
    [fileHandle closeFile];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [_delegate removeDownload:self];
        [self addGame:escapedPath.path];
    });
}

@end

@implementation RomDownloadManager

+ (id)sharedManager {
    static RomDownloadManager * sharedMyManager = nil;
    static dispatch_once_t onceToken;

    dispatch_once(&onceToken, ^{
        sharedMyManager = [[self alloc] init];
    });
    
    return sharedMyManager;
    
}

- (instancetype)init {
    if (self = [super init]) {
        _activeDownloads = [NSMutableArray new];
    }
    
    return self;
}

- (void)addRequest:(NSURLRequest *)request {
    RomDownload *newDownload = [[RomDownload alloc] initWithRequest:request delegate:self];
    
    [_activeDownloads addObject:newDownload];
    
    [UIApplication sharedApplication].idleTimerDisabled = YES;
}

- (void)removeDownload:(RomDownload *)download {
    if (![_activeDownloads containsObject:download]) {
        return;
    }
    
    [_activeDownloads removeObject:download];
    
    [download stop];
    
    if (_activeDownloads.count == 0) {
        [UIApplication sharedApplication].idleTimerDisabled = NO;
    }
}

@end
