//
//  GameScene.m
//  MAME4apple
//
//  Created by Les Bird on 10/3/16.
//  Copyright © 2016 Les Bird. All rights reserved.
//

#import "GameScene.h"
#import <GameController/GameController.h>
#import <CloudKit/CloudKit.h>
#include "driver.h"
#import "HeaderView.h"
#import "SearchVC.h"

#if TARGET_OS_TV
#define USE_TABLEVIEW 0
#define USE_TOUCH_CONTROLS 0
#define TABLE_INSET_WIDTH 100
#define TABLE_INSET_HEIGHT 50
#else
#define USE_TABLEVIEW 1
#define USE_TOUCH_CONTROLS 1
#define TABLE_INSET_WIDTH 0
#define TABLE_INSET_HEIGHT 0
#endif
#define USE_RENDERTHREAD 0

#define VERSION_STRING "V1.2"

extern void parse_cmdline(int argc, char **argv, int game_index, char *override_default_rompath);

SKSpriteNode *frameBufferNode;
SKMutableTexture *frameBuffer;
UINT32 *frameBufferData;

int game_bitmap_update;

int runState = 0; // 0 = front-end, 1 = launch game
int game_index = 0; // currently selected game

#define MAX_GAME_LIST 50
int gameListCount;
SKLabelNode *gameList[MAX_GAME_LIST];
SKLabelNode *gameListDesc[MAX_GAME_LIST];
SKLabelNode *versionLabel;
SKNode *gameListNode;
SKLabelNode *coinButtonLabel;
SKLabelNode *startButtonLabel;
SKLabelNode *exitButtonLabel;
SKNode *buttonLayerNode;

SKShapeNode *debugTouchSprite;

#if USE_TOUCH_CONTROLS
UIButton *buttonCoin;
UIButton *buttonStart;
UIButton *buttonExit;
UIButton *buttonAction1;
UIButton *buttonAction2;
UIButton *buttonAction3;
UIButton *buttonAction4;
int actionButtonY[4] = {300, 400, 500, 600};
// onscreen joystick
SKShapeNode *onscreenJoystickLeft;
SKShapeNode *onscreenNubLeft;
CGPoint onscreenJoystickLeftAnchor;
#endif

UINT32 onscreenButton[ONSCREEN_BUTTON_MAX];

BOOL coinButtonPressed;
BOOL startButtonPressed;
BOOL exitButtonPressed;

typedef struct GameDriverList
{
    struct GameDriver *gameDriver;
    int gameIndex;
    BOOL hasRom;
} GameDriverList_t;

int gameDriverCount;
int gameDriverROMCount;
GameDriverList_t *gameDriverList;
GameDriverList_t *gameDriverROMList;
NSArray *sortedGameList;

UITableView *gameDriverTableView;

bitmap_t *screen = nil;

CGSize viewSize;

double deltaTime;
double prevTime;

NSUInteger touchTapCount;

// front end vars
int selected_game = -1;
BOOL buttonPress;
SKLabelNode *gameCountLabel;

int sortMethod;

GameScene *myObjectSelf;

@implementation GameScene
{
}

-(NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    //return gameDriverCount;
    return gameDriverROMCount;
}

-(UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *MyIdentifier = @"CellIdentifier";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:MyIdentifier];
    if (cell == nil)
    {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle  reuseIdentifier:MyIdentifier];
    }
    GameDriverList_t *gameDriver = &gameDriverROMList[indexPath.row];
    if ((gameDriver->gameDriver->flags & (GAME_NOT_WORKING | NOT_A_DRIVER)) != 0 || gameDriver->hasRom == FALSE)
    {
        cell.textLabel.textColor = [UIColor redColor];
    }
    else
    {
        cell.textLabel.textColor = [UIColor blackColor];
    }
    cell.textLabel.text = [NSString stringWithUTF8String:gameDriver->gameDriver->description];
    cell.detailTextLabel.text = [NSString stringWithFormat:@"%s %s", gameDriver->gameDriver->year, gameDriver->gameDriver->manufacturer];
    return cell;
}

-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    int i = (int)indexPath.row;
    selected_game = i;
    NSLog(@"i=%d selected_game=%d name=%s", i, selected_game, gameDriverROMList[i].gameDriver->description);
    runState = 1;
}

//-(NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
//{
//    static NSString *string = nil;
//    if (string == nil)
//    {
//        string = [NSString stringWithFormat:@"MAME4apple %s 2016 by Les Bird (www.lesbird.com)", VERSION_STRING];
//    }
//    return string;
//}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    HeaderView *view = (HeaderView *)[tableView dequeueReusableHeaderFooterViewWithIdentifier:[HeaderView sectionIndentifier]];
    
    if (view == nil) {
        view = [HeaderView nibView];
    }
    
    [view configure:[NSString stringWithFormat:@"MAME4apple %s 2016 by Les Bird (www.lesbird.com)", VERSION_STRING] actionSearch:^{
        UIViewController *vc = [[[self view] window] rootViewController];
        
        [vc performSegueWithIdentifier:@"segueSearch" sender:vc];
    }];
    
    return view;
}

-(NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
    return [NSString stringWithFormat:@"%d games", gameDriverROMCount];
}

