#include <linux/input-event-codes.h>
#include <cstddef>
#include <stdint.h>
#include "keycode_map.h"
#include <iostream>
using namespace std;

struct key_map {
	uint32_t godot_scancode;
	uint32_t eudev_scancode;
};

/* Copied+modified from core/os/keyboard.h */
enum {
	SPKEY = (1 << 24)
};

enum KeyList {
	GDKEY_ESCAPE = SPKEY | 0x01,
	GDKEY_TAB = SPKEY | 0x02,
	GDKEY_BACKTAB = SPKEY | 0x03,
	GDKEY_BACKSPACE = SPKEY | 0x04,
	GDKEY_ENTER = SPKEY | 0x05,
	GDKEY_KP_ENTER = SPKEY | 0x06,
	GDKEY_INSERT = SPKEY | 0x07,
	GDKEY_DELETE = SPKEY | 0x08,
	GDKEY_PAUSE = SPKEY | 0x09,
	GDKEY_PRINT = SPKEY | 0x0A,
	GDKEY_SYSREQ = SPKEY | 0x0B,
	GDKEY_CLEAR = SPKEY | 0x0C,
	GDKEY_HOME = SPKEY | 0x0D,
	GDKEY_END = SPKEY | 0x0E,
	GDKEY_LEFT = SPKEY | 0x0F,
	GDKEY_UP = SPKEY | 0x10,
	GDKEY_RIGHT = SPKEY | 0x11,
	GDKEY_DOWN = SPKEY | 0x12,
	GDKEY_PAGEUP = SPKEY | 0x13,
	GDKEY_PAGEDOWN = SPKEY | 0x14,
	GDKEY_SHIFT = SPKEY | 0x15,
	GDKEY_CONTROL = SPKEY | 0x16,
	GDKEY_META = SPKEY | 0x17,
	GDKEY_ALT = SPKEY | 0x18,
	GDKEY_CAPSLOCK = SPKEY | 0x19,
	GDKEY_NUMLOCK = SPKEY | 0x1A,
	GDKEY_SCROLLLOCK = SPKEY | 0x1B,
	GDKEY_F1 = SPKEY | 0x1C,
	GDKEY_F2 = SPKEY | 0x1D,
	GDKEY_F3 = SPKEY | 0x1E,
	GDKEY_F4 = SPKEY | 0x1F,
	GDKEY_F5 = SPKEY | 0x20,
	GDKEY_F6 = SPKEY | 0x21,
	GDKEY_F7 = SPKEY | 0x22,
	GDKEY_F8 = SPKEY | 0x23,
	GDKEY_F9 = SPKEY | 0x24,
	GDKEY_F10 = SPKEY | 0x25,
	GDKEY_F11 = SPKEY | 0x26,
	GDKEY_F12 = SPKEY | 0x27,
	GDKEY_F13 = SPKEY | 0x28,
	GDKEY_F14 = SPKEY | 0x29,
	GDKEY_F15 = SPKEY | 0x2A,
	GDKEY_F16 = SPKEY | 0x2B,
	GDKEY_KP_MULTIPLY = SPKEY | 0x81,
	GDKEY_KP_DIVIDE = SPKEY | 0x82,
	GDKEY_KP_SUBTRACT = SPKEY | 0x83,
	GDKEY_KP_PERIOD = SPKEY | 0x84,
	GDKEY_KP_ADD = SPKEY | 0x85,
	GDKEY_KP_0 = SPKEY | 0x86,
	GDKEY_KP_1 = SPKEY | 0x87,
	GDKEY_KP_2 = SPKEY | 0x88,
	GDKEY_KP_3 = SPKEY | 0x89,
	GDKEY_KP_4 = SPKEY | 0x8A,
	GDKEY_KP_5 = SPKEY | 0x8B,
	GDKEY_KP_6 = SPKEY | 0x8C,
	GDKEY_KP_7 = SPKEY | 0x8D,
	GDKEY_KP_8 = SPKEY | 0x8E,
	GDKEY_KP_9 = SPKEY | 0x8F,
	GDKEY_SUPER_L = SPKEY | 0x2C,
	GDKEY_SUPER_R = SPKEY | 0x2D,
	GDKEY_MENU = SPKEY | 0x2E,
	GDKEY_HYPER_L = SPKEY | 0x2F,
	GDKEY_HYPER_R = SPKEY | 0x30,
	GDKEY_HELP = SPKEY | 0x31,
	GDKEY_DIRECTION_L = SPKEY | 0x32,
	GDKEY_DIRECTION_R = SPKEY | 0x33,
	GDKEY_BACK = SPKEY | 0x40,
	GDKEY_FORWARD = SPKEY | 0x41,
	GDKEY_STOP = SPKEY | 0x42,
	GDKEY_REFRESH = SPKEY | 0x43,
	GDKEY_VOLUMEDOWN = SPKEY | 0x44,
	GDKEY_VOLUMEMUTE = SPKEY | 0x45,
	GDKEY_VOLUMEUP = SPKEY | 0x46,
	GDKEY_BASSBOOST = SPKEY | 0x47,
	GDKEY_BASSUP = SPKEY | 0x48,
	GDKEY_BASSDOWN = SPKEY | 0x49,
	GDKEY_TREBLEUP = SPKEY | 0x4A,
	GDKEY_TREBLEDOWN = SPKEY | 0x4B,
	GDKEY_MEDIAPLAY = SPKEY | 0x4C,
	GDKEY_MEDIASTOP = SPKEY | 0x4D,
	GDKEY_MEDIAPREVIOUS = SPKEY | 0x4E,
	GDKEY_MEDIANEXT = SPKEY | 0x4F,
	GDKEY_MEDIARECORD = SPKEY | 0x50,
	GDKEY_HOMEPAGE = SPKEY | 0x51,
	GDKEY_FAVORITES = SPKEY | 0x52,
	GDKEY_SEARCH = SPKEY | 0x53,
	GDKEY_STANDBY = SPKEY | 0x54,
	GDKEY_OPENURL = SPKEY | 0x55,
	GDKEY_LAUNCHMAIL = SPKEY | 0x56,
	GDKEY_LAUNCHMEDIA = SPKEY | 0x57,
	GDKEY_LAUNCH0 = SPKEY | 0x58,
	GDKEY_LAUNCH1 = SPKEY | 0x59,
	GDKEY_LAUNCH2 = SPKEY | 0x5A,
	GDKEY_LAUNCH3 = SPKEY | 0x5B,
	GDKEY_LAUNCH4 = SPKEY | 0x5C,
	GDKEY_LAUNCH5 = SPKEY | 0x5D,
	GDKEY_LAUNCH6 = SPKEY | 0x5E,
	GDKEY_LAUNCH7 = SPKEY | 0x5F,
	GDKEY_LAUNCH8 = SPKEY | 0x60,
	GDKEY_LAUNCH9 = SPKEY | 0x61,
	GDKEY_LAUNCHA = SPKEY | 0x62,
	GDKEY_LAUNCHB = SPKEY | 0x63,
	GDKEY_LAUNCHC = SPKEY | 0x64,
	GDKEY_LAUNCHD = SPKEY | 0x65,
	GDKEY_LAUNCHE = SPKEY | 0x66,
	GDKEY_LAUNCHF = SPKEY | 0x67,
	GDKEY_UNKNOWN = SPKEY | 0xFFFFFF,
	GDKEY_SPACE = 0x0020,
	GDKEY_EXCLAM = 0x0021,
	GDKEY_QUOTEDBL = 0x0022,
	GDKEY_NUMBERSIGN = 0x0023,
	GDKEY_DOLLAR = 0x0024,
	GDKEY_PERCENT = 0x0025,
	GDKEY_AMPERSAND = 0x0026,
	GDKEY_APOSTROPHE = 0x0027,
	GDKEY_PARENLEFT = 0x0028,
	GDKEY_PARENRIGHT = 0x0029,
	GDKEY_ASTERISK = 0x002A,
	GDKEY_PLUS = 0x002B,
	GDKEY_COMMA = 0x002C,
	GDKEY_MINUS = 0x002D,
	GDKEY_PERIOD = 0x002E,
	GDKEY_SLASH = 0x002F,
	GDKEY_0 = 0x0030,
	GDKEY_1 = 0x0031,
	GDKEY_2 = 0x0032,
	GDKEY_3 = 0x0033,
	GDKEY_4 = 0x0034,
	GDKEY_5 = 0x0035,
	GDKEY_6 = 0x0036,
	GDKEY_7 = 0x0037,
	GDKEY_8 = 0x0038,
	GDKEY_9 = 0x0039,
	GDKEY_COLON = 0x003A,
	GDKEY_SEMICOLON = 0x003B,
	GDKEY_LESS = 0x003C,
	GDKEY_EQUAL = 0x003D,
	GDKEY_GREATER = 0x003E,
	GDKEY_QUESTION = 0x003F,
	GDKEY_AT = 0x0040,
	GDKEY_A = 0x0041,
	GDKEY_B = 0x0042,
	GDKEY_C = 0x0043,
	GDKEY_D = 0x0044,
	GDKEY_E = 0x0045,
	GDKEY_F = 0x0046,
	GDKEY_G = 0x0047,
	GDKEY_H = 0x0048,
	GDKEY_I = 0x0049,
	GDKEY_J = 0x004A,
	GDKEY_K = 0x004B,
	GDKEY_L = 0x004C,
	GDKEY_M = 0x004D,
	GDKEY_N = 0x004E,
	GDKEY_O = 0x004F,
	GDKEY_P = 0x0050,
	GDKEY_Q = 0x0051,
	GDKEY_R = 0x0052,
	GDKEY_S = 0x0053,
	GDKEY_T = 0x0054,
	GDKEY_U = 0x0055,
	GDKEY_V = 0x0056,
	GDKEY_W = 0x0057,
	GDKEY_X = 0x0058,
	GDKEY_Y = 0x0059,
	GDKEY_Z = 0x005A,
	GDKEY_BRACKETLEFT = 0x005B,
	GDKEY_BACKSLASH = 0x005C,
	GDKEY_BRACKETRIGHT = 0x005D,
	GDKEY_ASCIICIRCUM = 0x005E,
	GDKEY_UNDERSCORE = 0x005F,
	GDKEY_QUOTELEFT = 0x0060,
	GDKEY_BRACELEFT = 0x007B,
	GDKEY_BAR = 0x007C,
	GDKEY_BRACERIGHT = 0x007D,
	GDKEY_ASCIITILDE = 0x007E,
	GDKEY_NOBREAKSPACE = 0x00A0,
	GDKEY_EXCLAMDOWN = 0x00A1,
	GDKEY_CENT = 0x00A2,
	GDKEY_STERLING = 0x00A3,
	GDKEY_CURRENCY = 0x00A4,
	GDKEY_YEN = 0x00A5,
	GDKEY_BROKENBAR = 0x00A6,
	GDKEY_SECTION = 0x00A7,
	GDKEY_DIAERESIS = 0x00A8,
	GDKEY_COPYRIGHT = 0x00A9,
	GDKEY_ORDFEMININE = 0x00AA,
	GDKEY_GUILLEMOTLEFT = 0x00AB,
	GDKEY_NOTSIGN = 0x00AC,
	GDKEY_HYPHEN = 0x00AD,
	GDKEY_REGISTERED = 0x00AE,
	GDKEY_MACRON = 0x00AF,
	GDKEY_DEGREE = 0x00B0,
	GDKEY_PLUSMINUS = 0x00B1,
	GDKEY_TWOSUPERIOR = 0x00B2,
	GDKEY_THREESUPERIOR = 0x00B3,
	GDKEY_ACUTE = 0x00B4,
	GDKEY_MU = 0x00B5,
	GDKEY_PARAGRAPH = 0x00B6,
	GDKEY_PERIODCENTERED = 0x00B7,
	GDKEY_CEDILLA = 0x00B8,
	GDKEY_ONESUPERIOR = 0x00B9,
	GDKEY_MASCULINE = 0x00BA,
	GDKEY_GUILLEMOTRIGHT = 0x00BB,
	GDKEY_ONEQUARTER = 0x00BC,
	GDKEY_ONEHALF = 0x00BD,
	GDKEY_THREEQUARTERS = 0x00BE,
	GDKEY_QUESTIONDOWN = 0x00BF,
	GDKEY_AGRAVE = 0x00C0,
	GDKEY_AACUTE = 0x00C1,
	GDKEY_ACIRCUMFLEX = 0x00C2,
	GDKEY_ATILDE = 0x00C3,
	GDKEY_ADIAERESIS = 0x00C4,
	GDKEY_ARING = 0x00C5,
	GDKEY_AE = 0x00C6,
	GDKEY_CCEDILLA = 0x00C7,
	GDKEY_EGRAVE = 0x00C8,
	GDKEY_EACUTE = 0x00C9,
	GDKEY_ECIRCUMFLEX = 0x00CA,
	GDKEY_EDIAERESIS = 0x00CB,
	GDKEY_IGRAVE = 0x00CC,
	GDKEY_IACUTE = 0x00CD,
	GDKEY_ICIRCUMFLEX = 0x00CE,
	GDKEY_IDIAERESIS = 0x00CF,
	GDKEY_ETH = 0x00D0,
	GDKEY_NTILDE = 0x00D1,
	GDKEY_OGRAVE = 0x00D2,
	GDKEY_OACUTE = 0x00D3,
	GDKEY_OCIRCUMFLEX = 0x00D4,
	GDKEY_OTILDE = 0x00D5,
	GDKEY_ODIAERESIS = 0x00D6,
	GDKEY_MULTIPLY = 0x00D7,
	GDKEY_OOBLIQUE = 0x00D8,
	GDKEY_UGRAVE = 0x00D9,
	GDKEY_UACUTE = 0x00DA,
	GDKEY_UCIRCUMFLEX = 0x00DB,
	GDKEY_UDIAERESIS = 0x00DC,
	GDKEY_YACUTE = 0x00DD,
	GDKEY_THORN = 0x00DE,
	GDKEY_SSHARP = 0x00DF,
	GDKEY_DIVISION = 0x00F7,
	GDKEY_YDIAERESIS = 0x00FF,
};

