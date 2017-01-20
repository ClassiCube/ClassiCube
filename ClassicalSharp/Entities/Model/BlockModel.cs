// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Physics;
using ClassicalSharp.Renderers;
using ClassicalSharp.Textures;
using OpenTK;

namespace ClassicalSharp.Model {

	public class BlockModel : IModel {
		
		byte block = (byte)Block.Air;
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
		}
		
		public override void CreateParts() { }
		
		public override float NameYOffset { get { return height + 0.075f; } }
		
		public override float GetEyeY(Entity entity) {
			byte block = Byte.Parse(entity.ModelName);
			float minY = game.BlockInfo.MinBB[block].Y;
			float maxY = game.BlockInfo.MaxBB[block].Y;
			return block == 0 ? 1 : (minY + maxY) / 2;
		}
		
		static Vector3 colShrink = new Vector3(0.75f/16, 0.75f/16, 0.75f/16);
		public override Vector3 CollisionSize {
			get { return (maxBB - minBB) - colShrink; } // to fit slightly inside
		}
		
		static Vector3 offset = new Vector3(-0.5f, -0.5f, -0.5f);
		public override AABB PickingBounds {
			get { return new AABB(minBB, maxBB).Offset(offset); }
		}
		
		public void CalcState(byte block) {
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
		
		public override bool ShouldRender(Entity p, FrustumCulling culling) {
			block = Utils.FastByte(p.ModelName);
			CalcState(block);
			return base.ShouldRender(p, culling);
		}
		
		public override float RenderDistance(Entity p) {
			block = Utils.FastByte(p.ModelName);
			CalcState(block);
			return base.RenderDistance(p);
		}
		
		int lastTexId = -1, brightCol;
		protected override void DrawModel(Entity p) {
			brightCol = FastColour.WhitePacked;
			
			// TODO: using 'is' is ugly, but means we can avoid creating
			// a string every single time held block changes.
			if (p is FakePlayer) {
				Player realP = game.LocalPlayer;
				Vector3I P = Vector3I.Floor(realP.EyePosition);
				col = game.World.IsValidPos(P) ? game.Lighting.LightCol(P.X, P.Y, P.Z) : game.Lighting.Outside;
				
				// Adjust pitch so angle when looking straight down is 0.
				float adjPitch = realP.PitchDegrees - 90;
				if (adjPitch < 0) adjPitch += 360;
				
				// Adjust colour so held block is brighter when looking straght up
				float t = Math.Abs(adjPitch - 180) / 180;
				float colScale = Utils.Lerp(0.9f, 0.7f, t);
				col = FastColour.ScalePacked(col, colScale);
				block = ((FakePlayer)p).Block;
			} else {
				block = Utils.FastByte(p.ModelName);
			}
			
			if (game.BlockInfo.Tinted[block]) {
                FastColour fogCol = game.BlockInfo.FogColour[block];
                brightCol = fogCol.Pack();
                
                FastColour newCol = FastColour.Unpack(col);
                newCol *= fogCol;
                col = newCol.Pack();
			}
			
			CalcState(block);
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
				YQuad(0, Side.Bottom, FastColour.ShadeYBottom);
				if (SwitchOrder) {
					XQuad(maxBB.X - 0.5f, Side.Right, FastColour.ShadeX);
					ZQuad(maxBB.Z - 0.5f, Side.Back,  FastColour.ShadeZ);					
					XQuad(minBB.X - 0.5f, Side.Left,  FastColour.ShadeX);
					ZQuad(minBB.Z - 0.5f, Side.Front, FastColour.ShadeZ);
				} else {
					ZQuad(minBB.Z - 0.5f, Side.Front, FastColour.ShadeZ);
					XQuad(maxBB.X - 0.5f, Side.Right, FastColour.ShadeX);
					ZQuad(maxBB.Z - 0.5f, Side.Back,  FastColour.ShadeZ);
					XQuad(minBB.X - 0.5f, Side.Left,  FastColour.ShadeX);
				}
				YQuad(height, Side.Top, 1.0f);
			}
		}
		
