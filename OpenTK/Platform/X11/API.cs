#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * Contributions from Erik Ylvisaker
 * See license.txt for license info
 */
#endregion

using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

#pragma warning disable 3019    // CLS-compliance checking
#pragma warning disable 0649    // struct members not explicitly initialized
#pragma warning disable 0169    // field / method is never used.
#pragma warning disable 0414    // field assigned but never used.

namespace OpenTK.Platform.X11
{
    #region Types

    // using XID = System.Int32;
    using Window = System.IntPtr;
    using Drawable = System.IntPtr;
    using Font = System.IntPtr;
    using Pixmap = System.IntPtr;
    using Cursor = System.IntPtr;
    using Colormap = System.IntPtr;
    using GContext = System.IntPtr;
    using KeySym = System.IntPtr;
    using Mask = System.IntPtr;
    using Atom = System.IntPtr;
    using VisualID = System.IntPtr;
    using Time = System.IntPtr;
    using KeyCode = System.Byte;    // Or maybe ushort?

    using Display = System.IntPtr;
    using XPointer = System.IntPtr;

    // Randr and Xrandr
    using Bool = System.Boolean;
    using XRRScreenConfiguration = System.IntPtr; // opaque datatype
    using Rotation = System.UInt16;
    using Status = System.Int32;
    using SizeID = System.UInt16;

    #endregion

    #region internal static class API

    internal static class API
    {
        static Display defaultDisplay;
        static int screenCount;

        internal static Display DefaultDisplay { get { return defaultDisplay; } }
        internal static int ScreenCount { get { return screenCount; } }        
        internal static object Lock = new object();

        static API() {
            defaultDisplay = Functions.XOpenDisplay_Safe(IntPtr.Zero);                
            if (defaultDisplay == IntPtr.Zero)
                throw new PlatformException("Could not establish connection to the X-Server.");

            screenCount = Functions.XScreenCount(DefaultDisplay);
            Debug.Print("Display connection: {0}, Screen count: {1}", DefaultDisplay, ScreenCount);

            //AppDomain.CurrentDomain.ProcessExit += new EventHandler(CurrentDomain_ProcessExit);
        }

        static void CurrentDomain_ProcessExit(object sender, EventArgs e)
        {
            if (defaultDisplay != IntPtr.Zero)
            {
                Functions.XCloseDisplay(defaultDisplay);
                defaultDisplay = IntPtr.Zero;
            }
        }

        #region Pointer and Keyboard functions

        /// <summary>
        /// The XGetKeyboardMapping() function returns the symbols for the specified number of KeyCodes starting with first_keycode.
        /// </summary>
        /// <param name="display">Specifies the connection to the X server.</param>
        /// <param name="first_keycode">Specifies the first KeyCode that is to be returned.</param>
        /// <param name="keycode_count">Specifies the number of KeyCodes that are to be returned</param>
        /// <param name="keysyms_per_keycode_return">Returns the number of KeySyms per KeyCode.</param>
        /// <returns></returns>
        /// <remarks>
        /// <para>The value specified in first_keycode must be greater than or equal to min_keycode as returned by XDisplayKeycodes(), or a BadValue error results. In addition, the following expression must be less than or equal to max_keycode as returned by XDisplayKeycodes(): </para>
        /// <para>first_keycode + keycode_count - 1 </para>
        /// <para>If this is not the case, a BadValue error results. The number of elements in the KeySyms list is: </para>
        /// <para>keycode_count * keysyms_per_keycode_return </para>
        /// <para>KeySym number N, counting from zero, for KeyCode K has the following index in the list, counting from zero: </para>
        /// <para> (K - first_code) * keysyms_per_code_return + N </para>
        /// <para>The X server arbitrarily chooses the keysyms_per_keycode_return value to be large enough to report all requested symbols. A special KeySym value of NoSymbol is used to fill in unused elements for individual KeyCodes. To free the storage returned by XGetKeyboardMapping(), use XFree(). </para>
        /// <para>XGetKeyboardMapping() can generate a BadValue error.</para>
        /// <para>Diagnostics:</para>
        /// <para>BadValue:    Some numeric value falls outside the range of values accepted by the request. Unless a specific range is specified for an argument, the full range defined by the argument's type is accepted. Any argument defined as a set of alternatives can generate this error.</para>
        /// </remarks>
        [DllImport("libX11")]
        public static extern KeySym XGetKeyboardMapping(Display display, KeyCode first_keycode, int keycode_count,
            ref int keysyms_per_keycode_return);

