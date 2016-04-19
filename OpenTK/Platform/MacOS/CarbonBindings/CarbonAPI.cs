//  Created by Erik Ylvisaker on 3/17/08.
//  Copyright 2008. All rights reserved.

using System;
using System.Runtime.InteropServices;
using System.Drawing;
using EventTime = System.Double;


namespace OpenTK.Platform.MacOS.Carbon
{

	#region --- Types defined in MacTypes.h ---

	[StructLayout(LayoutKind.Sequential)]
	public struct CarbonPoint
	{
		public short V;
		public short H;

		public CarbonPoint(int x, int y)
		{
			V = (short)x;
			H = (short)y;
		}
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct Rect
	{
		short top;
		short left;
		short bottom;
		short right;

		public Rect(short _left, short _top, short _width, short _height) {
			top = _top;
			left = _left;
			bottom = (short)(_top + _height);
			right = (short)(_left + _width);
		}

		public short X {
			get { return left; }
		}
		
		public short Y {
			get { return top; }
		}
		
		public short Width {
			get { return (short)(right - left); }
		}
		
		public short Height {
			get { return (short)(bottom - top); }
		}

		public override string ToString() {
			return string.Format(
				"Rect: [{0}, {1}, {2}, {3}]", X, Y, Width, Height);
		}

		public Rectangle ToRectangle() {
			return new Rectangle(X, Y, Width, Height);
		}
	}

	#endregion
	#region --- Types defined in HIGeometry.h ---

	[StructLayout(LayoutKind.Sequential)]
	public struct HIPoint {
		public IntPtr xVal;
		public IntPtr yVal;
		
		public float X {
			get { return GetFloat( xVal ); }
			set { SetFloat( ref xVal, value ); }
		}
		
		public float Y {
			get { return GetFloat( yVal ); }
			set { SetFloat( ref yVal, value ); }
		}
		
		static unsafe float GetFloat( IntPtr val ) {
			if( IntPtr.Size == 8 ) {
				long raw = val.ToInt64();
				return (float)(*((double*)&raw));
			} else {
				int raw = val.ToInt32();
				return *((float*)&raw);
			}
		}
		
		static unsafe void SetFloat( ref IntPtr val, float x ) {
			if( IntPtr.Size == 8 ) {
				long raw = 0;
				*((double*)&raw) = x;
				val = new IntPtr( raw );
			} else {
				int raw = 0;
				*((float*)&raw) = x;
				val = new IntPtr( raw );
			}
		}
	}
	
	[StructLayout(LayoutKind.Sequential)]
	public struct HIRect {
		public HIPoint Origin;
		public HIPoint Size;

		public override string ToString() {
			return string.Format(
				"Rect: [{0}, {1}, {2}, {3}]", Origin.X, Origin.Y, Size.X, Size.Y);
		}
	}

	#endregion

	public struct EventInfo {
		
		public EventInfo(IntPtr eventRef) {
			EventClass = API.GetEventClass(eventRef);
			EventKind = API.GetEventKind(eventRef);
		}

		public uint EventKind;
		public EventClass EventClass;

		public override string ToString() {
			return "Event: " + EventClass + ",kind: " + EventKind;
		}
	}
	
	#region --- Types defined in CarbonEvents.h ---

	enum EventAttributes : uint
	{
		kEventAttributeNone = 0,
		kEventAttributeUserEvent = (1 << 0),
		kEventAttributeMonitored = 1 << 3,
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct EventTypeSpec
	{
		public EventTypeSpec(EventClass evtClass, AppEventKind evtKind)
		{
			this.EventClass = evtClass;
			this.EventKind = (uint)evtKind;
		}
		public EventTypeSpec(EventClass evtClass, AppleEventKind appleKind)
		{
			this.EventClass = evtClass;
			this.EventKind = (uint)appleKind;
		}
		public EventTypeSpec(EventClass evtClass, MouseEventKind evtKind)
		{
			this.EventClass = evtClass;
			this.EventKind = (uint)evtKind;
		}
		public EventTypeSpec(EventClass evtClass, KeyboardEventKind evtKind)
		{
			this.EventClass = evtClass;
			this.EventKind = (uint)evtKind;
		}
		public EventTypeSpec(EventClass evtClass, WindowEventKind evtKind)
		{
			this.EventClass = evtClass;
			this.EventKind = (uint)evtKind;
		}

		public EventClass EventClass;
		public uint EventKind;
	}

