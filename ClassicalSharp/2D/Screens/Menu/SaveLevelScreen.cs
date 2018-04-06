// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Map;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public sealed class SaveLevelScreen : MenuScreen {
		
		public SaveLevelScreen(Game game) : base(game) {
		}
		
		InputWidget input;
		TextWidget desc;
		const int overwriteIndex = 2;
		static FastColour grey = new FastColour(150, 150, 150);
		
		public override void Render(double delta) {
			RenderMenuBounds();
			game.Graphics.Texturing = true;
			RenderWidgets(widgets, delta);
			if (desc != null) desc.Render(delta);
			game.Graphics.Texturing = false;
			
			int cX = game.Width / 2, cY = game.Height / 2;
			game.Graphics.Draw2DQuad(cX - 250, cY + 90, 500, 2, grey);
			if (textPath == null) return;
			SaveMap(textPath);
			textPath = null;
		}
		
		public override bool HandlesKeyPress(char key) {
			RemoveOverwrites();
			return input.HandlesKeyPress(key);
		}
		
		public override bool HandlesKeyDown(Key key) {
			RemoveOverwrites();
			if (input.HandlesKeyDown(key)) return true;
			return base.HandlesKeyDown(key);
		}
		
		public override bool HandlesKeyUp(Key key) {
			return input.HandlesKeyUp(key);
		}
		
		public override void Init() {
			base.Init();
			game.Keyboard.KeyRepeat = true;
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16);
			ContextRecreated();
		}
		
		protected override void ContextLost() {
			DisposeDescWidget();
			base.ContextLost();
		}
		
		protected override void ContextRecreated() {
			input = MenuInputWidget.Create(game, 500, 30, "", regularFont, new PathValidator())
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -30);
			input.ShowCaret = true;
			
			widgets = new Widget[] {
				ButtonWidget.Create(game, 300, "Save", titleFont, SaveClassic)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, 20),
				ButtonWidget.Create(game, 200, "Save schematic", titleFont, SaveSchematic)
					.SetLocation(Anchor.Centre, Anchor.Centre, -150, 120),
				TextWidget.Create(game, "&eCan be imported into MCEdit", regularFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 110, 120),				
				MakeBack(false, titleFont, SwitchPause),
				input,
				null, // description widget placeholder				
			};
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			base.Dispose();
		}
		
		void SaveClassic(Game game, Widget widget) { DoSave(widget, ".cw"); }	
		void SaveSchematic(Game game, Widget widget) { DoSave(widget, ".schematic"); }
		
		void DoSave(Widget widget, string ext) {
			string text = input.Text.ToString();
			if (text.Length == 0) {
				MakeDescWidget("&ePlease enter a filename"); return;
			}
			string file = Path.ChangeExtension(text, ext);
			text = Path.Combine(Program.AppDirectory, "maps");
			text = Path.Combine(text, file);
			
			ButtonWidget btn = (ButtonWidget)widget;
			if (File.Exists(text) && btn.OptName == null) {
				btn.SetText("&cOverwrite existing?");
				btn.OptName = "O";
			} else {
				// NOTE: We don't immediately save here, because otherwise the 'saving...'
				// will not be rendered in time because saving is done on the main thread.
				MakeDescWidget("Saving..");
				textPath = text;
				RemoveOverwrites();
			}
		}
		
		void RemoveOverwrites() {
			RemoveOverwrite(widgets[0], "Save"); 
			RemoveOverwrite(widgets[1], "Save schematic");
		}
		
		void RemoveOverwrite(Widget widget, string defaultText) {
			ButtonWidget button = (ButtonWidget)widget;
			if (button.OptName == null) return;
			
			button.OptName = null;
			button.SetText(defaultText);
		}
		
		string textPath;
		void SaveMap(string path) {
			bool classic = path.EndsWith(".cw");
			try {
				if (File.Exists(path))
					File.Delete(path);
				using (FileStream fs = new FileStream(path, FileMode.CreateNew, FileAccess.Write)) {
					IMapFormatExporter exporter = null;
					if (classic) exporter = new MapCwExporter();
					else exporter = new MapSchematicExporter();
					exporter.Save(fs, game);
				}
			} catch (Exception ex) {
				ErrorHandler.LogError("saving map", ex);
				MakeDescWidget("&cError while trying to save map");
				return;
			}
			game.Chat.Add("&eSaved map to: " + Path.GetFileName(path));
			game.Gui.SetNewScreen(new PauseScreen(game));
		}
		
		void MakeDescWidget(string text) {
			DisposeDescWidget();
			desc = TextWidget.Create(game, text, regularFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 65);
		}
		
		void DisposeDescWidget() {
			if (desc != null) {
				desc.Dispose();
				desc = null;
			}
		}
	}
}