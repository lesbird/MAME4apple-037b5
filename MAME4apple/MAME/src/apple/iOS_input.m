#include <Foundation/Foundation.h>
#include <GameController/GameController.h>
#include "driver.h"

int use_mouse;
int joystick;
int use_hotrod;
int steadykey;

int joyAnalogLeftX[2];
int joyAnalogLeftY[2];
int joyAnalogRightX[2];
int joyAnalogRightY[2];
int joyAnalogDeadzone = 64;

static int key[256];

extern int touchInputX;
extern int touchInputY;
extern int touchTapCount;
extern CGPoint startTouchPos;

enum
{
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E,  // 0
    KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,  // 5
    KEY_K, KEY_L, KEY_M, KEY_N, KEY_O,  // 10
    KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,  // 15
    KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y,  // 20
    KEY_Z,                              // 25
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,  // 26
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,  // 31
    KEY_0_PAD,                          // 36
    KEY_1_PAD,
    KEY_2_PAD,
    KEY_3_PAD,
    KEY_4_PAD,
    KEY_5_PAD,
    KEY_6_PAD,
    KEY_7_PAD,
    KEY_8_PAD,
    KEY_9_PAD,
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,     // 46
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,    // 51
    KEY_F11, KEY_F12,                           // 56
    KEY_ESC, KEY_TILDE, KEY_MINUS, KEY_EQUALS,  // 58
    KEY_BACKSPACE, KEY_TAB, KEY_OPENBRACE, KEY_CLOSEBRACE,  // 62
    KEY_ENTER, KEY_COLON, KEY_QUOTE, KEY_BACKSLASH, KEY_BACKSLASH2, // 66
    KEY_COMMA, KEY_STOP, KEY_SLASH, KEY_SPACE,  // 71
    KEY_INSERT, KEY_DEL, KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDN, // 75
    KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,  // 81
    KEY_SLASH_PAD, KEY_ASTERISK, KEY_MINUS_PAD, KEY_PLUS_PAD, KEY_DEL_PAD, KEY_ENTER_PAD,   // 85
    KEY_PRTSCR, KEY_PAUSE, KEY_LSHIFT, KEY_RSHIFT, KEY_LCONTROL, KEY_RCONTROL,  // 91
    KEY_ALT, KEY_ALTGR, KEY_LWIN, KEY_RWIN, // 97
    KEY_MENU, KEY_SCRLOCK, KEY_NUMLOCK, KEY_CAPSLOCK,   // 101
    KEY_MAX
};