        /// <summary>
        /// The XDisplayKeycodes() function returns the min-keycodes and max-keycodes supported by the specified display.
        /// </summary>
        /// <param name="display">Specifies the connection to the X server.</param>
        /// <param name="min_keycodes_return">Returns the minimum number of KeyCodes</param>
        /// <param name="max_keycodes_return">Returns the maximum number of KeyCodes.</param>
        /// <remarks> The minimum number of KeyCodes returned is never less than 8, and the maximum number of KeyCodes returned is never greater than 255. Not all KeyCodes in this range are required to have corresponding keys.</remarks>
        [DllImport("libX11")]
        public static extern void XDisplayKeycodes(Display display, ref int min_keycodes_return, ref int max_keycodes_return);

        #endregion

        [DllImport("libX11")]
        public static extern KeySym XLookupKeysym(ref XKeyEvent key_event, int index);

    }
    #endregion

    #region X11 Structures

    #region internal class XVisualInfo

    [StructLayout(LayoutKind.Sequential)]
    struct XVisualInfo
    {
        public IntPtr Visual;
        public VisualID VisualID;
        public int Screen;
        public int Depth;
        public XVisualClass Class;
        public long RedMask;
        public long GreenMask;
        public long blueMask;
        public int ColormapSize;
        public int BitsPerRgb;
    }

    #endregion

    #region internal struct XRRScreenSize

    internal struct XRRScreenSize
    {
        internal int Width, Height;
        internal int MWidth, MHeight;
    }

    #endregion

    #region unsafe internal struct Screen

    unsafe internal struct Screen
    {
        XExtData ext_data;    /* hook for extension to hang buffer */
        IntPtr display;     /* back pointer to display structure */ /* _XDisplay */
        Window root;        /* Root window id. */
        int width, height;    /* width and height of screen */
        int mwidth, mheight;    /* width and height of  in millimeters */
        int ndepths;        /* number of depths possible */
        //Depth *depths;        /* list of allowable depths on the screen */
        int root_depth;        /* bits per pixel */
        //Visual* root_visual;    /* root visual */
        IntPtr default_gc;        /* GC for the root root visual */   // GC
        Colormap cmap;        /* default color map */
        UIntPtr white_pixel;    // unsigned long
        UIntPtr black_pixel;    /* White and Black pixel values */  // unsigned long
        int max_maps, min_maps;    /* max and min color maps */
        int backing_store;    /* Never, WhenMapped, Always */
        Bool save_unders;    
        long root_input_mask;    /* initial root input mask */
    }

    #endregion

    #region unsafe internal class XExtData

    unsafe internal class XExtData
    {
        int number;        /* number returned by XRegisterExtension */
        XExtData next;    /* next item on list of buffer for structure */
        delegate int FreePrivateDelegate(XExtData extension);
        FreePrivateDelegate FreePrivate;    /* called to free private storage */
        XPointer private_data;    /* buffer private to this extension. */
    }

    #endregion
    
    #region Motif
    
    [StructLayout(LayoutKind.Sequential)]
    internal struct MotifWmHints
    {
        internal IntPtr flags;
        internal IntPtr functions;
        internal IntPtr decorations;
        internal IntPtr input_mode;
        internal IntPtr status;

        public override string ToString ()
        {
            return string.Format("MotifWmHints <flags={0}, functions={1}, decorations={2}, input_mode={3}, status={4}", (MotifFlags) flags.ToInt32 (), (MotifFunctions) functions.ToInt32 (), (MotifDecorations) decorations.ToInt32 (), (MotifInputMode) input_mode.ToInt32 (), status.ToInt32 ());
        }
    }
    
