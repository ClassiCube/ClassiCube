// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class InventoryScreen : Screen {
		
		TableWidget table;
		Font font;
		bool deferredSelect;
		public InventoryScreen(Game game) : base(game) {
			font = new Font(game.FontName, 16);
		}
		
		void MoveToSelected() {
			table.SetBlockTo(game.Inventory.Selected);
			table.Recreate();
			deferredSelect = false;
			
			// User is holding invalid block
			if (table.SelectedIndex == -1) {
				table.MakeDescTex(game.Inventory.Selected);
			}
		}

		public override void Init() {
			table = new TableWidget(game);
			table.font = font;
			table.ElementsPerRow = game.PureClassic ? 9 : 10;
			table.Init();
			
			// Can't immediately move to selected here, because cursor visibility 
			// might be toggled after Init() is called. This causes the cursor to 
			// be moved back to the middle of the window.
			deferredSelect = true;

			Events.BlockPermissionsChanged += OnBlockChanged;
			Events.BlockDefinitionChanged  += OnBlockChanged;
			Keyboard.KeyRepeat = true;
			Events.ContextLost += ContextLost;
			Events.ContextRecreated += ContextRecreated;
		}
		
		public override void Render(double delta) {
			if (deferredSelect) MoveToSelected();
			table.Render(delta); 
		}
		
		public override void Dispose() {
			font.Dispose();
			table.Dispose();
			Events.BlockPermissionsChanged -= OnBlockChanged;
			Events.BlockDefinitionChanged  -= OnBlockChanged;
			Keyboard.KeyRepeat = false;
			Events.ContextLost -= ContextLost;
			Events.ContextRecreated -= ContextRecreated;
		}
		
		public override void OnResize() { table.Reposition(); }
		
		void OnBlockChanged() {
			table.OnInventoryChanged();
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
		
		public override bool HandlesMouseDown(int mouseX, int mouseY, MouseButton button) {
			if (table.scroll.draggingMouse || game.Gui.hudScreen.hotbar.HandlesMouseDown(mouseX, mouseY, button))
				return true;
			
			bool handled = table.HandlesMouseDown(mouseX, mouseY, button);
			if ((!handled || table.PendingClose) && button == MouseButton.Left) {
				bool hotbar = game.Input.ControlDown || game.Input.ShiftDown;
				if (!hotbar) game.Gui.SetNewScreen(null);
			}
			return true;
		}
		
		public override bool HandlesMouseUp(int mouseX, int mouseY, MouseButton button) {
			return table.HandlesMouseUp(mouseX, mouseY, button);
		}	
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			return table.HandlesMouseMove(mouseX, mouseY);
		}
		
		public override bool HandlesMouseScroll(float delta) {
			bool hotbar = game.Input.AltDown || game.Input.ControlDown || game.Input.ShiftDown;
			if (hotbar) return false;
			return table.HandlesMouseScroll(delta);
		}		
				
		protected override void ContextLost() { table.Dispose(); }
		
		protected override void ContextRecreated() { table.Recreate(); }
	}
}
