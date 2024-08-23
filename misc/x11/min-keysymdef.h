/***********************************************************
Copyright 1987, 1994, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * The "X11 Window System Protocol" standard defines in Appendix A the
 * keysym codes. These 29-bit integer values identify characters or
 * functions associated with each key (e.g., via the visible
 * engraving) of a keyboard layout. This file assigns mnemonic macro
 * names for these keysyms.
 *
 * This file is also compiled (by src/util/makekeys.c in libX11) into
 * hash tables that can be accessed with X11 library functions such as
 * XStringToKeysym() and XKeysymToString().
 *
 * Where a keysym corresponds one-to-one to an ISO 10646 / Unicode
 * character, this is noted in a comment that provides both the U+xxxx
 * Unicode position, as well as the official Unicode name of the
 * character.
 *
 * Where the correspondence is either not one-to-one or semantically
 * unclear, the Unicode position and name are enclosed in
 * parentheses. Such legacy keysyms should be considered deprecated
 * and are not recommended for use in future keyboard mappings.
 *
 * For any future extension of the keysyms with characters already
 * found in ISO 10646 / Unicode, the following algorithm shall be
 * used. The new keysym code position will simply be the character's
 * Unicode number plus 0x01000000. The keysym values in the range
 * 0x01000100 to 0x0110ffff are reserved to represent Unicode
 * characters in the range U+0100 to U+10FFFF.
 *
 * While most newer Unicode-based X11 clients do already accept
 * Unicode-mapped keysyms in the range 0x01000100 to 0x0110ffff, it
 * will remain necessary for clients -- in the interest of
 * compatibility with existing servers -- to also understand the
 * existing legacy keysym values in the range 0x0100 to 0x20ff.
 *
 * Where several mnemonic names are defined for the same keysym in this
 * file, all but the first one listed should be considered deprecated.
 *
 * Mnemonic names for keysyms are defined in this file with lines
 * that match one of these Perl regular expressions:
 *
 *    /^\#define XK_([a-zA-Z_0-9]+)\s+0x([0-9a-f]+)\s*\/\* U\+([0-9A-F]{4,6}) (.*) \*\/\s*$/
 *    /^\#define XK_([a-zA-Z_0-9]+)\s+0x([0-9a-f]+)\s*\/\*\(U\+([0-9A-F]{4,6}) (.*)\)\*\/\s*$/
 *    /^\#define XK_([a-zA-Z_0-9]+)\s+0x([0-9a-f]+)\s*(\/\*\s*(.*)\s*\*\/)?\s*$/
 *
 * Before adding new keysyms, please do consider the following: In
 * addition to the keysym names defined in this file, the
 * XStringToKeysym() and XKeysymToString() functions will also handle
 * any keysym string of the form "U0020" to "U007E" and "U00A0" to
 * "U10FFFF" for all possible Unicode characters. In other words,
 * every possible Unicode character has already a keysym string
 * defined algorithmically, even if it is not listed here. Therefore,
 * defining an additional keysym macro is only necessary where a
 * non-hexadecimal mnemonic name is needed, or where the new keysym
 * does not represent any existing Unicode character.
 *
 * When adding new keysyms to this file, do not forget to also update the
 * following as needed:
 *
 *   - the mappings in src/KeyBind.c in the libX11 repo
 *     https://gitlab.freedesktop.org/xorg/lib/libx11
 *
 *   - the protocol specification in specs/keysyms.xml in this repo
 *     https://gitlab.freedesktop.org/xorg/proto/xorgproto
 *
 */

#define XK_VoidSymbol                  0xffffff  /* Void symbol */
#define XK_MISCELLANY
#define XK_XKB_KEYS
#define XK_LATIN1

#ifdef XK_MISCELLANY
/*
 * TTY function keys, cleverly chosen to map to ASCII, for convenience of
 * programming, but could have been arbitrary (at the cost of lookup
 * tables in client code).
 */

#define XK_BackSpace                     0xff08  /* Back space, back char */
#define XK_Tab                           0xff09
#define XK_Linefeed                      0xff0a  /* Linefeed, LF */
#define XK_Clear                         0xff0b
#define XK_Return                        0xff0d  /* Return, enter */
#define XK_Pause                         0xff13  /* Pause, hold */
#define XK_Scroll_Lock                   0xff14
#define XK_Sys_Req                       0xff15
#define XK_Escape                        0xff1b
#define XK_Delete                        0xffff  /* Delete, rubout */

