// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public partial class InventoryScreen : Screen {
		
		TableWidget table;
		Font font;
		public InventoryScreen(Game game) : base(game) {
			font = new Font(game.FontName, 16);
			HandlesAllInput = true;
		}

		public override void Init() {
			InitTableWidget();
			game.Events.BlockPermissionsChanged += OnBlockChanged;
			game.Events.BlockDefinitionChanged += OnBlockChanged;
			game.Keyboard.KeyRepeat = true;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
		}
		
		void InitTableWidget() {
			table = new TableWidget(game);
			table.font = font;
			table.ElementsPerRow = game.PureClassic ? 9 : 10;
			table.Init();
		}
		
		public override void Render(double delta) { table.Render(delta); }
		
		public override void OnResize(int width, int height) { table.Reposition(); }
		
		public override void Dispose() {
			font.Dispose();
			table.Dispose();
			game.Events.BlockPermissionsChanged -= OnBlockChanged;
			game.Events.BlockDefinitionChanged -= OnBlockChanged;
			game.Keyboard.KeyRepeat = false;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}

		void OnBlockChanged(object sender, EventArgs e) {
			table.OnInventoryChanged();
		}		
		
		protected override void ContextLost() { table.Dispose(); }
		
		protected override void ContextRecreated() { table.Recreate(); }
		
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			return table.HandlesMouseMove(mouseX, mouseY);
		}
		
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
			if (table.scroll.draggingMouse || game.Gui.hudScreen.hotbar.HandlesMouseClick(mouseX, mouseY, button))
				return true;
			
			bool handled = table.HandlesMouseClick(mouseX, mouseY, button);
			if ((!handled || table.PendingClose) && button == MouseButton.Left) {
				bool hotbar = game.Input.ControlDown;
				if (!hotbar) game.Gui.SetNewScreen(null);
			}
			return true;
		}
		
		public override bool HandlesMouseScroll(float delta) {
			bool hotbar = game.Input.AltDown || game.Input.ControlDown || game.Input.ShiftDown;
			if (hotbar) return false;
			return table.HandlesMouseScroll(delta);
		}
		
		public override bool HandlesMouseUp(int mouseX, int mouseY, MouseButton button) {
			return table.HandlesMouseUp(mouseX, mouseY, button);
		}

		// We want the user to be able to press B to exit the inventory menu
		// however since we use KeyRepeat = true, we must wait until the first time they released B
		// before marking the next press as closing the menu
		bool releasedInv;
		
		public override bool HandlesKeyDown(Key key) {
			if (key == game.Mapping(KeyBind.PauseOrExit)) {
				game.Gui.SetNewScreen(null);
			} else if (key == game.Mapping(KeyBind.Inventory) && releasedInv) {
				game.Gui.SetNewScreen(null);
			} else if (key == Key.Enter && table.SelectedIndex != -1) {
				game.Inventory.Selected = table.Elements[table.SelectedIndex];
				game.Gui.SetNewScreen(null);
			} else if (table.HandlesKeyDown(key)) {
			} else {
				game.Gui.hudScreen.hotbar.HandlesKeyDown(key);
			}
			return true;
		}
		
		public override bool HandlesKeyUp(Key key) {
			if (key == game.Mapping(KeyBind.Inventory)) {
				releasedInv = true; return true;
			}
			return game.Gui.hudScreen.hotbar.HandlesKeyUp(key);
		}
	}
}