    [Flags]
    internal enum MotifFlags
    {
        Functions    = 1,
        Decorations  = 2,
        InputMode    = 4,
        Status       = 8
    }

    [Flags]
    internal enum MotifFunctions
    {
        All         = 0x01,
        Resize      = 0x02,
        Move        = 0x04,
        Minimize    = 0x08,
        Maximize    = 0x10,
        Close       = 0x20
    }

    [Flags]
    internal enum MotifDecorations
    {
        All         = 0x01,
        Border      = 0x02,
        ResizeH     = 0x04,
        Title       = 0x08,
        Menu        = 0x10,
        Minimize    = 0x20,
        Maximize    = 0x40,
        
    }

    [Flags]
    internal enum MotifInputMode
    {
        Modeless                = 0,
        ApplicationModal        = 1,
        SystemModal             = 2,
        FullApplicationModal    = 3
    }
    
    #endregion

    #endregion

    #region X11 Constants and Enums

    internal enum ErrorCodes : int
    {
        Success = 0,
        BadRequest = 1,
        BadValue = 2,
        BadWindow = 3,
        BadPixmap = 4,
        BadAtom = 5,
        BadCursor = 6,
        BadFont = 7,
        BadMatch = 8,
        BadDrawable = 9,
        BadAccess = 10,
        BadAlloc = 11,
        BadColor = 12,
        BadGC = 13,
        BadIDChoice = 14,
        BadName = 15,
        BadLength = 16,
        BadImplementation = 17,
    }

    #region XKey

    /// <summary>
    /// Defines LATIN-1 and miscellaneous keys.
    /// </summary>
    internal enum XKey
    {
        /*
         * TTY function keys, cleverly chosen to map to ASCII, for convenience of
         * programming, but could have been arbitrary (at the cost of lookup
         * tables in client code).
         */

        BackSpace                   = 0xff08,  /* Back space, back char */
        Tab                         = 0xff09,
        Linefeed                    = 0xff0a,  /* Linefeed, LF */
        Clear                       = 0xff0b,
        Return                      = 0xff0d,  /* Return, enter */
        Pause                       = 0xff13,  /* Pause, hold */
        Scroll_Lock                 = 0xff14,
        Sys_Req                     = 0xff15,
        Escape                      = 0xff1b,
        Delete                      = 0xffff,  /* Delete, rubout */



        /* International & multi-key character composition */

        Multi_key                   = 0xff20,  /* Multi-key character compose */
        Codeinput                   = 0xff37,
        SingleCandidate             = 0xff3c,
        MultipleCandidate           = 0xff3d,
        PreviousCandidate           = 0xff3e,
                
        /* Japanese keyboard support */

        Kanji                       = 0xff21,  /* Kanji, Kanji convert */
        Muhenkan                    = 0xff22,  /* Cancel Conversion */
        Henkan_Mode                 = 0xff23,  /* Start/Stop Conversion */
        Henkan                      = 0xff23,  /* Alias for Henkan_Mode */
        Romaji                      = 0xff24,  /* to Romaji */
        Hiragana                    = 0xff25,  /* to Hiragana */
        Katakana                    = 0xff26,  /* to Katakana */
        Hiragana_Katakana           = 0xff27,  /* Hiragana/Katakana toggle */
        Zenkaku                     = 0xff28,  /* to Zenkaku */
        Hankaku                     = 0xff29,  /* to Hankaku */
        Zenkaku_Hankaku             = 0xff2a,  /* Zenkaku/Hankaku toggle */
        Touroku                     = 0xff2b,  /* Add to Dictionary */
        Massyo                      = 0xff2c,  /* Delete from Dictionary */
        Kana_Lock                   = 0xff2d,  /* Kana Lock */
        Kana_Shift                  = 0xff2e,  /* Kana Shift */
        Eisu_Shift                  = 0xff2f,  /* Alphanumeric Shift */
        Eisu_toggle                 = 0xff30,  /* Alphanumeric toggle */
        Kanji_Bangou                = 0xff37,  /* Codeinput */
        Zen_Koho                    = 0xff3d,  /* Multiple/All Candidate(s) */
        Mae_Koho                    = 0xff3e,  /* Previous Candidate */

