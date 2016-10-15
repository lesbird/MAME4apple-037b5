//
//  GameScene.m
//  MAME4apple
//
//  Created by Les Bird on 10/3/16.
//  Copyright © 2016 Les Bird. All rights reserved.
//

#import "GameScene.h"
#import <GameController/GameController.h>
#include "driver.h"

#define USE_TABLEVIEW 0
#define USE_RENDERTHREAD 0

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

// touch vars
float scrollDir;
float swipeSens = 10;
NSUInteger touchTapCount;

// front end vars
int selected_game;
BOOL buttonPress;
SKLabelNode *gameCountLabel;

GameScene *myObjectSelf;

@implementation GameScene
{
}

// table views don't allow game controller control
-(NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return gameDriverCount;
}

-(UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *MyIdentifier = @"CellIdentifier";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:MyIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault  reuseIdentifier:MyIdentifier];
    }
    cell.textLabel.text = [NSString stringWithUTF8String:gameDriverList[indexPath.row].gameDriver->description];
    return cell;
}

-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    selected_game = (int)indexPath.row;
    runState = 1;
}

int list_step = 40; // gap between lines in game list

- (void)didMoveToView:(SKView *)view
{
    [self initAndSortDriverArray];
    
    gameListNode = [SKNode node];
    [self addChild:gameListNode];

    viewSize = view.bounds.size;
    
    NSLog(@"view bounds=%f,%f", viewSize.width, viewSize.height);
    
#if USE_TABLEVIEW
    gameDriverTableView = [[UITableView alloc] initWithFrame:view.bounds];
    gameDriverTableView.autoresizingMask = UIViewAutoresizingFlexibleHeight|UIViewAutoresizingFlexibleWidth;
    gameDriverTableView.delegate = self;
    gameDriverTableView.dataSource = self;
    [gameDriverTableView reloadData];
    [self.view addSubview:gameDriverTableView];
#else
    int height = view.bounds.size.height;
    int width = view.bounds.size.width;

    gameListCount = height / list_step;
    int list_height = gameListCount * list_step;
    for (int i = 0; i < gameListCount; i++)
    {
        float y = (list_height / 2) - (list_step / 2) - (i * list_step);
        gameList[i] = [SKLabelNode labelNodeWithText:@"game"];
        gameList[i].position = CGPointMake(-(width / 2.5f), y);
        gameList[i].horizontalAlignmentMode = SKLabelHorizontalAlignmentModeLeft;
        gameList[i].fontName = @"Courier-Bold";
        gameList[i].fontSize = 18;
        [gameListNode addChild:gameList[i]];
        y -= 15;
        gameListDesc[i] = [SKLabelNode labelNodeWithText:@"desc"];
        gameListDesc[i].position = CGPointMake(-(width / 2.5f), y);
        gameListDesc[i].horizontalAlignmentMode = SKLabelHorizontalAlignmentModeLeft;
        gameListDesc[i].fontName = @"Courier-Bold";
        gameListDesc[i].fontSize = 12;
        gameListDesc[i].fontColor = [UIColor yellowColor];
        [gameListNode addChild:gameListDesc[i]];
    }

    [self computeFrameBufferScale:width height:height];
    [self updateGameList];
#endif
    
    gameCountLabel = [SKLabelNode labelNodeWithText:@""];
    gameCountLabel.position = CGPointMake(-(width / 2) + 16, -(height / 2) + 0);
    gameCountLabel.horizontalAlignmentMode = SKLabelHorizontalAlignmentModeLeft;
    gameCountLabel.fontName = @"Courier-Bold";
    gameCountLabel.fontSize = 16;
    [gameListNode addChild:gameCountLabel];
    
    versionLabel = [SKLabelNode labelNodeWithText:[NSString stringWithFormat:@"MAME4apple %s 2016 by Les Bird (www.lesbird.com)", VERSION_STRING]];
    versionLabel.position = CGPointMake(0, -(height / 2) + 16);
    versionLabel.fontName = @"Courier-Bold";
    versionLabel.fontSize = 20;
    [gameListNode addChild:versionLabel];
    
    debugTouchSprite = [SKShapeNode shapeNodeWithCircleOfRadius:32];
    debugTouchSprite.fillColor = [UIColor whiteColor];
    debugTouchSprite.hidden = YES;
    [self addChild:debugTouchSprite];
    
    self.backgroundColor = [UIColor blackColor];
    
    char *argv[] = {""};
    parse_cmdline(0, argv, game_index, nil);
    
    myObjectSelf = self;
}

-(void)initAndSortDriverArray
{
    // count number of games
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
    
    // sort array
    qsort(gameDriverList, gameDriverCount, sizeof(GameDriverList_t), sortByDesc);
}

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

void objc_alloc_framebuffer(int width, int height, int depth, int attributes, int orientation)
{
    [myObjectSelf alloc_frame_buffer:width :height :depth :attributes :orientation];
}