static struct KeyboardInfo keylist[] =
{
    { "A",			KEY_A,				KEYCODE_A },
    { "B",			KEY_B,				KEYCODE_B },
    { "C",			KEY_C,				KEYCODE_C },
    { "D",			KEY_D,				KEYCODE_D },
    { "E",			KEY_E,				KEYCODE_E },
    { "F",			KEY_F,				KEYCODE_F },
    { "G",			KEY_G,				KEYCODE_G },
    { "H",			KEY_H,				KEYCODE_H },
    { "I",			KEY_I,				KEYCODE_I },
    { "J",			KEY_J,				KEYCODE_J },
    { "K",			KEY_K,				KEYCODE_K },
    { "L",			KEY_L,				KEYCODE_L },
    { "M",			KEY_M,				KEYCODE_M },
    { "N",			KEY_N,				KEYCODE_N },
    { "O",			KEY_O,				KEYCODE_O },
    { "P",			KEY_P,				KEYCODE_P },
    { "Q",			KEY_Q,				KEYCODE_Q },
    { "R",			KEY_R,				KEYCODE_R },
    { "S",			KEY_S,				KEYCODE_S },
    { "T",			KEY_T,				KEYCODE_T },
    { "U",			KEY_U,				KEYCODE_U },
    { "V",			KEY_V,				KEYCODE_V },
    { "W",			KEY_W,				KEYCODE_W },
    { "X",			KEY_X,				KEYCODE_X },
    { "Y",			KEY_Y,				KEYCODE_Y },
    { "Z",			KEY_Z,				KEYCODE_Z },
    { "0",			KEY_0,				KEYCODE_0 },
    { "1",			KEY_1,				KEYCODE_1 },
    { "2",			KEY_2,				KEYCODE_2 },
    { "3",			KEY_3,				KEYCODE_3 },
    { "4",			KEY_4,				KEYCODE_4 },
    { "5",			KEY_5,				KEYCODE_5 },
    { "6",			KEY_6,				KEYCODE_6 },
    { "7",			KEY_7,				KEYCODE_7 },
    { "8",			KEY_8,				KEYCODE_8 },
    { "9",			KEY_9,				KEYCODE_9 },
    { "0 PAD",		KEY_0_PAD,			KEYCODE_0_PAD },
    { "1 PAD",		KEY_1_PAD,			KEYCODE_1_PAD },
    { "2 PAD",		KEY_2_PAD,			KEYCODE_2_PAD },
    { "3 PAD",		KEY_3_PAD,			KEYCODE_3_PAD },
    { "4 PAD",		KEY_4_PAD,			KEYCODE_4_PAD },
    { "5 PAD",		KEY_5_PAD,			KEYCODE_5_PAD },
    { "6 PAD",		KEY_6_PAD,			KEYCODE_6_PAD },
    { "7 PAD",		KEY_7_PAD,			KEYCODE_7_PAD },
    { "8 PAD",		KEY_8_PAD,			KEYCODE_8_PAD },
    { "9 PAD",		KEY_9_PAD,			KEYCODE_9_PAD },
    { "F1",			KEY_F1,				KEYCODE_F1 },
    { "F2",			KEY_F2,				KEYCODE_F2 },
    { "F3",			KEY_F3,				KEYCODE_F3 },
    { "F4",			KEY_F4,				KEYCODE_F4 },
    { "F5",			KEY_F5,				KEYCODE_F5 },
    { "F6",			KEY_F6,				KEYCODE_F6 },
    { "F7",			KEY_F7,				KEYCODE_F7 },
    { "F8",			KEY_F8,				KEYCODE_F8 },
    { "F9",			KEY_F9,				KEYCODE_F9 },
    { "F10",		KEY_F10,			KEYCODE_F10 },
    { "F11",		KEY_F11,			KEYCODE_F11 },
    { "F12",		KEY_F12,			KEYCODE_F12 },
    { "ESC",		KEY_ESC,			KEYCODE_ESC },
    { "~",			KEY_TILDE,			KEYCODE_TILDE },
    { "-",          KEY_MINUS,          KEYCODE_MINUS },
    { "=",          KEY_EQUALS,         KEYCODE_EQUALS },
    { "BKSPACE",	KEY_BACKSPACE,		KEYCODE_BACKSPACE },
    { "TAB",		KEY_TAB,			KEYCODE_TAB },
    { "[",          KEY_OPENBRACE,      KEYCODE_OPENBRACE },
    { "]",          KEY_CLOSEBRACE,     KEYCODE_CLOSEBRACE },
    { "ENTER",		KEY_ENTER,			KEYCODE_ENTER },
    { ";",          KEY_COLON,          KEYCODE_COLON },
    { ":",          KEY_QUOTE,          KEYCODE_QUOTE },
    { "\\",         KEY_BACKSLASH,      KEYCODE_BACKSLASH },
    { "<",          KEY_BACKSLASH2,     KEYCODE_BACKSLASH2 },
    { ",",          KEY_COMMA,          KEYCODE_COMMA },
    { ".",          KEY_STOP,           KEYCODE_STOP },
    { "/",          KEY_SLASH,          KEYCODE_SLASH },
    { "SPACE",		KEY_SPACE,			KEYCODE_SPACE },
    { "INS",		KEY_INSERT,			KEYCODE_INSERT },
    { "DEL",		KEY_DEL,			KEYCODE_DEL },
    { "HOME",		KEY_HOME,			KEYCODE_HOME },
    { "END",		KEY_END,			KEYCODE_END },
    { "PGUP",		KEY_PGUP,			KEYCODE_PGUP },
    { "PGDN",		KEY_PGDN,			KEYCODE_PGDN },
    { "LEFT",		KEY_LEFT,			KEYCODE_LEFT },
    { "RIGHT",		KEY_RIGHT,			KEYCODE_RIGHT },
    { "UP",			KEY_UP,				KEYCODE_UP },
    { "DOWN",		KEY_DOWN,			KEYCODE_DOWN },
    { "/ PAD",      KEY_SLASH_PAD,      KEYCODE_SLASH_PAD },
    { "* PAD",      KEY_ASTERISK,       KEYCODE_ASTERISK },
    { "- PAD",      KEY_MINUS_PAD,      KEYCODE_MINUS_PAD },
    { "+ PAD",      KEY_PLUS_PAD,       KEYCODE_PLUS_PAD },
    { ". PAD",      KEY_DEL_PAD,        KEYCODE_DEL_PAD },
    { "ENTER PAD",  KEY_ENTER_PAD,      KEYCODE_ENTER_PAD },
    { "PRTSCR",     KEY_PRTSCR,         KEYCODE_PRTSCR },
    { "PAUSE",      KEY_PAUSE,          KEYCODE_PAUSE },
    { "LSHIFT",		KEY_LSHIFT,			KEYCODE_LSHIFT },
    { "RSHIFT",		KEY_RSHIFT,			KEYCODE_RSHIFT },
    { "LCTRL",		KEY_LCONTROL,		KEYCODE_LCONTROL },
    { "RCTRL",		KEY_RCONTROL,		KEYCODE_RCONTROL },
    { "ALT",		KEY_ALT,			KEYCODE_LALT },
    { "ALTGR",		KEY_ALTGR,			KEYCODE_RALT },
    { "LWIN",		KEY_LWIN,			KEYCODE_OTHER },
    { "RWIN",		KEY_RWIN,			KEYCODE_OTHER },
    { "MENU",		KEY_MENU,			KEYCODE_OTHER },
    { "SCRLOCK",    KEY_SCRLOCK,        KEYCODE_SCRLOCK },
    { "NUMLOCK",    KEY_NUMLOCK,        KEYCODE_NUMLOCK },
    { "CAPSLOCK",   KEY_CAPSLOCK,       KEYCODE_CAPSLOCK },
    { 0, 0, 0 }	/* end of table */
};