-(NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView
{
    return sectionIndexArray;
}

-(NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index
{
    //NSLog(@"section index=%ld %@", (long)index, (NSString *)sectionIndexArray[index]);
    [self jumpToSection:sectionIndexArray[index]];
    
    if (selected_game < 0) {
        return 0;
    }
    
    NSIndexPath *indexPath = [NSIndexPath indexPathForRow:selected_game inSection:0];
    [gameDriverTableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionMiddle];
    return selected_game;
}

NSMutableArray *sectionIndexArray;

-(void)setupSectionIndex
{
    NSMutableArray *indexArray = [[NSMutableArray alloc] init];
    [indexArray addObject:@"0"];
    [indexArray addObject:@"1"];
    [indexArray addObject:@"2"];
    [indexArray addObject:@"3"];
    [indexArray addObject:@"4"];
    [indexArray addObject:@"5"];
    [indexArray addObject:@"6"];
    [indexArray addObject:@"7"];
    [indexArray addObject:@"8"];
    [indexArray addObject:@"9"];
    [indexArray addObject:@"A"];
    [indexArray addObject:@"B"];
    [indexArray addObject:@"C"];
    [indexArray addObject:@"D"];
    [indexArray addObject:@"E"];
    [indexArray addObject:@"F"];
    [indexArray addObject:@"G"];
    [indexArray addObject:@"H"];
    [indexArray addObject:@"I"];
    [indexArray addObject:@"J"];
    [indexArray addObject:@"K"];
    [indexArray addObject:@"L"];
    [indexArray addObject:@"M"];
    [indexArray addObject:@"N"];
    [indexArray addObject:@"O"];
    [indexArray addObject:@"P"];
    [indexArray addObject:@"Q"];
    [indexArray addObject:@"R"];
    [indexArray addObject:@"S"];
    [indexArray addObject:@"T"];
    [indexArray addObject:@"U"];
    [indexArray addObject:@"V"];
    [indexArray addObject:@"W"];
    [indexArray addObject:@"X"];
    [indexArray addObject:@"Y"];
    [indexArray addObject:@"Z"];
    
    sectionIndexArray = indexArray;
}

#if USE_TOUCH_CONTROLS
-(void)coinButtonPressed
{
    NSLog(@"coinButtonPressed");
    coinButtonPressed = TRUE;
}

-(void)coinButtonReleased
{
    NSLog(@"coinButtonReleased");
    coinButtonPressed = FALSE;
}

-(void)startButtonPressed
{
    NSLog(@"startButtonPressed");
    startButtonPressed = TRUE;
}

-(void)startButtonReleased
{
    NSLog(@"startButtonReleased");
    startButtonPressed = FALSE;
}

-(void)exitButtonPressed
{
    NSLog(@"exitButtonPressed");
    exitButtonPressed = TRUE;
}

-(void)exitButtonReleased
{
    NSLog(@"exitButtonReleased");
    exitButtonPressed = FALSE;
}

-(void)action1ButtonPressed
{
    onscreenButton[0] = 1;
}

-(void)action1ButtonReleased
{
    onscreenButton[0] = 0;
}

-(void)action2ButtonPressed
{
    onscreenButton[1] = 1;
}

-(void)action2ButtonReleased
{
    onscreenButton[1] = 0;
}

-(void)action3ButtonPressed
{
    onscreenButton[2] = 1;
}

-(void)action3ButtonReleased
{
    onscreenButton[2] = 0;
}

-(void)action4ButtonPressed
{
    onscreenButton[3] = 1;
}

-(void)action4ButtonReleased
{
    onscreenButton[3] = 0;
}

-(UIButton *)createButton:(NSString *)title
{
    UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    [button setTitle:title forState:UIControlStateNormal];
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [button setTitleColor:[UIColor blackColor] forState:UIControlStateSelected];
    [button setBackgroundColor:[UIColor darkGrayColor]];
    [button setAlpha:0.5f];
    [[button layer] setBorderWidth:2];
    [[button layer] setBorderColor:[UIColor whiteColor].CGColor];
    return button;
}
#endif

int list_step = 40; // gap between lines in game list

- (void)didMoveToView:(SKView *)view
{
    [self initAndSortDriverArray];
    
#if !USE_TABLEVIEW
    gameListNode = [SKNode node];
    gameListNode.name = @"gamelistnode";
    [self addChild:gameListNode];
#endif

    viewSize = view.bounds.size;
    
    NSLog(@"view bounds=%f,%f", viewSize.width, viewSize.height);
    
#if USE_TABLEVIEW
    [self setupSectionIndex];
    CGRect insetbounds = CGRectInset(view.bounds, TABLE_INSET_WIDTH, TABLE_INSET_HEIGHT);
    gameDriverTableView = [[UITableView alloc] initWithFrame:insetbounds];
    gameDriverTableView.autoresizingMask = UIViewAutoresizingFlexibleHeight|UIViewAutoresizingFlexibleWidth;
    gameDriverTableView.sectionHeaderHeight = 64;
    gameDriverTableView.sectionFooterHeight = 64;
    gameDriverTableView.delegate = self;
    gameDriverTableView.dataSource = self;
    [self.view addSubview:gameDriverTableView];
#else
    int height = view.bounds.size.height;
    int width = view.bounds.size.width;
    
    float x = -(width / 2.5f);
    
    gameListCount = height / list_step;
    int list_height = gameListCount * list_step;
    for (int i = 0; i < gameListCount; i++)
    {
        float y = (list_height / 2) - (list_step / 2) - (i * list_step);
        gameList[i] = [SKLabelNode labelNodeWithText:@"game"];
        gameList[i].name = @"gamelistlabel";
        gameList[i].position = CGPointMake(x, y);
        gameList[i].horizontalAlignmentMode = SKLabelHorizontalAlignmentModeLeft;
        gameList[i].fontName = @"Courier-Bold";
        gameList[i].fontSize = 18;
        [gameListNode addChild:gameList[i]];
        y -= 15;
        gameListDesc[i] = [SKLabelNode labelNodeWithText:@"desc"];
        gameListDesc[i].name = @"gamelistdesc";
        gameListDesc[i].position = CGPointMake(x, y);
        gameListDesc[i].horizontalAlignmentMode = SKLabelHorizontalAlignmentModeLeft;
        gameListDesc[i].fontName = @"Courier-Bold";
        gameListDesc[i].fontSize = 12;
        gameListDesc[i].fontColor = [UIColor yellowColor];
        [gameListNode addChild:gameListDesc[i]];
    }
    
    [self computeFrameBufferScale:width height:height];
#endif
    
    [self initFrameBuffer];
    
#if USE_TABLEVIEW
    frameBufferNode.hidden = YES;
#else
    gameCountLabel = [SKLabelNode labelNodeWithText:@""];
    gameCountLabel.name = @"gamecountlabel";
    gameCountLabel.position = CGPointMake(-(width / 2) + 16, -(height / 2) + 0);
    gameCountLabel.horizontalAlignmentMode = SKLabelHorizontalAlignmentModeLeft;
    gameCountLabel.fontName = @"Courier-Bold";
    gameCountLabel.fontSize = 16;
    [gameListNode addChild:gameCountLabel];
    
    versionLabel = [SKLabelNode labelNodeWithText:[NSString stringWithFormat:@"MAME4apple %s 2016 by Les Bird (www.lesbird.com)", VERSION_STRING]];
    versionLabel.name = @"versionlabel";
    versionLabel.position = CGPointMake(0, -(height / 2) + 16);
    versionLabel.fontName = @"Courier-Bold";
    versionLabel.fontSize = 20;
    [gameListNode addChild:versionLabel];
#endif
    
    [self initOnScreenJoystick];
    
#if USE_TOUCH_CONTROLS
    [self initOnScreenButtons];
    //debugTouchSprite = [SKShapeNode shapeNodeWithCircleOfRadius:32];
    //debugTouchSprite.name = @"debugtouchsprite";
    //debugTouchSprite.fillColor = [UIColor whiteColor];
    //debugTouchSprite.hidden = YES;
    //[self addChild:debugTouchSprite];
#endif
    
    // for iCade support
    iCadeReaderView *control = [[iCadeReaderView alloc] initWithFrame:CGRectZero];
    [self.view addSubview:control];
    control.active = YES;
    control.delegate = self;
    
    self.backgroundColor = [UIColor blackColor];
    
    char *argv[] = {""};
    parse_cmdline(0, argv, game_index, nil);
    
    [self checkAvailableROMs:NO];
    
#if USE_TABLEVIEW
    [gameDriverTableView reloadData];
#else
    [self updateGameList];
#endif
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateRoms) name:@"UPDATE_ROMSET" object:nil];
    
    myObjectSelf = self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

-(void)initAndSortDriverArray
{
    // count number of games
    if (gameDriverCount == 0)
    {
        gameDriverCount = 0;
        while (drivers[gameDriverCount] != 0)
        {
            gameDriverCount++;
        }
        
        // allocate game list array
        gameDriverList = (GameDriverList_t *)malloc(sizeof(GameDriverList_t) * gameDriverCount);
        
        for (int i = 0; i < gameDriverCount; i++)
        {
            gameDriverList[i].gameDriver = (struct GameDriver *)drivers[i];
            gameDriverList[i].gameIndex = i;
            gameDriverList[i].hasRom = FALSE;
        }
    }
    
    // sort array
    switch (sortMethod)
    {
        default:
            qsort(gameDriverList, gameDriverCount, sizeof(GameDriverList_t), sortByDesc);
            break;
        case 1:
            qsort(gameDriverList, gameDriverCount, sizeof(GameDriverList_t), sortByManufacturer);
            break;
    }
    
    //#if USE_TABLEVIEW
    //    [gameDriverTableView reloadData];
    //#endif
}

extern const char *getROMpath();

- (void)updateRoms {
    [self checkAvailableROMs:YES];
    selected_game = -1;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [gameDriverTableView reloadData];
    });
}