// Note: if this doesn't work well, we should try a solution backtracking
// xkb_keysms into xkb_scancodes from xkb_keymaps
static struct key_map map[] = {
	{ GDKEY_ESCAPE, KEY_ESC },
	{ GDKEY_TAB, KEY_TAB },
	{ GDKEY_BACKSPACE, KEY_BACKSPACE },
	{ GDKEY_ENTER, KEY_ENTER },
	{ GDKEY_KP_ENTER, KEY_KPENTER },
	{ GDKEY_INSERT, KEY_INSERT },
	{ GDKEY_DELETE, KEY_DELETE },
	{ GDKEY_PAUSE, KEY_PAUSE },
	{ GDKEY_PRINT, KEY_PRINT },
	{ GDKEY_SYSREQ, KEY_SYSRQ },
	{ GDKEY_CLEAR, KEY_CLEAR },
	{ GDKEY_HOME, KEY_HOME },
	{ GDKEY_END, KEY_END },
	{ GDKEY_LEFT, KEY_LEFT },
	{ GDKEY_UP, KEY_UP },
	{ GDKEY_RIGHT, KEY_RIGHT },
	{ GDKEY_DOWN, KEY_DOWN },
	{ GDKEY_PAGEUP, KEY_PAGEUP },
	{ GDKEY_PAGEDOWN, KEY_PAGEDOWN },
	{ GDKEY_SHIFT, KEY_LEFTSHIFT },
	{ GDKEY_CONTROL, KEY_LEFTCTRL },
	{ GDKEY_META, KEY_LEFTMETA },
	{ GDKEY_ALT, KEY_LEFTALT },
	{ GDKEY_CAPSLOCK, KEY_CAPSLOCK },
	{ GDKEY_NUMLOCK, KEY_NUMLOCK },
	{ GDKEY_SCROLLLOCK, KEY_SCROLLLOCK },
	{ GDKEY_F1, KEY_F1 },
	{ GDKEY_F2, KEY_F2 },
	{ GDKEY_F3, KEY_F3 },
	{ GDKEY_F4, KEY_F4 },
	{ GDKEY_F5, KEY_F5 },
	{ GDKEY_F6, KEY_F6 },
	{ GDKEY_F7, KEY_F7 },
	{ GDKEY_F8, KEY_F8 },
	{ GDKEY_F9, KEY_F9 },
	{ GDKEY_F10, KEY_F10 },
	{ GDKEY_F11, KEY_F11 },
	{ GDKEY_F12, KEY_F12 },
	{ GDKEY_F13, KEY_F13 },
	{ GDKEY_F14, KEY_F14 },
	{ GDKEY_F15, KEY_F15 },
	{ GDKEY_F16, KEY_F16 },
	{ GDKEY_KP_MULTIPLY, KEY_KPASTERISK },
	{ GDKEY_KP_DIVIDE, KEY_KPSLASH },
	{ GDKEY_KP_SUBTRACT, KEY_KPMINUS },
	{ GDKEY_KP_PERIOD, KEY_KPDOT },
	{ GDKEY_KP_ADD, KEY_KPPLUS },
	{ GDKEY_KP_0, KEY_KP0 },
	{ GDKEY_KP_1, KEY_KP1 },
	{ GDKEY_KP_2, KEY_KP2 },
	{ GDKEY_KP_3, KEY_KP3 },
	{ GDKEY_KP_4, KEY_KP4 },
	{ GDKEY_KP_5, KEY_KP5 },
	{ GDKEY_KP_6, KEY_KP6 },
	{ GDKEY_KP_7, KEY_KP7 },
	{ GDKEY_KP_8, KEY_KP8 },
	{ GDKEY_KP_9, KEY_KP9 },
	{ GDKEY_SUPER_L, KEY_LEFTMETA },
	{ GDKEY_SUPER_R, KEY_RIGHTMETA },
	{ GDKEY_MENU, KEY_MENU },
	{ GDKEY_HELP, KEY_HELP },
	{ GDKEY_BACK, KEY_BACK },
	{ GDKEY_FORWARD, KEY_FORWARD },
	{ GDKEY_STOP, KEY_STOP },
	{ GDKEY_REFRESH, KEY_REFRESH },
	{ GDKEY_VOLUMEDOWN, KEY_VOLUMEDOWN },
	{ GDKEY_VOLUMEMUTE, KEY_MUTE },
	{ GDKEY_VOLUMEUP, KEY_VOLUMEUP },
	{ GDKEY_BASSBOOST, KEY_BASSBOOST },
	{ GDKEY_MEDIAPLAY, KEY_PLAYPAUSE },
	{ GDKEY_MEDIASTOP, KEY_STOPCD },
	{ GDKEY_MEDIAPREVIOUS, KEY_PREVIOUSSONG },
	{ GDKEY_MEDIANEXT, KEY_NEXTSONG },
	{ GDKEY_MEDIARECORD, KEY_RECORD },
	{ GDKEY_HOMEPAGE, KEY_HOMEPAGE },
	{ GDKEY_FAVORITES, KEY_FAVORITES },
	{ GDKEY_SEARCH, KEY_SEARCH },
	{ GDKEY_STANDBY, KEY_SUSPEND },
	{ GDKEY_OPENURL, KEY_OPEN },
	{ GDKEY_LAUNCHMAIL, KEY_MAIL },
	{ GDKEY_LAUNCHMEDIA, KEY_MEDIA },
	{ GDKEY_UNKNOWN, KEY_UNKNOWN },
	{ GDKEY_SPACE, KEY_SPACE },
	{ GDKEY_APOSTROPHE, KEY_APOSTROPHE },
	{ GDKEY_PARENLEFT, KEY_KPLEFTPAREN },
	{ GDKEY_PARENRIGHT, KEY_KPRIGHTPAREN },
	{ GDKEY_ASTERISK, KEY_KPASTERISK }, //Is KEY_KPASTERISK the right asterisk?
	{ GDKEY_PLUS, KEY_KPPLUS }, //Is KEY_KPPLUS the right plus?
	{ GDKEY_COMMA, KEY_COMMA },
	{ GDKEY_MINUS, KEY_MINUS },
	{ GDKEY_PERIOD, KEY_DOT },
	{ GDKEY_SLASH, KEY_SLASH },
	{ GDKEY_0, KEY_0 },
	{ GDKEY_1, KEY_1 },
	{ GDKEY_2, KEY_2 },
	{ GDKEY_3, KEY_3 },
	{ GDKEY_4, KEY_4 },
	{ GDKEY_5, KEY_5 },
	{ GDKEY_6, KEY_6 },
	{ GDKEY_7, KEY_7 },
	{ GDKEY_8, KEY_8 },
	{ GDKEY_9, KEY_9 },
	{ GDKEY_SEMICOLON, KEY_SEMICOLON },
	{ GDKEY_EQUAL, KEY_EQUAL },
	//{ GDKEY_QUESTION, KEY_QUESTION }, //Doesn't work
	{ GDKEY_A, KEY_A },
	{ GDKEY_B, KEY_B },
	{ GDKEY_C, KEY_C },
	{ GDKEY_D, KEY_D },
	{ GDKEY_E, KEY_E },
	{ GDKEY_F, KEY_F },
	{ GDKEY_G, KEY_G },
	{ GDKEY_H, KEY_H },
	{ GDKEY_I, KEY_I },
	{ GDKEY_J, KEY_J },
	{ GDKEY_K, KEY_K },
	{ GDKEY_L, KEY_L },
	{ GDKEY_M, KEY_M },
	{ GDKEY_N, KEY_N },
	{ GDKEY_O, KEY_O },
	{ GDKEY_P, KEY_P },
	{ GDKEY_Q, KEY_Q },
	{ GDKEY_R, KEY_R },
	{ GDKEY_S, KEY_S },
	{ GDKEY_T, KEY_T },
	{ GDKEY_U, KEY_U },
	{ GDKEY_V, KEY_V },
	{ GDKEY_W, KEY_W },
	{ GDKEY_X, KEY_X },
	{ GDKEY_Y, KEY_Y },
	{ GDKEY_Z, KEY_Z },
	{ GDKEY_BACKSLASH, KEY_BACKSLASH },
	{ GDKEY_BRACELEFT, KEY_LEFTBRACE },
	{ GDKEY_BRACERIGHT, KEY_RIGHTBRACE },
	{ GDKEY_YEN, KEY_YEN },
	{ GDKEY_PLUSMINUS, KEY_KPPLUSMINUS },
	{ GDKEY_MULTIPLY, KEY_KPASTERISK },
	{ GDKEY_DIVISION, KEY_KPSLASH },
	/* Massive hack because godot lies about giving us scancodes */
	{ GDKEY_EXCLAM, KEY_1 },
	{ GDKEY_AT, KEY_2 },
	{ GDKEY_NUMBERSIGN, KEY_3 },
	{ GDKEY_DOLLAR, KEY_4 },
	{ GDKEY_PERCENT, KEY_5 },
	{ GDKEY_ASCIICIRCUM, KEY_6 },
	{ GDKEY_AMPERSAND, KEY_7 },
	{ GDKEY_COLON, KEY_SEMICOLON },

