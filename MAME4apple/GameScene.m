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

#if TARGET_OS_TV
#define USE_TABLEVIEW 0
#else
#define USE_TABLEVIEW 1
#endif
#define USE_RENDERTHREAD 0
#define USE_TOUCH_CONTROLS 0

#define VERSION_STRING "V1.0"

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
SKShapeNode *onscreenJoystickLeft;
SKShapeNode *onscreenNubLeft;
CGPoint onscreenJoystickLeftAnchor;

SKShapeNode *onscreenJoystickRight;
SKShapeNode *onscreenNubRight;
CGPoint onscreenJoystickRightAnchor;

SKShapeNode *onscreenButtonSprite[ONSCREEN_BUTTON_MAX];
// button positions - percentage of screen width/height
float onscreenButtonX[ONSCREEN_BUTTON_MAX] = { 0.8f, 0.9f, 0.7f, 0.8f, 0.2f, 0.4f, 0.6f, 0.8f};
float onscreenButtonY[ONSCREEN_BUTTON_MAX] = { 0.7f, 0.5f, 0.5f, 0.3f, 0.8f, 0.8f, 0.8f, 0.8f};
float onscreenButtonSize = 40;

SKShapeNode *coinButtonSprite;
SKShapeNode *startButtonSprite;
SKShapeNode *exitButtonSprite;
SKShapeNode *sortButtonSprite;
#endif
UINT32 onscreenButton[ONSCREEN_BUTTON_MAX];

BOOL coinButtonPressed;
BOOL startButtonPressed;
BOOL exitButtonPressed;

typedef struct GameDriverList
{
    struct GameDriver *gameDriver;
    int gameIndex;
} GameDriverList_t;

int gameDriverCount;
GameDriverList_t *gameDriverList;
NSArray *sortedGameList;

UITableView *gameDriverTableView;

bitmap_t *screen = nil;

CGSize viewSize;

double deltaTime;
double prevTime;

#if USE_TOUCH_CONTROLS
// touch vars
float touchAdvTimer;
float touchAdvDelay = 0.1f;
#endif
NSUInteger touchTapCount;

// front end vars
int selected_game;
BOOL buttonPress;
SKLabelNode *gameCountLabel;

int sortMethod;

GameScene *myObjectSelf;

@implementation GameScene
{
}

-(NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return gameDriverCount;
}

-(UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *MyIdentifier = @"CellIdentifier";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:MyIdentifier];
    if (cell == nil)
    {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle  reuseIdentifier:MyIdentifier];
    }
    if ((gameDriverList[indexPath.row].gameDriver->flags & (GAME_NOT_WORKING | NOT_A_DRIVER)) != 0)
    {
        cell.textLabel.textColor = [UIColor redColor];
    }
    else
    {
        cell.textLabel.textColor = [UIColor blackColor];
    }
    cell.textLabel.text = [NSString stringWithUTF8String:gameDriverList[indexPath.row].gameDriver->description];
    cell.detailTextLabel.text = [NSString stringWithFormat:@"%s %s", gameDriverList[indexPath.row].gameDriver->year, gameDriverList[indexPath.row].gameDriver->manufacturer];
    return cell;
}

-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    selected_game = (int)indexPath.row;
    runState = 1;
}

-(NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
    static NSString *string = nil;
    if (string == nil)
    {
        string = [NSString stringWithFormat:@"MAME4apple %s 2016 by Les Bird (www.lesbird.com)", VERSION_STRING];
    }
    return string;
}

-(NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
    static NSString *string = nil;
    if (string == nil)
    {
        string = [NSString stringWithFormat:@"%d games", gameDriverCount];
    }
    return string;
}

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
    gameDriverTableView = [[UITableView alloc] initWithFrame:view.bounds];
    gameDriverTableView.autoresizingMask = UIViewAutoresizingFlexibleHeight|UIViewAutoresizingFlexibleWidth;
    gameDriverTableView.sectionHeaderHeight = 64;
    gameDriverTableView.sectionFooterHeight = 64;
    gameDriverTableView.delegate = self;
    gameDriverTableView.dataSource = self;
    [gameDriverTableView reloadData];
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
    [self updateGameList];
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
    
#if USE_TOUCH_CONTROLS
    [self initOnscreenControls];
#endif
    
#if 0
    debugTouchSprite = [SKShapeNode shapeNodeWithCircleOfRadius:32];
    debugTouchSprite.name = @"debugtouchsprite";
    debugTouchSprite.fillColor = [UIColor whiteColor];
    debugTouchSprite.hidden = YES;
    [self addChild:debugTouchSprite];