-(void)checkAvailableROMs:(BOOL)update
{
    int count = 0;
    NSString *romsPath = [NSHomeDirectory() stringByAppendingPathComponent:@"Documents/roms"];
    
    BOOL bIsDir;
    for (int i = 0; i < gameDriverCount; i++)
    {
        NSString *path = [NSString stringWithUTF8String:((update == NO) ? getROMpath() : [romsPath cStringUsingEncoding:NSUTF8StringEncoding])];
        path = [path stringByAppendingPathComponent:[NSString stringWithUTF8String:gameDriverList[i].gameDriver->name]];
        path = [path stringByAppendingPathExtension:@"zip"];
        if ([[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&bIsDir])
        {
            if ((gameDriverList[i].gameDriver->flags & (GAME_NOT_WORKING | NOT_A_DRIVER)) == 0)
            {
                //NSLog(@"ROM %s exists", gameDriverList[i].gameDriver->name);
                gameDriverList[i].hasRom = TRUE;
                count++;
            }
        }
        else
        {
            //NSLog(@"ROM %s does not exist", gameDriverList[i].gameDriver->name);
            gameDriverList[i].hasRom = FALSE;
        }
    }
    // we want to show only games with existing ROMs
    int n = 0;
    gameDriverROMList = (GameDriverList_t *)malloc(sizeof(GameDriverList_t) * count);
    for (int i = 0; i < gameDriverCount; i++)
    {
        if (gameDriverList[i].hasRom)
        {
            gameDriverROMList[n].gameDriver = gameDriverList[i].gameDriver;
            gameDriverROMList[n].gameIndex = gameDriverList[i].gameIndex;
            gameDriverROMList[n].hasRom = TRUE;
            n++;
        }
    }
    gameDriverROMCount = count;
}

-(void)initOnScreenJoystick
{
#if USE_TOUCH_CONTROLS
    // left side joystick
    onscreenJoystickLeftAnchor = CGPointMake(-256, -256);
    onscreenJoystickLeft = [SKShapeNode shapeNodeWithCircleOfRadius:64];
    onscreenJoystickLeft.name = @"joystickleft";
    onscreenJoystickLeft.position = onscreenJoystickLeftAnchor;
    onscreenJoystickLeft.fillColor = [UIColor darkGrayColor];
    onscreenJoystickLeft.alpha = 0.2f;
    [self addChild:onscreenJoystickLeft];
    
    onscreenNubLeft = [SKShapeNode shapeNodeWithCircleOfRadius:32];
    onscreenNubLeft.name = @"nubleft";
    onscreenNubLeft.fillColor = [UIColor redColor];
    [onscreenJoystickLeft addChild:onscreenNubLeft];
    
    onscreenJoystickLeft.hidden = YES;
#endif
}

-(void)initOnScreenButtons
{
#if USE_TOUCH_CONTROLS
    int buttonWidth = 100;
    int buttonHeight = 90;
    int buttonHeight2 = 50;
    
    if ([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPhone)
    {
        buttonWidth = 50;
        buttonHeight = 50;
        buttonHeight2 = 50;
        actionButtonY[0] = 50;
        actionButtonY[1] = 110;
        actionButtonY[2] = 170;
        actionButtonY[3] = 230;
    }
    
    buttonCoin = [self createButton:@"COIN"];
    [buttonCoin addTarget:self action:@selector(coinButtonPressed) forControlEvents:UIControlEventTouchDown];
    [buttonCoin addTarget:self action:@selector(coinButtonReleased) forControlEvents:UIControlEventTouchUpInside];
    buttonCoin.frame = CGRectMake(0, 10, buttonWidth, buttonHeight2);
    [self.view addSubview:buttonCoin];
    
    buttonStart = [self createButton:@"START"];
    [buttonStart addTarget:self action:@selector(startButtonPressed) forControlEvents:UIControlEventTouchDown];
    [buttonStart addTarget:self action:@selector(startButtonReleased) forControlEvents:UIControlEventTouchUpInside];
    buttonStart.frame = CGRectMake(viewSize.width - buttonWidth, 10, buttonWidth, buttonHeight2);
    [self.view addSubview:buttonStart];
    
    buttonExit = [self createButton:@"EXIT"];
    [buttonExit addTarget:self action:@selector(exitButtonPressed) forControlEvents:UIControlEventTouchDown];
    [buttonExit addTarget:self action:@selector(exitButtonReleased) forControlEvents:UIControlEventTouchUpInside];
    buttonExit.frame = CGRectMake(0, viewSize.height - (buttonHeight2 + 10), buttonWidth, buttonHeight2);
    [self.view addSubview:buttonExit];
    
    buttonAction1 = [self createButton:@"BUTTON1"];
    [buttonAction1 addTarget:self action:@selector(action1ButtonPressed) forControlEvents:UIControlEventTouchDown];
    [buttonAction1 addTarget:self action:@selector(action1ButtonReleased) forControlEvents:UIControlEventTouchUpInside];
    buttonAction1.frame = CGRectMake(viewSize.width - buttonWidth, viewSize.height - actionButtonY[0], buttonWidth, buttonHeight);
    [self.view addSubview:buttonAction1];
    
    buttonAction2 = [self createButton:@"BUTTON2"];
    [buttonAction2 addTarget:self action:@selector(action2ButtonPressed) forControlEvents:UIControlEventTouchDown];
    [buttonAction2 addTarget:self action:@selector(action2ButtonReleased) forControlEvents:UIControlEventTouchUpInside];
    buttonAction2.frame = CGRectMake(viewSize.width - buttonWidth, viewSize.height - actionButtonY[1], buttonWidth, buttonHeight);
    [self.view addSubview:buttonAction2];
    
    buttonAction3 = [self createButton:@"BUTTON3"];
    [buttonAction3 addTarget:self action:@selector(action3ButtonPressed) forControlEvents:UIControlEventTouchDown];
    [buttonAction3 addTarget:self action:@selector(action3ButtonReleased) forControlEvents:UIControlEventTouchUpInside];
    buttonAction3.frame = CGRectMake(viewSize.width - buttonWidth, viewSize.height - actionButtonY[2], buttonWidth, buttonHeight);
    [self.view addSubview:buttonAction3];
    
    buttonAction4 = [self createButton:@"BUTTON4"];
    [buttonAction4 addTarget:self action:@selector(action4ButtonPressed) forControlEvents:UIControlEventTouchDown];
    [buttonAction4 addTarget:self action:@selector(action4ButtonReleased) forControlEvents:UIControlEventTouchUpInside];
    buttonAction4.frame = CGRectMake(viewSize.width - buttonWidth, viewSize.height - actionButtonY[3], buttonWidth, buttonHeight);
    [self.view addSubview:buttonAction4];
    
    [self handleOnscreenButtonsEnable:NO];
#endif
}

// iCade support
CGPoint iCadeStickCenter;
BOOL iCadeButtonState[ICADEBUTTON_MAX];
BOOL iCadeDetected;

- (void)setState:(BOOL)state forButton:(iCadeState)button
{
    CGPoint center =  iCadeStickCenter;
    const CGFloat offset = 50.0f;
    
    switch (button) {
        case iCadeButtonA:
            iCadeButtonState[ICADEBUTTON_A] = state;
            break;
        case iCadeButtonB:
            iCadeButtonState[ICADEBUTTON_B] = state;
            break;
        case iCadeButtonC:
            iCadeButtonState[ICADEBUTTON_C] = state;
            break;
        case iCadeButtonD:
            iCadeButtonState[ICADEBUTTON_D] = state;
            break;
        case iCadeButtonE:
            iCadeButtonState[ICADEBUTTON_E] = state;
            break;
        case iCadeButtonF:
            iCadeButtonState[ICADEBUTTON_F] = state;
            break;
        case iCadeButtonG:
            iCadeButtonState[ICADEBUTTON_G] = state;
            break;
        case iCadeButtonH:
            iCadeButtonState[ICADEBUTTON_H] = state;
            break;
            
        case iCadeJoystickUp:
            if (state) {
                center.y -= offset;
            } else {
                center.y += offset;
            }
            break;
        case iCadeJoystickRight:
            if (state) {
                center.x += offset;
            } else {
                center.x -= offset;
            }
            break;
        case iCadeJoystickDown:
            if (state) {
                center.y += offset;
            } else {
                center.y -= offset;
            }
            break;
        case iCadeJoystickLeft:
            if (state) {
                center.x -= offset;
            } else {
                center.x += offset;
            }
            break;
            
        default:
            break;
    }
    iCadeStickCenter = center;
    iCadeDetected = TRUE;
    
    //NSLog(@"setState state=%d button=%d", state, button);
}

- (void)buttonDown:(iCadeState)button
{
    [self setState:YES forButton:button];
}

- (void)buttonUp:(iCadeState)button
{
    [self setState:NO forButton:button];
}
// iCade support end

// sorting functions
int sortByDesc(const void *game1, const void *game2)
{
    const GameDriverList_t *ptr1 = (GameDriverList_t *)game1;
    const GameDriverList_t *ptr2 = (GameDriverList_t *)game2;
    return strcmp(ptr1->gameDriver->description, ptr2->gameDriver->description);
}

int sortByName(const void *game1, const void *game2)
{
    const GameDriverList_t *ptr1 = (GameDriverList_t *)game1;
    const GameDriverList_t *ptr2 = (GameDriverList_t *)game2;
    return strcmp(ptr1->gameDriver->name, ptr2->gameDriver->name);
}

int sortByManufacturer(const void *game1, const void *game2)
{
    const GameDriverList_t *ptr1 = (GameDriverList_t *)game1;
    const GameDriverList_t *ptr2 = (GameDriverList_t *)game2;
    return strcmp(ptr1->gameDriver->manufacturer, ptr2->gameDriver->manufacturer);
}

// C function that calls into Objective-C function to allocate a frame buffer
void objc_alloc_framebuffer(int width, int height, int depth, int attributes, int orientation)
{
    [myObjectSelf alloc_frame_buffer:width :height :depth :attributes :orientation];
}

SKSpriteNode *testSpriteNode;
SKSpriteNode *testSpriteNode2;

UINT32 gameScreenWidth;
UINT32 gameScreenHeight;

#define TRIPLE_BUFFER 0
#if TRIPLE_BUFFER
// try a triple buffer approach - result did not work as expected
#define NUM_FRAMEBUFFERS 3
bitmap_t *frameBufferArray[NUM_FRAMEBUFFERS] = { nil };
int currentFrameIndex = 0;
int renderFrameIndex = 0;
#endif

-(void)initFrameBuffer
{
    UINT32 bufferWidth = 1024;
    UINT32 bufferHeight = 1024;
    
#if TRIPLE_BUFFER
    NSLog(@"using triple buffer");
    for (int i = 0; i < NUM_FRAMEBUFFERS; i++)
    {
        frameBufferArray[i] = (bitmap_t *)malloc(sizeof(bitmap_t));
        frameBufferArray[i]->bitmap = osd_alloc_bitmap(bufferWidth, bufferHeight, 32);
    }
    
    [self flip];
    [self renderFlip];
#else
    NSLog(@"using single buffer");
    screen = (bitmap_t *)malloc(sizeof(bitmap_t));
    screen->bitmap = osd_alloc_bitmap(bufferWidth, bufferHeight, 32);
    frameBufferData = (UINT32 *)screen->bitmap->_private;
#endif
    
    long w = screen->bitmap->line[1] - screen->bitmap->line[0];
    long h = bufferHeight;
    
    //NSLog(@"screenSize=%d,%d gameScreenSize=%d,%d frameBufferSize=%ld,%ld", screenWidth, screenHeight, width, height, w, h);
    
    UINT32 w4 = (UINT32)w / 4;
    
    NSLog(@"textureSize=%d,%d", w4, (UINT32)h);
    frameBuffer = [SKMutableTexture mutableTextureWithSize:CGSizeMake(w4, h)];
    frameBufferNode = [SKSpriteNode spriteNodeWithTexture:frameBuffer];
    frameBufferNode.name = @"framebuffernode";
    frameBufferNode.position = CGPointMake(0, 32);
    [self addChild:frameBufferNode];
}

// point to next frame buffer - called from ios_blit
-(void)flip
{
#if TRIPLE_BUFFER
    screen = frameBufferArray[currentFrameIndex];
    currentFrameIndex = (currentFrameIndex + 1) % NUM_FRAMEBUFFERS;
#endif
}

void objc_flip()
{
    if (myObjectSelf != nil)
    {
        [myObjectSelf flip];
    }
}

// point to next frame buffer - called from update modifyPixelDataWithBlock
-(void)renderFlip
{
#if TRIPLE_BUFFER
    frameBufferData = (UINT32 *)frameBufferArray[renderFrameIndex]->bitmap->_private;
    renderFrameIndex = (renderFrameIndex + 1) % NUM_FRAMEBUFFERS;
#endif
    game_bitmap_update = 0;
}

// allocate a frame buffer for the emulated game
-(void)alloc_frame_buffer:(int)width :(int)height :(int)depth :(int)attributes :(int)orientation
{
    NSLog(@"alloc_frame_buffer width=%d height=%d", width, height);
    
    gameScreenWidth = width;
    gameScreenHeight = height;
    
    [self computeFrameBufferScale:gameScreenWidth height:gameScreenHeight];
    
    // run the render loop in another thread (experimental and not needed)
    //[NSThread detachNewThreadSelector:@selector(renderLoop) toTarget:self withObject:nil];
    
    frameBufferNode.hidden = NO;
}

// free the game frame buffer
-(void)free_frame_buffer
{
    osd_clearbitmap(screen->bitmap);
    game_bitmap_update = 1; // force a texture update
    
    frameBufferNode.hidden = YES;
}

// fill the frame buffer with debug colors
void fillBufferData(UINT32 *buf, int width, int height)
{
    NSLog(@"fillBufferData()");
    for (int i = 0; i < height; i++)
    {
        int n = i * width;
        for (int j = 0; j < width; j++)
        {
            UINT32 c = 0; //0xFF0000FF;
            if (i == 0 || i == height - 1 || j == 0 || j == width - 1)
            {
                c = 0; // 0x20202020;
            }
            buf[n + j] = c;
        }
    }
}

// scale to fit screen orientation
-(void)computeFrameBufferScale:(UINT32)width height:(UINT32)height
{
    BOOL aspecthx2 = NO;
    
    UINT32 w = width;
    UINT32 h = height;
    
    //NSLog(@"computeFrameBufferScale w=%d h=%d sizex=%d sizey=%d minx=%d miny=%d maxx=%d maxy=%d", w, h,
    //      Machine->scrbitmap->width, Machine->scrbitmap->height,
    //      Machine->drv->default_visible_area.min_x, Machine->drv->default_visible_area.min_y,
    //      Machine->drv->default_visible_area.max_x, Machine->drv->default_visible_area.max_y);
    
    if (Machine != nil && Machine->drv != nil)
    {
        // handle oddball aspect ratios like Blasteroids
        if ((Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_1_2) != 0)
        {
            aspecthx2 = YES;
            h *= 2;
        }
    }
    
    if (viewSize.height < viewSize.width) // landscape
    {
        // fit to height
        float scaley = (float)viewSize.height / ((float)h + 32);
        frameBufferNode.yScale = -scaley; // flip the Y since OpenGL inverts the texture data
        frameBufferNode.xScale = scaley;
    }
    else
    {
        // fit to width
        float scalex = (float)viewSize.width / ((float)w + 32);
        frameBufferNode.yScale = -scalex; // flip the Y since OpenGL inverts the texture data
        frameBufferNode.xScale = scalex;
    }
    
    if (aspecthx2)
    {
        frameBufferNode.yScale *= 2;
    }
    
#if !USE_TABLEVIEW
    float x = viewSize.width / 2;
    float y = viewSize.height / 2;
    
    for (int i = 0; i < gameListCount; i++)
    {
        float lx = viewSize.width / 2.5f;
        
        float ly1 = gameList[i].position.y;
        gameList[i].position = CGPointMake(-lx, ly1);
        float ly2 = gameListDesc[i].position.y;
        gameListDesc[i].position = CGPointMake(-lx, ly2);
        if (ly1 < -(y - 32))
        {
            gameList[i].hidden = YES;
            gameListDesc[i].hidden = YES;
        }
        else
        {
            gameList[i].hidden = NO;
            gameListDesc[i].hidden = NO;
        }
    }
#endif
    
#if USE_TOUCH_CONTROLS
    int buttonWidth = 100;
    int buttonHeight = 90;
    int buttonHeight2 = 50;
    if ([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPhone)
    {
        buttonWidth = 50;
        buttonHeight = 50;
        buttonHeight2 = 50;
        actionButtonY[0] = 50;
        actionButtonY[1] = 110;
        actionButtonY[2] = 170;
        actionButtonY[3] = 230;
    }
    buttonCoin.frame = CGRectMake(0, 10, buttonWidth, buttonHeight2);
    buttonStart.frame = CGRectMake(viewSize.width - buttonWidth, 10, buttonWidth, buttonHeight2);
    buttonExit.frame = CGRectMake(0, viewSize.height - (buttonHeight2 + 10), buttonWidth, buttonHeight2);
    buttonAction1.frame = CGRectMake(viewSize.width - buttonWidth, viewSize.height - actionButtonY[0], buttonWidth, buttonHeight);
    buttonAction2.frame = CGRectMake(viewSize.width - buttonWidth, viewSize.height - actionButtonY[1], buttonWidth, buttonHeight);
    buttonAction3.frame = CGRectMake(viewSize.width - buttonWidth, viewSize.height - actionButtonY[2], buttonWidth, buttonHeight);
    buttonAction4.frame = CGRectMake(viewSize.width - buttonWidth, viewSize.height - actionButtonY[3], buttonWidth, buttonHeight);
#endif
    
#if !USE_TABLEVIEW
    gameCountLabel.position = CGPointMake(-x + 16, -y + 16);
    versionLabel.position = CGPointMake(0, -y + 16);
#endif
}

CGPoint CGPointClamp(CGPoint p, float range)
{
    float dx = p.x;
    float dy = p.y;
    float dist = sqrtf((dx * dx) + (dy * dy));
    if (dist > range)
    {
        dx /= dist;
        dy /= dist;
        dx *= range;
        dy *= range;
    }
    return CGPointMake(dx, dy);
}

-(void)handleOnscreenJoystickAnchor:(CGPoint)touchPos
{
#if USE_TOUCH_CONTROLS
    if (touchPos.x < 0)
    {
        onscreenJoystickLeftAnchor = touchPos;
        onscreenJoystickLeft.position = touchPos;
        onscreenNubLeft.position = CGPointZero;
        onscreenJoystickLeft.hidden = NO;
    }
#endif
}

-(void)handleOnscreenJoystickMove:(CGPoint)touchPos
{
#if USE_TOUCH_CONTROLS
    if (touchPos.x < 0)
    {
        CGPoint offset = CGPointMake(touchPos.x - onscreenJoystickLeftAnchor.x, touchPos.y - onscreenJoystickLeftAnchor.y);
        offset = CGPointClamp(offset, 64);
        onscreenNubLeft.position = offset;
        touchInputX = offset.x / 8.0f;
        touchInputY = offset.y / 8.0f;
    }
#endif
}

-(void)handleOnscreenJoystickOff:(CGPoint)touchPos
{
#if USE_TOUCH_CONTROLS
    if (touchPos.x < 0)
    {
        onscreenJoystickLeft.hidden = YES;
    }
#endif
}

extern BOOL buttonState;

void OnScreenButtonsEnable(BOOL on)
{
    if (on)
    {
        [myObjectSelf performSelectorOnMainThread:@selector(handleOnscreenButtonsOn) withObject:nil waitUntilDone:YES];
    }
    else
    {
        [myObjectSelf performSelectorOnMainThread:@selector(handleOnscreenButtonsOff) withObject:nil waitUntilDone:YES];
    }
}

-(void)handleOnscreenButtonsOn
{
    [self handleOnscreenButtonsEnable:YES];
}

-(void)handleOnscreenButtonsOff
{
    [self handleOnscreenButtonsEnable:NO];
}

-(void)handleOnscreenButtonsEnable:(BOOL)on
{
#if USE_TOUCH_CONTROLS
    BOOL hidden = (on ? NO : YES);
    {
        NSLog(@"handleOnscreenButtonsEnable:%d", on);
        [buttonCoin setHidden:hidden];
        [buttonStart setHidden:hidden];
        [buttonExit setHidden:hidden];
        [buttonAction1 setHidden:hidden];
        [buttonAction2 setHidden:hidden];
        [buttonAction3 setHidden:hidden];
        [buttonAction4 setHidden:hidden];
    }
    buttonState = on;
#endif
}

int touchInputY;
int touchInputX;
int touchSens = 25;
CGPoint startTouchPos;

-(void)touchDownAtPoint:(CGPoint)pos
{
    startTouchPos = pos;
    [self handleOnscreenJoystickAnchor:pos];
}

-(void)touchMovedToPoint:(CGPoint)pos
{
    [self handleOnscreenJoystickMove:pos];
    startTouchPos = pos;
}

-(void)touchUpAtPoint:(CGPoint)pos
{
    touchInputX = 0;
    touchInputY = 0;
    [self handleOnscreenJoystickOff:pos];
}

-(void)touchTapAtPoint:(CGPoint)pos
{
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches)
    {
        [self touchDownAtPoint:[t locationInNode:self]];
    }
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches)
    {
        [self touchMovedToPoint:[t locationInNode:self]];
    }
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches)
    {
        [self touchUpAtPoint:[t locationInNode:self]];
        touchTapCount = t.tapCount;
#if USE_TOUCH_CONTROLS
        if (touchTapCount > 0)
        {
            [self touchTapAtPoint:[t locationInNode:self]];
        }
#endif
    }
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches)
    {
        [self touchUpAtPoint:[t locationInNode:self]];
    }
}