	public enum EventClass : int
	{
		/*
        kEventClassMouse            = FOUR_CHAR_CODE('mous'),
        kEventClassKeyboard         = FOUR_CHAR_CODE('keyb'),
        kEventClassTextInput        = FOUR_CHAR_CODE('text'),
        kEventClassApplication      = FOUR_CHAR_CODE('appl'),
        kEventClassAppleEvent       = FOUR_CHAR_CODE('eppc'),
        kEventClassMenu             = FOUR_CHAR_CODE('menu'),
        kEventClassWindow           = FOUR_CHAR_CODE('wind'),
        kEventClassControl          = FOUR_CHAR_CODE('cntl'),
        kEventClassCommand          = FOUR_CHAR_CODE('cmds')
		 */
		Mouse = 0x6d6f7573,
		Keyboard = 0x6b657962,
		Application = 0x6170706c,
		AppleEvent = 0x65707063,
		Menu = 0x6d656e75,
		Window = 0x77696e64,
	}
	public enum WindowEventKind : int
	{
		// window events
		WindowUpdate = 1,
		WindowDrawContent = 2,
		WindowDrawStructure = 3,
		WindowEraseContent = 4,
		WindowActivate = 5,
		WindowDeactivate = 6,
		WindowSizeChanged = 23,
		WindowBoundsChanging = 26,
		WindowBoundsChanged = 27,
		WindowClickDragRgn    = 32,
		WindowClickResizeRgn  = 33,
		WindowClickCollapseRgn = 34,
		WindowClickCloseRgn   = 35,
		WindowClickZoomRgn    = 36,
		WindowClickContentRgn = 37,
		WindowClickProxyIconRgn = 38,
		WindowClose = 72,
		WindowClosed = 73,
	}
	public enum MouseEventKind : int
	{
		MouseDown = 1,
		MouseUp = 2,
		MouseMoved = 5,
		MouseDragged = 6,
		MouseEntered = 8,
		MouseExited = 9,
		WheelMoved = 10,
	}
	public enum MacOSMouseButton : short
	{
		Primary = 1,
		Secondary = 2,
		Tertiary = 3,
	}

	public enum KeyboardEventKind : int
	{
		// raw keyboard events
		RawKeyDown = 1,
		RawKeyRepeat = 2,
		RawKeyUp = 3,
		RawKeyModifiersChanged = 4,
	}

	public enum AppEventKind : int
	{
		// application events
		AppActivated = 1,
		AppDeactivated = 2,
		AppQuit = 3,
		AppLaunchNotification = 4,
	}

	public enum AppleEventKind : int
	{
		AppleEvent = 1,
	}

	public enum EventParamName : int
	{
		WindowRef = 0x77696e64,           // typeWindowRef,

		// Mouse Events
		MouseLocation = 0x6d6c6f63,       // typeHIPoint
		WindowMouseLocation = 0x776d6f75, // typeHIPoint
		MouseButton = 0x6d62746e,         // typeMouseButton
		ClickCount = 0x63636e74,          // typeUInt32
		MouseWheelAxis = 0x6d776178,      // typeMouseWheelAxis
		MouseWheelDelta = 0x6d77646c,     // typeSInt32
		MouseDelta = 0x6d647461,          // typeHIPoint

		// Keyboard events
		KeyCode = 0x6b636f64,             // typeUInt32
		KeyMacCharCode = 0x6b636872,      // typechar
		KeyModifiers = 0x6b6d6f64,        // typeUInt32
		
	}
	public enum EventParamType : int
	{
		typeWindowRef = 0x77696e64,

		typeMouseButton = 0x6d62746e,
		typeMouseWheelAxis = 0x6d776178,
		typeHIPoint = 0x68697074,
		typeHISize = 0x6869737a,
		typeHIRect = 0x68697263,

		typeChar = 0x54455854,

		typeUInt32 = 0x6d61676e,
		typeSInt32 = 0x6c6f6e67,
		typeSInt16 = 0x73686f72,
		typeSInt64 = 0x636f6d70,
		typeIEEE32BitFloatingPoint = 0x73696e67,
		typeIEEE64BitFloatingPoint = 0x646f7562,
	}