/* Cursor control & motion */

#define XK_Home                          0xff50
#define XK_Left                          0xff51  /* Move left, left arrow */
#define XK_Up                            0xff52  /* Move up, up arrow */
#define XK_Right                         0xff53  /* Move right, right arrow */
#define XK_Down                          0xff54  /* Move down, down arrow */
#define XK_Prior                         0xff55  /* Prior, previous */
#define XK_Page_Up                       0xff55
#define XK_Next                          0xff56  /* Next */
#define XK_Page_Down                     0xff56
#define XK_End                           0xff57  /* EOL */
#define XK_Begin                         0xff58  /* BOL */


/* Misc functions */

#define XK_Select                        0xff60  /* Select, mark */
#define XK_Print                         0xff61
#define XK_Execute                       0xff62  /* Execute, run, do */
#define XK_Insert                        0xff63  /* Insert, insert here */
#define XK_Undo                          0xff65
#define XK_Redo                          0xff66  /* Redo, again */
#define XK_Menu                          0xff67
#define XK_Find                          0xff68  /* Find, search */
#define XK_Cancel                        0xff69  /* Cancel, stop, abort, exit */
#define XK_Help                          0xff6a  /* Help */
#define XK_Break                         0xff6b
#define XK_Mode_switch                   0xff7e  /* Character set switch */
#define XK_script_switch                 0xff7e  /* Alias for mode_switch */
#define XK_Num_Lock                      0xff7f

/* Keypad functions, keypad numbers cleverly chosen to map to ASCII */

#define XK_KP_Space                      0xff80  /* Space */
#define XK_KP_Tab                        0xff89
#define XK_KP_Enter                      0xff8d  /* Enter */
#define XK_KP_F1                         0xff91  /* PF1, KP_A, ... */
#define XK_KP_F2                         0xff92
#define XK_KP_F3                         0xff93
#define XK_KP_F4                         0xff94
#define XK_KP_Home                       0xff95
#define XK_KP_Left                       0xff96
#define XK_KP_Up                         0xff97
#define XK_KP_Right                      0xff98
#define XK_KP_Down                       0xff99
#define XK_KP_Prior                      0xff9a
#define XK_KP_Page_Up                    0xff9a
#define XK_KP_Next                       0xff9b
#define XK_KP_Page_Down                  0xff9b
#define XK_KP_End                        0xff9c
#define XK_KP_Begin                      0xff9d
#define XK_KP_Insert                     0xff9e
#define XK_KP_Delete                     0xff9f
#define XK_KP_Equal                      0xffbd  /* Equals */
#define XK_KP_Multiply                   0xffaa
#define XK_KP_Add                        0xffab
#define XK_KP_Separator                  0xffac  /* Separator, often comma */
#define XK_KP_Subtract                   0xffad
#define XK_KP_Decimal                    0xffae
#define XK_KP_Divide                     0xffaf

#define XK_KP_0                          0xffb0
#define XK_KP_1                          0xffb1
#define XK_KP_2                          0xffb2
#define XK_KP_3                          0xffb3
#define XK_KP_4                          0xffb4
#define XK_KP_5                          0xffb5
#define XK_KP_6                          0xffb6
#define XK_KP_7                          0xffb7
#define XK_KP_8                          0xffb8
#define XK_KP_9                          0xffb9



/*
 * Auxiliary functions; note the duplicate definitions for left and right
 * function keys;  Sun keyboards and a few other manufacturers have such
 * function key groups on the left and/or right sides of the keyboard.
 * We've not found a keyboard with more than 35 function keys total.
 */

#define XK_F1                            0xffbe
#define XK_F2                            0xffbf
#define XK_F3                            0xffc0
#define XK_F4                            0xffc1
#define XK_F5                            0xffc2
#define XK_F6                            0xffc3
#define XK_F7                            0xffc4
#define XK_F8                            0xffc5
#define XK_F9                            0xffc6
#define XK_F10                           0xffc7
#define XK_F11                           0xffc8
#define XK_F12                           0xffc9
#define XK_F13                           0xffca
#define XK_F14                           0xffcb
#define XK_F15                           0xffcc
#define XK_F16                           0xffcd
#define XK_F17                           0xffce
#define XK_F18                           0xffcf
#define XK_F19                           0xffd0
#define XK_F20                           0xffd1
#define XK_F21                           0xffd2
#define XK_F22                           0xffd3
#define XK_F23                           0xffd4
#define XK_F24                           0xffd5