-(BOOL)next_game:(int)count
{
    if (!buttonPress)
    {
        selected_game += count;
        selected_game = (selected_game >= gameDriverROMCount) ? gameDriverROMCount - 1 : selected_game;
        return YES;
    }
    return NO;
}

-(BOOL)prev_game:(int)count
{
    if (!buttonPress)
    {
        selected_game -= count;
        selected_game = (selected_game > 0) ? selected_game : 0;
        return YES;
    }
    return NO;
}

-(BOOL)next_section
{
    if (buttonPress)
    {
        return NO;
    }
    char c = 0;
    if (sortMethod == 0)
    {
        c = gameDriverROMList[selected_game].gameDriver->description[0];
    }
    else
    {
        c = gameDriverROMList[selected_game].gameDriver->manufacturer[0];
    }
    for (int i = selected_game; i < gameDriverROMCount; i++)
    {
        if (sortMethod == 0)
        {
            if (gameDriverROMList[i].gameDriver->description[0] != c)
            {
                //NSLog(@"jumping from %c to %c", c, gameDriverList[i].gameDriver->description[0]);
                selected_game = i;
                return YES;
            }
        }
        else
        {
            if (gameDriverROMList[i].gameDriver->manufacturer[0] != c)
            {
                //NSLog(@"jumping from %c to %c", c, gameDriverList[i].gameDriver->description[0]);
                selected_game = i;
                return YES;
            }
        }
    }
    selected_game = gameDriverCount - 1;
    return YES;
}