	public enum EventMouseButton : int
	{
		Primary = 0,
		Secondary = 1,
		Tertiary = 2,
	}

	public enum WindowRegionCode : int
	{
		TitleBarRegion = 0,
		TitleTextRegion = 1,
		CloseBoxRegion = 2,
		ZoomBoxRegion = 3,
		DragRegion = 5,
		GrowRegion = 6,
		CollapseBoxRegion = 7,
		TitleProxyIconRegion = 8,
		StructureRegion = 32,
		ContentRegion = 33,
		UpdateRegion = 34,
		OpaqueRegion = 35,
		GlobalPortRegion = 40,
		ToolbarButtonRegion = 41
	};

	#endregion

	#region --- MacWindows.h ---

	public enum WindowClass : uint
	{
		Alert            = 1,             /* "I need your attention now."*/
		MovableAlert     = 2,             /* "I need your attention now, but I'm kind enough to let you switch out of this app to do other things."*/
		Modal            = 3,             /* system modal, not draggable*/
		MovableModal     = 4,             /* application modal, draggable*/
		Floating         = 5,             /* floats above all other application windows*/
		Document         = 6,             /* document windows*/
		Desktop          = 7,             /* desktop window (usually only one of these exists) - OS X only in CarbonLib 1.0*/
		Utility          = 8,             /* Available in CarbonLib 1.1 and later, and in Mac OS X*/
		Help             = 10,            /* Available in CarbonLib 1.1 and later, and in Mac OS X*/
		Sheet            = 11,            /* Available in CarbonLib 1.3 and later, and in Mac OS X*/
		Toolbar          = 12,            /* Available in CarbonLib 1.1 and later, and in Mac OS X*/
		Plain            = 13,            /* Available in CarbonLib 1.2.5 and later, and Mac OS X*/
		Overlay          = 14,            /* Available in Mac OS X*/
		SheetAlert       = 15,            /* Available in CarbonLib 1.3 and later, and in Mac OS X 10.1 and later*/
		AltPlain         = 16,            /* Available in CarbonLib 1.3 and later, and in Mac OS X 10.1 and later*/
		Drawer           = 20,            /* Available in Mac OS X 10.2 or later*/
		All              = 0xFFFFFFFFu    /* for use with GetFrontWindowOfClass, FindWindowOfClass, GetNextWindowOfClass*/
	}

	[Flags]
	public enum WindowAttributes : uint
	{
		NoAttributes         = 0u,         /* no attributes*/
		CloseBox             = (1u << 0),  /* window has a close box*/
		HorizontalZoom       = (1u << 1),  /* window has horizontal zoom box*/
		VerticalZoom         = (1u << 2),  /* window has vertical zoom box*/
		FullZoom             = (VerticalZoom | HorizontalZoom),
		CollapseBox          = (1u << 3),  /* window has a collapse box*/
		Resizable            = (1u << 4),  /* window is resizable*/
		SideTitlebar         = (1u << 5),  /* window wants a titlebar on the side    (floating window class only)*/
		NoUpdates            = (1u << 16), /* this window receives no update events*/
		NoActivates          = (1u << 17), /* this window receives no activate events*/
		NoBuffering          = (1u << 20), /* this window is not buffered (Mac OS X only)*/
		StandardHandler      = (1u << 25),
		InWindowMenu         = (1u << 27),
		LiveResize           = (1u << 28),
		StandardDocument     = (CloseBox | FullZoom | CollapseBox | Resizable),
		StandardFloating     = (CloseBox | CollapseBox)
	}

	public enum WindowPositionMethod : uint
	{
		CenterOnMainScreen = 1,
		CenterOnParentWindow = 2,
		CenterOnParentWindowScreen = 3,
		CascadeOnMainScreen = 4,
		CascadeOnParentWindow = 5,
		CascadeOnParentWindowScreen = 6,
		CascadeStartAtParentWindowScreen = 10,
		AlertPositionOnMainScreen = 7,
		AlertPositionOnParentWindow = 8,
		AlertPositionOnParentWindowScreen = 9
	}

	public delegate OSStatus MacOSEventHandler(IntPtr inCaller, IntPtr inEvent, IntPtr userData);