CGRect topLeftRect;
CGRect topRightRect;
CGRect botLeftRect;
CGRect botRightRect;
extern CGSize viewSize;

void apple_init_input()
{
    topLeftRect = CGRectMake(0, 0, 64, 64);
    topRightRect = CGRectMake(viewSize.width - 64, 0, 64, 64);
    botLeftRect = CGRectMake(0, viewSize.height - 64, 64, 64);
    botRightRect = CGRectMake(viewSize.width - 64, 0, 64, 64);
}

const struct KeyboardInfo *osd_get_key_list(void)
{
    return keylist;
}

void update_key_array()
{
    for (int i = 0; i < KEY_MAX; i++)
    {
        key[i] = 0;
    }
    NSArray *controllerList = [GCController controllers];
    if (controllerList.count > 0)
    {
        GCController *controller = (GCController *)[controllerList objectAtIndex:0];
        if (controller != nil)
        {
            if (controller.gamepad != nil)
            {
                if (controller.gamepad.buttonY.pressed)
                {
                    if (controller.gamepad.rightShoulder.pressed)
                    {
                        key[KEY_ESC] = 1;
                    }
                    else
                    {
                        key[KEY_LSHIFT] = 1;
                    }
                }
                if (controller.gamepad.leftShoulder.pressed)
                {
                    key[KEY_5] = 1;
                }
                if (controller.gamepad.rightShoulder.pressed)
                {
                    key[KEY_1] = 1;
                }
                if (controller.gamepad.dpad.up.pressed)
                {
                    key[KEY_UP] = 1;
                }
                if (controller.gamepad.dpad.right.pressed)
                {
                    key[KEY_RIGHT] = 1;
                }
                if (controller.gamepad.dpad.down.pressed)
                {
                    key[KEY_DOWN] = 1;
                }
                if (controller.gamepad.dpad.left.pressed)
                {
                    key[KEY_LEFT] = 1;
                }
                if (controller.gamepad.buttonA.pressed)
                {
                    key[KEY_LCONTROL] = 1;
                    key[KEY_ENTER] = 1;
                }
                if (controller.gamepad.buttonB.pressed)
                {
                    key[KEY_ALT] = 1;
                }
                if (controller.gamepad.buttonX.pressed)
                {
                    key[KEY_SPACE] = 1;
                }
            }
            if (controller.extendedGamepad != nil)
            {
                if (controller.extendedGamepad.leftTrigger.pressed)
                {
                    key[KEY_LSHIFT] = 1;
                }
                if (controller.extendedGamepad.leftTrigger.pressed)
                {
                    if (controller.extendedGamepad.rightTrigger.pressed)
                    {
                        key[KEY_TAB] = 0;
                    }
                    else
                    {
                        key[KEY_TAB] = 1;
                    }
                }
                if (controller.extendedGamepad.rightTrigger.pressed)
                {
                    key[KEY_F11] = 1;
                }
            }
        }
    }

    // analog stick left
    if (joyAnalogLeftY[0] < -joyAnalogDeadzone)
    {
        key[KEY_DOWN] = 1;
    }
    if (joyAnalogLeftY[0] > joyAnalogDeadzone)
    {
        key[KEY_UP] = 1;
    }
    if (joyAnalogLeftX[0] < -joyAnalogDeadzone)
    {
        key[KEY_LEFT] = 1;
    }
    if (joyAnalogLeftX[0] > joyAnalogDeadzone)
    {
        key[KEY_RIGHT] = 1;
    }
    // analog stick right
    if (joyAnalogLeftY[1] < -joyAnalogDeadzone)
    {
        key[KEY_F] = 1;
    }
    if (joyAnalogLeftY[1] > joyAnalogDeadzone)
    {
        key[KEY_R] = 1;
    }
    if (joyAnalogLeftX[1] < -joyAnalogDeadzone)
    {
        key[KEY_D] = 1;
    }
    if (joyAnalogLeftX[1] > joyAnalogDeadzone)
    {
        key[KEY_G] = 1;
    }

    // touch screen inputs (experimental)
    if (touchInputX > 0)
    {
        key[KEY_RIGHT] = 1;
    }
    if (touchInputX < 0)
    {
        key[KEY_LEFT] = 1;
    }
    if (touchInputY > 0)
    {
        key[KEY_UP] = 1;
    }
    if (touchInputY < 0)
    {
        key[KEY_DOWN] = 1;
    }
    if (touchTapCount > 0)
    {
        CGPoint convertPoint = CGPointMake(startTouchPos.x + (viewSize.width / 2), viewSize.height - (startTouchPos.y + (viewSize.height / 2)));
        if (CGRectContainsPoint(topLeftRect, convertPoint))
        {
            NSLog(@"Add Coin touched");
            key[KEY_5] = 1;
        }
        else if (CGRectContainsPoint(topRightRect, convertPoint))
        {
            NSLog(@"Start Game touched");
            key[KEY_1] = 1;
        }
        else if (CGRectContainsPoint(botLeftRect, convertPoint))
        {
            NSLog(@"Exit Game touched");
            key[KEY_ESC] = 1;
        }
        //else if (keycode == KEY_LCONTROL || keycode == KEY_ENTER)
        //{
        //    //touchTapCount = 0;
        //    //return 1;
        //}
    }
}