-(BOOL)prev_section
{
    if (buttonPress)
    {
        return NO;
    }
    char c = 0;
    if (sortMethod == 0)
    {
        c = gameDriverROMList[selected_game].gameDriver->description[0];
    }
    else
    {
        c = gameDriverROMList[selected_game].gameDriver->manufacturer[0];
    }
    for (int i = selected_game; i >= 0; i--)
    {
        if (sortMethod == 0)
        {
            if (gameDriverROMList[i].gameDriver->description[0] != c)
            {
                //NSLog(@"jumping from %c to %c", c, gameDriverList[i].gameDriver->description[0]);
                selected_game = i;
                return YES;
            }
        }
        else
        {
            if (gameDriverROMList[i].gameDriver->manufacturer[0] != c)
            {
                //NSLog(@"jumping from %c to %c", c, gameDriverList[i].gameDriver->description[0]);
                selected_game = i;
                return YES;
            }
        }
    }
    selected_game = 0;
    return YES;
}

-(void)jumpToSection:(NSString *)str
{
    const char *s = [str UTF8String];
    char c = s[0];
    if (sortMethod == 0) // sort by game name
    {
        for (int i = 0; i < gameDriverROMCount; i++)
        {
            if (gameDriverROMList[i].gameDriver->description[0] >= c)
            {
                selected_game = i;
                return;
            }
        }
    }
    else
    {
        for (int i = 0; i < gameDriverROMCount; i++)
        {
            if (gameDriverROMList[i].gameDriver->manufacturer[0] >= c)
            {
                selected_game = i;
                return;
            }
        }
    }
}

