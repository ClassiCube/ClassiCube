// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public sealed class ClassicKeyBindingsScreen : KeyBindingsScreen {
		
		public ClassicKeyBindingsScreen( Game game ) : base( game ) { }
		
		static KeyBind[] left = { KeyBind.Forward, KeyBind.Back,
			KeyBind.Jump, KeyBind.OpenChat, KeyBind.SetSpawn };
		static KeyBind[] right = { KeyBind.Left, KeyBind.Right,
			KeyBind.OpenInventory, KeyBind.ViewDistance, KeyBind.Respawn };
		
		static string[] leftDescs = { "Forward", "Back", "Jump", "Chat", "Save loc" };
		static string[] rightDescs = { "Left", "Right", "Build", "Toggle fog", "Load loc" };
		
		public override void Init() {
			base.Init();
			btnWidth = 301; btnHeight = 40;
			btnDistance = 48;
			widgets = new Widget[left.Length + right.Length + 3];
			SetBinds( left, leftDescs, right, rightDescs );
			MakeWidgets( -180 );
			
			widgets[index++] = MakeBack( false, titleFont,
			                            (g, w) => g.SetNewScreen( new ClassicOptionsScreen( g ) ) );
		}
	}
	
	public sealed class ClassicHacksKeyBindingsScreen : KeyBindingsScreen {
		
		public ClassicHacksKeyBindingsScreen( Game game ) : base( game ) { }
		
		static KeyBind[] left = { KeyBind.Forward, KeyBind.Back,
			KeyBind.Left, KeyBind.Right, KeyBind.Jump,
			KeyBind.Respawn, KeyBind.SetSpawn, KeyBind.OpenChat };
		static KeyBind[] right = { KeyBind.OpenInventory,
			KeyBind.ViewDistance, KeyBind.Speed, KeyBind.NoClip,
			KeyBind.Fly, KeyBind.FlyUp, KeyBind.FlyDown };
		
		static string[] leftDescs = { "Forward", "Back", "Left", "Right", "Jump",
			"Load loc", "Save loc", "Chat" };
		static string[] rightDescs = { "Build", "Toggle fog", "Speed", "Noclip",
			"Fly", "Fly up", "Fly down"	};
		
		public override void Init() {
			base.Init();
			widgets = new Widget[left.Length + right.Length + 3];
			SetBinds( left, leftDescs, right, rightDescs );
			MakeWidgets( -220 );
			
			widgets[index++] = MakeBack( false, titleFont,
			                            (g, w) => g.SetNewScreen( new ClassicOptionsScreen( g ) ) );
		}
	}
	
	public sealed class NormalKeyBindingsScreen : KeyBindingsScreen {
		
		public NormalKeyBindingsScreen( Game game ) : base( game ) { }
		
		static KeyBind[] left = { KeyBind.Forward, KeyBind.Back,
			KeyBind.Left, KeyBind.Right, KeyBind.Jump, KeyBind.Respawn };
		static KeyBind[] right = { KeyBind.SetSpawn, KeyBind.OpenChat,
			KeyBind.OpenInventory, KeyBind.ViewDistance, KeyBind.SendChat,
			KeyBind.PlayerList };
		
		static string[] leftDescs = { "Forward", "Back", "Left", "Right", "Jump",
			"Respawn" };
		static string[] rightDescs = { "Set spawn", "Chat", "Inventory", 
			"View distance", "Send chat", "Player list" };
		
		public override void Init() {
			base.Init();
			widgets = new Widget[left.Length + right.Length + 4];
			SetBinds( left, leftDescs, right, rightDescs );
			MakeWidgets( -180 );
			
			widgets[index++] = MakeBack( "Back to menu", 5, titleFont,
			                            (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) );
			widgets[index++] = ButtonWidget.Create(
				game, 0, 170, 300, 35, "Advanced key bindings",
				Anchor.Centre, Anchor.Centre, titleFont, NextClick );
		}
		
		void NextClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			game.SetNewScreen( new AdvancedKeyBindingsScreen( game ) );
		}
	}
	
	public sealed class AdvancedKeyBindingsScreen : KeyBindingsScreen {
		
		public AdvancedKeyBindingsScreen( Game game ) : base( game ) { }
		
		static KeyBind[] left = { KeyBind.Speed, KeyBind.NoClip,
			KeyBind.Fly, KeyBind.FlyUp, KeyBind.FlyDown,
			KeyBind.ExtendedInput, KeyBind.HideFps };
		static KeyBind[] right = { KeyBind.Screenshot, KeyBind.Fullscreen,
			KeyBind.ThirdPersonCamera, KeyBind.HideGui,
			KeyBind.ShowAxisLines, KeyBind.ZoomScrolling, KeyBind.HalfSpeed };
		
		static string[] leftDescs = { "Speed", "Noclip", "Fly", "Fly up", "Fly down",
			"Show ext input", "Hide FPS" };
		static string[] rightDescs = { "Take screenshot", "Fullscreen", "Third person",
			"Hide gui", "Show axis lines", "Zoom scrolling", "Half speed" };
		
		public override void Init() {
			base.Init();
			widgets = new Widget[left.Length + right.Length + 4];
			SetBinds( left, leftDescs, right, rightDescs );
			MakeWidgets( -180 );
			
			widgets[index++] = MakeBack( "Back to menu", 5, titleFont,
			                            (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) );
			widgets[index++] = ButtonWidget.Create(
				game, 0, 170, 300, 35, "Mouse key bindings",
				Anchor.Centre, Anchor.Centre, titleFont, NextClick );
		}
		
		void NextClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			game.SetNewScreen( new MouseKeyBindingsScreen( game ) );
		}
	}
	
	public sealed class MouseKeyBindingsScreen : KeyBindingsScreen {
		
		public MouseKeyBindingsScreen( Game game ) : base( game ) { }
		
		static KeyBind[] binds = { KeyBind.MouseLeft,
			KeyBind.MouseMiddle, KeyBind.MouseRight };
		static string[] descs = { "Left mouse", "Middle mouse", "Right mouse" };
		
		public override void Init() {
			base.Init();
			widgets = new Widget[binds.Length + 5];
			SetBinds( binds, descs, null, null );
			MakeWidgets( -180 );
			
			widgets[index++] = MakeBack( "Back to menu", 5, titleFont,
			                            (g, w) => g.SetNewScreen( new PauseScreen( g ) ) );
			widgets[index++] = ButtonWidget.Create(
				game, 0, 170, 300, 35, "Normal key bindings",
				Anchor.Centre, Anchor.Centre, titleFont, NextClick );
			widgets[index++] = ChatTextWidget.Create(
				game, 0, -40, "&eRight click to remove the key binding",
				Anchor.Centre, Anchor.Centre, regularFont );
		}
		
		void NextClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			game.SetNewScreen( new NormalKeyBindingsScreen( game ) );
		}
	}
}