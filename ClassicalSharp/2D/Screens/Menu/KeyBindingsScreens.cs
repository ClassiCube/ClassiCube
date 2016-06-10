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
			widgets = new Widget[left.Length + right.Length + 4];
			SetBinds( left, leftDescs, right, rightDescs );
			if( game.ClassicHacks ) {
				title = "Normal controls";
				rightPage = (g, w) => g.SetNewScreen( new ClassicHacksKeyBindingsScreen( g ) );
			} else {
				btnWidth = 301; btnHeight = 40; btnDistance = 48;
			}
			MakeWidgets( -140 );
		}
	}
	
	public sealed class ClassicHacksKeyBindingsScreen : KeyBindingsScreen {
		
		public ClassicHacksKeyBindingsScreen( Game game ) : base( game ) { }
		
		static KeyBind[] left = { KeyBind.Speed, KeyBind.NoClip, KeyBind.HalfSpeed };
		static KeyBind[] right = { KeyBind.Fly, KeyBind.FlyUp, KeyBind.FlyDown };
		
		static string[] leftDescs = { "Speed", "Noclip", "Half speed" };
		static string[] rightDescs = { "Fly", "Fly up", "Fly down"	};
		
		public override void Init() {
			base.Init();
			widgets = new Widget[left.Length + right.Length + 4];
			SetBinds( left, leftDescs, right, rightDescs );
			leftPage = (g, w) => g.SetNewScreen( new ClassicKeyBindingsScreen( g ) );
			title = "Hacks controls";
			MakeWidgets( -95 );
		}
	}
	
	public sealed class NormalKeyBindingsScreen : KeyBindingsScreen {
		
		public NormalKeyBindingsScreen( Game game ) : base( game ) { }
		
		static KeyBind[] left = { KeyBind.Forward, KeyBind.Back,
			KeyBind.Jump, KeyBind.OpenChat, KeyBind.SetSpawn, KeyBind.PlayerList };
		static KeyBind[] right = { KeyBind.Left, KeyBind.Right, 
			KeyBind.OpenInventory, KeyBind.ViewDistance, KeyBind.Respawn, KeyBind.SendChat };
		
		static string[] leftDescs = { "Forward", "Back", "Jump", "Chat", "Set spawn", "Player list" };
		static string[] rightDescs = { "Left", "Right", "Inventory", "Toggle fog", "Respawn", "Send chat" };
		
		public override void Init() {
			base.Init();
			widgets = new Widget[left.Length + right.Length + 4];
			SetBinds( left, leftDescs, right, rightDescs );
			title = "Normal controls";
			rightPage = (g, w) => g.SetNewScreen( new HacksKeyBindingsScreen( g ) );
			MakeWidgets( -140 );
		}
	}
	
	public sealed class HacksKeyBindingsScreen : KeyBindingsScreen {
		
		public HacksKeyBindingsScreen( Game game ) : base( game ) { }
		
		static KeyBind[] left = { KeyBind.Speed, KeyBind.NoClip,
			KeyBind.HalfSpeed };
		static KeyBind[] right = { KeyBind.Fly, KeyBind.FlyUp,
			KeyBind.FlyDown, KeyBind.ThirdPersonCamera };
		
		static string[] leftDescs = { "Speed", "Noclip", "Half speed" };
		static string[] rightDescs = { "Fly", "Fly up", "Fly down", "Third person" };
		
		public override void Init() {
			base.Init();
			widgets = new Widget[left.Length + right.Length + 4];
			title = "Hacks controls";
			SetBinds( left, leftDescs, right, rightDescs );
			leftPage = (g, w) => g.SetNewScreen( new NormalKeyBindingsScreen( g ) );
			rightPage = (g, w) => g.SetNewScreen( new OtherKeyBindingsScreen( g ) );
			MakeWidgets( -50 );
		}
	}
	
	public sealed class OtherKeyBindingsScreen : KeyBindingsScreen {
		
		public OtherKeyBindingsScreen( Game game ) : base( game ) { }
		
		static KeyBind[] left = { KeyBind.ExtendedInput,
			KeyBind.HideFps, KeyBind.HideGui };
		static KeyBind[] right = { KeyBind.Screenshot,
			KeyBind.Fullscreen,KeyBind.ShowAxisLines };
		
		static string[] leftDescs = { "Show ext input", "Hide FPS", "Hide gui" };
		static string[] rightDescs = { "Screenshot", "Fullscreen", "Show axis lines" };
		
		public override void Init() {
			base.Init();
			widgets = new Widget[left.Length + right.Length + 4];
			title = "Other controls";
			SetBinds( left, leftDescs, right, rightDescs );
			leftPage = (g, w) => g.SetNewScreen( new HacksKeyBindingsScreen( g ) );
			rightPage = (g, w) => g.SetNewScreen( new MouseKeyBindingsScreen( g ) );
			MakeWidgets( -50 );
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
			title = "Mouse key bindings";
			SetBinds( binds, descs, null, null );
			leftPage = (g, w) => g.SetNewScreen( new HacksKeyBindingsScreen( g ) );
			MakeWidgets( -50 );

			widgets[index++] = ChatTextWidget.Create(
				game, 0, 80, "&eRight click to remove the key binding",
				Anchor.Centre, Anchor.Centre, regularFont );
		}
	}
}