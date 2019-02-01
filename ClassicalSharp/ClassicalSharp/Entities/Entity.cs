// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Model;
using ClassicalSharp.Physics;
using OpenTK;
using BlockID = System.UInt16;

namespace ClassicalSharp.Entities {
	
	/// <summary> Contains a model, along with position, velocity, and rotation.
	/// May also contain other fields and properties. </summary>
	public abstract class Entity {
		
		public Entity(Game game) {
			this.game = game;
			anim = new AnimatedComponent(game, this);
		}
		
		public IModel Model;
		public string SkinName, ModelName;
		public BlockID ModelBlock;
		public Vector3 ModelScale = new Vector3(1.0f);
		public Vector3 Size;

		public int TextureId, MobTextureId;
		public short Health = 10;
		
		public Vector3 Position;
		public Vector3 Velocity;
		public float HeadX, HeadY, RotX, RotY, RotZ;
		
		protected Game game;
		protected internal bool onGround;
		internal float StepSize;
		internal int tickCount;
		internal Matrix4 transform;
		
		public SkinType SkinType;
		public AnimatedComponent anim;
		public float uScale = 1, vScale = 1;
		public bool NoShade;
		
		
		/// <summary> Rotation of the entity's head horizontally. (i.e. looking north or east) </summary>
		public float HeadYRadians { get { return HeadY * Utils.Deg2Rad; } }
		
		/// <summary> Rotation of the entity's head vertically. (i.e. looking up or down) </summary>
		public float HeadXRadians { get { return HeadX * Utils.Deg2Rad; } }
		
		public abstract void Tick(double delta);
		
		public abstract void SetLocation(LocationUpdate update, bool interpolate);
		
		public abstract void Despawn();
		
		/// <summary> Renders the entity's model, interpolating between the previous and next state. </summary>
		public abstract void RenderModel(double deltaTime, float t);
		
		/// <summary> Renders the entity's name over the top of its model. </summary>
		/// <remarks> Assumes that RenderModel was previously called this frame. </remarks>
		public abstract void RenderName();
		
		public virtual void ContextLost() { }
		public virtual void ContextRecreated() { }
		
		public Vector3 EyePosition {
			get { return new Vector3(Position.X, Position.Y + EyeHeight, Position.Z); }
		}
		public float EyeHeight { get { return Model.GetEyeY(this) * ModelScale.Y; } }
		
		public Matrix4 TransformMatrix(Vector3 scale, Vector3 pos) {
			Matrix4 m = Matrix4.Identity, tmp;

			Matrix4.Scale(out tmp, scale.X, scale.Y, scale.Z);
			Matrix4.Mult(out m, ref m, ref tmp);
			Matrix4.RotateZ(out tmp, -RotZ * Utils.Deg2Rad);
			Matrix4.Mult(out m, ref m, ref tmp);
			Matrix4.RotateX(out tmp, -RotX * Utils.Deg2Rad);
			Matrix4.Mult(out m, ref m, ref tmp);
			Matrix4.RotateY(out tmp, -RotY * Utils.Deg2Rad);
			Matrix4.Mult(out m, ref m, ref tmp);
			Matrix4.Translate(out tmp, pos.X, pos.Y, pos.Z);
			Matrix4.Mult(out m, ref m, ref tmp);
			
			//return rotZ * rotX * rotY * scale * translate;
			return m;
		}
		
		public virtual PackedCol Colour() {
			Vector3I P = Vector3I.Floor(EyePosition);
			return game.World.IsValidPos(P) ? game.Lighting.LightCol(P.X, P.Y, P.Z) : game.Lighting.Outside;
		}
		
		void SetBlockModel() {
			int raw = BlockInfo.Parse(ModelName);
			if (raw == -1) {
				// use default humanoid model
				Model = game.ModelCache.Models[0].Instance;
			} else {
				ModelName  = "block";
				ModelBlock = (BlockID)raw;
				Model = game.ModelCache.Get("block");
			}
		}
		
		public void SetModel(string model) {
			ModelScale = new Vector3(1.0f);
			int sep = model.IndexOf('|');
			string scale = sep == -1 ? null : model.Substring(sep + 1);
			ModelName = sep == -1 ? model : model.Substring(0, sep);
			
			if (Utils.CaselessEq(model, "giant")) {
				ModelName = "humanoid";
				ModelScale *= 2;
			}
			
			ModelBlock = Block.Air;
			Model = game.ModelCache.Get(ModelName);
			MobTextureId = 0;		
			if (Model == null) SetBlockModel();
			
			ParseScale(scale);
			Model.RecalcProperties(this);
			UpdateModelBounds();
			
			if (SkinName != null && Utils.IsUrlPrefix(SkinName, 0)) {
				MobTextureId = TextureId;
			}
		}
		