	public enum WindowPartCode : short
	{
		inDesk = 0,
		inNoWindow = 0,
		inMenuBar = 1,
		inSysWindow = 2,
		inContent = 3,
		inDrag = 4,
		inGrow = 5,
		inGoAway = 6,
		inZoomIn = 7,
		inZoomOut = 8,
		inCollapseBox = 11,
		inProxyIcon = 12,
		inToolbarButton = 13,
		inStructure = 15,
	}

	#endregion
	#region --- Enums from gestalt.h ---

	public enum GestaltSelector
	{
		SystemVersion       = 0x73797376,  // FOUR_CHAR_CODE("sysv"), /* system version*/
		SystemVersionMajor  = 0x73797331,  // FOUR_CHAR_CODE("sys1"), /* The major system version number; in 10.4.17 this would be the decimal value 10 */
		SystemVersionMinor  = 0x73797332,  // FOUR_CHAR_CODE("sys2"), /* The minor system version number; in 10.4.17 this would be the decimal value 4 */
		SystemVersionBugFix = 0x73797333,  // FOUR_CHAR_CODE("sys3") /* The bug fix system version number; in 10.4.17 this would be the decimal value 17 */
	};

	#endregion
	#region --- Process Manager ---

	public enum ProcessApplicationTransformState : int
	{
		kProcessTransformToForegroundApplication = 1,
	}

	public struct ProcessSerialNumber
	{
		public ulong high;
		public ulong low;
	}

	#endregion


	public enum HICoordinateSpace
	{
		_72DPIGlobal      = 1,
		ScreenPixel      = 2,
		Window           = 3,
		View             = 4
	};

	#region --- Carbon API Methods ---

	public class API
	{
		const string carbon = "/System/Library/Frameworks/Carbon.framework/Versions/Current/Carbon";
		
		[DllImport(carbon)]
		public static extern EventClass GetEventClass(IntPtr inEvent);
		[DllImport(carbon)]
		public static extern uint GetEventKind(IntPtr inEvent);

		#region --- Window Construction ---

		[DllImport(carbon)]
		public static extern OSStatus CreateNewWindow(WindowClass @class, WindowAttributes attributes, ref Rect r, out IntPtr window);

		[DllImport(carbon)]
		public static extern void DisposeWindow(IntPtr window);

		#endregion
		#region --- Showing / Hiding Windows ---

		[DllImport(carbon)]
		public static extern void ShowWindow(IntPtr window);
		[DllImport(carbon)]
		public static extern void HideWindow(IntPtr window);
		[DllImport(carbon)]
		public static extern bool IsWindowVisible(IntPtr window);
		[DllImport(carbon)]
		public static extern void SelectWindow(IntPtr window);

		#endregion
		#region --- Window Boundaries ---

		[DllImport(carbon)]
		public static extern OSStatus RepositionWindow(IntPtr window, IntPtr parentWindow, WindowPositionMethod method);
		[DllImport(carbon)]
		public static extern void SizeWindow(IntPtr window, short w, short h, bool fUpdate);
		[DllImport(carbon)]
		public static extern void MoveWindow(IntPtr window, short x, short y, bool fUpdate);

		[DllImport(carbon)]
		static extern OSStatus GetWindowBounds(IntPtr window, WindowRegionCode regionCode, out Rect globalBounds);
		public static Rect GetWindowBounds(IntPtr window, WindowRegionCode regionCode)
		{
			Rect retval;
			OSStatus error = GetWindowBounds(window, regionCode, out retval);
			CheckReturn( error );
			return retval;
		}

		//[DllImport(carbon)]
		//public static extern void MoveWindow(IntPtr window, short hGlobal, short vGlobal, bool front);

		#endregion
		#region --- Processing Events ---

		[DllImport(carbon)]
		static extern IntPtr GetEventDispatcherTarget();

		[DllImport(carbon,EntryPoint="ReceiveNextEvent")]
		static extern OSStatus ReceiveNextEvent(uint inNumTypes,
		                                        IntPtr inList,
		                                        double inTimeout,
		                                        bool inPullEvent,
		                                        out IntPtr outEvent);

		[DllImport(carbon)]
		static extern void SendEventToEventTarget(IntPtr theEvent, IntPtr theTarget);

		[DllImport(carbon)]
		static extern void ReleaseEvent(IntPtr theEvent);