  //George: Let's extend Drew's hack to the other special keys:
	{ GDKEY_LESS, KEY_COMMA },
	{ GDKEY_EQUAL , KEY_EQUAL },
	{ GDKEY_GREATER , KEY_DOT},
  //{ GDKEY_QUESTION, KEY_QUESTION }, //Doesn't work
  { GDKEY_QUESTION, KEY_SLASH },      //..but the hack does
	{ GDKEY_QUOTEDBL, KEY_APOSTROPHE },
	{ GDKEY_UNDERSCORE, KEY_MINUS },
	{ GDKEY_KP_PERIOD, KEY_KPDOT },
  { GDKEY_QUOTELEFT, KEY_GRAVE},
	{ GDKEY_ASCIITILDE, KEY_GRAVE },
  { GDKEY_BAR, KEY_BACKSLASH },
	{ GDKEY_BRACKETLEFT, KEY_LEFTBRACE },
	{ GDKEY_BRACKETRIGHT, KEY_RIGHTBRACE },
  { GDKEY_QUESTION, KEY_QUESTION },
};

uint32_t eudev_from_godot(uint32_t godot) {
  //cout << "godot key: " << godot << endl;
	for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i) {
		if (map[i].godot_scancode == godot) {
			return map[i].eudev_scancode;
		}
	}
	return 0;
}