        /* 0xff31 thru 0xff3f are under XK_KOREAN */

        /* Cursor control & motion */

        Home                        = 0xff50,
        Left                        = 0xff51,  /* Move left, left arrow */
        Up                          = 0xff52,  /* Move up, up arrow */
        Right                       = 0xff53,  /* Move right, right arrow */
        Down                        = 0xff54,  /* Move down, down arrow */
        Prior                       = 0xff55,  /* Prior, previous */
        Page_Up                     = 0xff55,
        Next                        = 0xff56,  /* Next */
        Page_Down                   = 0xff56,
        End                         = 0xff57,  /* EOL */
        Begin                       = 0xff58,  /* BOL */


        /* Misc functions */

        Select                      = 0xff60,  /* Select, mark */
        Print                       = 0xff61,
        Execute                     = 0xff62,  /* Execute, run, do */
        Insert                      = 0xff63,  /* Insert, insert here */
        Undo                        = 0xff65,
        Redo                        = 0xff66,  /* Redo, again */
        Menu                        = 0xff67,
        Find                        = 0xff68,  /* Find, search */
        Cancel                      = 0xff69,  /* Cancel, stop, abort, exit */
        Help                        = 0xff6a,  /* Help */
        Break                       = 0xff6b,
        Mode_switch                 = 0xff7e,  /* Character set switch */
        script_switch               = 0xff7e,  /* Alias for mode_switch */
        Num_Lock                    = 0xff7f,

        /* Keypad functions, keypad numbers cleverly chosen to map to ASCII */

        KP_Space                    = 0xff80,  /* Space */
        KP_Tab                      = 0xff89,
        KP_Enter                    = 0xff8d,  /* Enter */
        KP_F1                       = 0xff91,  /* PF1, KP_A, ... */
        KP_F2                       = 0xff92,
        KP_F3                       = 0xff93,
        KP_F4                       = 0xff94,
        KP_Home                     = 0xff95,
        KP_Left                     = 0xff96,
        KP_Up                       = 0xff97,
        KP_Right                    = 0xff98,
        KP_Down                     = 0xff99,
        KP_Prior                    = 0xff9a,
        KP_Page_Up                  = 0xff9a,
        KP_Next                     = 0xff9b,
        KP_Page_Down                = 0xff9b,
        KP_End                      = 0xff9c,
        KP_Begin                    = 0xff9d,
        KP_Insert                   = 0xff9e,
        KP_Delete                   = 0xff9f,
        KP_Equal                    = 0xffbd,  /* Equals */
        KP_Multiply                 = 0xffaa,
        KP_Add                      = 0xffab,
        KP_Separator                = 0xffac,  /* Separator, often comma */
        KP_Subtract                 = 0xffad,
        KP_Decimal                  = 0xffae,
        KP_Divide                   = 0xffaf,

        KP_0                        = 0xffb0,
        KP_1                        = 0xffb1,
        KP_2                        = 0xffb2,
        KP_3                        = 0xffb3,
        KP_4                        = 0xffb4,
        KP_5                        = 0xffb5,
        KP_6                        = 0xffb6,
        KP_7                        = 0xffb7,
        KP_8                        = 0xffb8,
        KP_9                        = 0xffb9,

        /*
         * Auxiliary functions; note the duplicate definitions for left and right
         * function keys;  Sun keyboards and a few other manufacturers have such
         * function key groups on the left and/or right sides of the keyboard.
         * We've not found a keyboard with more than 35 function keys total.
         */