		public static void SendEvent(IntPtr theEvent)
		{
			IntPtr theTarget = GetEventDispatcherTarget();
			SendEventToEventTarget(theEvent, theTarget);
		}

		// Processes events in the queue and then returns.
		public static void ProcessEvents()
		{
			IntPtr theEvent;
			IntPtr theTarget = GetEventDispatcherTarget();
			
			for (;;)
			{
				OSStatus status = ReceiveNextEvent(0, IntPtr.Zero, 0.0, true, out theEvent);

				if (status == OSStatus.EventLoopTimedOut)
					break;

				if (status != OSStatus.NoError)
				{
					Debug.Print("Message Loop status: {0}", status);
					break;
				}
				if (theEvent == IntPtr.Zero)
					break;

				try
				{
					SendEventToEventTarget(theEvent, theTarget);
				}
				catch (System.ExecutionEngineException e)
				{
					Console.Error.WriteLine("ExecutionEngineException caught.");
					Console.Error.WriteLine("theEvent: " + new EventInfo(theEvent).ToString());
					Console.Error.WriteLine(e.Message);
					Console.Error.WriteLine(e.StackTrace);
				}

				ReleaseEvent(theEvent);
			}

		}

		#region --- Processing apple event ---

		[StructLayout(LayoutKind.Sequential)]

		struct EventRecord
		{
			public ushort what;
			public uint message;
			public uint when;
			public CarbonPoint where;
			public uint modifiers;
		}

		[DllImport(carbon)]
		static extern bool ConvertEventRefToEventRecord(IntPtr inEvent, out EventRecord outEvent);

		[DllImport(carbon)]
		static extern OSStatus AEProcessAppleEvent(ref EventRecord theEventRecord);

		static public void ProcessAppleEvent(IntPtr inEvent)
		{
			EventRecord record;

			ConvertEventRefToEventRecord(inEvent, out record);
			AEProcessAppleEvent(ref record);
		}

		#endregion

		#endregion
		#region --- Getting Event Parameters ---

		[DllImport(carbon)]
		static extern OSStatus CreateEvent( IntPtr inAllocator,
		                                   EventClass inClassID, UInt32 kind, EventTime when,
		                                   EventAttributes flags, out IntPtr outEvent);
		
		public static IntPtr CreateWindowEvent(WindowEventKind kind) {
			IntPtr retval;
			OSStatus stat = CreateEvent(IntPtr.Zero, EventClass.Window, (uint)kind,
			                            0, EventAttributes.kEventAttributeNone, out retval);

			if (stat != OSStatus.NoError)
				throw new MacOSException(stat);
			return retval;
		}

		[DllImport(carbon)]
		static extern OSStatus GetEventParameter(
			IntPtr inEvent, EventParamName inName, EventParamType inDesiredType,
			IntPtr outActualType, int inBufferSize, IntPtr outActualSize, IntPtr outData);

		public unsafe static MacOSKeyCode GetEventKeyboardKeyCode(IntPtr inEvent) {
			int code;
			OSStatus result = API.GetEventParameter(inEvent,
			                                        EventParamName.KeyCode, EventParamType.typeUInt32, IntPtr.Zero,
			                                        sizeof(uint), IntPtr.Zero, (IntPtr)(void*)&code);

			if (result != OSStatus.NoError)
				throw new MacOSException(result);
			return (MacOSKeyCode)code;
		}

		public unsafe static char GetEventKeyboardChar(IntPtr inEvent) {
			char code;
			OSStatus result = API.GetEventParameter(inEvent,
			                                        EventParamName.KeyMacCharCode, EventParamType.typeChar, IntPtr.Zero,
			                                        Marshal.SizeOf(typeof(char)), IntPtr.Zero, (IntPtr)(void*)&code);

			if (result != OSStatus.NoError)
				throw new MacOSException(result);
			return code;
		}

		public unsafe static MacOSMouseButton GetEventMouseButton(IntPtr inEvent) {
			int button;
			OSStatus result = API.GetEventParameter(inEvent,
			                                        EventParamName.MouseButton, EventParamType.typeMouseButton, IntPtr.Zero,
			                                        sizeof(short), IntPtr.Zero, (IntPtr)(void*)&button);

			if (result != OSStatus.NoError)
				throw new MacOSException(result);
			return (MacOSMouseButton)button;
		}
		
