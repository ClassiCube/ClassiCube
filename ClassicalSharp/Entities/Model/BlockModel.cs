// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Physics;
using ClassicalSharp.Renderers;
using ClassicalSharp.Textures;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Model {

	public class BlockModel : IModel {
		
		BlockID block = Block.Air;
		float height;
		TerrainAtlas1D atlas;
		Vector3 minBB, maxBB;
		ModelCache cache;
		int lastTexIndex = -1;
		
		public BlockModel(Game game) : base(game) {
			cache = game.ModelCache;
			Bobbing = false;
			UsesSkin = false;
			Pushes = false;
		}
		
		public override void CreateParts() { }
		
		public override float NameYOffset { get { return height + 0.075f; } }
		
		public override float GetEyeY(Entity entity) {
			BlockID block = entity.ModelBlock;
			float minY = BlockInfo.MinBB[block].Y;
			float maxY = BlockInfo.MaxBB[block].Y;
			return block == Block.Air ? 1 : (minY + maxY) / 2;
		}
		
		static Vector3 colShrink = new Vector3(0.75f/16, 0.75f/16, 0.75f/16);
		public override Vector3 CollisionSize {
			get { return (maxBB - minBB) - colShrink; } // to fit slightly inside
		}
		
		static Vector3 offset = new Vector3(-0.5f, 0.0f, -0.5f);
		public override AABB PickingBounds {
			get { return new AABB(minBB, maxBB).Offset(offset); }
		}
		
		public override void RecalcProperties(Entity p) {
			BlockID block = p.ModelBlock;
			if (BlockInfo.Draw[block] == DrawType.Gas) {
				minBB = Vector3.Zero;
				maxBB = Vector3.One;
				height = 1;
			} else {
				minBB = BlockInfo.MinBB[block];
				maxBB = BlockInfo.MaxBB[block];
				height = maxBB.Y - minBB.Y;
			}
		}

		public override void DrawModel(Entity p) {
			block = p.ModelBlock;
			RecalcProperties(p);
			
			if (BlockInfo.FullBright[block]) {
				for (int i = 0; i < cols.Length; i++)
					cols[i] = FastColour.WhitePacked;
			}
			if (BlockInfo.Draw[block] == DrawType.Gas) return;
			
			lastTexIndex = -1;
			atlas = game.TerrainAtlas1D;
			bool sprite = BlockInfo.Draw[block] == DrawType.Sprite;
			DrawParts(sprite);
			if (index == 0) return;
			
			if (sprite) game.Graphics.FaceCulling = true;
			lastTexIndex = texIndex; Flush();
			if (sprite) game.Graphics.FaceCulling = false;
		}
		
		void Flush() {
			if (lastTexIndex != -1) {
				game.Graphics.BindTexture(atlas.TexIds[lastTexIndex]);
				UpdateVB();
			}
			
			lastTexIndex = texIndex;
			index = 0;
		}
		
		CuboidDrawer drawer = new CuboidDrawer();
		void DrawParts(bool sprite) {
			if (sprite) {
				SpriteXQuad(false, false);
				SpriteXQuad(false, true);
				SpriteZQuad(false, false);
				SpriteZQuad(false, true);
				
				SpriteZQuad(true, false);
				SpriteZQuad(true, true);
				SpriteXQuad(true, false);
				SpriteXQuad(true, true);
			} else {
				drawer.elementsPerAtlas1D = atlas.elementsPerAtlas1D;
				drawer.invVerElementSize = atlas.invElementSize;
				
				drawer.minBB = BlockInfo.MinBB[block]; drawer.minBB.Y = 1 - drawer.minBB.Y;
				drawer.maxBB = BlockInfo.MaxBB[block]; drawer.maxBB.Y = 1 - drawer.maxBB.Y;
				
				Vector3 min = BlockInfo.RenderMinBB[block];
				Vector3 max = BlockInfo.RenderMaxBB[block];
				drawer.x1 = min.X - 0.5f; drawer.y1 = min.Y; drawer.z1 = min.Z - 0.5f;
				drawer.x2 = max.X - 0.5f; drawer.y2 = max.Y; drawer.z2 = max.Z - 0.5f;
				
				drawer.Tinted = BlockInfo.Tinted[block];
				drawer.TintColour = BlockInfo.FogColour[block];
				
				drawer.Bottom(1, cols[1], GetTex(Side.Bottom), cache.vertices, ref index);
				drawer.Front(1, cols[3], GetTex(Side.Front), cache.vertices, ref index);
				drawer.Right(1, cols[5], GetTex(Side.Right), cache.vertices, ref index);
				drawer.Back(1,  cols[2], GetTex(Side.Back),  cache.vertices, ref index);
				drawer.Left(1,  cols[4], GetTex(Side.Left),  cache.vertices, ref index);
				drawer.Top(1, cols[0], GetTex(Side.Top), cache.vertices, ref index);
			}
		}
		
		int GetTex(int side) {
			int texLoc = BlockInfo.GetTextureLoc(block, side);
			texIndex = texLoc / atlas.elementsPerAtlas1D;
			
			if (lastTexIndex != texIndex) Flush();
			return texLoc;
		}
		
		void SpriteZQuad(bool firstPart, bool mirror) {
			int texLoc = BlockInfo.GetTextureLoc(block, Side.Back);
			TextureRec rec = atlas.GetTexRec(texLoc, 1, out texIndex);
			if (lastTexIndex != texIndex) Flush();
			
			int col = cols[0];
			float p1 = 0, p2 = 0;
			if (firstPart) { // Need to break into two quads for when drawing a sprite model in hand.
				if (mirror) { rec.U1 = 0.5f; p1 = -5.5f/16; }
				else {        rec.U2 = 0.5f; p2 = -5.5f/16; }
			} else {
				if (mirror) { rec.U2 = 0.5f; p2 = 5.5f/16; }
				else {        rec.U1 = 0.5f; p1 = 5.5f/16; }
			}
			
			cache.vertices[index++] = new VertexP3fT2fC4b(p1, 0, p1, rec.U2, rec.V2, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(p1, 1, p1, rec.U2, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(p2, 1, p2, rec.U1, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(p2, 0, p2, rec.U1, rec.V2, col);
		}

		void SpriteXQuad(bool firstPart, bool mirror) {
			int texLoc = BlockInfo.GetTextureLoc(block, Side.Right);
			TextureRec rec = atlas.GetTexRec(texLoc, 1, out texIndex);
			if (lastTexIndex != texIndex) Flush();

			int col = cols[0];
			float x1 = 0, x2 = 0, z1 = 0, z2 = 0;
			if (firstPart) {
				if (mirror) { rec.U2 = 0.5f; x2 = -5.5f/16; z2 = 5.5f/16; }
				else {        rec.U1 = 0.5f; x1 = -5.5f/16; z1 = 5.5f/16; }
			} else {
				if (mirror) { rec.U1 = 0.5f; x1 = 5.5f/16; z1 = -5.5f/16; }
				else {        rec.U2 = 0.5f; x2 = 5.5f/16; z2 = -5.5f/16; }
			}

			cache.vertices[index++] = new VertexP3fT2fC4b(x1, 0, z1, rec.U2, rec.V2, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(x1, 1, z1, rec.U2, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(x2, 1, z2, rec.U1, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(x2, 0, z2, rec.U1, rec.V2, col);
		}
	}
}