// called 60 times/sec
-(void)update:(CFTimeInterval)currentTime
{
    deltaTime = currentTime - prevTime;
    if (!CGSizeEqualToSize(viewSize, self.view.bounds.size))
    {
        // screen was rotated
        [self screenWasRotated];
    }
    
    if (runState == 1) // run the game
    {
        if (gameDriverTableView != nil)
        {
            [gameDriverTableView setHidden:YES];
        }
        
        game_index = gameDriverROMList[selected_game].gameIndex;
        [NSThread detachNewThreadSelector:@selector(runGameThread) toTarget:self withObject:nil];
        runState = 2;
    }
#if !USE_RENDERTHREAD
    //if (runState == 2) // rendering the frames
    {
#if !TRIPLE_BUFFER
        if (game_bitmap_update == 1)
#endif
        {
            if (frameBufferData != nil)
            {
                if (frameBuffer != nil)
                {
                    [frameBuffer modifyPixelDataWithBlock:^(void *pixelData, size_t lengthInBytes) {
                        memcpy(pixelData, frameBufferData, lengthInBytes);
                        [self renderFlip];
                    }];
                }
            }
        }
    }
#endif
    if (runState == 3) // clean up and return to front end
    {
        if (gameDriverTableView != nil)
        {
            [gameDriverTableView setHidden:NO];
        }
        [self handleOnscreenButtonsEnable:NO];
        
        [self free_frame_buffer];
        runState = 0;
    }
    
    if (runState == 0) // front end - select a game to play from the list
    {
        [self handleFrontEnd];
    }
    
    prevTime = currentTime;
}

