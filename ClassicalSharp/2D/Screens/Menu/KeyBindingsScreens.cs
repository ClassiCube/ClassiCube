// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public sealed class ClassicKeyBindingsScreen : KeyBindingsScreen {
		
		public ClassicKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			// See comment in KeyMap() constructor for why this is necessary.
			left = new KeyBind[5];
			left[0] = KeyBind.Forward; left[1] = KeyBind.Back; left[2] = KeyBind.Jump;
			left[3] = KeyBind.Chat; left[4] = KeyBind.SetSpawn;
			right = new KeyBind[5];
			right[0] = KeyBind.Left; right[1] = KeyBind.Right; right[2] = KeyBind.Inventory;
			right[3] = KeyBind.ToggleFog; right[4] = KeyBind.Respawn;
			leftDesc = new string[] { "Forward", "Back", "Jump", "Chat", "Save loc" };
			rightDesc = new string[] { "Left", "Right", "Build", "Toggle fog", "Load loc" };			
			
			if (game.ClassicHacks) {
				title = "Normal controls";
				rightPage = (g, w) => g.Gui.SetNewScreen(new ClassicHacksKeyBindingsScreen(g));
			} else {
				btnWidth = 300;
			}
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[left.Length + right.Length + 4];
			MakeWidgets(-140);
		}
	}
	
	public sealed class ClassicHacksKeyBindingsScreen : KeyBindingsScreen {
		
		public ClassicHacksKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			left = new KeyBind[3];
			left[0] = KeyBind.Speed; left[1] = KeyBind.NoClip; left[2] = KeyBind.HalfSpeed;
			right = new KeyBind[3];
			right[0] = KeyBind.Fly; right[1] = KeyBind.FlyUp; right[2] = KeyBind.FlyDown;
			leftDesc = new string[] { "Speed", "Noclip", "Half speed" };
			rightDesc = new string[] { "Fly", "Fly up", "Fly down"	};
			
			
			leftPage = (g, w) => g.Gui.SetNewScreen(new ClassicKeyBindingsScreen(g));
			title = "Hacks controls";
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[left.Length + right.Length + 4];
			MakeWidgets(-95);
		}
	}
	
	public sealed class NormalKeyBindingsScreen : KeyBindingsScreen {
		
		public NormalKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			left = new KeyBind[6];
			left[0] = KeyBind.Forward; left[1] = KeyBind.Back; left[2] = KeyBind.Jump;
			left[3] = KeyBind.Chat; left[4] = KeyBind.SetSpawn; left[5] = KeyBind.PlayerList;
			right = new KeyBind[6];
			right[0] = KeyBind.Left; right[1] = KeyBind.Right; right[2] = KeyBind.Inventory;
			right[3] = KeyBind.ToggleFog; right[4] = KeyBind.Respawn; right[5] = KeyBind.SendChat;
			leftDesc = new string[] { "Forward", "Back", "Jump", "Chat", "Set spawn", "Player list" };
			rightDesc = new string[] { "Left", "Right", "Inventory", "Toggle fog", "Respawn", "Send chat" };
			
			title = "Normal controls";
			rightPage = (g, w) => g.Gui.SetNewScreen(new HacksKeyBindingsScreen(g));
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[left.Length + right.Length + 4];
			MakeWidgets(-140);
		}
	}
	
	public sealed class HacksKeyBindingsScreen : KeyBindingsScreen {
		
		public HacksKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			left = new KeyBind[3];
			left[0] = KeyBind.Speed; left[1] = KeyBind.NoClip; left[2] = KeyBind.HalfSpeed;
			right = new KeyBind[4];
			right[0] = KeyBind.Fly; right[1] = KeyBind.FlyUp; right[2] = KeyBind.FlyDown;
			right[3] = KeyBind.ThirdPerson;
			leftDesc = new string[] { "Speed", "Noclip", "Half speed" };
			rightDesc = new string[] { "Fly", "Fly up", "Fly down", "Third person" };
			
			
			title = "Hacks controls";
			leftPage = (g, w) => g.Gui.SetNewScreen(new NormalKeyBindingsScreen(g));
			rightPage = (g, w) => g.Gui.SetNewScreen(new OtherKeyBindingsScreen(g));
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[left.Length + right.Length + 4];
			MakeWidgets(-50);
		}
	}
	
	public sealed class OtherKeyBindingsScreen : KeyBindingsScreen {
		
		public OtherKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			left = new KeyBind[3];
			left[0] = KeyBind.ExtInput; left[1] = KeyBind.HideFps; left[2] = KeyBind.HideGui;
			right = new KeyBind[4];
			right[0] = KeyBind.Screenshot; right[1] = KeyBind.Fullscreen; right[2] = KeyBind.AxisLines; right[3] = KeyBind.Autorotate;
			leftDesc = new string[] { "Show ext input", "Hide FPS", "Hide gui" };
			rightDesc = new string[] { "Screenshot", "Fullscreen", "Show axis lines", "Toggle auto-rotate" };
			
			title = "Other controls";
			leftPage = (g, w) => g.Gui.SetNewScreen(new HacksKeyBindingsScreen(g));
			rightPage = (g, w) => g.Gui.SetNewScreen(new MouseKeyBindingsScreen(g));
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[left.Length + right.Length + 4];
			MakeWidgets(-50);
		}
	}
	
	public sealed class MouseKeyBindingsScreen : KeyBindingsScreen {
		
		public MouseKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			left = new KeyBind[3];
			left[0] = KeyBind.MouseLeft; left[1] = KeyBind.MouseMiddle; left[2] = KeyBind.MouseRight;
			leftDesc = new string[] { "Left", "Middle", "Right" };
			
			title = "Mouse key bindings";
			leftPage = (g, w) => g.Gui.SetNewScreen(new HacksKeyBindingsScreen(g));
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[left.Length + 5];
			MakeWidgets(-50);
			
			widgets[index++] = TextWidget.Create(game, "&eRight click to remove the key binding", regularFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 100);
		}
	}
}