// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.Generator;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Singleplayer;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public sealed class GenLevelScreen : MenuScreen {		
		public GenLevelScreen(Game game) : base(game) { }

		MenuInputWidget selected;

		public override bool HandlesKeyPress(char key) {
			return selected == null || selected.HandlesKeyPress(key);
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (selected != null && selected.HandlesKeyDown(key)) return true;
			return base.HandlesKeyDown(key);
		}
		
		public override bool HandlesKeyUp(Key key) {
			return selected == null || selected.HandlesKeyUp(key);
		}
		
		public override void Init() {
			base.Init();
			game.Keyboard.KeyRepeat = true;
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[] {
				MakeInput(-80, false, game.World.Width.ToString()),
				MakeInput(-40, false, game.World.Height.ToString()),
				MakeInput(0, false, game.World.Length.ToString()),
				MakeInput(40, true, ""),
				
				MakeLabel(-150, -80, "Width:"), 
				MakeLabel(-150, -40, "Height:"),
				MakeLabel(-150, 0, "Length:"), 
				MakeLabel(-140, 40, "Seed:"),
				TextWidget.Create(game, "Generate new level", textFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -130),
				
				ButtonWidget.Create(game, 200, "Flatgrass", titleFont, GenFlatgrassClick)
					.SetLocation(Anchor.Centre, Anchor.Centre, -120, 100),
				ButtonWidget.Create(game, 200, "Vanilla", titleFont, GenNotchyClick)
					.SetLocation(Anchor.Centre, Anchor.Centre, 120, 100),
				MakeBack(false, titleFont, SwitchPause),
			};
		}
		
		InputWidget MakeInput(int y, bool seed, string value) {
			MenuInputValidator validator = seed ? new SeedValidator() : new IntegerValidator(1, 8192);
			InputWidget input = MenuInputWidget.Create(game, 200, 30, value, textFont, validator)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, y);
			
			input.MenuClick = InputClick;
			return input;
		}
		
		TextWidget MakeLabel(int x, int y, string text) {
			TextWidget label = TextWidget.Create(game, text, textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, x, y);
			
			label.XOffset = -110 - label.Width / 2;
			label.Reposition();
			label.Colour = new FastColour(224, 224, 224);
			return label;
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			base.Dispose();
		}
		
		void InputClick(Game game, Widget widget) {
			if (selected != null) selected.ShowCaret = false;
			
			selected = (MenuInputWidget)widget;
			selected.HandlesMouseDown(game.Mouse.X, game.Mouse.Y, MouseButton.Left);
			selected.ShowCaret = true;
		}
		
		void GenFlatgrassClick(Game game, Widget widget) {
			GenerateMap(new FlatGrassGenerator());
		}
		
		void GenNotchyClick(Game game, Widget widget) {
			GenerateMap(new NotchyGenerator());
		}
		
		void GenerateMap(IMapGenerator gen) {
			int width = GetInt(0), height = GetInt(1);
			int length = GetInt(2), seed = GetSeedInt(3);
			
			long volume = (long)width * height * length;
			if (volume > int.MaxValue) {
				game.Chat.Add("&cThe generated map's volume is too big.");
			} else if (width == 0 || height == 0 || length == 0) {
				game.Chat.Add("&cOne of the map dimensions is invalid.");
			} else {
				game.Server.BeginGeneration(width, height, length, seed, gen);
			}
		}
		
		int GetInt(int index) {
			MenuInputWidget input = (MenuInputWidget)widgets[index];
			string text = input.Text.ToString();
			
			if (!input.Validator.IsValidValue(text)) return 0;
			return Int32.Parse(text);
		}
		
		int GetSeedInt(int index) {
			MenuInputWidget input = (MenuInputWidget)widgets[index];
			string text = input.Text.ToString();
			if (text == "") return new Random().Next();
			
			if (!input.Validator.IsValidValue(text)) return 0;
			return Int32.Parse(text);
		}
	}
	
	public sealed class ClassicGenLevelScreen : MenuScreen {	
		public ClassicGenLevelScreen(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[] {
				Make(-100, "Small",  GenSmallClick),
				Make( -50, "Normal", GenMediumClick),
				Make(   0, "Huge",   GenHugeClick),
				MakeBack(false, titleFont, SwitchPause),
			};
		}
		
		ButtonWidget Make(int y, string text, ClickHandler onClick) {
			return ButtonWidget.Create(game, 400, text, titleFont, onClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, y);
		}
		
		void GenSmallClick(Game game, Widget widget) { DoGen(128); }		
		void GenMediumClick(Game game, Widget widget) { DoGen(256); }
		void GenHugeClick(Game game, Widget widget) { DoGen(512); }
		
		void DoGen(int size) {
			int seed = new Random().Next();
			IMapGenerator gen = new NotchyGenerator();
			game.Server.BeginGeneration(size, 64, size, seed, gen);
		}
	}
}