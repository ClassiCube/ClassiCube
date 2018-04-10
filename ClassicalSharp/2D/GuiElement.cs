// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	public delegate void ClickHandler(Game g, Widget w);
	
	public abstract class GuiElement : IDisposable {
		
		protected Game game;
		public GuiElement(Game game) { this.game = game; }
		
		public abstract void Init();
		
		public abstract void Render(double delta);
		
		public abstract void Dispose();
		
		/// <summary> Causes the gui element to recreate all of its sub-elements and/or textures. </summary>
		/// <remarks> Typically used when bitmap font changes. </remarks>
		public virtual void Recreate() { Dispose(); Init(); }
		
		public virtual bool HandlesKeyDown(Key key) { return false; }
		
		public virtual bool HandlesKeyPress(char key) { return false; }
		
		public virtual bool HandlesKeyUp(Key key) { return false; }
		
		public virtual bool HandlesMouseDown(int mouseX, int mouseY, MouseButton button) { return false; }
		
		public virtual bool HandlesMouseMove(int mouseX, int mouseY) { return false; }
		
		public virtual bool HandlesMouseScroll(float delta) { return false; }
		
		public virtual bool HandlesMouseUp(int mouseX, int mouseY, MouseButton button) { return false; }
		
		protected static int CalcPos(Anchor anchor, int offset, int size, int axisLen) {
			if (anchor == Anchor.Min) return offset;
			if (anchor == Anchor.Max) return axisLen - size - offset;
			return (axisLen - size) / 2 + offset;
		}
		
		public static bool Contains(int recX, int recY, int width, int height, int x, int y) {
			return x >= recX && y >= recY && x < recX + width && y < recY + height;
		}
	}
	
	/// <summary> Represents a container of widgets and other 2D elements. </summary>
	/// <remarks> May cover the entire game window. </remarks>
	public abstract class Screen : GuiElement {
		
		public Screen(Game game) : base(game) { }

		public bool HandlesAllInput, BlocksWorld, HidesHud, RenderHudOver;

		public abstract void OnResize(int width, int height);
		
		protected abstract void ContextLost();
		
		protected abstract void ContextRecreated();
	}
	
	/// <summary> Represents an individual 2D gui component. </summary>
	public abstract class Widget : GuiElement {
		
		public Widget(Game game) : base(game) { }
		
		public ClickHandler MenuClick;
		public bool Active, Disabled;
		public int X, Y, Width, Height;
		public Anchor HorizontalAnchor, VerticalAnchor;
		public int XOffset, YOffset;
		
		public virtual void Reposition() {
			X = CalcPos(HorizontalAnchor, XOffset, Width, game.Width);
			Y = CalcPos(VerticalAnchor, YOffset, Height, game.Height);
		}
		
		public bool Contains(int x, int y) {
			return GuiElement.Contains(X, Y, Width, Height, x, y);
		}
	}
}