		void YQuad(float y, int side, float shade) {
			int texId = game.BlockInfo.GetTextureLoc(block, side), texIndex = 0;
			TextureRec rec = atlas.GetTexRec(texId, 1, out texIndex);
			FlushIfNotSame(texIndex);
			int col = bright ? brightCol : (NoShade ? this.col : FastColour.ScalePacked(this.col, shade));
			
			float vOrigin = (texId % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.U1 = minBB.X; rec.U2 = maxBB.X;
			rec.V1 = vOrigin + minBB.Z * atlas.invElementSize;
			rec.V2 = vOrigin + maxBB.Z * atlas.invElementSize * 15.99f/16f;
			
			cache.vertices[index++] = new VertexP3fT2fC4b(minBB.X - 0.5f, y, minBB.Z - 0.5f, rec.U1, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(maxBB.X - 0.5f, y, minBB.Z - 0.5f, rec.U2, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(maxBB.X - 0.5f, y, maxBB.Z - 0.5f, rec.U2, rec.V2, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(minBB.X - 0.5f, y, maxBB.Z - 0.5f, rec.U1, rec.V2, col);
		}

		void ZQuad(float z, int side, float shade) {
			int texId = game.BlockInfo.GetTextureLoc(block, side), texIndex = 0;
			TextureRec rec = atlas.GetTexRec(texId, 1, out texIndex);
			FlushIfNotSame(texIndex);
			int col = bright ? brightCol : (NoShade ? this.col : FastColour.ScalePacked(this.col, shade));
			
			if (side == Side.Back) {
				rec.U1 = minBB.X; rec.U2 = maxBB.X * 15.99f/16f;
			} else {
				rec.U1 = (1 - minBB.X); rec.U2 = (1 - maxBB.X) * 15.99f/16f;
			}
			
			float vOrigin = (texId % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.V1 = vOrigin + (1 - minBB.Y) * atlas.invElementSize;
			rec.V2 = vOrigin + (1 - maxBB.Y) * atlas.invElementSize * 15.99f/16f;
			
			cache.vertices[index++] = new VertexP3fT2fC4b(minBB.X - 0.5f, 0, z, rec.U1, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(minBB.X - 0.5f, height, z, rec.U1, rec.V2, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(maxBB.X - 0.5f, height, z, rec.U2, rec.V2, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(maxBB.X - 0.5f, 0, z, rec.U2, rec.V1, col);
		}

		void XQuad(float x, int side, float shade) {
			int texId = game.BlockInfo.GetTextureLoc(block, side), texIndex = 0;
			TextureRec rec = atlas.GetTexRec(texId, 1, out texIndex);
			FlushIfNotSame(texIndex);
			int col = bright ? brightCol : (NoShade ? this.col : FastColour.ScalePacked(this.col, shade));
			
			if (side == Side.Left) {
				rec.U1 = minBB.Z; rec.U2 = maxBB.Z * 15.99f/16f;
			} else {
				rec.U1 = (1 - minBB.Z); rec.U2 = (1 - maxBB.Z) * 15.99f/16f;
			}
			
			float vOrigin = (texId % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.V1 = vOrigin + (1 - minBB.Y) * atlas.invElementSize;
			rec.V2 = vOrigin + (1 - maxBB.Y) * atlas.invElementSize * 15.99f/16f;			
			
			cache.vertices[index++] = new VertexP3fT2fC4b(x, 0, minBB.Z - 0.5f, rec.U1, rec.V1, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(x, height, minBB.Z - 0.5f, rec.U1, rec.V2, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(x, height, maxBB.Z - 0.5f, rec.U2, rec.V2, col);
			cache.vertices[index++] = new VertexP3fT2fC4b(x, 0, maxBB.Z - 0.5f, rec.U2, rec.V1, col);
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
			int col = bright ? brightCol : this.col;

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
			int col = bright ? brightCol : this.col;
			
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