SKSpriteNode *testSpriteNode;
SKSpriteNode *testSpriteNode2;

UINT32 gameScreenWidth;
UINT32 gameScreenHeight;

// allocate a frame buffer for the emulated game
-(void)alloc_frame_buffer:(int)width :(int)height :(int)depth :(int)attributes :(int)orientation
{
    UINT32 screenWidth = self.view.bounds.size.width;
    UINT32 screenHeight = self.view.bounds.size.height;

    gameScreenWidth = width;
    gameScreenHeight = height;
    
    UINT32 bufferWidth = 1024;
    UINT32 bufferHeight = 1024;
    
    if (screen == nil)
    {
        screen = (bitmap_t *)malloc(sizeof(bitmap_t));
        screen->bitmap = osd_alloc_bitmap(bufferWidth, bufferHeight, 32);
        frameBufferData = (UINT32 *)screen->bitmap->_private;

        long w = screen->bitmap->line[1] - screen->bitmap->line[0];
        long h = bufferHeight;
        
        NSLog(@"screenSize=%d,%d gameScreenSize=%d,%d frameBufferSize=%ld,%ld", screenWidth, screenHeight, width, height, w, h);
        
        UINT32 w4 = (UINT32)w / 4;
        //fillBufferData(frameBufferData, w4, (UINT32)h);
        
        NSLog(@"textureSize=%d,%d", w4, (UINT32)h);
        frameBuffer = [SKMutableTexture mutableTextureWithSize:CGSizeMake(w4, h)];
        frameBufferNode = [SKSpriteNode spriteNodeWithTexture:frameBuffer];
        
        [self addChild:frameBufferNode];
        
        /* touch buttons - disabled for now
        buttonLayerNode = [SKNode node];
        [self addChild:buttonLayerNode];
        
        float x = screenWidth / 2;
        float y = screenHeight / 2;
        coinButtonLabel = [SKLabelNode labelNodeWithText:@"COIN"];
        coinButtonLabel.position = CGPointMake(-x + 64, y - 64);
        coinButtonLabel.fontName = @"Courier-Bold";
        coinButtonLabel.fontSize = 24;
        [buttonLayerNode addChild:coinButtonLabel];
        
        startButtonLabel = [SKLabelNode labelNodeWithText:@"START"];
        startButtonLabel.position = CGPointMake(x - 64, y - 64);
        startButtonLabel.fontName = @"Courier-Bold";
        startButtonLabel.fontSize = 24;
        [buttonLayerNode addChild:startButtonLabel];
        
        exitButtonLabel = [SKLabelNode labelNodeWithText:@"EXIT"];
        exitButtonLabel.position = CGPointMake(-x + 64, -y + 64);
        exitButtonLabel.fontName = @"Courier-Bold";
        exitButtonLabel.fontSize = 24;
        [self addChild:exitButtonLabel];
         */
    }
    
    [self computeFrameBufferScale:gameScreenWidth height:gameScreenHeight];

    /* debug to check centering
    if (testSpriteNode == nil)
    {
        testSpriteNode = [SKSpriteNode spriteNodeWithImageNamed:@"1024x1024.jpg"];
        [self addChild:testSpriteNode];
    }
    
    if (testSpriteNode2 == nil)
    {
        testSpriteNode2 = [SKSpriteNode spriteNodeWithImageNamed:@"1024x1024.jpg"];
        testSpriteNode2.xScale = 0.3f;
        testSpriteNode2.yScale = 0.3f;
        [self addChild:testSpriteNode2];
    }
    */
    
    // run the render loop in another thread (experimental and not needed)
    //[NSThread detachNewThreadSelector:@selector(renderLoop) toTarget:self withObject:nil];
    
    frameBufferNode.hidden = NO;
}

