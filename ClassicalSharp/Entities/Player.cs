// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Network;
using ClassicalSharp.Particles;
using OpenTK;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Entities {

	public abstract class Player : Entity {
		
		public string DisplayName, SkinName;
		protected Texture nameTex;
		internal bool fetchedSkin;
		
		public Player(Game game) : base(game) {
			StepSize = 0.5f;
			SetModel("humanoid");
		}
		
		public override void Despawn() {
			Player first = FirstOtherWithSameSkin();
			if (first == null) {
				game.Graphics.DeleteTexture(ref TextureId);
				ResetSkin();
			}
			ContextLost();
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
				IDrawer2D.Cols['\xFF'] = new FastColour(80, 80, 80);
				game.Drawer2D.DrawText(ref args, 3, 3);
				IDrawer2D.Cols['\xFF'] = default(FastColour);
				
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
			
			Vector3 pos;
			Model.RecalcProperties(this);
			Vector3.TransformY(Model.NameYOffset, ref transform, out pos);
			float scale = Math.Min(1, Model.NameScale * ModelScale.Y) / 70f;
			int col = FastColour.WhitePacked;
			Vector2 size = new Vector2(nameTex.Width * scale, nameTex.Height * scale);
			
			if (game.Entities.NamesMode == NameMode.AllUnscaled && game.LocalPlayer.Hacks.CanSeeAllNames) {
				// Get W component of transformed position
				Matrix4 mat;
				Matrix4.Mult(out mat, ref game.View, ref game.Projection); // TODO: This mul is slow, avoid it
				float tempW = pos.X * mat.Row0.W + pos.Y * mat.Row1.W + pos.Z * mat.Row2.W + mat.Row3.W;
				size.X *= tempW * 0.2f; size.Y *= tempW * 0.2f;
			}
			
			int index = 0;
			TextureRec rec; rec.U1 = 0; rec.V1 = 0; rec.U2 = nameTex.U2; rec.V2 = nameTex.V2;
			Particle.DoRender(game, ref size, ref pos, ref rec, col, gfx.texVerts, ref index);
			
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			gfx.UpdateDynamicVb_IndexedTris(gfx.texVb, gfx.texVerts, 4);
		}
		
		protected void CheckSkin() {
			if (!fetchedSkin && Model.UsesSkin) {
				Player first = FirstOtherWithSameSkinAndFetchedSkin();
				if (first == null) {
					game.AsyncDownloader.DownloadSkin(SkinName, SkinName);
				} else {
					ApplySkin(first);
				}
				fetchedSkin = true;
			}
			
			Request item;
			if (!game.AsyncDownloader.TryGetItem(SkinName, out item)) return;
			if (item == null || item.Data == null) { SetSkinAll(true); return; }
			
			Bitmap bmp = (Bitmap)item.Data;
			game.Graphics.DeleteTexture(ref TextureId);
			
			SetSkinAll(true);		
			EnsurePow2(ref bmp);			
			SkinType = Utils.GetSkinType(bmp);
			
			if (SkinType == SkinType.Invalid) {
				SetSkinAll(true);
			} else {
				if (Model.UsesHumanSkin) ClearHat(bmp, SkinType);
				TextureId = game.Graphics.CreateTexture(bmp, true, false);
				SetSkinAll(false);
			}
			bmp.Dispose();
		}
		
		Player FirstOtherWithSameSkin() {
			Entity[] entities = game.Entities.List;
			for (int i = 0; i < EntityList.MaxCount; i++) {
				if (entities[i] == null || entities[i] == this) continue;
				Player p = entities[i] as Player;
				if (p != null && p.SkinName == SkinName) return p;
			}
			return null;
		}
		
		Player FirstOtherWithSameSkinAndFetchedSkin() {
			Entity[] entities = game.Entities.List;
			for (int i = 0; i < EntityList.MaxCount; i++) {
				if (entities[i] == null || entities[i] == this) continue;
				Player p = entities[i] as Player;
				if (p != null && p.SkinName == SkinName && p.fetchedSkin) return p;
			}
			return null;
		}
		
		// Apply or reset skin, for all players with same skin
		void SetSkinAll(bool reset) {
			Entity[] entities = game.Entities.List;
			for (int i = 0; i < EntityList.MaxCount; i++) {
				if (entities[i] == null) continue;
				Player p = entities[i] as Player;
				if (p == null || p.SkinName != SkinName) continue;
				
				if (reset) { p.ResetSkin(); } 
				else { p.ApplySkin(this); }
			}
		}
		
		void ApplySkin(Player src) {
			TextureId = src.TextureId;
			MobTextureId = -1;
			SkinType = src.SkinType;
			uScale = src.uScale; vScale = src.vScale;
			
			// Custom mob textures.
			if (Utils.IsUrlPrefix(SkinName, 0)) MobTextureId = TextureId;
		}
		
		internal void ResetSkin() {
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