		public unsafe static int GetEventMouseWheelDelta(IntPtr inEvent) {
			int delta;
			OSStatus result = API.GetEventParameter(inEvent,
			                                        EventParamName.MouseWheelDelta, EventParamType.typeSInt32,
			                                        IntPtr.Zero, sizeof(int), IntPtr.Zero, (IntPtr)(void*)&delta);

			if (result != OSStatus.NoError)
				throw new MacOSException(result);
			return delta;
		}

		public unsafe static OSStatus GetEventWindowMouseLocation(IntPtr inEvent, out HIPoint pt) {
			HIPoint point;
			OSStatus result = API.GetEventParameter(inEvent,
			                                        EventParamName.WindowMouseLocation, EventParamType.typeHIPoint, IntPtr.Zero,
			                                        Marshal.SizeOf(typeof(HIPoint)), IntPtr.Zero, (IntPtr)(void*)&point);

			pt = point;
			return result;
		}

		public unsafe static OSStatus GetEventWindowRef(IntPtr inEvent, out IntPtr windowRef) {
			IntPtr retval;
			OSStatus result = API.GetEventParameter(inEvent,
			                                        EventParamName.WindowRef, EventParamType.typeWindowRef, IntPtr.Zero,
			                                        sizeof(IntPtr), IntPtr.Zero, (IntPtr)(void*)&retval);

			windowRef = retval;
			return result;
		}

		public unsafe static OSStatus GetEventMouseLocation(IntPtr inEvent, out HIPoint pt) {
			HIPoint point;
			OSStatus result = API.GetEventParameter(inEvent,
			                                        EventParamName.MouseLocation, EventParamType.typeHIPoint, IntPtr.Zero,
			                                        Marshal.SizeOf(typeof(HIPoint)), IntPtr.Zero, (IntPtr)(void*)&point);

			pt = point;
			return result;
		}
		
		public unsafe static MacOSKeyModifiers GetEventKeyModifiers(IntPtr inEvent) {
			uint code;
			OSStatus result = API.GetEventParameter(inEvent,
			                                        EventParamName.KeyModifiers, EventParamType.typeUInt32, IntPtr.Zero,
			                                        sizeof(uint), IntPtr.Zero, (IntPtr)(void*)&code);

			if (result != OSStatus.NoError)
				throw new MacOSException(result);
			return (MacOSKeyModifiers)code;
		}

		#endregion
		#region --- Event Handlers ---

		[DllImport(carbon)]
		static extern OSStatus InstallEventHandler( IntPtr eventTargetRef, IntPtr handlerProc,
		                                           int numtypes, EventTypeSpec[] typeList,
		                                           IntPtr userData, IntPtr handlerRef);

		public static void InstallWindowEventHandler(IntPtr windowRef, IntPtr uppHandlerProc,
		                                               EventTypeSpec[] eventTypes, IntPtr userData, IntPtr handlerRef)
		{
			IntPtr windowTarget = GetWindowEventTarget(windowRef);
			OSStatus error = InstallEventHandler(windowTarget, uppHandlerProc,
			                                      eventTypes.Length, eventTypes,
			                                      userData, handlerRef);
			CheckReturn( error );
		}

		public static void InstallApplicationEventHandler(IntPtr uppHandlerProc,
		                                                    EventTypeSpec[] eventTypes, IntPtr userData, IntPtr handlerRef)
		{
			OSStatus error = InstallEventHandler(GetApplicationEventTarget(), uppHandlerProc,
			                                      eventTypes.Length, eventTypes,
			                                      userData, handlerRef);
			CheckReturn( error );
		}

		[DllImport(carbon)]
		public static extern OSStatus RemoveEventHandler(IntPtr inHandlerRef);

		#endregion
		#region --- GetWindowEventTarget ---

		[DllImport(carbon)]
		public static extern IntPtr GetWindowEventTarget(IntPtr window);

		[DllImport(carbon)]
		public static extern IntPtr GetApplicationEventTarget();

		#endregion
		#region --- UPP Event Handlers ---
		
		[DllImport(carbon)]
		public static extern IntPtr NewEventHandlerUPP(MacOSEventHandler handler);

		[DllImport(carbon)]
		public static extern void DisposeEventHandlerUPP(IntPtr userUPP);