#endif
    
    // for iCade support
    iCadeReaderView *control = [[iCadeReaderView alloc] initWithFrame:CGRectZero];
    [self.view addSubview:control];
    control.active = YES;
    control.delegate = self;
    
    self.backgroundColor = [UIColor blackColor];
    
    char *argv[] = {""};
    parse_cmdline(0, argv, game_index, nil);
    
    myObjectSelf = self;
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
    
#if USE_TABLEVIEW
    [gameDriverTableView reloadData];
#endif
}

#if USE_TOUCH_CONTROLS
-(void)initOnscreenControls
{
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

    // right side joystick
    onscreenJoystickRightAnchor = CGPointMake(-256, -256);
    onscreenJoystickRight = [SKShapeNode shapeNodeWithCircleOfRadius:64];
    onscreenJoystickRight.name = @"joystickright";
    onscreenJoystickRight.position = onscreenJoystickRightAnchor;
    onscreenJoystickRight.fillColor = [UIColor darkGrayColor];
    onscreenJoystickRight.alpha = 0.2f;
    [self addChild:onscreenJoystickRight];
    
    onscreenNubRight = [SKShapeNode shapeNodeWithCircleOfRadius:32];
    onscreenJoystickLeft.name = @"nubright";
    onscreenNubRight.fillColor = [UIColor greenColor];
    [onscreenJoystickRight addChild:onscreenNubRight];
    
    onscreenJoystickRight.hidden = YES;

    // onscreen buttons
    CGPoint buttonPos;
    float w = viewSize.width / 2;
    float h = viewSize.height / 2;
    onscreenButtonSprite[ONSCREEN_BUTTON_A] = [SKShapeNode shapeNodeWithCircleOfRadius:onscreenButtonSize];
    onscreenButtonSprite[ONSCREEN_BUTTON_A].name = @"button_a";
    buttonPos = CGPointMake(w * onscreenButtonX[ONSCREEN_BUTTON_A], -h * onscreenButtonY[ONSCREEN_BUTTON_A]);
    onscreenButtonSprite[ONSCREEN_BUTTON_A].position = buttonPos;
    onscreenButtonSprite[ONSCREEN_BUTTON_A].fillColor = [UIColor greenColor];
    onscreenButtonSprite[ONSCREEN_BUTTON_A].alpha = 0.2f;
    [self addChild:onscreenButtonSprite[ONSCREEN_BUTTON_A]];

    onscreenButtonSprite[ONSCREEN_BUTTON_B] = [SKShapeNode shapeNodeWithCircleOfRadius:onscreenButtonSize];
    onscreenButtonSprite[ONSCREEN_BUTTON_B].name = @"button_b";
    buttonPos = CGPointMake(w * onscreenButtonX[ONSCREEN_BUTTON_B], -h * onscreenButtonY[ONSCREEN_BUTTON_B]);
    onscreenButtonSprite[ONSCREEN_BUTTON_B].position = buttonPos;
    onscreenButtonSprite[ONSCREEN_BUTTON_B].fillColor = [UIColor redColor];
    onscreenButtonSprite[ONSCREEN_BUTTON_B].alpha = 0.2f;
    [self addChild:onscreenButtonSprite[ONSCREEN_BUTTON_B]];
    
    onscreenButtonSprite[ONSCREEN_BUTTON_C] = [SKShapeNode shapeNodeWithCircleOfRadius:onscreenButtonSize];
    onscreenButtonSprite[ONSCREEN_BUTTON_C].name = @"button_c";
    buttonPos = CGPointMake(w * onscreenButtonX[ONSCREEN_BUTTON_C], -h * onscreenButtonY[ONSCREEN_BUTTON_C]);
    onscreenButtonSprite[ONSCREEN_BUTTON_C].position = buttonPos;
    onscreenButtonSprite[ONSCREEN_BUTTON_C].fillColor = [UIColor blueColor];
    onscreenButtonSprite[ONSCREEN_BUTTON_C].alpha = 0.2f;
    [self addChild:onscreenButtonSprite[ONSCREEN_BUTTON_C]];
    
    onscreenButtonSprite[ONSCREEN_BUTTON_D] = [SKShapeNode shapeNodeWithCircleOfRadius:onscreenButtonSize];
    onscreenButtonSprite[ONSCREEN_BUTTON_D].name = @"button_d";
    buttonPos = CGPointMake(w * onscreenButtonX[ONSCREEN_BUTTON_D], -h * onscreenButtonY[ONSCREEN_BUTTON_D]);
    onscreenButtonSprite[ONSCREEN_BUTTON_D].position = buttonPos;
    onscreenButtonSprite[ONSCREEN_BUTTON_D].fillColor = [UIColor yellowColor];
    onscreenButtonSprite[ONSCREEN_BUTTON_D].alpha = 0.2f;
    [self addChild:onscreenButtonSprite[ONSCREEN_BUTTON_D]];
    
    coinButtonSprite = [SKShapeNode shapeNodeWithCircleOfRadius:onscreenButtonSize];
    coinButtonSprite.name = @"coinbutton";
    buttonPos = CGPointMake(-w + 16, h - 16);
    coinButtonSprite.position = buttonPos;
    coinButtonSprite.fillColor = [UIColor greenColor];
    coinButtonSprite.alpha = 0.2f;
    [self addChild:coinButtonSprite];
    
    startButtonSprite = [SKShapeNode shapeNodeWithCircleOfRadius:onscreenButtonSize];
    startButtonSprite.name = @"startbutton";
    buttonPos = CGPointMake(w - 16, h - 16);
    startButtonSprite.position = buttonPos;
    startButtonSprite.fillColor = [UIColor greenColor];
    startButtonSprite.alpha = 0.2f;
    [self addChild:startButtonSprite];
    
    exitButtonSprite = [SKShapeNode shapeNodeWithCircleOfRadius:onscreenButtonSize];
    exitButtonSprite.name = @"exitbutton";
    buttonPos = CGPointMake(-w + 16, -h + 16);
    exitButtonSprite.position = buttonPos;
    exitButtonSprite.fillColor = [UIColor redColor];
    exitButtonSprite.alpha = 0.2f;
    [self addChild:exitButtonSprite];
    
    sortButtonSprite = [SKShapeNode shapeNodeWithCircleOfRadius:onscreenButtonSize];
    sortButtonSprite.name = @"sortbutton";
    buttonPos = CGPointMake(0, h - 16);
    sortButtonSprite.position = buttonPos;
    sortButtonSprite.fillColor = [UIColor yellowColor];
    sortButtonSprite.alpha = 0.2f;
    [self addChild:sortButtonSprite];
    
    [self handleOnscreenButtonsEnable:NO];
}
#endif

