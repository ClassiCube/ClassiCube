#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * Contributions from Erik Ylvisaker
 * See license.txt for license info
 */
 #endregion

using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using RECT = OpenTK.Platform.Windows.Win32Rectangle;

namespace OpenTK.Platform.Windows {
	
	internal static class API {
		
		[DllImport("user32.dll"), SuppressUnmanagedCodeSecurity]
		internal static extern bool SetWindowPos(IntPtr handle, IntPtr insertAfter, int x, int y, int cx, int cy, SetWindowPosFlags flags);
		
		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern bool AdjustWindowRect([In, Out] ref Win32Rectangle lpRect, WindowStyle dwStyle, bool bMenu);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern bool AdjustWindowRectEx(ref Win32Rectangle lpRect, WindowStyle dwStyle, bool bMenu, ExtendedWindowStyle dwExStyle);

		[DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto), SuppressUnmanagedCodeSecurity]
		internal static extern IntPtr CreateWindowEx(ExtendedWindowStyle ExStyle, IntPtr ClassAtom, IntPtr WindowName, WindowStyle Style,
			int X, int Y, int Width, int Height, IntPtr HandleToParentWindow, IntPtr Menu, IntPtr Instance, IntPtr Param);
		
		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern bool DestroyWindow(IntPtr windowHandle);	
		