        F1                          = 0xffbe,
        F2                          = 0xffbf,
        F3                          = 0xffc0,
        F4                          = 0xffc1,
        F5                          = 0xffc2,
        F6                          = 0xffc3,
        F7                          = 0xffc4,
        F8                          = 0xffc5,
        F9                          = 0xffc6,
        F10                         = 0xffc7,
        F11                         = 0xffc8,
        F12                         = 0xffc9,
        F13                         = 0xffca,
        F14                         = 0xffcb,
        F15                         = 0xffcc,
        F16                         = 0xffcd,
        F17                         = 0xffce,
        F18                         = 0xffcf,
        F19                         = 0xffd0,
        F20                         = 0xffd1,
        F21                         = 0xffd2,
        F22                         = 0xffd3,
        F23                         = 0xffd4,
        F24                         = 0xffd5,
        F25                         = 0xffd6,
        F26                         = 0xffd7,
        F27                         = 0xffd8,
        F28                         = 0xffd9,
        F29                         = 0xffda,
        F30                         = 0xffdb,
        F31                         = 0xffdc,
        F32                         = 0xffdd,
        F33                         = 0xffde,
        F34                         = 0xffdf,
        F35                         = 0xffe0,

        /* Modifiers */

        Shift_L                     = 0xffe1,  /* Left shift */
        Shift_R                     = 0xffe2,  /* Right shift */
        Control_L                   = 0xffe3,  /* Left control */
        Control_R                   = 0xffe4,  /* Right control */
        Caps_Lock                   = 0xffe5,  /* Caps lock */
        Shift_Lock                  = 0xffe6,  /* Shift lock */

        Meta_L                      = 0xffe7,  /* Left meta */
        Meta_R                      = 0xffe8,  /* Right meta */
        Alt_L                       = 0xffe9,  /* Left alt */
        Alt_R                       = 0xffea,  /* Right alt */
        Super_L                     = 0xffeb,  /* Left super */
        Super_R                     = 0xffec,  /* Right super */
        Hyper_L                     = 0xffed,  /* Left hyper */
        Hyper_R                     = 0xffee,  /* Right hyper */

        /*
         * Latin 1
         * (ISO/IEC 8859-1 = Unicode U+0020..U+00FF)
         * Byte 3 = 0
         */

