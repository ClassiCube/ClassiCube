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
		bool bright;
		Vector3 minBB, maxBB;
		public bool NoShade = false, SwitchOrder = false;
		public float CosX = 1, SinX = 0;
		ModelCache cache;
		
		public BlockModel(Game game) : base(game) {
			cache = game.ModelCache;
			Bobbing = false;
			UsesSkin = false;
		}
		
		public override void CreateParts() { }
		
		public override float NameYOffset { get { return height + 0.075f; } }
		
		public override float GetEyeY(Entity entity) {
			BlockID block = Byte.Parse(entity.ModelName);
			float minY = game.BlockInfo.MinBB[block].Y;
			float maxY = game.BlockInfo.MaxBB[block].Y;
			return block == 0 ? 1 : (minY + maxY) / 2;
		}
		
		static Vector3 colShrink = new Vector3(0.75f/16, 0.75f/16, 0.75f/16);
		public override Vector3 CollisionSize {
			get { return (maxBB - minBB) - colShrink; } // to fit slightly inside
		}
		
		static Vector3 offset = new Vector3(-0.5f, 0.0f, -0.5f);
		public override AABB PickingBounds {
			get { return new AABB(minBB, maxBB).Offset(offset); }
		}
		
		public void CalcState(BlockID block) {
			if (game.BlockInfo.Draw[block] == DrawType.Gas) {
				bright = false;
				minBB = Vector3.Zero;
				maxBB = Vector3.One;
				height = 1;
			} else {
				bright = game.BlockInfo.FullBright[block];
				minBB = game.BlockInfo.MinBB[block];
				maxBB = game.BlockInfo.MaxBB[block];
				height = maxBB.Y - minBB.Y;
				if (game.BlockInfo.Draw[block] == DrawType.Sprite)
					height = 1;
			}
		}
		
		public override float RenderDistance(Entity p) {
			block = Utils.FastByte(p.ModelName);
			CalcState(block);
			return base.RenderDistance(p);
		}
		
		int lastTexId = -1;
		protected override void DrawModel(Entity p) {
			// TODO: using 'is' is ugly, but means we can avoid creating
			// a string every single time held block changes.
			if (p is FakePlayer) {
				Player realP = game.LocalPlayer;
				Vector3I P = Vector3I.Floor(realP.EyePosition);
				col = game.World.IsValidPos(P) ? game.Lighting.LightCol(P.X, P.Y, P.Z) : game.Lighting.Outside;
				
				// Adjust pitch so angle when looking straight down is 0.
				float adjHeadX = realP.HeadX - 90;
				if (adjHeadX < 0) adjHeadX += 360;
				
				// Adjust colour so held block is brighter when looking straght up
				float t = Math.Abs(adjHeadX - 180) / 180;
				float colScale = Utils.Lerp(0.9f, 0.7f, t);
				col = FastColour.ScalePacked(col, colScale);
				block = ((FakePlayer)p).Block;
			} else {
				block = Utils.FastByte(p.ModelName);
			}
			
			CalcState(block);
			if (!(p is FakePlayer)) NoShade = bright;
			if (bright) col = FastColour.WhitePacked;
			if (game.BlockInfo.Draw[block] == DrawType.Gas) return;
			
			lastTexId = -1;
			atlas = game.TerrainAtlas1D;
			bool sprite = game.BlockInfo.Draw[block] == DrawType.Sprite;
			DrawParts(sprite);
			if (index == 0) return;
			
			IGraphicsApi gfx = game.Graphics;
			gfx.BindTexture(lastTexId);
			TransformVertices();
			
			if (sprite) gfx.FaceCulling = true;
			UpdateVB();
			if (sprite) gfx.FaceCulling = false;
		}
		
		void FlushIfNotSame(int texIndex) {
			int texId = game.TerrainAtlas1D.TexIds[texIndex];
			if (texId == lastTexId) return;
			
			if (lastTexId != -1) {
				game.Graphics.BindTexture(lastTexId);
				TransformVertices();
				UpdateVB();
			}
			lastTexId = texId;
			index = 0;
		}
		
		void TransformVertices() {
			for (int i = 0; i < index; i++) {
				VertexP3fT2fC4b v = cache.vertices[i];
				float t = 0;
				t = CosX * v.Y + SinX * v.Z; v.Z = -SinX * v.Y + CosX * v.Z; v.Y = t;        // Inlined RotX
				cache.vertices[i] = v;
			}
		}
		
		CuboidDrawer drawer = new CuboidDrawer();
		void DrawParts(bool sprite) {
			// SwitchOrder is needed for held block, which renders without depth testing
			if (sprite) {
				if (SwitchOrder) {
					SpriteZQuad(Side.Back, false);
					SpriteXQuad(Side.Right, false);
				} else {
					SpriteXQuad(Side.Right, false);
					SpriteZQuad(Side.Back, false);
				}
				
				if (SwitchOrder) {
					SpriteXQuad(Side.Right, true);
					SpriteZQuad(Side.Back, true);
				} else {
					SpriteZQuad(Side.Back, true);
					SpriteXQuad(Side.Right, true);
				}
			} else {
				drawer.elementsPerAtlas1D = atlas.elementsPerAtlas1D;
				drawer.invVerElementSize = atlas.invElementSize;
				
				drawer.minBB = game.BlockInfo.MinBB[block]; drawer.minBB.Y = 1 - drawer.minBB.Y; 				
				drawer.maxBB = game.BlockInfo.MaxBB[block]; drawer.maxBB.Y = 1 - drawer.maxBB.Y;
				
				Vector3 min = game.BlockInfo.RenderMinBB[block];
				Vector3 max = game.BlockInfo.RenderMaxBB[block];
				drawer.x1 = min.X - 0.5f; drawer.y1 = min.Y; drawer.z1 = min.Z - 0.5f;
				drawer.x2 = max.X - 0.5f; drawer.y2 = max.Y; drawer.z2 = max.Z - 0.5f;
				
				drawer.Tinted = game.BlockInfo.Tinted[block];
				drawer.TintColour = game.BlockInfo.FogColour[block];
				
				drawer.Bottom(1, Colour(FastColour.ShadeYBottom), GetTex(Side.Bottom), cache.vertices, ref index);
				if (SwitchOrder) {
					drawer.Right(1, Colour(FastColour.ShadeX), GetTex(Side.Right), cache.vertices, ref index);
					drawer.Back(1,  Colour(FastColour.ShadeZ), GetTex(Side.Back),  cache.vertices, ref index);
					drawer.Left(1,  Colour(FastColour.ShadeX), GetTex(Side.Left),  cache.vertices, ref index);
					drawer.Front(1, Colour(FastColour.ShadeZ), GetTex(Side.Front), cache.vertices, ref index);
				} else {
					drawer.Front(1, Colour(FastColour.ShadeZ), GetTex(Side.Front), cache.vertices, ref index);
					drawer.Right(1, Colour(FastColour.ShadeX), GetTex(Side.Right), cache.vertices, ref index);
					drawer.Back(1,  Colour(FastColour.ShadeZ), GetTex(Side.Back),  cache.vertices, ref index);
					drawer.Left(1,  Colour(FastColour.ShadeX), GetTex(Side.Left),  cache.vertices, ref index);
				}
				drawer.Top(1, Colour(1.0f), GetTex(Side.Top), cache.vertices, ref index);
			}
		}
		
		int GetTex(int side) {
			int texId = game.BlockInfo.GetTextureLoc(block, side);
			texIndex = texId / atlas.elementsPerAtlas1D;
			
			FlushIfNotSame(texIndex);
			return texId;
		}
		
		int Colour(float shade) {
			return NoShade ? col : FastColour.ScalePacked(col, shade); 
		}
		
		void SpriteZQuad(int side, bool firstPart) {
			SpriteZQuad(side, firstPart, false);
			SpriteZQuad(side, firstPart, true);
		}
		
		void SpriteZQuad(int side, bool firstPart, bool mirror) {
			int texId = game.BlockInfo.GetTextureLoc(block, side), texIndex = 0;
			TextureRec rec = atlas.GetTexRec(texId, 1, out texIndex);
			FlushIfNotSame(texIndex);
			if (height != 1)
				rec.V2 = rec.V1 + height * atlas.invElementSize * (15.99f/16f);
			int col = this.col;

			float p1 = 0, p2 = 0;
			if (firstPart) { // Need to break into two quads for when drawing a sprite model in hand.
				if (mirror) { rec.U1 = 0.5f; p1 = -5.5f/16; }
				else { rec.U2 = 0.5f; p2 = -5.5f/16; }
			} else {
				if (mirror) { rec.U2 = 0.5f; p2 = 5.5f/16; }
				else { rec.U1 = 0.5f; p1 = 5.5f/16; }
			}
			
			cache.vertices[index++] = new VertexP3fT2fC4b(p1, 0, p1, rec.U2, rec.V2, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(p1, 1, p1, rec.U2, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(p2, 1, p2, rec.U1, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(p2, 0, p2, rec.U1, rec.V2, col);
		}
		
		void SpriteXQuad(int side, bool firstPart) {
			SpriteXQuad(side, firstPart, false);
			SpriteXQuad(side, firstPart, true);
		}

		void SpriteXQuad(int side, bool firstPart, bool mirror) {
			int texId = game.BlockInfo.GetTextureLoc(block, side), texIndex = 0;
			TextureRec rec = atlas.GetTexRec(texId, 1, out texIndex);
			FlushIfNotSame(texIndex);
			if (height != 1)
				rec.V2 = rec.V1 + height * atlas.invElementSize * (15.99f/16f);
			int col = this.col;
			
			float x1 = 0, x2 = 0, z1 = 0, z2 = 0;
			if (firstPart) {
				if (mirror) { rec.U2 = 0.5f; x2 = -5.5f/16; z2 = 5.5f/16; }
				else { rec.U1 = 0.5f; x1 = -5.5f/16; z1 = 5.5f/16; }
			} else {
				if (mirror) { rec.U1 = 0.5f; x1 = 5.5f/16; z1 = -5.5f/16; }
				else { rec.U2 = 0.5f; x2 = 5.5f/16; z2 = -5.5f/16; }
			}

			cache.vertices[index++] = new VertexP3fT2fC4b(x1, 0, z1, rec.U2, rec.V2, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(x1, 1, z1, rec.U2, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(x2, 1, z2, rec.U1, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(x2, 0, z2, rec.U1, rec.V2, col);
		}
	}
}