		#endregion
		#region --- Process Manager ---

		[DllImport(carbon)]
		public static extern int TransformProcessType(ref Carbon.ProcessSerialNumber psn, ProcessApplicationTransformState type);
		[DllImport(carbon)]
		public static extern int GetCurrentProcess(ref Carbon.ProcessSerialNumber psn);
		[DllImport(carbon)]
		public static extern int SetFrontProcess(ref Carbon.ProcessSerialNumber psn);

		#endregion
		#region --- Setting Dock Tile ---

		[DllImport(carbon)]
		public extern static IntPtr CGColorSpaceCreateDeviceRGB();
		[DllImport(carbon)]
		public extern static IntPtr CGDataProviderCreateWithData(IntPtr info, IntPtr data, int size, IntPtr releasefunc);
		[DllImport(carbon)]
		public extern static IntPtr CGImageCreate(int width, int height, int bitsPerComponent, int bitsPerPixel, int bytesPerRow, IntPtr colorspace, 
		                                          uint bitmapInfo, IntPtr provider, IntPtr decode, int shouldInterpolate, int intent);
		[DllImport(carbon)]
		public extern static void SetApplicationDockTileImage(IntPtr imageRef);
		[DllImport(carbon)]
		public extern static void RestoreApplicationDockTileImage();
		[DllImport(carbon)]
		public extern static void CGImageRelease(IntPtr image);
		[DllImport(carbon)]
		public extern static void CGDataProviderRelease(IntPtr provider);
		[DllImport(carbon)]
		public extern static void CGColorSpaceRelease(IntPtr space);		
		[DllImport(carbon)]
		public extern static void CGContextDrawImage( IntPtr context, HIRect rect, IntPtr image );
		[DllImport(carbon)]
		public extern static void CGContextSynchronize( IntPtr context );
		[DllImport(carbon)]
		public extern static OSStatus QDBeginCGContext( IntPtr port, ref IntPtr context );
		[DllImport(carbon)]
		public extern static OSStatus QDEndCGContext( IntPtr port, ref IntPtr context );
		#endregion
		#region --- Clipboard ---
		
		[DllImport (carbon)]
		public static extern IntPtr CFDataCreate(IntPtr allocator, IntPtr buf, Int32 length);
		[DllImport (carbon)]
		public static extern IntPtr CFDataGetBytePtr(IntPtr data);
		[DllImport (carbon)]
		public static extern int PasteboardSynchronize(IntPtr pbref);		

		[DllImport (carbon)]
		public static extern OSStatus PasteboardClear(IntPtr pbref);
		[DllImport (carbon)]
		public static extern OSStatus PasteboardCreate(IntPtr str, out IntPtr pbref);
		[DllImport (carbon)]
		public static extern OSStatus PasteboardCopyItemFlavorData(IntPtr pbref, UInt32 itemid, IntPtr key, out IntPtr data);
		[DllImport (carbon)]
		public static extern OSStatus PasteboardGetItemCount(IntPtr pbref, out UInt32 count);
		[DllImport (carbon)]
		public static extern OSStatus PasteboardGetItemIdentifier(IntPtr pbref, UInt32 itemindex, out UInt32 itemid);
		[DllImport (carbon)]
		public static extern OSStatus PasteboardPutItemFlavor(IntPtr pbref, UInt32 itemid, IntPtr key, IntPtr data, UInt32 flags);
		
		#endregion

		[DllImport(carbon)]
		public static extern OSStatus ActivateWindow (IntPtr inWindow, bool inActivate);

		[DllImport(carbon)]
		public static extern void RunApplicationEventLoop();

		[DllImport(carbon)]
		public static extern void QuitApplicationEventLoop();

		#region --- SetWindowTitle ---

		[DllImport(carbon)]
		static extern void SetWindowTitleWithCFString(IntPtr windowRef, IntPtr title);

		public static void SetWindowTitle(IntPtr windowRef, string title)
		{
			IntPtr str = __CFStringMakeConstantString(title);

			Debug.Print("Setting window title: {0},   CFstring : {1},  Text : {2}", windowRef, str, title);

			SetWindowTitleWithCFString(windowRef, str);

			// Apparently releasing this reference to the CFConstantString here
			// causes the program to crash on the fourth window created.  But I am
			// afraid that not releasing the string would result in a memory leak, but that would
			// only be a serious issue if the window title is changed a lot.
			//CFRelease(str);
		}