// iCade support
CGPoint iCadeStickCenter;
BOOL iCadeButtonState[ICADEBUTTON_MAX];

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

-(void)initFrameBuffer
{
    UINT32 bufferWidth = 1024;
    UINT32 bufferHeight = 1024;
    
    screen = (bitmap_t *)malloc(sizeof(bitmap_t));
    screen->bitmap = osd_alloc_bitmap(bufferWidth, bufferHeight, 32);
    frameBufferData = (UINT32 *)screen->bitmap->_private;
    
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

// allocate a frame buffer for the emulated game
-(void)alloc_frame_buffer:(int)width :(int)height :(int)depth :(int)attributes :(int)orientation
{
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
    CGPoint buttonPos;
    buttonPos = CGPointMake(x * onscreenButtonX[ONSCREEN_BUTTON_A], -y * onscreenButtonY[ONSCREEN_BUTTON_A]);
    onscreenButtonSprite[ONSCREEN_BUTTON_A].position = buttonPos;
    
    buttonPos = CGPointMake(x * onscreenButtonX[ONSCREEN_BUTTON_B], -y * onscreenButtonY[ONSCREEN_BUTTON_B]);
    onscreenButtonSprite[ONSCREEN_BUTTON_B].position = buttonPos;
    
    buttonPos = CGPointMake(x * onscreenButtonX[ONSCREEN_BUTTON_C], -y * onscreenButtonY[ONSCREEN_BUTTON_C]);
    onscreenButtonSprite[ONSCREEN_BUTTON_C].position = buttonPos;
    
    buttonPos = CGPointMake(x * onscreenButtonX[ONSCREEN_BUTTON_D], -y * onscreenButtonY[ONSCREEN_BUTTON_D]);
    onscreenButtonSprite[ONSCREEN_BUTTON_D].position = buttonPos;

    if (coinButtonSprite != nil)
    {
        coinButtonSprite.position = CGPointMake(-x + 16, y - 16);
    }
    if (startButtonSprite != nil)
    {
        startButtonSprite.position = CGPointMake(x - 16, y - 16);
    }
    if (exitButtonSprite != nil)
    {
        exitButtonSprite.position = CGPointMake(-x + 16, -y + 16);
    }
    if (sortButtonSprite != nil)
    {
        sortButtonSprite.position = CGPointMake(0, y - 16);
    }
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

#if USE_TOUCH_CONTROLS
-(void)handleOnscreenButtonsEnable:(BOOL)on
{
    for (int i = 0; i < ONSCREEN_BUTTON_MAX; i++)
    {
        if (onscreenButtonSprite[i] != nil)
        {
            onscreenButtonSprite[i].hidden = (on ? NO : YES);
        }
    }
    coinButtonSprite.hidden = (on ? NO : YES);
    startButtonSprite.hidden = (on ? NO : YES);
    exitButtonSprite.hidden = (on ? NO : YES);
    sortButtonSprite.hidden = (on ? NO : YES);
}

-(void)handleOnscreenJoystickAnchor:(CGPoint)touchPos
{
    if (touchPos.x < 0)
    {
        onscreenJoystickLeftAnchor = touchPos;
        onscreenJoystickLeft.position = touchPos;
        onscreenNubLeft.position = CGPointZero;
        onscreenJoystickLeft.hidden = NO;
    }
    
    if (touchPos.x > 0)
    {
    }
    
}

-(void)handleOnscreenJoystickMove:(CGPoint)touchPos
{
    if (touchPos.x < 0)
    {
        CGPoint offset = CGPointMake(touchPos.x - onscreenJoystickLeftAnchor.x, touchPos.y - onscreenJoystickLeftAnchor.y);
        offset = CGPointClamp(offset, 64);
        onscreenNubLeft.position = offset;
        touchInputX = offset.x / 8.0f;
        touchInputY = offset.y / 8.0f;
    }
}

-(void)handleOnscreenJoystickOff:(CGPoint)touchPos
{
    if (touchPos.x < 0)
    {
        onscreenJoystickLeft.hidden = YES;
    }
}

-(void)handleOnscreenButtons:(CGPoint)touchPos on:(BOOL)on
{
    int nearestIndex = -1;
    SKNode *node = [self nodeAtPoint:touchPos];
    if (node != nil)
    {
        //NSLog(@"touched node=%@", node.name);
        for (int i = 0; i < ONSCREEN_BUTTON_MAX; i++)
        {
            if (onscreenButtonSprite[i] != nil && onscreenButtonSprite[i].hidden == NO)
            {
                if (onscreenButtonSprite[i] == node)
                {
                    nearestIndex = i;
                    break;
                }
            }
        }
        if (nearestIndex == -1)
        {
            if (on)
            {
                if ([node.name isEqualToString:@"coinbutton"])
                {
                    coinButtonPressed = TRUE;
                }
                else if ([node.name isEqualToString:@"startbutton"])
                {
                    startButtonPressed = TRUE;
                }
                else if ([node.name isEqualToString:@"sortbutton"])
                {
                    NSLog(@"sort");
                    sortMethod = (sortMethod == 0 ? 1 : 0);
                    [self initAndSortDriverArray];
                    [self updateGameList];
                }
                else if ([node.name isEqualToString:@"exitbutton"])
                {
                    exitButtonPressed = TRUE;
                }
            }
            else
            {
                coinButtonPressed = FALSE;
                startButtonPressed = FALSE;
                exitButtonPressed = FALSE;
            }
        }
        if (nearestIndex >= 0)
        {
            if (on)
            {
                onscreenButtonSprite[nearestIndex].fillColor = [UIColor darkGrayColor];
            }
            else
            {
                UIColor *color = nil;
                switch (nearestIndex)
                {
                    case ONSCREEN_BUTTON_A:
                        color = [UIColor greenColor];
                        break;
                    case ONSCREEN_BUTTON_B:
                        color = [UIColor redColor];
                        break;
                    case ONSCREEN_BUTTON_C:
                        color = [UIColor blueColor];
                        break;
                    case ONSCREEN_BUTTON_D:
                        color = [UIColor yellowColor];
                        break;
                }
                if (color != nil)
                {
                    onscreenButtonSprite[nearestIndex].fillColor = color;
                }
            }
            onscreenButton[nearestIndex] = (on) ? 1 : 0;
        }
    }
    [self handleOnscreenButtonsEnable:YES];
}
#endif

CGPoint startTouchPos;
-(void)touchDownAtPoint:(CGPoint)pos
{
    startTouchPos = pos;

#if USE_TOUCH_CONTROLS
    [self handleOnscreenJoystickAnchor:pos];
    [self handleOnscreenButtons:pos on:YES];
#endif
    
    //debugTouchSprite.hidden = NO;
    //debugTouchSprite.position = startTouchPos;
}

int touchInputY;
int touchInputX;

-(void)touchMovedToPoint:(CGPoint)pos
{
#if USE_TOUCH_CONTROLS
    [self handleOnscreenJoystickMove:pos];
#endif
    
    startTouchPos = pos;
    
    //debugTouchSprite.position = startTouchPos;
}

-(void)touchUpAtPoint:(CGPoint)pos
{
    touchInputX = 0;
    touchInputY = 0;
#if USE_TOUCH_CONTROLS
    [self handleOnscreenJoystickOff:pos];
    [self handleOnscreenButtons:pos on:NO];
#endif
    
    //debugTouchSprite.hidden = YES;
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
        selected_game = (selected_game >= gameDriverCount) ? gameDriverCount - 1 : selected_game;
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
        c = gameDriverList[selected_game].gameDriver->description[0];
    }
    else
    {
        c = gameDriverList[selected_game].gameDriver->manufacturer[0];
    }
    for (int i = selected_game; i < gameDriverCount; i++)
    {
        if (sortMethod == 0)
        {
            if (gameDriverList[i].gameDriver->description[0] != c)
            {
                //NSLog(@"jumping from %c to %c", c, gameDriverList[i].gameDriver->description[0]);
                selected_game = i;
                return YES;
            }
        }
        else
        {
            if (gameDriverList[i].gameDriver->manufacturer[0] != c)
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
        c = gameDriverList[selected_game].gameDriver->description[0];
    }
    else
    {
        c = gameDriverList[selected_game].gameDriver->manufacturer[0];
    }
    for (int i = selected_game; i >= 0; i--)
    {
        if (sortMethod == 0)
        {
            if (gameDriverList[i].gameDriver->description[0] != c)
            {
                //NSLog(@"jumping from %c to %c", c, gameDriverList[i].gameDriver->description[0]);
                selected_game = i;
                return YES;
            }
        }
        else
        {
            if (gameDriverList[i].gameDriver->manufacturer[0] != c)
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
        
        game_index = gameDriverList[selected_game].gameIndex;
        [NSThread detachNewThreadSelector:@selector(runGameThread) toTarget:self withObject:nil];
        runState = 2;
    }
#if !USE_RENDERTHREAD
    //if (runState == 2) // rendering the frames
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
#endif
    if (runState == 3) // clean up and return to front end
    {
        if (gameDriverTableView != nil)
        {
            [gameDriverTableView setHidden:NO];
        }

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
#if USE_TOUCH_CONTROLS
        [self handleOnscreenButtonsEnable:NO];
#endif
        for (int i = 0; i < controllerList.count; i++)
        {
            GCController *controller = (GCController *)[controllerList objectAtIndex:i];
            if (controller != nil)
            {
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
    
    // disabling touch controls until a better solution comes along - touch controls are really a bad way to play arcade games anyways
#if USE_TOUCH_CONTROLS
    // handle touch controls
    touchAdvTimer += deltaTime;
    {
        if (touchInputY < 0)
        {
            if (touchAdvTimer >= touchAdvDelay)
            {
                update_list = [self next_game:1];
                touchAdvTimer = 0;
            }
        }
        else if (touchInputY > 0)
        {
            if (touchAdvTimer >= touchAdvDelay)
            {
                update_list = [self prev_game:1];
                touchAdvTimer = 0;
            }
        }
        else if (touchInputX > 0)
        {
            if (touchAdvTimer >= touchAdvDelay)
            {
                update_list = [self next_game:9];
                touchAdvTimer = 0;
            }
        }
        else if (touchInputX < 0)
        {
            if (touchAdvTimer >= touchAdvDelay)
            {
                update_list = [self prev_game:9];
                touchAdvTimer = 0;
            }
        }
        else if (touchTapCount == 2)
        {
            buttonPress = NO;
            touchTapCount = 0;
            runState = 1;
        }
    }
#endif
    
    if (update_list)
    {
#if USE_TABLEVIEW
        NSIndexPath *indexPath = [NSIndexPath indexPathForRow:selected_game inSection:0];
        [gameDriverTableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionMiddle];
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
        struct GameDriver *game_driver = gameDriverList[n].gameDriver;
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
