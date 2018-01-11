// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Physics;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ChibiModel : HumanoidModel {
		
		public ChibiModel(Game window) : base(window) {
			MaxScale = 3.0f;
			ShadowScale = 0.5f;
		}
		
		const float size = 0.5f;
		protected override void MakeDescriptions() {
			base.MakeDescriptions();
			head = MakeBoxBounds(-4, 12, -4, 4, 20, 4).RotOrigin(0, 13, 0);
			torso = torso.Scale(size);
			lLeg = lLeg.Scale(size);
			rLeg = rLeg.Scale(size);
			lArm = lArm.Scale(size);
			rArm = rArm.Scale(size);
			offset = 0.5f * size;
		}
		
		public override float NameYOffset { get { return 20.2f/16; } }
		
		public override float GetEyeY(Entity entity) { return 14/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3(4/16f + 0.6f/16f, 20.1f/16f, 4/16f + 0.6f/16f); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB(-4/16f, 0, -4/16f, 4/16f, 16/16f, 4/16f); }
		}
	}
	
	public class SittingModel : IModel {
		
		public SittingModel(Game window) : base(window) {
			CalcHumanAnims = true;
			UsesHumanSkin = true;
			ShadowScale = 0.5f;
		}
		
		const int sitOffset = 10;
		public override void CreateParts() { }
		
		public override float NameYOffset { get { return 32/16f + 0.5f/16f; } }
		
		public override float GetEyeY(Entity entity) { return (26 - sitOffset)/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3(8/16f + 0.6f/16f, (28.1f - sitOffset)/16f, 8/16f + 0.6f/16f); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB(-8/16f, 0, -4/16f, 8/16f, (32 - sitOffset)/16f, 4/16f); }
		}
		
		protected internal override Matrix4 TransformMatrix(Entity p, Vector3 pos) {
			pos.Y -= (sitOffset / 16f) * p.ModelScale.Y;
			return p.TransformMatrix(p.ModelScale, pos);
		}
		
		public override void DrawModel(Entity p) {
			p.anim.leftLegX = 1.5f; p.anim.rightLegX = 1.5f;
			p.anim.leftLegZ = -0.1f; p.anim.rightLegZ = 0.1f;
			
			IModel model = game.ModelCache.Models[0].Instance;
			model.SetupState(p);
			model.DrawModel(p);
		}
	}

	public class HeadModel : HumanoidModel {
		
		public HeadModel(Game window) : base(window) { Pushes = false; }
		public override void CreateParts() { }
		
		public override float GetEyeY(Entity entity) { return 6/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3(7.9f/16f, 7.9f/16f, 7.9f/16f); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB(-4/16f, 0, -4/16f, 4/16f, 8/16f, 4/16f); }
		}
		
		protected internal override Matrix4 TransformMatrix(Entity p, Vector3 pos) {
			pos.Y -= (24 / 16f) * p.ModelScale.Y;
			return p.TransformMatrix(p.ModelScale, pos);
		}

		protected override void RenderParts(Entity p) {
			HumanoidModel human = (HumanoidModel)game.ModelCache.Models[0].Instance;
			vertices = human.vertices;
			
			ModelPart part = human.Set.Head; part.RotY += 4 / 16f;
			DrawRotate(-p.HeadXRadians, 0, 0, part, true);
			UpdateVB();
			
			game.Graphics.AlphaTest = true;
			index = 0;
			part = human.Set.Hat; part.RotY += 4 / 16f;
			DrawRotate(-p.HeadXRadians, 0, 0, part, true);
			UpdateVB();
		}
	}

	public class ArmModel : HumanoidModel {
		
		Matrix4 translate;
		bool classicArms;
		public ArmModel(Game window) : base(window) { Pushes = false; }
		
		public override void CreateParts() {
			classicArms = game.ClassicArmModel;
			SetTranslationMatrix();
		}
		
		void SetTranslationMatrix() {
			if (game.ClassicArmModel) {
				// TODO: Position's not quite right.
				// Matrix4.Translate(out m, -6 / 16f + 0.2f, -12 / 16f - 0.20f, 0);
				// is better, but that breaks the dig animation
				Matrix4.Translate(out translate, -6 / 16f,         -12 / 16f - 0.10f, 0);
			} else {
				Matrix4.Translate(out translate, -6 / 16f + 0.10f, -12 / 16f - 0.26f, 0);
			}
		}

		public override float NameYOffset { get { return 0.5f; } }
		public override float GetEyeY(Entity entity) { return 0.5f; }
		public override Vector3 CollisionSize { get { return Vector3.One; } }
		public override AABB PickingBounds { get { return new AABB(-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f); } }
		
		protected override void RenderParts(Entity p) {
			HumanoidModel human = (HumanoidModel)game.ModelCache.Models[0].Instance;
			vertices = human.vertices;
			// If user changes option while game is running
			if (classicArms != game.ClassicArmModel) {
				classicArms = game.ClassicArmModel;
				SetTranslationMatrix();
			}

			Matrix4 m;
			Matrix4.Mult(out m, ref p.transform, ref game.Graphics.View);
			Matrix4.Mult(out m, ref translate, ref m);
			game.Graphics.LoadMatrix(ref m);
			
			SkinType skinType = p.SkinType;
			ModelSet model = skinType == SkinType.Type64x64Slim ? human.SetSlim :
				(skinType == SkinType.Type64x64 ? human.Set64 : human.Set);
			
			Rotate = RotateOrder.YZX;
			DrawArmPart(model.RightArm);
			UpdateVB();
			
			if (skinType != SkinType.Type64x32) {
				index = 0;
				game.Graphics.AlphaTest = true;
				DrawArmPart(model.RightArmLayer);
				UpdateVB();
				game.Graphics.AlphaTest = false;
			}
			Rotate = RotateOrder.ZYX;
		}
		
		void DrawArmPart(ModelPart part) {
			part.RotX += 1 / 16.0f; part.RotY -= 4 / 16.0f;
			if (game.ClassicArmModel) {
				DrawRotate(0, -90 * Utils.Deg2Rad, 120 * Utils.Deg2Rad, part, false);
			} else {
				DrawRotate(-20 * Utils.Deg2Rad, -70 * Utils.Deg2Rad, 135 * Utils.Deg2Rad, part, false);
			}
		}
	}
	
	public class CorpseModel : IModel {
		
		public CorpseModel(Game window) : base(window) {
			UsesHumanSkin = true;
		}
		
		public override void CreateParts() { }
		
		public override float NameYOffset { get { return 32/16f + 0.5f/16f; } }
		
		public override float GetEyeY(Entity entity) { return 26/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3(8/16f + 0.6f/16f, 28.1f/16f, 8/16f + 0.6f/16f); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB(-8/16f, 0, -4/16f, 8/16f, 32/16f, 4/16f); }
		}
		
		public override void DrawModel(Entity p) {
			p.anim.leftLegX = 0.025f; p.anim.rightLegX = 0.025f;
			p.anim.leftArmX = 0.025f; p.anim.rightArmX = 0.025f;
			p.anim.leftLegZ = -0.15f; p.anim.rightLegZ =  0.15f;
			p.anim.leftArmZ = -0.20f; p.anim.rightArmZ =  0.20f;
			
			IModel model = game.ModelCache.Models[0].Instance;
			model.SetupState(p);
			model.DrawModel(p);
		}
	}
}