		#endregion
		
		[DllImport(carbon)]
		static extern IntPtr __CFStringMakeConstantString(string cStr);

		[DllImport(carbon)]
		static extern void CFRelease(IntPtr cfStr);

		[DllImport(carbon)]
		public static extern OSStatus CallNextEventHandler(IntPtr nextHandler, IntPtr theEvent);

		[DllImport(carbon)]
		public static extern IntPtr GetWindowPort(IntPtr windowRef);
		
		#region --- Menus ---

		[DllImport(carbon)]
		public static extern IntPtr AcquireRootMenu();


		#endregion

		[DllImport(carbon)]
		public static extern bool IsWindowCollapsed(IntPtr windowRef);

		[DllImport(carbon)]
		public static extern OSStatus CollapseWindow(IntPtr windowRef, bool collapse);
		
		public static void CheckReturn(OSStatus error ) {
			if( error != OSStatus.NoError )
				throw new MacOSException( error );
		}

		[DllImport(carbon, EntryPoint="IsWindowInStandardState")]
		static extern bool _IsWindowInStandardState(IntPtr windowRef, IntPtr inIdealSize, IntPtr outIdealStandardState);

		public static bool IsWindowInStandardState(IntPtr windowRef)
		{
			return _IsWindowInStandardState(windowRef, IntPtr.Zero, IntPtr.Zero);
		}

		[DllImport(carbon)]
		public unsafe static extern OSStatus ZoomWindowIdeal(IntPtr windowRef, short inPartCode, ref CarbonPoint toIdealSize);

		[DllImport(carbon)]
		public unsafe static extern OSStatus DMGetGDeviceByDisplayID(
			IntPtr displayID, out IntPtr displayDevice, Boolean failToMain);
		
		[DllImport(carbon)]
		public unsafe static extern IntPtr HIGetMousePosition( HICoordinateSpace space, IntPtr obj, ref HIPoint point );

		#region Nonworking HIPointConvert routines

		// These seem to crash when called, and I haven't figured out why.
		// Currently a workaround is used to convert from screen to client coordinates.

		//[DllImport(carbon, EntryPoint="HIPointConvert")]
		//extern static OSStatus _HIPointConvert(ref HIPoint ioPoint,
		//    HICoordinateSpace inSourceSpace, IntPtr inSourceObject,
		//    HICoordinateSpace inDestinationSpace, IntPtr inDestinationObject);

		//public static HIPoint HIPointConvert(HIPoint inPoint,
		//    HICoordinateSpace inSourceSpace, IntPtr inSourceObject,
		//    HICoordinateSpace inDestinationSpace, IntPtr inDestinationObject)
		//{
		//    OSStatus error = _HIPointConvert(ref inPoint, inSourceSpace, inSourceObject, inDestinationSpace, inDestinationObject);

		//    if (error != OSStatus.NoError)
		//    {
		//        throw new MacOSException(error);
		//    }

		//    return inPoint;
		//}

		//[DllImport(carbon, EntryPoint = "HIViewConvertPoint")]
		//extern static OSStatus _HIViewConvertPoint(ref HIPoint inPoint, IntPtr inSourceView, IntPtr inDestView);

		//public static HIPoint HIViewConvertPoint( HIPoint point, IntPtr sourceHandle, IntPtr destHandle)
		//{
		//    //Carbon.Rect window_bounds = new Carbon.Rect();
		//    //Carbon.API.GetWindowBounds(handle, WindowRegionCode.StructureRegion /*32*/, out window_bounds);

		//    //point.X -= window_bounds.X;
		//    //point.Y -= window_bounds.Y;

		//    OSStatus error = _HIViewConvertPoint(ref point, sourceHandle, destHandle);

		//    if (error != OSStatus.NoError)
		//    {
		//        throw new MacOSException(error);
		//    }

		//   return point;
		//}

		#endregion

		const string gestaltlib = "/System/Library/Frameworks/Carbon.framework/Versions/Current/Carbon";


		[DllImport(gestaltlib)]
		public static extern OSStatus Gestalt(GestaltSelector selector, out int response);
	}

	#endregion
	
}