        space                       = 0x0020,  /* U+0020 SPACE */
        exclam                      = 0x0021,  /* U+0021 EXCLAMATION MARK */
        quotedbl                    = 0x0022,  /* U+0022 QUOTATION MARK */
        numbersign                  = 0x0023,  /* U+0023 NUMBER SIGN */
        dollar                      = 0x0024,  /* U+0024 DOLLAR SIGN */
        percent                     = 0x0025,  /* U+0025 PERCENT SIGN */
        ampersand                   = 0x0026,  /* U+0026 AMPERSAND */
        apostrophe                  = 0x0027,  /* U+0027 APOSTROPHE */
        quoteright                  = 0x0027,  /* deprecated */
        parenleft                   = 0x0028,  /* U+0028 LEFT PARENTHESIS */
        parenright                  = 0x0029,  /* U+0029 RIGHT PARENTHESIS */
        asterisk                    = 0x002a,  /* U+002A ASTERISK */
        plus                        = 0x002b,  /* U+002B PLUS SIGN */
        comma                       = 0x002c,  /* U+002C COMMA */
        minus                       = 0x002d,  /* U+002D HYPHEN-MINUS */
        period                      = 0x002e,  /* U+002E FULL STOP */
        slash                       = 0x002f,  /* U+002F SOLIDUS */
        Number0                           = 0x0030,  /* U+0030 DIGIT ZERO */
        Number1                           = 0x0031,  /* U+0031 DIGIT ONE */
        Number2                           = 0x0032,  /* U+0032 DIGIT TWO */
        Number3                           = 0x0033,  /* U+0033 DIGIT THREE */
        Number4                           = 0x0034,  /* U+0034 DIGIT FOUR */
        Number5                           = 0x0035,  /* U+0035 DIGIT FIVE */
        Number6                           = 0x0036,  /* U+0036 DIGIT SIX */
        Number7                           = 0x0037,  /* U+0037 DIGIT SEVEN */
        Number8                           = 0x0038,  /* U+0038 DIGIT EIGHT */
        Number9                     = 0x0039,  /* U+0039 DIGIT NINE */
        colon                       = 0x003a,  /* U+003A COLON */
        semicolon                   = 0x003b,  /* U+003B SEMICOLON */
        less                        = 0x003c,  /* U+003C LESS-THAN SIGN */
        equal                       = 0x003d,  /* U+003D EQUALS SIGN */
        greater                     = 0x003e,  /* U+003E GREATER-THAN SIGN */
        question                    = 0x003f,  /* U+003F QUESTION MARK */
        at                          = 0x0040,  /* U+0040 COMMERCIAL AT */
        A                           = 0x0041,  /* U+0041 LATIN CAPITAL LETTER A */
        B                           = 0x0042,  /* U+0042 LATIN CAPITAL LETTER B */
        C                           = 0x0043,  /* U+0043 LATIN CAPITAL LETTER C */
        D                           = 0x0044,  /* U+0044 LATIN CAPITAL LETTER D */
        E                           = 0x0045,  /* U+0045 LATIN CAPITAL LETTER E */
        F                           = 0x0046,  /* U+0046 LATIN CAPITAL LETTER F */
        G                           = 0x0047,  /* U+0047 LATIN CAPITAL LETTER G */
        H                           = 0x0048,  /* U+0048 LATIN CAPITAL LETTER H */
        I                           = 0x0049,  /* U+0049 LATIN CAPITAL LETTER I */
        J                           = 0x004a,  /* U+004A LATIN CAPITAL LETTER J */
        K                           = 0x004b,  /* U+004B LATIN CAPITAL LETTER K */
        L                           = 0x004c,  /* U+004C LATIN CAPITAL LETTER L */
        M                           = 0x004d,  /* U+004D LATIN CAPITAL LETTER M */
        N                           = 0x004e,  /* U+004E LATIN CAPITAL LETTER N */
        O                           = 0x004f,  /* U+004F LATIN CAPITAL LETTER O */
        P                           = 0x0050,  /* U+0050 LATIN CAPITAL LETTER P */
        Q                           = 0x0051,  /* U+0051 LATIN CAPITAL LETTER Q */
        R                           = 0x0052,  /* U+0052 LATIN CAPITAL LETTER R */
        S                           = 0x0053,  /* U+0053 LATIN CAPITAL LETTER S */
        T                           = 0x0054,  /* U+0054 LATIN CAPITAL LETTER T */
        U                           = 0x0055,  /* U+0055 LATIN CAPITAL LETTER U */
        V                           = 0x0056,  /* U+0056 LATIN CAPITAL LETTER V */
        W                           = 0x0057,  /* U+0057 LATIN CAPITAL LETTER W */
        X                           = 0x0058,  /* U+0058 LATIN CAPITAL LETTER X */
        Y                           = 0x0059,  /* U+0059 LATIN CAPITAL LETTER Y */
        Z                           = 0x005a,  /* U+005A LATIN CAPITAL LETTER Z */
        bracketleft                 = 0x005b,  /* U+005B LEFT SQUARE BRACKET */
        backslash                   = 0x005c,  /* U+005C REVERSE SOLIDUS */
        bracketright                = 0x005d,  /* U+005D RIGHT SQUARE BRACKET */
        asciicircum                 = 0x005e,  /* U+005E CIRCUMFLEX ACCENT */
        underscore                  = 0x005f,  /* U+005F LOW LINE */
        grave                       = 0x0060,  /* U+0060 GRAVE ACCENT */
        quoteleft                   = 0x0060,  /* deprecated */
        a                           = 0x0061,  /* U+0061 LATIN SMALL LETTER A */
        b                           = 0x0062,  /* U+0062 LATIN SMALL LETTER B */
        c                           = 0x0063,  /* U+0063 LATIN SMALL LETTER C */
        d                           = 0x0064,  /* U+0064 LATIN SMALL LETTER D */
        e                           = 0x0065,  /* U+0065 LATIN SMALL LETTER E */
        f                           = 0x0066,  /* U+0066 LATIN SMALL LETTER F */
        g                           = 0x0067,  /* U+0067 LATIN SMALL LETTER G */
        h                           = 0x0068,  /* U+0068 LATIN SMALL LETTER H */
        i                           = 0x0069,  /* U+0069 LATIN SMALL LETTER I */
        j                           = 0x006a,  /* U+006A LATIN SMALL LETTER J */
        k                           = 0x006b,  /* U+006B LATIN SMALL LETTER K */
        l                           = 0x006c,  /* U+006C LATIN SMALL LETTER L */
        m                           = 0x006d,  /* U+006D LATIN SMALL LETTER M */
        n                           = 0x006e,  /* U+006E LATIN SMALL LETTER N */
        o                           = 0x006f,  /* U+006F LATIN SMALL LETTER O */
        p                           = 0x0070,  /* U+0070 LATIN SMALL LETTER P */
        q                           = 0x0071,  /* U+0071 LATIN SMALL LETTER Q */
        r                           = 0x0072,  /* U+0072 LATIN SMALL LETTER R */
        s                           = 0x0073,  /* U+0073 LATIN SMALL LETTER S */
        t                           = 0x0074,  /* U+0074 LATIN SMALL LETTER T */
        u                           = 0x0075,  /* U+0075 LATIN SMALL LETTER U */
        v                           = 0x0076,  /* U+0076 LATIN SMALL LETTER V */
        w                           = 0x0077,  /* U+0077 LATIN SMALL LETTER W */
        x                           = 0x0078,  /* U+0078 LATIN SMALL LETTER X */
        y                           = 0x0079,  /* U+0079 LATIN SMALL LETTER Y */
        z                           = 0x007a,  /* U+007A LATIN SMALL LETTER Z */
        braceleft                   = 0x007b,  /* U+007B LEFT CURLY BRACKET */
        bar                         = 0x007c,  /* U+007C VERTICAL LINE */
        braceright                  = 0x007d,  /* U+007D RIGHT CURLY BRACKET */
        asciitilde                  = 0x007e,  /* U+007E TILDE */
    }

