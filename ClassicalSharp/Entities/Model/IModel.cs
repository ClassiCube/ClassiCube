// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using ClassicalSharp.Physics;
using OpenTK;

namespace ClassicalSharp.Model {
	
	/// <summary> Describes the type of skin that a humanoid model uses. </summary>
	public enum SkinType { Type64x32, Type64x64, Type64x64Slim, Invalid }
	
	/// <summary> Contains a set of quads and/or boxes that describe a 3D object as well as
	/// the bounding boxes that contain the entire set of quads and/or boxes. </summary>
	public abstract class IModel {
		protected Game game;
		protected const int quadVertices = 4;
		protected const int boxVertices = 6 * quadVertices;
		protected RotateOrder Rotate = RotateOrder.ZYX;
		internal bool initalised;

		public IModel(Game game) { this.game = game; }
		
		public abstract void CreateParts();
		
		/// <summary> Whether the entity should be slightly bobbed up and down when rendering. </summary>
		/// <remarks> e.g. for players when their legs are at the peak of their swing,
		/// the whole model will be moved slightly down. </remarks>
		public bool Bobbing = true;

		/// <summary> Whether this entity requires downloading of a skin texture. </summary>
		public bool UsesSkin = true;
		
		/// <summary> Whether humanoid animations should be calculated, instead of normal animations. </summary>
		public bool CalcHumanAnims;
		
		/// <summary> Gravity applied to this entity. </summary>
		public float Gravity = 0.08f;
		
		/// <summary> Drag applied to the entity. </summary>
		public Vector3 Drag = new Vector3(0.91f, 0.98f, 0.91f);
		
		/// <summary> Friction applied to the entity when is on the ground. </summary>
		public Vector3 GroundFriction = new Vector3(0.6f, 1.0f, 0.6f);
		
		
		/// <summary> Vertical offset from the model's feet/base that the name texture should be drawn at. </summary>
		public abstract float NameYOffset { get; }
		
		/// <summary> Vertical offset from the model's feet/base that the model's eye is located. </summary>
		public abstract float GetEyeY(Entity entity);
		
		/// <summary> The maximum scale the entity can have (for collisions and rendering). </summary>
		public float MaxScale = 2.0f;
		
		/// <summary> Scaling factor applied, multiplied by the entity's current model scale. </summary>
		public float ShadowScale = 1.0f;
		
		/// <summary> Scaling factor applied, multiplied by the entity's current model scale. </summary>
		public float NameScale = 1.0f;
		
		
		/// <summary> The size of the bounding box that is used when
		/// performing collision detection for this model. </summary>
		public abstract Vector3 CollisionSize { get; }
		
		/// <summary> Bounding box that contains this model,
		/// assuming that the model is not rotated at all.</summary>
		public abstract AABB PickingBounds { get; }
		
		protected float cosHead, sinHead;
		protected float uScale, vScale;
		
		/// <summary> Returns whether the model should be rendered based on the given entity's position. </summary>
		public static bool ShouldRender(Entity p, FrustumCulling culling) {
			Vector3 pos = p.Position;
			AABB bb = p.PickingBounds;
			
			float maxLen = Math.Max(bb.Width, Math.Max(bb.Height, bb.Length));
			pos.Y += bb.Height / 2; // centre Y coordinate
			return culling.SphereInFrustum(pos.X, pos.Y, pos.Z, maxLen);
		}
		
		/// <summary> Returns the closest distance of the given entity to the camera. </summary>
		public static float RenderDistance(Entity p, Vector3 camPos) {
			Vector3 pos = p.Position;
			AABB bb = p.modelAABB;
			pos.Y += bb.Height / 2; // centre Y coordinate
			
			float dx = MinDist(camPos.X - pos.X, bb.Width / 2);
			float dy = MinDist(camPos.Y - pos.Y, bb.Height / 2);
			float dz = MinDist(camPos.Z - pos.Z, bb.Length / 2);
			return dx * dx + dy * dy + dz * dz;
		}
		
		static float MinDist(float dist, float extent) {
			// Compare min coord, centre coord, and max coord
			float dMin = Math.Abs(dist - extent), dMax = Math.Abs(dist + extent);
			return Math.Min(Math.Abs(dist), Math.Min(dMin, dMax));
		}
		
		/// <summary> Sets up the state for, then renders an entity model,
		/// based on the given entity's position and orientation. </summary>
		public void Render(Entity p) {
			Vector3 pos = p.Position;
			if (Bobbing) pos.Y += p.anim.bobbingModel;
			SetupState(p);

			game.Graphics.SetBatchFormat(VertexFormat.P3fT2fC4b);
			game.Graphics.PushMatrix();
			
			Matrix4 m = TransformMatrix(p, pos);
			p.transform = m;
			game.Graphics.MultiplyMatrix(ref m);
			DrawModel(p);
			game.Graphics.PopMatrix();
		}
		
		public void SetupState(Entity p) {
			index = 0;
			int col = p.Colour();
			uScale = 1 / 64f; vScale = 1 / 32f;
			
			cols[0] = col;
			if (!p.NoShade) {
				cols[1] = FastColour.ScalePacked(col, FastColour.ShadeYBottom);
				cols[2] = FastColour.ScalePacked(col, FastColour.ShadeZ);
				cols[4] = FastColour.ScalePacked(col, FastColour.ShadeX);
			} else {
				cols[1] = col; cols[2] = col; cols[4] = col;
			}
			cols[3] = cols[2]; cols[5] = cols[4];
			
			float yawDelta = p.HeadY - p.RotY;
			cosHead = (float)Math.Cos(yawDelta * Utils.Deg2Rad);
			sinHead = (float)Math.Sin(yawDelta * Utils.Deg2Rad);
		}
		