		[DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto), SuppressUnmanagedCodeSecurity]
		internal static extern ushort RegisterClassEx(ref ExtendedWindowClass window_class);

		[DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto), SuppressUnmanagedCodeSecurity]
		internal static extern short UnregisterClass(IntPtr className, IntPtr instance);
		
		internal static IntPtr SetWindowLong_N(IntPtr handle, GetWindowLongOffsets item, IntPtr newValue) {
			return IntPtr.Size == 4 ? (IntPtr)SetWindowLong(handle, item, newValue.ToInt32()) :
				SetWindowLongPtr(handle, item, newValue);
		}

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		static extern int SetWindowLong(IntPtr hWnd, GetWindowLongOffsets nIndex, int dwNewLong);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		static extern IntPtr SetWindowLongPtr(IntPtr hWnd, GetWindowLongOffsets nIndex, IntPtr dwNewLong);

		internal static UIntPtr GetWindowLong_N(IntPtr handle, GetWindowLongOffsets index) {
			return IntPtr.Size == 4 ? (UIntPtr)GetWindowLong(handle, index) : GetWindowLongPtr(handle, index);
		}

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		static extern uint GetWindowLong(IntPtr hWnd, GetWindowLongOffsets nIndex);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		static extern UIntPtr GetWindowLongPtr(IntPtr hWnd, GetWindowLongOffsets nIndex);
		
		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern IntPtr GetForegroundWindow();

		[DllImport("User32.dll"), SuppressUnmanagedCodeSecurity]
		internal static extern bool PeekMessage(ref MSG msg, IntPtr hWnd, int messageFilterMin, int messageFilterMax, int flags);

		[DllImport("user32.dll", CharSet = CharSet.Auto), SuppressUnmanagedCodeSecurity]
		internal static extern IntPtr SendMessage(IntPtr hWnd, WindowMessage Msg, IntPtr wParam, IntPtr lParam);
		
		[DllImport("User32.dll", CharSet = CharSet.Auto), SuppressUnmanagedCodeSecurity]
		internal static extern bool PostMessage(IntPtr hWnd, WindowMessage Msg, IntPtr wParam, IntPtr lParam);
		
		[DllImport("User32.dll"), SuppressUnmanagedCodeSecurity]
		internal static extern IntPtr DispatchMessage(ref MSG msg);

		[DllImport("User32.dll"), SuppressUnmanagedCodeSecurity]
		internal static extern bool TranslateMessage(ref MSG lpMsg);

		[DllImport("User32.dll", CharSet = CharSet.Auto), SuppressUnmanagedCodeSecurity]
		public extern static IntPtr DefWindowProc(IntPtr hWnd, WindowMessage msg, IntPtr wParam, IntPtr lParam);
		
		[DllImport("user32.dll"), SuppressUnmanagedCodeSecurity]
		internal static extern IntPtr GetDC(IntPtr hwnd);

		[DllImport("user32.dll"), SuppressUnmanagedCodeSecurity]
		internal static extern bool ReleaseDC(IntPtr hwnd, IntPtr DC);

		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		internal static extern int ChoosePixelFormat(IntPtr dc, ref PixelFormatDescriptor pfd);
		
		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		internal static extern int DescribePixelFormat(IntPtr deviceContext, int pixel, int pfdSize, ref PixelFormatDescriptor pixelFormat);
		
		[DllImport("gdi32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern bool SetPixelFormat(IntPtr dc, int format, ref PixelFormatDescriptor pfd);
		
		[DllImport("gdi32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern bool SwapBuffers(IntPtr dc);

		[DllImport("kernel32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern IntPtr LoadLibrary(string dllName);

		[DllImport("kernel32", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern uint MapVirtualKey(VirtualKeys vkey, MapVirtualKeyType uMapType);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern bool ShowWindow(IntPtr hWnd, ShowWindowCommand nCmdShow);		
		
		[DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto), SuppressUnmanagedCodeSecurity]
		internal static extern bool SetWindowText(IntPtr hWnd, [MarshalAs(UnmanagedType.LPTStr)] string lpString);
		
		[DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto), SuppressUnmanagedCodeSecurity]
		internal static extern int GetWindowText(IntPtr hWnd, [MarshalAs(UnmanagedType.LPTStr), In, Out] StringBuilder lpString, int nMaxCount);
		
		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		//internal static extern bool ScreenToClient(IntPtr hWnd, ref POINT point);
		internal static extern bool ScreenToClient(IntPtr hWnd, ref Point point);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal extern static bool GetClientRect(IntPtr windowHandle, out Win32Rectangle clientRectangle);

		[DllImport("user32.dll"), SuppressUnmanagedCodeSecurity]
		public static extern bool IsWindowVisible(IntPtr intPtr);

		[DllImport("user32.dll"), SuppressUnmanagedCodeSecurity]
		public static extern IntPtr LoadCursor(IntPtr hInstance, IntPtr lpCursorName);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		public static extern bool SetForegroundWindow(IntPtr hWnd);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		public static extern bool BringWindowToTop(IntPtr hWnd);

		[DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto), SuppressUnmanagedCodeSecurity]
		public static extern int ChangeDisplaySettingsEx([MarshalAs(UnmanagedType.LPTStr)] string lpszDeviceName,
		                                                  DeviceMode lpDevMode, IntPtr hwnd, ChangeDisplaySettingsEnum dwflags, IntPtr lParam);

		[DllImport("user32.dll", SetLastError = true, CharSet=CharSet.Auto), SuppressUnmanagedCodeSecurity]
		public static extern bool EnumDisplayDevices([MarshalAs(UnmanagedType.LPTStr)] string lpDevice,
		                                             int iDevNum, [In, Out] WindowsDisplayDevice lpDisplayDevice, uint dwFlags);

		[DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto), SuppressUnmanagedCodeSecurity]
		internal static extern bool EnumDisplaySettings([MarshalAs(UnmanagedType.LPTStr)] string device_name,
		                                                int graphics_mode, [In, Out] DeviceMode device_mode);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		public static extern bool GetMonitorInfo(IntPtr hMonitor, ref MonitorInfo lpmi);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		public static extern IntPtr MonitorFromWindow(IntPtr hwnd, MonitorFrom dwFlags);
		
		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		public static extern bool TrackMouseEvent(ref TrackMouseEventStructure lpEventTrack);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern bool GetCursorPos(ref POINT point);

		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern bool SetCursorPos(int x, int y);
		
		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern bool ShowCursor( int value );
		
		[DllImport("user32.dll", SetLastError = true), SuppressUnmanagedCodeSecurity]
		internal static extern ushort GetKeyState( int code );
	}

	internal struct Constants {
		// Device mode types (found in wingdi.h)
		internal const int DM_BITSPERPEL = 0x00040000;
		internal const int DM_PELSWIDTH = 0x00080000;
		internal const int DM_PELSHEIGHT = 0x00100000;
		internal const int DM_DISPLAYFLAGS = 0x00200000;
		internal const int DM_DISPLAYFREQUENCY = 0x00400000;

		// ChangeDisplaySettings results (found in winuser.h)
		internal const int DISP_CHANGE_SUCCESSFUL = 0;
		internal const int DISP_CHANGE_RESTART = 1;
		internal const int DISP_CHANGE_FAILED = -1;
	}
	
	internal struct CreateStruct {
		internal IntPtr lpCreateParams;
		internal IntPtr hInstance;
		internal IntPtr hMenu;
		internal IntPtr hwndParent;
		internal int cy;
		internal int cx;
		internal int y;
		internal int x;
		internal int style;
		[MarshalAs(UnmanagedType.LPTStr)]
		internal string lpszName;
		[MarshalAs(UnmanagedType.LPTStr)]
		internal string lpszClass;
		internal uint dwExStyle;
	}

	struct StyleStruct {
		public WindowStyle Old;
		public WindowStyle New;
	}
	
	[StructLayout(LayoutKind.Sequential)]
	internal struct PixelFormatDescriptor {
		internal short Size;
		internal short Version;
		internal PixelFormatDescriptorFlags Flags;
		internal PixelType PixelType;
		internal byte ColorBits;
		internal byte RedBits;
		internal byte RedShift;
		internal byte GreenBits;
		internal byte GreenShift;
		internal byte BlueBits;
		internal byte BlueShift;
		internal byte AlphaBits;
		internal byte AlphaShift;
		internal byte AccumBits;
		internal byte AccumRedBits;
		internal byte AccumGreenBits;
		internal byte AccumBlueBits;
		internal byte AccumAlphaBits;
		internal byte DepthBits;
		internal byte StencilBits;
		internal byte AuxBuffers;
		internal byte LayerType;
		private byte Reserved;
		internal int LayerMask;
		internal int VisibleMask;
		internal int DamageMask;
		
		public const short DefaultVersion = 1;
		public const short DefaultSize = 40;
	}
	
	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
	internal class DeviceMode {
		internal DeviceMode() {
			Size = (short)Marshal.SizeOf(this);
		}

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
		internal string DeviceName;
		internal short SpecVersion;
		internal short DriverVersion;
		private short Size;
		internal short DriverExtra;
		internal int Fields;

		internal POINT Position;
		internal uint DisplayOrientation;
		internal uint DisplayFixedOutput;

		internal short Color;
		internal short Duplex;
		internal short YResolution;
		internal short TTOption;
		internal short Collate;
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
		internal string FormName;
		internal short LogPixels;
		internal int BitsPerPel;
		internal int PelsWidth;
		internal int PelsHeight;
		internal int DisplayFlags;
		internal int DisplayFrequency;
		internal int ICMMethod;
		internal int ICMIntent;
		internal int MediaType;
		internal int DitherType;
		internal int Reserved1;
		internal int Reserved2;
		internal int PanningWidth;
		internal int PanningHeight;
	}
	
	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
	internal class WindowsDisplayDevice {
		
		internal WindowsDisplayDevice() {
			size = Marshal.SizeOf(this);
		}
		readonly int size;
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
		internal string DeviceName;
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
		internal string DeviceString;
		internal DisplayDeviceStateFlags StateFlags;    // uint
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
		internal string DeviceID;
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
		internal string DeviceKey;
	}

	[StructLayout(LayoutKind.Sequential)]
	internal struct WindowClass {
		internal ClassStyle Style;
		[MarshalAs(UnmanagedType.FunctionPtr)]
		internal WindowProcedure WindowProcedure;
		internal int ClassExtraBytes;
		internal int WindowExtraBytes;
		//[MarshalAs(UnmanagedType.
		internal IntPtr Instance;
		internal IntPtr Icon;
		internal IntPtr Cursor;
		internal IntPtr BackgroundBrush;
		//[MarshalAs(UnmanagedType.LPStr)]
		internal IntPtr MenuName;
		[MarshalAs(UnmanagedType.LPTStr)]
		internal string ClassName;

		internal static int SizeInBytes = Marshal.SizeOf(default(WindowClass));
	}
		
	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
	struct ExtendedWindowClass
	{
		public uint Size;
		public ClassStyle Style;
		[MarshalAs(UnmanagedType.FunctionPtr)]
		public WindowProcedure WndProc;
		public int cbClsExtra;
		public int cbWndExtra;
		public IntPtr Instance;
		public IntPtr Icon;
		public IntPtr Cursor;
		public IntPtr Background;
		public IntPtr MenuName;
		public IntPtr ClassName;
		public IntPtr IconSm;

		public static uint SizeInBytes = (uint)Marshal.SizeOf(default(ExtendedWindowClass));
	}

	[StructLayout(LayoutKind.Sequential)]
	internal struct WindowPosition
	{
		internal IntPtr hwnd;
		internal IntPtr hwndInsertAfter;
		internal int x;
		internal int y;
		internal int cx;
		internal int cy;
		[MarshalAs(UnmanagedType.U4)]
		internal SetWindowPosFlags flags;
	}
	
	[Flags]
	internal enum SetWindowPosFlags : int
	{
		NOSIZE          = 0x0001,
		NOMOVE          = 0x0002,
		NOZORDER        = 0x0004,
		NOREDRAW        = 0x0008,
		NOACTIVATE      = 0x0010,
		FRAMECHANGED    = 0x0020, /* The frame changed: send WM_NCCALCSIZE */
		SHOWWINDOW      = 0x0040,
		HIDEWINDOW      = 0x0080,
		NOCOPYBITS      = 0x0100,
		NOOWNERZORDER   = 0x0200, /* Don't do owner Z ordering */
		NOSENDCHANGING  = 0x0400, /* Don't send WM_WINDOWPOSCHANGING */

		DRAWFRAME       = FRAMECHANGED,
		NOREPOSITION    = NOOWNERZORDER,

		DEFERERASE      = 0x2000,
		ASYNCWINDOWPOS  = 0x4000
	}

	[StructLayout(LayoutKind.Sequential)]
	internal struct Win32Rectangle {
		
		internal Win32Rectangle(int width, int height) {
			left = top = 0;
			right = width;
			bottom = height;
		}

		internal int left;
		internal int top;
		internal int right;
		internal int bottom;

		internal int Width { get { return right - left; } }
		internal int Height { get { return bottom - top; } }

		public override string ToString() {
			return String.Format("({0},{1})-({2},{3})", left, top, right, bottom);
		}

		internal Rectangle ToRectangle() {
			return Rectangle.FromLTRB(left, top, right, bottom);
		}

		internal static Win32Rectangle From(Rectangle value) {
			Win32Rectangle rect = new Win32Rectangle();
			rect.left = value.Left;
			rect.right = value.Right;
			rect.top = value.Top;
			rect.bottom = value.Bottom;
			return rect;
		}

		internal static Win32Rectangle From(Size value) {
			Win32Rectangle rect = new Win32Rectangle();
			rect.left = 0;
			rect.right = value.Width;
			rect.top = 0;
			rect.bottom = value.Height;
			return rect;
		}
	}

	struct MonitorInfo {
		public int Size;
		public RECT Monitor;
		public RECT Work;
		public uint Flags;

		public static readonly int SizeInBytes = Marshal.SizeOf(default(MonitorInfo));
	}
	
	struct TrackMouseEventStructure {
		public int Size;
		public TrackMouseEventFlags Flags;
		public IntPtr TrackWindowHandle;
		public uint HoverTime;

		public static readonly int SizeInBytes = Marshal.SizeOf(typeof(TrackMouseEventStructure));
	}
	
	enum GWL {
		WNDPROC = (-4),
		IntPtr = (-6),
		HWNDPARENT = (-8),
		STYLE = (-16),
		EXSTYLE = (-20),
		USERDATA = (-21),
		ID = (-12),
	}

	internal enum SizeMessage {
		MAXHIDE = 4,
		MAXIMIZED = 2,
		MAXSHOW = 3,
		MINIMIZED = 1,
		RESTORED = 0
	}
	
	internal enum DisplayModeSettings {
		Current = -1,
		Registry = -2
	}

	[Flags]
	internal enum DisplayDeviceStateFlags {
		None              = 0x00000000,
		AttachedToDesktop = 0x00000001,
		MultiDriver       = 0x00000002,
		PrimaryDevice     = 0x00000004,
		MirroringDriver   = 0x00000008,
		VgaCompatible     = 0x00000010,
		Removable         = 0x00000020,
		ModesPruned       = 0x08000000,
		Remote            = 0x04000000,
		Disconnect        = 0x02000000,

		// Child device state
		Active            = 0x00000001,
		Attached          = 0x00000002,
	}
	
	[Flags]
	internal enum ChangeDisplaySettingsEnum {
		// ChangeDisplaySettings types (found in winuser.h)
		UpdateRegistry = 0x00000001,
		Test = 0x00000002,
		Fullscreen = 0x00000004,
	}

	[Flags]
	internal enum WindowStyle : uint {
		Overlapped = 0x00000000,
		Popup = 0x80000000,
		Child = 0x40000000,
		Minimize = 0x20000000,
		Visible = 0x10000000,
		Disabled = 0x08000000,
		ClipSiblings = 0x04000000,
		ClipChildren = 0x02000000,
		Maximize = 0x01000000,
		Caption = 0x00C00000,    // Border | DialogFrame
		Border = 0x00800000,
		DialogFrame = 0x00400000,
		VScroll = 0x00200000,
		HScreen = 0x00100000,
		SystemMenu = 0x00080000,
		ThickFrame = 0x00040000,
		Group = 0x00020000,
		TabStop = 0x00010000,

		MinimizeBox = 0x00020000,
		MaximizeBox = 0x00010000,

		Tiled = Overlapped,
		Iconic = Minimize,
		SizeBox = ThickFrame,
		TiledWindow = OverlappedWindow,

		// Common window styles:
		OverlappedWindow = Overlapped | Caption | SystemMenu | ThickFrame | MinimizeBox | MaximizeBox,
		PopupWindow = Popup | Border | SystemMenu,
		ChildWindow = Child
	}

	[Flags]
	internal enum ExtendedWindowStyle : uint {
		WindowEdge = 0x00000100,
		ApplicationWindow = 0x00040000,
	}
	
	internal enum GetWindowLongOffsets : int {
		STYLE = (-16),
	}
	
	[Flags]
	internal enum PixelFormatDescriptorFlags : int {
		DOUBLEBUFFER = 0x01,
		DRAW_TO_WINDOW = 0x04,
		SUPPORT_OPENGL = 0x20,
		DEPTH_DONTCARE = 0x20000000,
	}	
	
	internal enum PixelType : byte {
		RGBA = 0,
		INDEXED = 1
	}
	
	[Flags]
	internal enum ClassStyle
	{
		//None            = 0x0000,
		VRedraw = 0x0001,
		HRedraw = 0x0002,
		DoubleClicks = 0x0008,
		OwnDC = 0x0020,
		ClassDC = 0x0040,
		ParentDC = 0x0080,
		NoClose = 0x0200,
		SaveBits = 0x0800,
		ByteAlignClient = 0x1000,
		ByteAlignWindow = 0x2000,
		GlobalClass = 0x4000,

		Ime = 0x00010000,

		// #if(_WIN32_WINNT >= 0x0501)
		DropShadow = 0x00020000
			// #endif /* _WIN32_WINNT >= 0x0501 */
	}
	
	internal enum VirtualKeys : short {
		// Virtual Key, Standard Set
		BACK         = 0x08,
		TAB          = 0x09,

		CLEAR        = 0x0C,
		RETURN       = 0x0D,

		SHIFT        = 0x10,
		CONTROL      = 0x11,
		MENU         = 0x12,
		PAUSE        = 0x13,
		CAPITAL      = 0x14,

		ESCAPE       = 0x1B,
		
		SPACE        = 0x20,
		PRIOR        = 0x21,
		NEXT         = 0x22,
		END          = 0x23,
		HOME         = 0x24,
		LEFT         = 0x25,
		UP           = 0x26,
		RIGHT        = 0x27,
		DOWN         = 0x28,
		PRINT        = 0x2A,
		SNAPSHOT     = 0x2C,
		INSERT       = 0x2D,
		DELETE       = 0x2E,
		HELP         = 0x2F,
		
		// 0 - 9 are the same as ASCII '0' - '9' (0x30 - 0x39)
		// A - Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)

		LWIN         = 0x5B,
		RWIN         = 0x5C,
		APPS         = 0x5D,
		SLEEP        = 0x5F,
		NUMPAD0      = 0x60,
		NUMPAD1      = 0x61,
		NUMPAD2      = 0x62,
		NUMPAD3      = 0x63,
		NUMPAD4      = 0x64,
		NUMPAD5      = 0x65,
		NUMPAD6      = 0x66,
		NUMPAD7      = 0x67,
		NUMPAD8      = 0x68,
		NUMPAD9      = 0x69,
		MULTIPLY     = 0x6A,
		ADD          = 0x6B,
		SEPARATOR    = 0x6C,
		SUBTRACT     = 0x6D,
		DECIMAL      = 0x6E,
		DIVIDE       = 0x6F,
		F1           = 0x70,
		F24          = 0x87,

		NUMLOCK      = 0x90,
		SCROLL       = 0x91,

		/* L* & R* - left and right Alt, Ctrl and Shift virtual keys.
		 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
		 * No other API or message will distinguish left and right keys in this way. */
		LSHIFT       = 0xA0,
		RSHIFT       = 0xA1,
		LCONTROL     = 0xA2,
		RCONTROL     = 0xA3,
		LMENU        = 0xA4,
		RMENU        = 0xA5,

		OEM_1        = 0xBA,   // ';:' for US
		OEM_PLUS     = 0xBB,   // '+' any country
		OEM_COMMA    = 0xBC,   // ',' any country
		OEM_MINUS    = 0xBD,   // '-' any country
		OEM_PERIOD   = 0xBE,   // '.' any country
		OEM_2        = 0xBF,   // '/?' for US
		OEM_3        = 0xC0,   // '`~' for US

		OEM_4        = 0xDB,  //  '[{' for US
		OEM_5        = 0xDC,  //  '\|' for US
		OEM_6        = 0xDD,  //  ']}' for US
		OEM_7        = 0xDE,  //  ''"' for US
		OEM_8        = 0xDF,
	}

	enum MouseKeys {
		None = 0x00, //No mouse button was pressed.		
		Left = 0x01, // The left mouse button was pressed.	
		Right = 0x02, // The right mouse button was pressed.		
		Middle = 0x10, // The middle mouse button was pressed.		
		XButton1 = 0x20, // The first XButton was pressed.		
		XButton2 = 0x40, // The second XButton was pressed.
	}
	
	internal enum WindowMessage : uint {
		NULL = 0x0000,
		CREATE = 0x0001,
		DESTROY = 0x0002,
		MOVE = 0x0003,
		SIZE = 0x0005,
		ACTIVATE = 0x0006,
		KILLFOCUS = 0x0008,
		CLOSE = 0x0010,
		ERASEBKGND = 0x0014,
		SHOWWINDOW = 0x0018,
		WINDOWPOSCHANGED = 0x0047,
		STYLECHANGED = 0x007D,
		SETICON = 0x0080,
		KEYDOWN = 0x0100,
		KEYFIRST = 0x0100,
		KEYUP = 0x0101,
		CHAR = 0x0102,
		SYSKEYDOWN = 0x0104,
		SYSKEYUP = 0x0105,
		SYSCHAR = 0x0106,
		MOUSEMOVE = 0x0200,
		LBUTTONDOWN = 0x0201,
		LBUTTONUP = 0x0202,
		RBUTTONDOWN = 0x0204,
		RBUTTONUP = 0x0205,
		MBUTTONDOWN = 0x0207,
		MBUTTONUP = 0x0208,
		MOUSEWHEEL = 0x020A,
		XBUTTONDOWN = 0x020B,
		XBUTTONUP = 0x020C,
		ENTERMENULOOP = 0x0211,
		EXITMENULOOP = 0x0212,
		ENTERSIZEMOVE = 0x0231,
		EXITSIZEMOVE = 0x0232,
		MOUSEHOVER = 0x02A1,
		MOUSELEAVE = 0x02A3,
	}
	
	internal enum ShowWindowCommand {
		HIDE            = 0,
		SHOWNORMAL      = 1,
		NORMAL          = 1,
		SHOWMINIMIZED   = 2,
		SHOWMAXIMIZED   = 3,
		MAXIMIZE        = 3,
		SHOWNOACTIVATE  = 4,
		SHOW            = 5,
		MINIMIZE        = 6,
		SHOWMINNOACTIVE = 7,
		SHOWNA          = 8,
		RESTORE         = 9,
		SHOWDEFAULT     = 10,
		FORCEMINIMIZE   = 11,
		//MAX             = 11,

		// Old ShowWindow() Commands
		//HIDE_WINDOW        = 0,
		//SHOW_OPENWINDOW    = 1,
		//SHOW_ICONWINDOW    = 2,
		//SHOW_FULLSCREEN    = 3,
		//SHOW_OPENNOACTIVATE= 4,
	}
	
	internal enum MapVirtualKeyType {
		VirtualKeyToScanCode = 0,
		ScanCodeToVirtualKey = 1,
		VirtualKeyToCharacter = 2,
		ScanCodeToVirtualKeyExtended = 3,
		VirtualKeyToScanCodeExtended = 4,
	}
	
	enum MonitorFrom {
		Null = 0,
		Primary = 1,
		Nearest = 2,
	}
	
	[Flags]
	enum TrackMouseEventFlags : uint {
		HOVER = 0x00000001,
		LEAVE = 0x00000002,
		NONCLIENT = 0x00000010,
		QUERY = 0x40000000,
		CANCEL = 0x80000000,
	}

	internal delegate IntPtr WindowProcedure(IntPtr handle, WindowMessage message, IntPtr wParam, IntPtr lParam);
	
	[StructLayout(LayoutKind.Sequential)]
	internal struct MSG {
		internal IntPtr HWnd;
		internal WindowMessage Message;
		internal IntPtr WParam;
		internal IntPtr LParam;
		internal uint Time;
		internal POINT Point;

		public override string ToString() {
			return String.Format("msg=0x{0:x} ({1}) hwnd=0x{2:x} wparam=0x{3:x} lparam=0x{4:x} pt=0x{5:x}", (int)Message, Message.ToString(), HWnd.ToInt32(), WParam.ToInt32(), LParam.ToInt32(), Point);
		}
	}
	
	[StructLayout(LayoutKind.Sequential)]
	internal struct POINT {
		internal int X;
		internal int Y;

		internal POINT(int x, int y) {
			this.X = x;
			this.Y = y;
		}

		internal Point ToPoint() {
			return new Point(X, Y);
		}

		public override string ToString() {
			return "Point {" + X.ToString() + ", " + Y.ToString() + ")";
		}
	}
}