		public void UpdateModelBounds() {
			Size = Model.CollisionSize * ModelScale;
			modelAABB = Model.PickingBounds;
			modelAABB.Min *= ModelScale;
			modelAABB.Max *= ModelScale;
		}
		
		void ParseScale(string scale) {
			if (scale == null) return;
			float value;
			if (!Utils.TryParseDecimal(scale, out value)) return;
			
			Utils.Clamp(ref value, 0.01f, Model.MaxScale);
			ModelScale = new Vector3(value);
		}
		
		
		internal AABB modelAABB;
		
		/// <summary> Returns the bounding box that contains the model, without any rotations applied. </summary>
		public AABB PickingBounds { get { return modelAABB.Offset(Position); } }
		
		/// <summary> Bounding box of the model that collision detection is performed with, in world coordinates. </summary>
		public AABB Bounds { get { return AABB.Make(Position, Size); } }
		
		/// <summary> Constant offset used to avoid floating point roundoff errors. </summary>
		public const float Adjustment = 0.001f;
		
		/// <summary> Determines whether any of the blocks that intersect the
		/// given bounding box satisfy the given condition. </summary>
		public bool TouchesAny(AABB bounds, Predicate<BlockID> condition) {
			Vector3I min = Vector3I.Floor(bounds.Min);
			Vector3I max = Vector3I.Floor(bounds.Max);

			AABB blockBB;
			Vector3 v;
			min.X = min.X < 0 ? 0 : min.X; max.X = max.X > game.World.MaxX ? game.World.MaxX : max.X;
			min.Y = min.Y < 0 ? 0 : min.Y; max.Y = max.Y > game.World.MaxY ? game.World.MaxY : max.Y;
			min.Z = min.Z < 0 ? 0 : min.Z; max.Z = max.Z > game.World.MaxZ ? game.World.MaxZ : max.Z;
			
			for (int y = min.Y; y <= max.Y; y++) {
				v.Y = y;
				for (int z = min.Z; z <= max.Z; z++) {
					v.Z = z;
					for (int x = min.X; x <= max.X; x++) {
						v.X = x;
						
						BlockID block = game.World.GetBlock(x, y, z);
						blockBB.Min = v + BlockInfo.MinBB[block];
						blockBB.Max = v + BlockInfo.MaxBB[block];
						
						if (!blockBB.Intersects(bounds)) continue;
						if (condition(block)) return true;
					}
				}
			}
			return false;
		}
		
		/// <summary> Determines whether any of the blocks that intersect the
		/// given bounding box satisfy the given condition. </summary>
		public bool TouchesAny(AABB bounds, Predicate<byte> condition) {
			return TouchesAny(bounds, delegate(BlockID bl) { return condition((byte)bl); });
		}
		
		/// <summary> Determines whether any of the blocks that intersect the
		/// bounding box of this entity are rope. </summary>
		public bool TouchesAnyRope() {
			AABB bounds = Bounds;
			bounds.Max.Y += 0.5f/16f;
			return TouchesAny(bounds, touchesRope);
		}
		static Predicate<BlockID> touchesRope = IsRope;
		static bool IsRope(BlockID b) { return BlockInfo.ExtendedCollide[b] == CollideType.ClimbRope; }
		
		
		static readonly Vector3 liqExpand = new Vector3(0.25f/16f, 0/16f, 0.25f/16f);
		
		/// <summary> Determines whether any of the blocks that intersect the
		/// bounding box of this entity are lava or still lava. </summary>
		public bool TouchesAnyLava() {
			// NOTE: Original classic client uses offset (so you can only climb up
			// alternating liquid-solid elevators on two sides)
			AABB bounds = Bounds.Offset(liqExpand);
			return TouchesAny(bounds, touchesAnyLava);
		}
		static Predicate<BlockID> touchesAnyLava = IsLava;
		static bool IsLava(BlockID b) { return BlockInfo.ExtendedCollide[b] == CollideType.LiquidLava; }

		/// <summary> Determines whether any of the blocks that intersect the
		/// bounding box of this entity are water or still water. </summary>
		public bool TouchesAnyWater() {
			AABB bounds = Bounds.Offset(liqExpand);
			return TouchesAny(bounds, touchesAnyWater);
		}
		static Predicate<BlockID> touchesAnyWater = IsWater;
		static bool IsWater(BlockID b) { return BlockInfo.ExtendedCollide[b] == CollideType.LiquidWater; }
	}
}