		/// <summary> Performs the actual rendering of an entity model. </summary>
		public abstract void DrawModel(Entity p);
		
		/// <summary> Sends the updated vertex data to the GPU. </summary>
		protected void UpdateVB() {
			ModelCache cache = game.ModelCache;
			game.Graphics.UpdateDynamicIndexedVb(
				DrawMode.Triangles, cache.vb, cache.vertices, index);
			index = 0;
		}
		
		
		protected internal virtual Matrix4 TransformMatrix(Entity p, Vector3 pos) {
			return p.TransformMatrix(p.ModelScale, pos);
		}
		
		protected int[] cols = new int[6];
		protected internal ModelVertex[] vertices;
		protected internal int index, texIndex;
		
		protected int GetTexture(int pTex) {
			return pTex > 0 ? pTex : game.ModelCache.Textures[texIndex].TexID;
		}
		
		
		protected BoxDesc MakeBoxBounds(int x1, int y1, int z1, int x2, int y2, int z2) {
			return ModelBuilder.MakeBoxBounds(x1, y1, z1, x2, y2, z2);
		}
		
		protected BoxDesc MakeRotatedBoxBounds(int x1, int y1, int z1, int x2, int y2, int z2) {
			return ModelBuilder.MakeRotatedBoxBounds(x1, y1, z1, x2, y2, z2);
		}
		
		protected ModelPart BuildBox(BoxDesc desc) {
			return ModelBuilder.BuildBox(this, desc);
		}
		
		protected ModelPart BuildRotatedBox(BoxDesc desc) {
			return ModelBuilder.BuildRotatedBox(this, desc);
		}
		
		protected void DrawPart(ModelPart part) {
			VertexP3fT2fC4b vertex = default(VertexP3fT2fC4b);
			VertexP3fT2fC4b[] finVertices = game.ModelCache.vertices;
			
			for (int i = 0; i < part.Count; i++) {
				ModelVertex v = vertices[part.Offset + i];
				vertex.X = v.X; vertex.Y = v.Y; vertex.Z = v.Z;
				vertex.Colour = cols[i >> 2];
				
				vertex.U = v.U * uScale; vertex.V = v.V * vScale;
				int quadI = i & 3;
				if (quadI == 0 || quadI == 3) vertex.V -= 0.01f * vScale;
				if (quadI == 2 || quadI == 3) vertex.U -= 0.01f * uScale;
				finVertices[index++] = vertex;
			}
		}
		
		protected void DrawRotate(float angleX, float angleY, float angleZ, ModelPart part, bool head) {
			float cosX = (float)Math.Cos(-angleX), sinX = (float)Math.Sin(-angleX);
			float cosY = (float)Math.Cos(-angleY), sinY = (float)Math.Sin(-angleY);
			float cosZ = (float)Math.Cos(-angleZ), sinZ = (float)Math.Sin(-angleZ);
			float x = part.RotX, y = part.RotY, z = part.RotZ;
			VertexP3fT2fC4b vertex = default(VertexP3fT2fC4b);
			VertexP3fT2fC4b[] finVertices = game.ModelCache.vertices;
			
			for (int i = 0; i < part.Count; i++) {
				ModelVertex v = vertices[part.Offset + i];
				v.X -= x; v.Y -= y; v.Z -= z;
				float t = 0;
				
				// Rotate locally
				if (Rotate == RotateOrder.ZYX) {
					t = cosZ * v.X + sinZ * v.Y; v.Y = -sinZ * v.X + cosZ * v.Y; v.X = t; // Inlined RotZ
					t = cosY * v.X - sinY * v.Z; v.Z  = sinY * v.X + cosY * v.Z; v.X = t; // Inlined RotY
					t = cosX * v.Y + sinX * v.Z; v.Z = -sinX * v.Y + cosX * v.Z; v.Y = t; // Inlined RotX
				} else if (Rotate == RotateOrder.XZY) {
					t = cosX * v.Y + sinX * v.Z; v.Z = -sinX * v.Y + cosX * v.Z; v.Y = t; // Inlined RotX
					t = cosZ * v.X + sinZ * v.Y; v.Y = -sinZ * v.X + cosZ * v.Y; v.X = t; // Inlined RotZ
					t = cosY * v.X - sinY * v.Z; v.Z =  sinY * v.X + cosY * v.Z; v.X = t; // Inlined RotY
				}
				
				// Rotate globally
				if (head) {
					t = cosHead * v.X - sinHead * v.Z; v.Z = sinHead * v.X + cosHead * v.Z; v.X = t; // Inlined RotY
				}
				vertex.X = v.X + x; vertex.Y = v.Y + y; vertex.Z = v.Z + z;
				vertex.Colour = cols[i >> 2];
				
				vertex.U = v.U * uScale; vertex.V = v.V * vScale;
				int quadI = i & 3;
				if (quadI == 0 || quadI == 3) vertex.V -= 0.01f * vScale;
				if (quadI == 2 || quadI == 3) vertex.U -= 0.01f * uScale;
				finVertices[index++] = vertex;
			}
		}
		
		protected enum RotateOrder { ZYX, XZY }
	}
}