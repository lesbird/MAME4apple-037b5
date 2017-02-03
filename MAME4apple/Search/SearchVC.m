//
//  SearchVC.m
//  MAME4apple
//
//  Created by WhiteTiger "sgama la rete" on 04/12/2016.
//  Copyright © 2016 Les Bird. All rights reserved.
//

#import "SearchVC.h"
#import "RomDownloadManager.h"

@interface SearchVC () <UIWebViewDelegate>

@property (nonatomic, weak) IBOutlet UIWebView *webView;
@property (nonatomic, weak) IBOutlet UITextField *tfUrlField;

@end

@implementation SearchVC {
    NSString *lastURL;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [self setup];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)dealloc {
    // NOP
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
    NSString *urlString = @"http://www.google.com/search?q=MAME+Roms";
    
    [[self tfUrlField] setText:urlString];
    [[self webView] loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlString]]];
    
    lastURL = urlString;
}

#pragma mark - Private Methods

- (void)setup {
    [[self.webView scrollView] setBounces:NO];
}

#pragma mark - IBAction Methods

- (IBAction)actionGo:(id)sender {
    [self.view endEditing:YES];
    
    NSString *urlString = [[self tfUrlField] text];
    
    if (!([urlString hasPrefix:@"http://"] || [urlString hasPrefix:@"https://"])) {
        urlString = [NSString stringWithFormat:@"http://%@", urlString];
    }
    
    [[self webView] loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlString]]];
}

- (IBAction)actionClose:(id)sender {
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)actionBack:(id)sender {
    if ([[self webView] canGoBack]) {
        [[self webView] goBack];
    }
}

- (IBAction)actionForward:(id)sender {
    if ([[self webView] canGoForward]) {
        [[self webView] goForward];
    }
}

#pragma UIWebViewDelegate

- (void)webViewDidStartLoad:(UIWebView *)webView {
    NSString *newURL = webView.request.URL.absoluteString;
    
    if (![newURL isEqualToString:lastURL] && [newURL length] > 0) {
        self.tfUrlField.text = newURL;
        lastURL = newURL;
    }
}

- (void)webViewDidFinishLoad:(UIWebView *)webView {
    NSString *newURL = webView.request.URL.absoluteString;
    
    if (![newURL isEqualToString:lastURL] && [newURL length] > 0) {
        self.tfUrlField.text = newURL;
        lastURL = newURL;
    }
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType {
    NSString *fileExtension = request.URL.pathExtension.lowercaseString;
    
    if ([fileExtension isEqualToString:@"zip"]) {
        NSLog(@"Downloading %@", request.URL);
        
        // lastProgress = 0.0;
        // [ZAActivityBar showSuccessWithStatus:[NSString stringWithFormat:@"Downloading started: %@", request.URL.lastPathComponent] duration:5];
        
        NSURL *requestedURL = request.URL;
        NSURLRequest *theRequest = [NSURLRequest requestWithURL:requestedURL cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:60];
        
        [[RomDownloadManager sharedManager] addRequest:theRequest];
        
        [self actionClose:nil];
        
        return NO;
        
    } else {
        // dispatch_async(dispatch_get_main_queue(), ^{
        //     UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Error!" message:@"The file is not compatible." preferredStyle:UIAlertControllerStyleAlert];
        //     [alert addAction:[UIAlertAction actionWithTitle:@"Close" style:UIAlertActionStyleCancel handler:nil]];
        //
        //     [self presentViewController:alert animated:YES completion:nil];
        // });
    }
    
    return ![self.tfUrlField isFirstResponder];
}

@end
