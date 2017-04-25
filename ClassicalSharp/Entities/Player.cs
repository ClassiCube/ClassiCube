// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Network;
using OpenTK;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Entities {

	public abstract partial class Player : Entity {
		
		public string DisplayName, SkinName, SkinIdentifier;
		internal ShadowComponent shadow;
		protected Texture nameTex;
		internal bool fetchedSkin;
		
		public Player(Game game) : base(game) {
			StepSize = 0.5f;
			shadow = new ShadowComponent(game, this);
			SetModel("humanoid");
		}
		
		public override void Despawn() {
			game.Graphics.DeleteTexture(ref TextureId);
			game.Graphics.DeleteTexture(ref nameTex.ID);
		}
		
		public override void ContextLost() {
			game.Graphics.DeleteTexture(ref nameTex.ID);
		}
		
		public override void ContextRecreated() { UpdateName(); }
		
		protected void MakeNameTexture() {
			using (Font font = new Font(game.FontName, 24)) {
				DrawTextArgs args = new DrawTextArgs(DisplayName, font, false);
				bool bitmapped = game.Drawer2D.UseBitmappedChat; // we want names to always be drawn without
				game.Drawer2D.UseBitmappedChat = true;
				Size size = game.Drawer2D.MeasureSize(ref args);
				
				if (size.Width == 0) {
					nameTex = new Texture(-1, 0, 0, 0, 0, 1, 1);
				} else {
					nameTex = MakeNameTextureImpl(size, args);
				}
				game.Drawer2D.UseBitmappedChat = bitmapped;
			}
		}
		
		Texture MakeNameTextureImpl(Size size, DrawTextArgs args) {
			size.Width += 3; size.Height += 3;
			
			using (IDrawer2D drawer = game.Drawer2D)
				using (Bitmap bmp = IDrawer2D.CreatePow2Bitmap(size))
			{
				drawer.SetBitmap(bmp);
				args.Text = "&\xFF" + Utils.StripColours(args.Text);
				game.Drawer2D.Colours['\xFF'] = new FastColour(80, 80, 80);
				game.Drawer2D.DrawText(ref args, 3, 3);
				game.Drawer2D.Colours['\xFF'] = default(FastColour);
				
				args.Text = DisplayName;
				game.Drawer2D.DrawText(ref args, 0, 0);
				return game.Drawer2D.Make2DTexture(bmp, size, 0, 0);
			}
		}
		
		public void UpdateName() {
			ContextLost();
			if (game.Graphics.LostContext) return;
			MakeNameTexture();
		}
		
		protected void DrawName() {
			if (nameTex.ID == 0) MakeNameTexture();
			if (nameTex.ID == -1) return;
			
			IGraphicsApi gfx = game.Graphics;
			gfx.BindTexture(nameTex.ID);
			Vector3 pos = Position; pos.Y += Model.NameYOffset * ModelScale;
			float scale = Math.Min(1, Model.NameScale * ModelScale) / 70f;
			
			Vector3 p111, p121, p212, p222;
			int col = FastColour.WhitePacked;
			Vector2 size = new Vector2(nameTex.Width * scale, nameTex.Height * scale);
			Utils.CalcBillboardPoints(size, pos, ref game.View, out p111, out p121, out p212, out p222);
			gfx.texVerts[0] = new VertexP3fT2fC4b(ref p111, nameTex.U1, nameTex.V2, col);
			gfx.texVerts[1] = new VertexP3fT2fC4b(ref p121, nameTex.U1, nameTex.V1, col);
			gfx.texVerts[2] = new VertexP3fT2fC4b(ref p222, nameTex.U2, nameTex.V1, col);
			gfx.texVerts[3] = new VertexP3fT2fC4b(ref p212, nameTex.U2, nameTex.V2, col);
			
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			gfx.UpdateDynamicIndexedVb(DrawMode.Triangles, gfx.texVb, gfx.texVerts, 4);
		}
		
		protected void CheckSkin() {
			if (!fetchedSkin && Model.UsesSkin) {
				game.AsyncDownloader.DownloadSkin(SkinIdentifier, SkinName);
				fetchedSkin = true;
			}
			
			DownloadedItem item;
			if (!game.AsyncDownloader.TryGetItem(SkinIdentifier, out item)) return;
			
			if (item == null || item.Data == null) { ResetSkin(); return; }
			
			Bitmap bmp = (Bitmap)item.Data;
			game.Graphics.DeleteTexture(ref TextureId);
			uScale = 1; vScale = 1;
			EnsurePow2(ref bmp);
			
			SkinType = Utils.GetSkinType(bmp);
			if (SkinType == SkinType.Invalid) {
				string format = "Skin {0} has unsupported dimensions({1}, {2}), reverting to default.";
				Utils.LogDebug(format, SkinName, bmp.Width, bmp.Height);
				bmp.Dispose();
				
				ResetSkin(); return;
			}
			
			if (Model is HumanoidModel)
				ClearHat(bmp, SkinType);
			TextureId = game.Graphics.CreateTexture(bmp, true);
			MobTextureId = -1;
			
			// Custom mob textures.
			if (Utils.IsUrlPrefix(SkinName, 0) && item.TimeAdded >= lastModelChange)
				MobTextureId = TextureId;
			bmp.Dispose();
		}
		
		void ResetSkin() {
			uScale = 1; vScale = 1;
			MobTextureId = -1;
			TextureId = -1;
			SkinType = game.DefaultPlayerSkinType;
		}
		
		unsafe static void ClearHat(Bitmap bmp, SkinType skinType) {
			using (FastBitmap fastBmp = new FastBitmap(bmp, true, false)) {
				int sizeX = (bmp.Width / 64) * 32;
				int yScale = skinType == SkinType.Type64x32 ? 32 : 64;
				int sizeY = (bmp.Height / yScale) * 16;
				
				// determine if we actually need filtering
				for (int y = 0; y < sizeY; y++) {
					int* row = fastBmp.GetRowPtr(y);
					row += sizeX;
					for (int x = 0; x < sizeX; x++) {
						byte alpha = (byte)(row[x] >> 24);
						if (alpha != 255) return;
					}
				}
				
				// only perform filtering when the entire hat is opaque
				int fullWhite = FastColour.White.ToArgb();
				int fullBlack = FastColour.Black.ToArgb();
				for (int y = 0; y < sizeY; y++) {
					int* row = fastBmp.GetRowPtr(y);
					row += sizeX;
					for (int x = 0; x < sizeX; x++) {
						int pixel = row[x];
						if (pixel == fullWhite || pixel == fullBlack) row[x] = 0;
					}
				}
			}
		}
		
		void EnsurePow2(ref Bitmap bmp) {
			int width = Utils.NextPowerOf2(bmp.Width);
			int height = Utils.NextPowerOf2(bmp.Height);
			if (width == bmp.Width && height == bmp.Height) return;
			
			Bitmap scaled = Platform.CreateBmp(width, height);
			using (FastBitmap src = new FastBitmap(bmp, true, true))
				using (FastBitmap dst = new FastBitmap(scaled, true, false))
			{
				for (int y = 0; y < src.Height; y++)
					FastBitmap.CopyRow(y, y, src, dst, src.Width);
			}
			
			uScale = (float)bmp.Width / width;
			vScale = (float)bmp.Height / height;
			bmp.Dispose();
			bmp = scaled;
		}
	}
}