// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Physics;
using OpenTK;

namespace ClassicalSharp.Model {

	public class CreeperModel : IModel {
		
		public CreeperModel(Game window) : base(window) { SurivalScore = 200; }

		/// <inheritdoc/>		
		public override void CreateParts() {
			vertices = new ModelVertex[boxVertices * 6];
			Head =  BuildBox(MakeBoxBounds(-4, 18, -4, 4, 26, 4)
			                 .TexOrigin(0, 0)
			                 .RotOrigin(0, 18, 0));
			Torso = BuildBox(MakeBoxBounds(-4, 6, -2, 4, 18, 2)
			                 .TexOrigin(16, 16));
			
			LeftLegFront = BuildBox(MakeBoxBounds(-4, 0, -6, 0, 6, -2)
			                        .TexOrigin(0, 16)
			                        .RotOrigin(0, 6, -2));
			RightLegFront = BuildBox(MakeBoxBounds(0, 0, -6, 4, 6, -2)
			                         .TexOrigin(0, 16)
			                         .RotOrigin(0, 6, -2));
			LeftLegBack = BuildBox(MakeBoxBounds(-4, 0, 2, 0, 6, 6)
			                       .TexOrigin(0, 16)
			                       .RotOrigin(0, 6, 2));
			RightLegBack = BuildBox(MakeBoxBounds(0, 0, 2, 4, 6, 6)
			                        .TexOrigin(0, 16)
			                        .RotOrigin(0, 6, 2));
		}
			
		public override float NameYOffset { get { return 1.7f; } }
	
		public override float GetEyeY(Entity entity) { return 22/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3(8/16f, 26/16f, 8/16f); }
		}

		public override AABB PickingBounds {
			get { return new AABB(-4/16f, 0, -6/16f, 4/16f, 26/16f, 6/16f); }
		}

		public override void DrawModel(Entity p) {
			game.Graphics.BindTexture(GetTexture(p));
			DrawRotate(-p.HeadXRadians, 0, 0, Head, true);

			DrawPart(Torso);
			DrawRotate(p.anim.leftLegX, 0, 0, LeftLegFront, false);
			DrawRotate(p.anim.rightLegX, 0, 0, RightLegFront, false);
			DrawRotate(p.anim.rightLegX, 0, 0, LeftLegBack, false);
			DrawRotate(p.anim.leftLegX, 0, 0, RightLegBack, false);
			UpdateVB();
		}
		
		ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
	}
}