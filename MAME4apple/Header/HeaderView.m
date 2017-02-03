//
//  HeaverView.m
//  MAME4apple
//
//  Created by WhiteTiger "sgama la rete" on 04/12/2016.
//  Copyright © 2016 Les Bird. All rights reserved.
//

#import "HeaderView.h"

typedef void (^CallBack)(void);

@interface HeaderView ()

@property (nonatomic, weak) IBOutlet UILabel *lblTitle;
@property (nonatomic, copy) CallBack callback;

@end

@implementation HeaderView

- (NSString *)reuseIdentifier {
    return [HeaderView sectionIndentifier];
}

+ (NSString *)sectionIndentifier {
    return [NSString stringWithFormat:@"%@.identifier", [HeaderView nibName]];
}

+ (instancetype)nibView {
    return [[[UINib nibWithNibName:[HeaderView nibName] bundle:nil] instantiateWithOwner:self options:nil] firstObject];
}

- (void)configure:(NSString *)title actionSearch:(CallBack)actionSearch {
    [self.lblTitle setText:title];
    self.callback = actionSearch;
}

- (void)setBackgroundColor:(UIColor *)backgroundColor {
    // NOP
    // Remove warning for: Setting the background color on UITableViewHeaderFooterView has been deprecated. Please use contentView.backgroundColor instead.
}

#pragma mark - Private Methods

+ (NSString *)nibName {
    return NSStringFromClass([HeaderView class]);
}

#pragma mark - IBOutlet Methods

- (IBAction)actionSearch:(id)sender {
    if (self.callback != nil) {
        self.callback();
    }
}

@end