int osd_is_key_pressed(int keycode)
{
    return key[keycode];
}

int osd_readkey_unicode(int flush)
{
    return 0;
}

int osd_wait_keypress(void)
{
    return 0;
}

void poll_joysticks(void)
{
    update_key_array();
}

void osd_poll_joysticks(void)
{
    poll_joysticks();
}

void osd_led_w(int led,int on)
{
}

//

#define JOYCODE(joy,stick,axis_or_button,dir) \
((((dir)&0x03)<<14)|(((axis_or_button)&0x3f)<<8)|(((stick)&0x1f)<<3)|(((joy)&0x07)<<0))

#define GET_JOYCODE_JOY(code) (((code)>>0)&0x07)
#define GET_JOYCODE_STICK(code) (((code)>>3)&0x1f)
#define GET_JOYCODE_AXIS(code) (((code)>>8)&0x3f)
#define GET_JOYCODE_BUTTON(code) GET_JOYCODE_AXIS(code)
#define GET_JOYCODE_DIR(code) (((code)>>14)&0x03)

#define MOUSECODE(mouse,button) JOYCODE(1,0,((mouse)-1)*3+(button),1)

#define MAX_JOY 256
#define MAX_JOY_NAME_LEN 40

static char joynames[MAX_JOY][MAX_JOY_NAME_LEN+1];	/* will be used to store names for the above */