    #endregion

#pragma warning disable 1591

    public enum XVisualClass : int
    {
        StaticGray = 0,
        GrayScale = 1,
        StaticColor = 2,
        PseudoColor = 3,
        TrueColor = 4,
        DirectColor = 5,
    }

#pragma warning restore 1591

    [Flags]
    internal enum XVisualInfoMask
    {
        No = 0x0,
        ID = 0x1,
        Screen = 0x2,
        Depth = 0x4,
        Class = 0x8,
        Red = 0x10,
        Green = 0x20,
        Blue = 0x40,
        ColormapSize = 0x80,
        BitsPerRGB = 0x100,
        All = 0x1FF,
    }

    #endregion

    internal static partial class Functions
    {
        internal const string X11Library = "libX11";

        #region Xrandr

        const string XrandrLibrary = "libXrandr.so.2";

        [DllImport(XrandrLibrary)]
        public static extern Bool XRRQueryExtension(Display dpy, ref int event_basep, ref int error_basep);

        [DllImport(XrandrLibrary)]
        public static extern Status XRRQueryVersion(Display dpy, ref int major_versionp, ref int minor_versionp);

        [DllImport(XrandrLibrary)]
        public static extern XRRScreenConfiguration XRRGetScreenInfo(Display dpy, Drawable draw);

        [DllImport(XrandrLibrary)]
        public static extern void XRRFreeScreenConfigInfo(XRRScreenConfiguration config);

        [DllImport(XrandrLibrary)]
        public static extern Status XRRSetScreenConfig(Display dpy, XRRScreenConfiguration config,
            Drawable draw, int size_index, ref Rotation rotation, Time timestamp);

        [DllImport(XrandrLibrary)]
        public static extern Status XRRSetScreenConfigAndRate(Display dpy, XRRScreenConfiguration config,
            Drawable draw, int size_index, Rotation rotation, short rate, Time timestamp);

        [DllImport(XrandrLibrary)]
        public static extern Rotation XRRConfigRotations(XRRScreenConfiguration config, ref Rotation current_rotation);

        [DllImport(XrandrLibrary)]
        public static extern Time XRRConfigTimes(XRRScreenConfiguration config, ref Time config_timestamp);