/* Modifiers */

#define XK_Shift_L                       0xffe1  /* Left shift */
#define XK_Shift_R                       0xffe2  /* Right shift */
#define XK_Control_L                     0xffe3  /* Left control */
#define XK_Control_R                     0xffe4  /* Right control */
#define XK_Caps_Lock                     0xffe5  /* Caps lock */
#define XK_Shift_Lock                    0xffe6  /* Shift lock */

#define XK_Meta_L                        0xffe7  /* Left meta */
#define XK_Meta_R                        0xffe8  /* Right meta */
#define XK_Alt_L                         0xffe9  /* Left alt */
#define XK_Alt_R                         0xffea  /* Right alt */
#define XK_Super_L                       0xffeb  /* Left super */
#define XK_Super_R                       0xffec  /* Right super */
#define XK_Hyper_L                       0xffed  /* Left hyper */
#define XK_Hyper_R                       0xffee  /* Right hyper */
#endif /* XK_MISCELLANY */

/*
 * Keyboard (XKB) Extension function and modifier keys
 * (from Appendix C of "The X Keyboard Extension: Protocol Specification")
 * Byte 3 = 0xfe
 */

#ifdef XK_XKB_KEYS
#define XK_ISO_Lock                      0xfe01
#define XK_ISO_Level2_Latch              0xfe02
#define XK_ISO_Level3_Shift              0xfe03
#define XK_ISO_Level3_Latch              0xfe04
#define XK_ISO_Level3_Lock               0xfe05
#define XK_ISO_Level5_Shift              0xfe11
#define XK_ISO_Level5_Latch              0xfe12
#define XK_ISO_Level5_Lock               0xfe13
#define XK_ISO_Group_Shift               0xff7e  /* Alias for mode_switch */
#define XK_ISO_Group_Latch               0xfe06
#define XK_ISO_Group_Lock                0xfe07
#define XK_ISO_Next_Group                0xfe08
#define XK_ISO_Next_Group_Lock           0xfe09
#define XK_ISO_Prev_Group                0xfe0a
#define XK_ISO_Prev_Group_Lock           0xfe0b
#define XK_ISO_First_Group               0xfe0c
#define XK_ISO_First_Group_Lock          0xfe0d
#define XK_ISO_Last_Group                0xfe0e
#define XK_ISO_Last_Group_Lock           0xfe0f

#define XK_ISO_Left_Tab                  0xfe20
#define XK_ISO_Move_Line_Up              0xfe21
#define XK_ISO_Move_Line_Down            0xfe22
#define XK_ISO_Partial_Line_Up           0xfe23
#define XK_ISO_Partial_Line_Down         0xfe24
#define XK_ISO_Partial_Space_Left        0xfe25
#define XK_ISO_Partial_Space_Right       0xfe26
#define XK_ISO_Set_Margin_Left           0xfe27
#define XK_ISO_Set_Margin_Right          0xfe28
#define XK_ISO_Release_Margin_Left       0xfe29
#define XK_ISO_Release_Margin_Right      0xfe2a
#define XK_ISO_Release_Both_Margins      0xfe2b
#define XK_ISO_Fast_Cursor_Left          0xfe2c
#define XK_ISO_Fast_Cursor_Right         0xfe2d
#define XK_ISO_Fast_Cursor_Up            0xfe2e
#define XK_ISO_Fast_Cursor_Down          0xfe2f
#define XK_ISO_Continuous_Underline      0xfe30
#define XK_ISO_Discontinuous_Underline   0xfe31
#define XK_ISO_Emphasize                 0xfe32
#define XK_ISO_Center_Object             0xfe33
#define XK_ISO_Enter                     0xfe34

#endif /* XK_XKB_KEYS */