-(void)handleFrontEnd
{
    int pressCount = 0;
    BOOL update_list = NO;
    NSArray *controllerList = [GCController controllers];
    if (controllerList.count > 0)
    {
        for (int i = 0; i < controllerList.count; i++)
        {
            GCController *controller = (GCController *)[controllerList objectAtIndex:i];
            if (controller != nil)
            {
                if (controller.playerIndex != i)
                {
                    controller.playerIndex = i;
                }
                if (controller.gamepad.dpad.down.pressed)
                {
                    if (controller.gamepad.rightShoulder.pressed)
                    {
                        update_list = [self next_game:9];
                    }
                    else
                    {
                        update_list = [self next_game:1];
                    }
                    pressCount++;
                }
                else if (controller.gamepad.dpad.up.pressed)
                {
                    if (controller.gamepad.rightShoulder.pressed)
                    {
                        update_list = [self prev_game:9];
                    }
                    else
                    {
                        update_list = [self prev_game:1];
                    }
                    pressCount++;
                }
                else if (controller.gamepad.dpad.right.pressed)
                {
                    update_list = [self next_section];
                    pressCount++;
                }
                else if (controller.gamepad.dpad.left.pressed)
                {
                    update_list = [self prev_section];
                    pressCount++;
                }
                else if (controller.gamepad.buttonY.pressed)
                {
                    if (!buttonPress)
                    {
                        sortMethod = (sortMethod == 0 ? 1 : 0);
                        [self initAndSortDriverArray];
                        buttonPress = YES;
                    }
                    pressCount++;
                }
                else if (controller.gamepad.buttonA.pressed)
                {
                    if (!buttonPress)
                    {
                        buttonPress = YES;
                        runState = 1;
                    }
                    pressCount++;
                }
            }
        }
    }
    
    // iCade controls
    else if (iCadeStickCenter.y > 0)
    {
        if (iCadeButtonState[ICADEBUTTON_H])
        {
            update_list = [self next_game:9];
        }
        else
        {
            update_list = [self next_game:1];
        }
        pressCount++;
    }
    else if (iCadeStickCenter.y < 0)
    {
        if (iCadeButtonState[ICADEBUTTON_H])
        {
            update_list = [self prev_game:9];
        }
        else
        {
            update_list = [self prev_game:1];
        }
        pressCount++;
    }
    else if (iCadeStickCenter.x > 0)
    {
        update_list = [self next_section];
        pressCount++;
    }
    else if (iCadeStickCenter.x < 0)
    {
        update_list = [self prev_section];
        pressCount++;
    }
    else if (iCadeButtonState[ICADEBUTTON_A])
    {
        if (!buttonPress)
        {
            buttonPress = YES;
            runState = 1;
        }
        pressCount++;
    }
    
    if (pressCount == 0)
    {
        buttonPress = NO;
    }
    
    
    if (update_list)
    {
#if USE_TABLEVIEW
        NSIndexPath *indexPath = [NSIndexPath indexPathForRow:selected_game inSection:0];
        NSInteger numRows = [self tableView:gameDriverTableView numberOfRowsInSection:0];
        if (indexPath.section == 0 && indexPath.row < numRows) {
            [gameDriverTableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionMiddle];
            int i = (int)indexPath.row;
            NSLog(@"indexRow=%d selectedGame=%d gameIndex=%d name=%s", i, selected_game, gameDriverROMList[i].gameIndex, gameDriverROMList[i].gameDriver->description);
        }
#else
        [self updateGameList];
#endif
        buttonPress = YES;
    }
}

