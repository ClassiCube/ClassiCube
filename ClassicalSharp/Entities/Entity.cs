// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	/// <summary> Contains a model, along with position, velocity, and rotation.
	/// May also contain other fields and properties. </summary>
	public abstract partial class Entity {
		
		public Entity(Game game) {
			this.game = game;
			SkinType = game.DefaultPlayerSkinType;
			anim = new AnimatedComponent(game, this);
		}
		
		public IModel Model;
		public string ModelName;
		public float ModelScale = 1;
		public byte ID;
		public int TextureId = -1, MobTextureId = -1;
		public short Health = 20;
		
		public Vector3 Position;
		public Vector3 Velocity;
		public Vector3 OldVelocity;
		public float HeadX, HeadY, RotX, RotY, RotZ;
		
		protected Game game;
		protected internal bool onGround;
		internal float StepSize;
		internal int tickCount;
		
		public SkinType SkinType;
		public AnimatedComponent anim;
		internal float uScale = 1, vScale = 1;
		protected DateTime lastModelChange = new DateTime(1, 1, 1);
		
		
		/// <summary> Rotation of the entity's head horizontally. (i.e. looking north or east) </summary>
		public float HeadYRadians {
			get { return HeadY * Utils.Deg2Rad; }
			set { HeadY = value * Utils.Rad2Deg; }
		}
		
		/// <summary> Rotation of the entity's head vertically. (i.e. looking up or down) </summary>
		public float HeadXRadians {
			get { return HeadX * Utils.Deg2Rad; }
			set { HeadX = value * Utils.Rad2Deg; }
		}
		
		/// <summary> Returns the size of the model that is used for collision detection. </summary>
		public Vector3 Size;
		
		void UpdateModel() {
			BlockModel model = Model as BlockModel;
			if (model != null)
				model.CalcState(Utils.FastByte(ModelName));
		}
		
		public abstract void Tick(double delta);
		
		public abstract void SetLocation(LocationUpdate update, bool interpolate);
		
		public abstract void Despawn();
		
		/// <summary> Renders the entity's model, interpolating between the previous and next state. </summary>
		public abstract void RenderModel(double deltaTime, float t);
		
		/// <summary> Renders the entity's name over the top of its model. </summary>
		/// <remarks> Assumes that RenderModel was previously called this frame. </remarks>
		public abstract void RenderName();
		
		/// <summary> Gets the position of the player's eye in the world. </summary>
		public Vector3 EyePosition {
			get { return new Vector3(Position.X,
			                         Position.Y + Model.GetEyeY(this) * ModelScale, Position.Z); }
		}

		/// <summary> Gets the block just underneath the player's feet position. </summary>
		public byte BlockUnderFeet {
			get { return GetBlock(new Vector3(Position.X, Position.Y - 0.01f, Position.Z)); }
		}
		
		/// <summary> Gets the block at player's eye position. </summary>
		public byte BlockAtHead {
			get { return GetBlock(EyePosition); }
		}
		
		protected byte GetBlock(Vector3 coords) {
			return game.World.SafeGetBlock(Vector3I.Floor(coords));
		}
		
		internal Matrix4 TransformMatrix(float scale, Vector3 pos) {
			return 
				Matrix4.RotateZ(-RotZ * Utils.Deg2Rad) * 
				Matrix4.RotateX(-RotX * Utils.Deg2Rad) * 
				Matrix4.RotateY(-RotY * Utils.Deg2Rad) * 
				Matrix4.Scale(scale)                   * 
				Matrix4.Translate(pos.X, pos.Y, pos.Z);
		}
		
		
		public void SetModel(string model) {
			ModelScale = 1;
			int sep = model.IndexOf('|');
			string scale = sep == -1 ? null : model.Substring(sep + 1);
			ModelName = sep == -1 ? model : model.Substring(0, sep);
			
			if (Utils.CaselessEquals(model, "giant")) {
				ModelName = "humanoid";
				ModelScale *= 2;
			}
			
			Model = game.ModelCache.Get(ModelName);
			ParseScale(scale);
			lastModelChange = DateTime.UtcNow;
			MobTextureId = -1;
			
			UpdateModel();
			Size = Model.CollisionSize * ModelScale;
			modelAABB = Model.PickingBounds.Scale(ModelScale);
		}
		
		void ParseScale(string scale) {
			if (scale == null) return;
			float value;
			if (!Utils.TryParseDecimal(scale, out value)) return;
			
			Utils.Clamp(ref value, 0.25f, Model.MaxScale);
			ModelScale = value;
		}
	}
}