/*
 * Latin 1
 * (ISO/IEC 8859-1 = Unicode U+0020..U+00FF)
 * Byte 3 = 0
 */
#ifdef XK_LATIN1
#define XK_space                         0x0020  /* U+0020 SPACE */
#define XK_exclam                        0x0021  /* U+0021 EXCLAMATION MARK */
#define XK_quotedbl                      0x0022  /* U+0022 QUOTATION MARK */
#define XK_numbersign                    0x0023  /* U+0023 NUMBER SIGN */
#define XK_dollar                        0x0024  /* U+0024 DOLLAR SIGN */
#define XK_percent                       0x0025  /* U+0025 PERCENT SIGN */
#define XK_ampersand                     0x0026  /* U+0026 AMPERSAND */
#define XK_apostrophe                    0x0027  /* U+0027 APOSTROPHE */
#define XK_quoteright                    0x0027  /* deprecated */
#define XK_parenleft                     0x0028  /* U+0028 LEFT PARENTHESIS */
#define XK_parenright                    0x0029  /* U+0029 RIGHT PARENTHESIS */
#define XK_asterisk                      0x002a  /* U+002A ASTERISK */
#define XK_plus                          0x002b  /* U+002B PLUS SIGN */
#define XK_comma                         0x002c  /* U+002C COMMA */
#define XK_minus                         0x002d  /* U+002D HYPHEN-MINUS */
#define XK_period                        0x002e  /* U+002E FULL STOP */
#define XK_slash                         0x002f  /* U+002F SOLIDUS */
#define XK_0                             0x0030  /* U+0030 DIGIT ZERO */
#define XK_1                             0x0031  /* U+0031 DIGIT ONE */
#define XK_2                             0x0032  /* U+0032 DIGIT TWO */
#define XK_3                             0x0033  /* U+0033 DIGIT THREE */
#define XK_4                             0x0034  /* U+0034 DIGIT FOUR */
#define XK_5                             0x0035  /* U+0035 DIGIT FIVE */
#define XK_6                             0x0036  /* U+0036 DIGIT SIX */
#define XK_7                             0x0037  /* U+0037 DIGIT SEVEN */
#define XK_8                             0x0038  /* U+0038 DIGIT EIGHT */
#define XK_9                             0x0039  /* U+0039 DIGIT NINE */
#define XK_colon                         0x003a  /* U+003A COLON */
#define XK_semicolon                     0x003b  /* U+003B SEMICOLON */
#define XK_less                          0x003c  /* U+003C LESS-THAN SIGN */
#define XK_equal                         0x003d  /* U+003D EQUALS SIGN */
#define XK_greater                       0x003e  /* U+003E GREATER-THAN SIGN */
#define XK_question                      0x003f  /* U+003F QUESTION MARK */
#define XK_at                            0x0040  /* U+0040 COMMERCIAL AT */
#define XK_A                             0x0041  /* U+0041 LATIN CAPITAL LETTER A */
#define XK_B                             0x0042  /* U+0042 LATIN CAPITAL LETTER B */
#define XK_C                             0x0043  /* U+0043 LATIN CAPITAL LETTER C */
#define XK_D                             0x0044  /* U+0044 LATIN CAPITAL LETTER D */
#define XK_E                             0x0045  /* U+0045 LATIN CAPITAL LETTER E */
#define XK_F                             0x0046  /* U+0046 LATIN CAPITAL LETTER F */
#define XK_G                             0x0047  /* U+0047 LATIN CAPITAL LETTER G */
#define XK_H                             0x0048  /* U+0048 LATIN CAPITAL LETTER H */
#define XK_I                             0x0049  /* U+0049 LATIN CAPITAL LETTER I */
#define XK_J                             0x004a  /* U+004A LATIN CAPITAL LETTER J */
#define XK_K                             0x004b  /* U+004B LATIN CAPITAL LETTER K */
#define XK_L                             0x004c  /* U+004C LATIN CAPITAL LETTER L */
#define XK_M                             0x004d  /* U+004D LATIN CAPITAL LETTER M */
#define XK_N                             0x004e  /* U+004E LATIN CAPITAL LETTER N */
#define XK_O                             0x004f  /* U+004F LATIN CAPITAL LETTER O */
#define XK_P                             0x0050  /* U+0050 LATIN CAPITAL LETTER P */
#define XK_Q                             0x0051  /* U+0051 LATIN CAPITAL LETTER Q */
#define XK_R                             0x0052  /* U+0052 LATIN CAPITAL LETTER R */
#define XK_S                             0x0053  /* U+0053 LATIN CAPITAL LETTER S */
#define XK_T                             0x0054  /* U+0054 LATIN CAPITAL LETTER T */
#define XK_U                             0x0055  /* U+0055 LATIN CAPITAL LETTER U */
#define XK_V                             0x0056  /* U+0056 LATIN CAPITAL LETTER V */
#define XK_W                             0x0057  /* U+0057 LATIN CAPITAL LETTER W */
#define XK_X                             0x0058  /* U+0058 LATIN CAPITAL LETTER X */
#define XK_Y                             0x0059  /* U+0059 LATIN CAPITAL LETTER Y */
#define XK_Z                             0x005a  /* U+005A LATIN CAPITAL LETTER Z */
#define XK_bracketleft                   0x005b  /* U+005B LEFT SQUARE BRACKET */
#define XK_backslash                     0x005c  /* U+005C REVERSE SOLIDUS */
#define XK_bracketright                  0x005d  /* U+005D RIGHT SQUARE BRACKET */
#define XK_asciicircum                   0x005e  /* U+005E CIRCUMFLEX ACCENT */
#define XK_underscore                    0x005f  /* U+005F LOW LINE */
#define XK_grave                         0x0060  /* U+0060 GRAVE ACCENT */
#define XK_quoteleft                     0x0060  /* deprecated */
#define XK_a                             0x0061  /* U+0061 LATIN SMALL LETTER A */
#define XK_b                             0x0062  /* U+0062 LATIN SMALL LETTER B */
#define XK_c                             0x0063  /* U+0063 LATIN SMALL LETTER C */
#define XK_d                             0x0064  /* U+0064 LATIN SMALL LETTER D */
#define XK_e                             0x0065  /* U+0065 LATIN SMALL LETTER E */
#define XK_f                             0x0066  /* U+0066 LATIN SMALL LETTER F */
#define XK_g                             0x0067  /* U+0067 LATIN SMALL LETTER G */
#define XK_h                             0x0068  /* U+0068 LATIN SMALL LETTER H */
#define XK_i                             0x0069  /* U+0069 LATIN SMALL LETTER I */
#define XK_j                             0x006a  /* U+006A LATIN SMALL LETTER J */
#define XK_k                             0x006b  /* U+006B LATIN SMALL LETTER K */
#define XK_l                             0x006c  /* U+006C LATIN SMALL LETTER L */
#define XK_m                             0x006d  /* U+006D LATIN SMALL LETTER M */
#define XK_n                             0x006e  /* U+006E LATIN SMALL LETTER N */
#define XK_o                             0x006f  /* U+006F LATIN SMALL LETTER O */
#define XK_p                             0x0070  /* U+0070 LATIN SMALL LETTER P */
#define XK_q                             0x0071  /* U+0071 LATIN SMALL LETTER Q */
#define XK_r                             0x0072  /* U+0072 LATIN SMALL LETTER R */
#define XK_s                             0x0073  /* U+0073 LATIN SMALL LETTER S */
#define XK_t                             0x0074  /* U+0074 LATIN SMALL LETTER T */
#define XK_u                             0x0075  /* U+0075 LATIN SMALL LETTER U */
#define XK_v                             0x0076  /* U+0076 LATIN SMALL LETTER V */
#define XK_w                             0x0077  /* U+0077 LATIN SMALL LETTER W */
#define XK_x                             0x0078  /* U+0078 LATIN SMALL LETTER X */
#define XK_y                             0x0079  /* U+0079 LATIN SMALL LETTER Y */
#define XK_z                             0x007a  /* U+007A LATIN SMALL LETTER Z */
#define XK_braceleft                     0x007b  /* U+007B LEFT CURLY BRACKET */
#define XK_bar                           0x007c  /* U+007C VERTICAL LINE */
#define XK_braceright                    0x007d  /* U+007D RIGHT CURLY BRACKET */
#define XK_asciitilde                    0x007e  /* U+007E TILDE */
#endif /* XK_LATIN1 */