-(void)screenWasRotated
{
    viewSize = self.view.bounds.size;
    [self computeFrameBufferScale:gameScreenWidth height:gameScreenHeight];
}

// experimental - not needed - put the render loop in it's own thread
-(void)renderLoop
{
    while (1)
    {
        if (runState == 2) // rendering the frames
        {
            if (game_bitmap_update == 1)
            {
                if (frameBufferData != nil)
                {
                    if (frameBuffer != nil)
                    {
                        [frameBuffer modifyPixelDataWithBlock:^(void *pixelData, size_t lengthInBytes) {
                            memcpy(pixelData, frameBufferData, lengthInBytes);
                            game_bitmap_update = 0;
                        }];
                    }
                }
            }
        }
    }
}

-(void)updateGameList
{
#if !USE_TABLEVIEW
    int center = gameListCount / 2;
    int top = center - selected_game;
    for (int i = 0; i < gameListCount; i++)
    {
        if (i < top)
        {
            gameList[i].text = @"";
            gameListDesc[i].text = @"";
            continue;
        }
        int n = selected_game + i - center;
        if (n < gameDriverROMCount)
        {
            struct GameDriver *game_driver = gameDriverROMList[n].gameDriver;
            gameList[i].text = [NSString stringWithFormat:@"%s", game_driver->description];
            if ((game_driver->flags & (GAME_NOT_WORKING | NOT_A_DRIVER)) != 0)
            {
                gameList[i].fontColor = [UIColor redColor];
            }
            else if (i == center)
            {
                gameList[i].fontColor = [UIColor whiteColor];
            }
            else
            {
                gameList[i].fontColor = [UIColor grayColor];
            }
            gameListDesc[i].text = [NSString stringWithFormat:@"%s %s", game_driver->year, game_driver->manufacturer];
        }
        else
        {
            gameList[i].text = @"";
            gameListDesc[i].text = @"";
        }
        gameListDesc[i].fontColor = [UIColor yellowColor];
    }
    
    gameCountLabel.text = [NSString stringWithFormat:@"%d", gameDriverCount];
#endif
}

// run the emulation
-(void)runGameThread
{
    NSLog(@"runGameThread: game_index=%d", game_index);
    
    gameListNode.hidden = YES;
    
    int res = run_game(game_index);
    
    gameListNode.hidden = NO;
    
    NSLog(@"run_game(%d) exited with code %d", game_index, res);
    
    runState = 3;
    // return to frontend
}

@end