// free the game frame buffer
-(void)free_frame_buffer
{
    /*
    if (screen != nil)
    {
        //if (game_bitmap_update == 0)
        {
            [frameBufferNode removeFromParent];
            [frameBufferNode setTexture:nil];
            frameBuffer = nil;

            osd_free_bitmap(screen->bitmap);
            free(screen);
        
            screen = nil;
        }
    }
     */
    
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
    if (viewSize.height < viewSize.width)
    {
        // fit to height
        float scaley = (float)viewSize.height / ((float)height + 32);
        frameBufferNode.yScale = -scaley; // flip the Y since OpenGL inverts the texture data
        frameBufferNode.xScale = scaley;
    }
    else
    {
        // fit to width
        float scalex = (float)viewSize.width / ((float)width + 32);
        frameBufferNode.yScale = -scalex; // flip the Y since OpenGL inverts the texture data
        frameBufferNode.xScale = scalex;
    }
    
    float x = viewSize.width / 2;
    float y = viewSize.height / 2;

    for (int i = 0; i < gameListCount; i++)
    {
        float ly1 = gameList[i].position.y;
        gameList[i].position = CGPointMake(-(viewSize.width / 2.5f), ly1);
        float ly2 = gameListDesc[i].position.y;
        gameListDesc[i].position = CGPointMake(-(viewSize.width / 2.5f), ly2);
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
    if (coinButtonLabel != nil)
    {
        coinButtonLabel.position = CGPointMake(-x + 64, y - 64);
    }
    if (startButtonLabel != nil)
    {
        startButtonLabel.position = CGPointMake(x + 64, y - 64);
    }
    if (exitButtonLabel != nil)
    {
        exitButtonLabel.position = CGPointMake(-x + 64, y + 64);
    }

    gameCountLabel.position = CGPointMake(-x + 16, -y + 16);
    versionLabel.position = CGPointMake(0, -y + 16);
}

CGPoint startTouchPos;
-(void)touchDownAtPoint:(CGPoint)pos
{
    startTouchPos = pos;
    
    debugTouchSprite.hidden = NO;
    debugTouchSprite.position = startTouchPos;
}

int touchInputY;
int touchInputX;

float slideAccumulatorY;
float slideAccumulatorX;
-(void)touchMovedToPoint:(CGPoint)pos
{
    slideAccumulatorY += pos.y - startTouchPos.y;
    if (slideAccumulatorY < -swipeSens || slideAccumulatorY > swipeSens)
    {
        touchInputY = slideAccumulatorY < 0 ? -1 : 1;
        scrollDir = slideAccumulatorY;
        slideAccumulatorY = 0;
    }
    
    slideAccumulatorX += pos.x - startTouchPos.x;
    if (slideAccumulatorX < -swipeSens || slideAccumulatorX > swipeSens)
    {
        touchInputX = slideAccumulatorX < 0 ? -1 : 1;
        scrollDir = slideAccumulatorX;
        slideAccumulatorX = 0;
    }
    
    startTouchPos = pos;
    
    debugTouchSprite.position = startTouchPos;
}

-(void)touchUpAtPoint:(CGPoint)pos
{
    slideAccumulatorX = 0;
    slideAccumulatorY = 0;
    touchInputX = 0;
    touchInputY = 0;
    scrollDir = 0;
    
    debugTouchSprite.hidden = YES;
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
        return TRUE;
    }
    return FALSE;
}

-(BOOL)prev_game:(int)count
{
    if (!buttonPress)
    {
        selected_game -= count;
        selected_game = (selected_game > 0) ? selected_game : 0;
        return TRUE;
    }
    return FALSE;
}

-(BOOL)next_section
{
    if (buttonPress)
    {
        return FALSE;
    }
    char c = gameDriverList[selected_game].gameDriver->description[0];
    for (int i = selected_game; i < gameDriverCount; i++)
    {
        if (gameDriverList[i].gameDriver->description[0] != c)
        {
            //NSLog(@"jumping from %c to %c", c, gameDriverList[i].gameDriver->description[0]);
            selected_game = i;
            return TRUE;
        }
    }
    selected_game = gameDriverCount - 1;
    return TRUE;
}

-(BOOL)prev_section
{
    if (buttonPress)
    {
        return FALSE;
    }
    char c = gameDriverList[selected_game].gameDriver->description[0];
    for (int i = selected_game; i >= 0; i--)
    {
        if (gameDriverList[i].gameDriver->description[0] != c)
        {
            //NSLog(@"jumping from %c to %c", c, gameDriverList[i].gameDriver->description[0]);
            selected_game = i;
            return TRUE;
        }
    }
    selected_game = 0;
    return TRUE;
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
#if USE_TABLEVIEW
#else
    BOOL update_list = FALSE;
    NSArray *controllerList = [GCController controllers];
    if (controllerList.count > 0)
    {
        GCController *controller = (GCController *)[controllerList objectAtIndex:0];
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
            }
            else if (controller.gamepad.dpad.right.pressed)
            {
                update_list = [self next_section];
            }
            else if (controller.gamepad.dpad.left.pressed)
            {
                update_list = [self prev_section];
            }
            else if (controller.gamepad.buttonA.pressed)
            {
                if (!buttonPress)
                {
                    buttonPress = TRUE;
                    runState = 1;
                }
            }
            else
            {
                buttonPress = FALSE;
            }
        }
    }
    // handle touch controls
    {
        if (scrollDir > 0)
        {
            update_list = [self next_game:1];
            buttonPress = FALSE;
        }
        else if (scrollDir < 0)
        {
            update_list = [self prev_game:1];
            buttonPress = FALSE;
        }
        else if (touchTapCount == 2)
        {
            buttonPress = FALSE;
            touchTapCount = 0;
            runState = 1;
        }
        scrollDir = 0;
    }
    if (update_list)
    {
        [self updateGameList];
        buttonPress = TRUE;
    }
#endif
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
