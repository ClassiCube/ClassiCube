// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Physics;
using OpenTK;

namespace ClassicalSharp.Model {

	public class PigModel : IModel {
		
		public PigModel(Game window) : base(window) { SurivalScore = 10; }

		/// <inheritdoc/>		
		public override void CreateParts() {
			vertices = new ModelVertex[boxVertices * 6];
			Head = BuildBox(MakeBoxBounds(-4, 8, -14, 4, 16, -6)
			               .TexOrigin(0, 0)
			               .RotOrigin(0, 12, -6));
			Torso = BuildRotatedBox(MakeRotatedBoxBounds(-5, 6, -8, 5, 14, 8)
			                        .TexOrigin(28, 8));
			LeftLegFront = BuildBox(MakeBoxBounds(-5, 0, -7, -1, 6, -3)
			                        .TexOrigin(0, 16)
			                        .RotOrigin(0, 6, -5));
			RightLegFront = BuildBox(MakeBoxBounds(1, 0, -7, 5, 6, -3)
			                         .TexOrigin(0, 16)
			                         .RotOrigin(0, 6, -5));
			LeftLegBack = BuildBox(MakeBoxBounds(-5, 0, 5, -1, 6, 9)
			                       .TexOrigin(0, 16)
			                       .RotOrigin(0, 6, 7));
			RightLegBack = BuildBox(MakeBoxBounds(1, 0, 5, 5, 6, 9)
			                        .TexOrigin(0, 16)
			                        .RotOrigin(0, 6, 7));
		}
		
		/// <inheritdoc/>
		public override float NameYOffset { get { return 1.075f; } }

		/// <inheritdoc/>
		public override float GetEyeY(Entity entity) { return 12/16f; }

		/// <inheritdoc/>
		public override Vector3 CollisionSize {
			get { return new Vector3(14/16f, 14/16f, 14/16f); }
		}

		/// <inheritdoc/>
		public override AABB PickingBounds {
			get { return new AABB(-5/16f, 0, -14/16f, 5/16f, 16/16f, 9/16f); }
		}

		/// <inheritdoc/>
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