// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
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
			gfx.Texturing = true;
			RenderMenuWidgets(delta);
			input.Render(delta);
			if (desc != null) desc.Render(delta);
			gfx.Texturing = false;
			
			float cX = game.Width / 2, cY = game.Height / 2;
			gfx.Draw2DQuad(cX - 250, cY + 90, 500, 2, grey);
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
			if (key == Key.Escape) {
				game.Gui.SetNewScreen(null);
				return true;
			}
			return input.HandlesKeyDown(key);
		}
		
		public override bool HandlesKeyUp(Key key) {
			return input.HandlesKeyUp(key);
		}
		
		public override void Init() {
			game.Keyboard.KeyRepeat = true;
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16, FontStyle.Regular);
			
			input = MenuInputWidget.Create(game, 500, 30, "",
			                                     regularFont, new PathValidator())
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -30);
			input.ShowCaret = true;
			
			widgets = new Widget[] {
				ButtonWidget.Create(game, 301, 40, "Save", titleFont, SaveClassic)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, 20),
				ButtonWidget.Create(game, 201, 40, "Save schematic", titleFont, SaveSchematic)
					.SetLocation(Anchor.Centre, Anchor.Centre, -150, 120),
				TextWidget.Create(game, "&eCan be imported into MCEdit", regularFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 110, 120),
				null,
				MakeBack(false, titleFont,
				         (g, w) => g.Gui.SetNewScreen(new PauseScreen(g))),
			};
		}
		
		
		public override void OnResize(int width, int height) {
			input.CalculatePosition();
			base.OnResize(width, height);
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			input.Dispose();
			DisposeDescWidget();
			base.Dispose();
		}
		
		void SaveClassic(Game game, Widget widget, MouseButton btn, int x, int y) {
			DoSave(widget, btn, ".cw");
		}
		
		void SaveSchematic(Game game, Widget widget, MouseButton btn, int x, int y) {
			DoSave(widget, btn, ".schematic");
		}
		
		void DoSave(Widget widget, MouseButton mouseBtn, string ext) {
			if (mouseBtn != MouseButton.Left) return;
			
			string text = input.Text.ToString();
			if (text.Length == 0) {
				MakeDescWidget("&ePlease enter a filename"); return;
			}
			string file = Path.ChangeExtension(text, ext);
			text = Path.Combine(Program.AppDirectory, "maps");
			text = Path.Combine(text, file);
			
			if (File.Exists(text) && widget.Metadata == null) {
				((ButtonWidget)widget).SetText("&cOverwrite existing?");
				((ButtonWidget)widget).Metadata = true;
			} else {
				// NOTE: We don't immediately save here, because otherwise the 'saving...'
				// will not be rendered in time because saving is done on the main thread.
				MakeDescWidget("Saving..");
				textPath = text;
				RemoveOverwrites();
			}
		}
		
		void RemoveOverwrites() {
			RemoveOverwrite(widgets[0]); RemoveOverwrite(widgets[1]);
		}
		
		void RemoveOverwrite(Widget widget) {
			ButtonWidget button = (ButtonWidget)widget;
			if (button.Metadata == null) return;
			button.Metadata = null;
			button.SetText("Save");
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