        [DllImport(XrandrLibrary)]
        [return: MarshalAs(UnmanagedType.LPStruct)]
        public static extern XRRScreenSize XRRConfigSizes(XRRScreenConfiguration config, int[] nsizes);

        [DllImport(XrandrLibrary)]
        unsafe public static extern short* XRRConfigRates(XRRScreenConfiguration config, int size_index, int[] nrates);

        [DllImport(XrandrLibrary)]
        public static extern SizeID XRRConfigCurrentConfiguration(XRRScreenConfiguration config, out Rotation rotation);

        [DllImport(XrandrLibrary)]
        public static extern short XRRConfigCurrentRate(XRRScreenConfiguration config);

        [DllImport(XrandrLibrary)]
        public static extern int XRRRootToScreen(Display dpy, Window root);

        [DllImport(XrandrLibrary)]
        public static extern XRRScreenConfiguration XRRScreenConfig(Display dpy, int screen);

        [DllImport(XrandrLibrary)]
        public static extern XRRScreenConfiguration XRRConfig(ref Screen screen);

        [DllImport(XrandrLibrary)]
        public static extern void XRRSelectInput(Display dpy, Window window, int mask);

        /*
         * intended to take RRScreenChangeNotify,  or
         * ConfigureNotify (on the root window)
         * returns 1 if it is an event type it understands, 0 if not
         */
        [DllImport(XrandrLibrary)]
        public static extern int XRRUpdateConfiguration(ref XEvent @event);

        /*
         * the following are always safe to call, even if RandR is
         * not implemented on a screen
         */
        [DllImport(XrandrLibrary)]
        public static extern Rotation XRRRotations(Display dpy, int screen, ref Rotation current_rotation);

        [DllImport(XrandrLibrary)]
        unsafe static extern IntPtr XRRSizes(Display dpy, int screen, int* nsizes);

        public static XRRScreenSize[] XRRSizes(Display dpy, int screen)
        {
            XRRScreenSize[] sizes;
            //IntPtr ptr;
            int count;
            unsafe
            {
                //ptr = XRRSizes(dpy, screen, &nsizes);

                byte* data = (byte*)XRRSizes(dpy, screen, &count);//(byte*)ptr;
                if (count == 0)
                    return null;
                sizes = new XRRScreenSize[count];
                for (int i = 0; i < count; i++)
                {
                    sizes[i] = new XRRScreenSize();
                    sizes[i] = (XRRScreenSize)Marshal.PtrToStructure((IntPtr)data, typeof(XRRScreenSize));
                    data += Marshal.SizeOf(typeof(XRRScreenSize));
                }
                //XFree(ptr);   // Looks like we must not free this.
                return sizes;
            }
        }

        [DllImport(XrandrLibrary)]
        unsafe static extern short* XRRRates(Display dpy, int screen, int size_index, int* nrates);

        public static short[] XRRRates(Display dpy, int screen, int size_index)
        {
            short[] rates;
            int count;
            unsafe
            {
                short* data = (short*)XRRRates(dpy, screen, size_index, &count);
                if (count == 0)
                    return null;
                rates = new short[count];
                for (int i = 0; i < count; i++)
                    rates[i] = *(data + i);
            }
            return rates;
        }

        [DllImport(XrandrLibrary)]
        public static extern Time XRRTimes(Display dpy, int screen, out Time config_timestamp);

        #endregion

        #region Display, Screen and Window functions

        [DllImport(X11Library)]
        public static extern int XScreenCount(Display display);

        [DllImport(X11Library)]
        unsafe static extern int* XListDepths(Display display, int screen_number, int* count_return);

        public static int[] XListDepths(Display display, int screen_number)
        {
            unsafe
            {
                int count;
                int* data = XListDepths(display, screen_number, &count);
                if (count == 0)
                    return null;
                int[] depths = new int[count];
                for (int i = 0; i < count; i++)
                    depths[i] = *(data + i);

                return depths;
            }
        }

        #endregion
    }
}

#pragma warning restore 3019
#pragma warning restore 0649
#pragma warning restore 0169
#pragma warning restore 0414