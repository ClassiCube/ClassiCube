// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;

namespace ClassicalSharp.Gui.Widgets {
	public abstract class PlayerListWidget : Widget {
		
		protected readonly Font font;
		public PlayerListWidget(Game game, Font font) : base(game) {
			HorizontalAnchor = Anchor.Centre;
			VerticalAnchor = Anchor.Centre;
			this.font = font;
		}

		protected const int columnPadding = 5;
		protected const int boundsSize = 10;
		protected const int namesPerColumn = 20;
		
		protected int elementOffset = 0;
		protected int namesCount = 0;
		protected Texture[] textures;
		protected int columns;
		protected int xMin, xMax, yHeight;
		
		static FastColour topCol = new FastColour(0, 0, 0, 180);
		static FastColour bottomCol = new FastColour(50, 50, 50, 205);
		TextWidget overview;
		
		public override void Init() {
			overview = TextWidget.Create(game, "Connected players:", font)
				.SetLocation(Anchor.Centre, Anchor.LeftOrTop, 0, 0);
			
			CreateInitialPlayerInfo();
			SortPlayerInfo();
		}
		
		public abstract string GetNameUnder(int mouseX, int mouseY);
		
		public override void Render(double delta) {
			gfx.Texturing = false;
			int offset = overview.Height + 10;
			int height = Math.Max(300, Height + overview.Height);
			gfx.Draw2DQuad(X, Y - offset, Width, height, topCol, bottomCol);
			
			gfx.Texturing = true;
			overview.YOffset = Y - offset + 5;
			overview.Reposition();
			overview.Render(delta);
			
			for (int i = 0; i < namesCount; i++) {
				Texture tex = textures[i];
				int texY = tex.Y;
				tex.Y1 -= 10;
				if (tex.IsValid) tex.Render(gfx);
				tex.Y1 = texY;
			}
		}
		
		public override void Dispose() {
			for (int i = 0; i < namesCount; i++) {
				Texture tex = textures[i];
				gfx.DeleteTexture(ref tex);
				textures[i] = tex;
			}
			overview.Dispose();
		}
		
		protected void UpdateTableDimensions() {
			int width = xMax - xMin;
			X = xMin - boundsSize;
			Y = game.Height / 2 - yHeight / 2 - boundsSize;
			Width = width + boundsSize * 2;
			Height = yHeight + boundsSize * 2;
		}
		
		protected void CalcMaxColumnHeight() {
			yHeight = 0;
			for (int col = 0; col < columns; col++) {
				yHeight = Math.Max(GetColumnHeight(col), yHeight);
			}
		}
		
		protected int GetColumnWidth(int column) {
			int i = column * namesPerColumn;
			int maxWidth = 0;
			int maxIndex = Math.Min(namesCount, i + namesPerColumn);
			
			for (; i < maxIndex; i++)
				maxWidth = Math.Max(maxWidth, textures[i].Width);
			return maxWidth + columnPadding + elementOffset;
		}
		
		protected int GetColumnHeight(int column) {
			int i = column * namesPerColumn;
			int total = 0;
			int maxIndex = Math.Min(namesCount, i + namesPerColumn);
			
			for (; i < maxIndex; i++)
				total += textures[i].Height + 1;
			return total;
		}
		
		protected void SetColumnPos(int column, int x, int y) {
			int i = column * namesPerColumn;
			int maxIndex = Math.Min(namesCount, i + namesPerColumn);
			
			for (; i < maxIndex; i++) {
				Texture tex = textures[i];
				tex.X1 = x; tex.Y1 = y;
				
				y += tex.Height + 1;
				if (ShouldOffset(i))
					tex.X1 += elementOffset;
				textures[i] = tex;
			}
		}
		
		protected virtual bool ShouldOffset(int i) { return true; }
		
		public void RecalcYOffset() {
			YOffset = -Math.Max(0, game.Height / 4 - Height / 2);
		}
		
		public override void Reposition() {
			int oldX = X, oldY = Y;
			base.Reposition();
			
			for (int i = 0; i < namesCount; i++) {
				textures[i].X1 += X - oldX;
				textures[i].Y1 += Y - oldY;
			}
		}
		
		protected abstract void CreateInitialPlayerInfo();
		
		protected abstract void SortInfoList();
		
		protected void RemoveTextureAt(int i) {
			Texture tex = textures[i];
			gfx.DeleteTexture(ref tex);
			RemoveItemAt(textures, i);
			namesCount--;
			SortPlayerInfo();
		}
		
		protected void RemoveItemAt<T>(T[] array, int index) {
			for (int i = index; i < namesCount - 1; i++) {
				array[i] = array[i + 1];
			}
			array[namesCount - 1] = default(T);
		}
		
		protected void SortPlayerInfo() {
			columns = Utils.CeilDiv(namesCount, namesPerColumn);
			SortInfoList();
			columns = Utils.CeilDiv(namesCount, namesPerColumn);
			CalcMaxColumnHeight();
			int y = game.Height / 2 - yHeight / 2;
			int midCol = columns / 2;
			
			int centreX = game.Width / 2;
			int offset = 0;
			if (columns % 2 != 0) {
				// For an odd number of columns, the middle column is centred.
				offset = Utils.CeilDiv(GetColumnWidth(midCol), 2);
			}
			
			xMin = centreX - offset;
			for (int col = midCol - 1; col >= 0; col--) {
				xMin -= GetColumnWidth(col);
				SetColumnPos(col, xMin, y);
			}
			xMax = centreX - offset;
			for (int col = midCol; col < columns; col++) {
				SetColumnPos(col, xMax, y);
				xMax += GetColumnWidth(col);
			}
			
			OnSort();
			UpdateTableDimensions();
			RecalcYOffset();
			Reposition();
		}
		
		protected virtual void OnSort() {
			int width = 0, centreX = game.Width / 2;
			for (int col = 0; col < columns; col++)
				width += GetColumnWidth(col);
			if (width < 480) width = 480;
			
			xMin = centreX - width / 2;
			xMax = centreX + width / 2;
			
			int x = xMin, y = game.Height / 2 - yHeight / 2;
			for (int col = 0; col < columns; col++) {
				SetColumnPos(col, x, y);
				x += GetColumnWidth(col);
			}
		}
	}
}