static int joyequiv[][2] =
{
#if 1
    { JOYCODE(1,1,1,1),	JOYCODE_1_LEFT },
    { JOYCODE(1,1,1,2),	JOYCODE_1_RIGHT },
    { JOYCODE(1,1,2,1),	JOYCODE_1_UP },
    { JOYCODE(1,1,2,2),	JOYCODE_1_DOWN },
    { JOYCODE(1,0,1,0),	JOYCODE_1_BUTTON1 },
    { JOYCODE(1,0,2,0),	JOYCODE_1_BUTTON2 },
    { JOYCODE(1,0,3,0),	JOYCODE_1_BUTTON3 },
    { JOYCODE(1,0,4,0),	JOYCODE_1_BUTTON4 },
    { JOYCODE(1,0,5,0),	JOYCODE_1_BUTTON5 },
    { JOYCODE(1,0,6,0),	JOYCODE_1_BUTTON6 },
    { JOYCODE(2,1,1,1),	JOYCODE_2_LEFT },
    { JOYCODE(2,1,1,2),	JOYCODE_2_RIGHT },
    { JOYCODE(2,1,2,1),	JOYCODE_2_UP },
    { JOYCODE(2,1,2,2),	JOYCODE_2_DOWN },
    { JOYCODE(2,0,1,0),	JOYCODE_2_BUTTON1 },
    { JOYCODE(2,0,2,0),	JOYCODE_2_BUTTON2 },
    { JOYCODE(2,0,3,0),	JOYCODE_2_BUTTON3 },
    { JOYCODE(2,0,4,0),	JOYCODE_2_BUTTON4 },
    { JOYCODE(2,0,5,0),	JOYCODE_2_BUTTON5 },
    { JOYCODE(2,0,6,0),	JOYCODE_2_BUTTON6 },
    { JOYCODE(3,1,1,1),	JOYCODE_3_LEFT },
    { JOYCODE(3,1,1,2),	JOYCODE_3_RIGHT },
    { JOYCODE(3,1,2,1),	JOYCODE_3_UP },
    { JOYCODE(3,1,2,2),	JOYCODE_3_DOWN },
    { JOYCODE(3,0,1,0),	JOYCODE_3_BUTTON1 },
    { JOYCODE(3,0,2,0),	JOYCODE_3_BUTTON2 },
    { JOYCODE(3,0,3,0),	JOYCODE_3_BUTTON3 },
    { JOYCODE(3,0,4,0),	JOYCODE_3_BUTTON4 },
    { JOYCODE(3,0,5,0),	JOYCODE_3_BUTTON5 },
    { JOYCODE(3,0,6,0),	JOYCODE_3_BUTTON6 },
    { JOYCODE(4,1,1,1),	JOYCODE_4_LEFT },
    { JOYCODE(4,1,1,2),	JOYCODE_4_RIGHT },
    { JOYCODE(4,1,2,1),	JOYCODE_4_UP },
    { JOYCODE(4,1,2,2),	JOYCODE_4_DOWN },
    { JOYCODE(4,0,1,0),	JOYCODE_4_BUTTON1 },
    { JOYCODE(4,0,2,0),	JOYCODE_4_BUTTON2 },
    { JOYCODE(4,0,3,0),	JOYCODE_4_BUTTON3 },
    { JOYCODE(4,0,4,0),	JOYCODE_4_BUTTON4 },
    { JOYCODE(4,0,5,0),	JOYCODE_4_BUTTON5 },
    { JOYCODE(4,0,6,0),	JOYCODE_4_BUTTON6 },
#endif
    { 0,0 }
};

static struct JoystickInfo joylist[MAX_JOY] =
{
    /* will be filled later */
    { 0, 0, 0 }	/* end of table */
};

const struct JoystickInfo *osd_get_joy_list(void)
{
    return joylist;
}

int osd_is_joy_pressed(int joycode)
{
    return 0;
}

int osd_joystick_needs_calibration(void)
{
    return 0;
}

void osd_joystick_start_calibration(void)
{
}

char *osd_joystick_calibrate_next(void)
{
    return nil;
}

void osd_joystick_calibrate(void)
{
}

void osd_joystick_end_calibration(void)
{
}

void osd_trak_read(int player,int *deltax,int *deltay)
{
}

/* return values in the range -128 .. 128 (yes, 128, not 127) */
void osd_analogjoy_read(int player,int *analog_x, int *analog_y)
{
    if (player == 0)
    {
        NSArray *controllerList = [GCController controllers];
        if (controllerList.count > 0)
        {
            GCController *controller = (GCController *)[controllerList objectAtIndex:0];
            if (controller != nil)
            {
                int playerId = 0;
                joyAnalogLeftX[playerId] = (int)(controller.extendedGamepad.leftThumbstick.xAxis.value * 128);
                joyAnalogLeftY[playerId] = (int)(controller.extendedGamepad.leftThumbstick.yAxis.value * 128);
                joyAnalogRightX[playerId] = (int)(controller.extendedGamepad.rightThumbstick.xAxis.value * 128);
                joyAnalogRightY[playerId] = (int)(controller.extendedGamepad.rightThumbstick.yAxis.value * 128);
                //NSLog(@"joyAnalog left=%d,%d right=%d,%d", joyAnalogLeftX[0], joyAnalogLeftY[0], joyAnalogRightX[0], joyAnalogRightY[0]);
            }
        }
    }
    if (player == 0)
    {
        //NSLog(@"joyAnalogLeft=%d,%d", joyAnalogLeftX[0], joyAnalogLeftY[0]);
        *analog_x = joyAnalogLeftX[0];
        *analog_y = joyAnalogLeftY[0];
    }
    else if (player == 1)
    {
        //NSLog(@"joyAnalogRight=%d,%d", joyAnalogRightX[0], joyAnalogRightY[0]);
        *analog_x = joyAnalogRightX[0];
        *analog_y = joyAnalogRightY[0];
    }
}

void osd_customize_inputport_defaults(struct ipd *defaults)
{
}

