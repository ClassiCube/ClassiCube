// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;

namespace ClassicalSharp.Gui.Screens {
	public sealed class ClassicKeyBindingsScreen : KeyBindingsScreen {
		
		public ClassicKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			// See comment in KeyMap() constructor for why this is necessary.
			binds = new KeyBind[10];
			binds[0] = KeyBind.Forward; binds[1] = KeyBind.Back; binds[2] = KeyBind.Jump;
			binds[3] = KeyBind.Chat; binds[4] = KeyBind.SetSpawn;
			binds[5] = KeyBind.Left; binds[6] = KeyBind.Right; binds[7] = KeyBind.Inventory;
			binds[8] = KeyBind.ToggleFog; binds[9] = KeyBind.Respawn;
			
			desc = new string[] { "Forward", "Back", "Jump", "Chat", "Save loc",
				"Left", "Right", "Build", "Toggle fog", "Load loc" };
			
			if (game.ClassicHacks) rightPage = SwitchClassicHacks;
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[binds.Length + 4];
			if (game.ClassicHacks) {
				MakeWidgets(-140, -40, 5, "Normal controls", 260);
			} else {
				MakeWidgets(-140, -40, 5, "Controls", 300);
			}
		}
	}
	
	public sealed class ClassicHacksKeyBindingsScreen : KeyBindingsScreen {
		
		public ClassicHacksKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			binds = new KeyBind[6];
			binds[0] = KeyBind.Speed; binds[1] = KeyBind.NoClip; binds[2] = KeyBind.HalfSpeed;
			binds[3] = KeyBind.Fly; binds[4] = KeyBind.FlyUp; binds[5] = KeyBind.FlyDown;
			
			desc = new string[] { "Speed", "Noclip", "Half speed",
				"Fly", "Fly up", "Fly down"	};			
			
			leftPage = SwitchClassic;
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[binds.Length + 4];
			MakeWidgets(-90, -40, 3, "Hacks controls", 260);
		}
	}
	
	public sealed class NormalKeyBindingsScreen : KeyBindingsScreen {
		
		public NormalKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			binds = new KeyBind[12];
			binds[0] = KeyBind.Forward; binds[1] = KeyBind.Back; binds[2] = KeyBind.Jump;
			binds[3] = KeyBind.Chat; binds[4] = KeyBind.SetSpawn; binds[5] = KeyBind.PlayerList;
			binds[6] = KeyBind.Left; binds[7] = KeyBind.Right; binds[8] = KeyBind.Inventory;
			binds[9] = KeyBind.ToggleFog; binds[10] = KeyBind.Respawn; binds[11] = KeyBind.SendChat;
			
			desc = new string[] { "Forward", "Back", "Jump", "Chat", "Set spawn", "Player list",
				"Left", "Right", "Inventory", "Toggle fog", "Respawn", "Send chat" };
			
			rightPage = SwitchHacks;
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[binds.Length + 4];
			MakeWidgets(-140, 10, 6, "Normal controls", 260);
		}
	}
	
	public sealed class HacksKeyBindingsScreen : KeyBindingsScreen {
		
		public HacksKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			binds = new KeyBind[8];
			binds[0] = KeyBind.Speed; binds[1] = KeyBind.NoClip; binds[2] = KeyBind.HalfSpeed; binds[3] = KeyBind.ZoomScrolling;
			binds[4] = KeyBind.Fly; binds[5] = KeyBind.FlyUp; binds[6] = KeyBind.FlyDown; binds[7] = KeyBind.ThirdPerson;
			
			desc = new string[] { "Speed", "Noclip", "Half speed", "Scroll zoom",
				"Fly", "Fly up", "Fly down", "Third person" };
			
			leftPage = SwitchNormal;
			rightPage = SwitchOther;
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[binds.Length + 4];
			MakeWidgets(-40, 10, 4, "Hacks controls", 260);
		}
	}
	
	public sealed class OtherKeyBindingsScreen : KeyBindingsScreen {
		
		public OtherKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			binds = new KeyBind[12];
			binds[0] = KeyBind.ExtInput; binds[1] = KeyBind.HideFps; binds[2] = KeyBind.HideGui;
			binds[3] = KeyBind.HotbarSwitching; binds[4] = KeyBind.DropBlock;
			binds[5] = KeyBind.Screenshot; binds[6] = KeyBind.Fullscreen; binds[7] = KeyBind.AxisLines;
			binds[8] = KeyBind.Autorotate; binds[9] = KeyBind.SmoothCamera; binds[10] = KeyBind.IDOverlay;
			binds[11] = KeyBind.BreakableLiquids;
			
			desc = new string[] { "Show ext input", "Hide FPS", "Hide gui", "Hotbar switching", "Drop block",
				"Screenshot", "Fullscreen", "Show axis lines", "Auto-rotate", "Smooth camera", "ID overlay", "Breakable liquids" };
			
			leftPage = SwitchHacks;
			rightPage = SwitchMouse;
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[binds.Length + 4];
			MakeWidgets(-140, 10, 6, "Other controls", 260);
		}
	}
	
	public sealed class MouseKeyBindingsScreen : KeyBindingsScreen {
		
		public MouseKeyBindingsScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			binds = new KeyBind[3];
			binds[0] = KeyBind.MouseLeft; binds[1] = KeyBind.MouseMiddle; binds[2] = KeyBind.MouseRight;
			desc = new string[] { "Left", "Middle", "Right" };
			
			leftPage = SwitchOther;
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[binds.Length + 5];
			int index = MakeWidgets(-40, 10, -1, "Mouse key bindings", 260);
			
			widgets[index++] = TextWidget.Create(game, "&eRight click to remove the key binding", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 100);
		}
	}
}