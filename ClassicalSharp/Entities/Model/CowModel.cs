// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3

using System;

using ClassicalSharp.Entities;

using ClassicalSharp.Physics;

using OpenTK;



namespace ClassicalSharp.Model {



	public class CowModel : IModel {

		

		public CowModel(Game window) : base(window) { SurivalScore = 10; }



		/// <inheritdoc/>		

		public override void CreateParts() {

			vertices = new ModelVertex[boxVertices * 9];

			Head = BuildBox(MakeBoxBounds(-4, 16, -14, 4, 24, -8)

			               .TexOrigin(0, 0)

			               .RotOrigin(0, 20, -6));

			RightHorn = BuildBox(MakeBoxBounds(-5, 22, -13, -4, 25, -12)

			               .TexOrigin(22, 0)

			               .RotOrigin(0, 20, -6));

			LeftHorn = BuildBox(MakeBoxBounds(4, 22, -13, 5, 25, -12)

			               .TexOrigin(22, 0)

			               .RotOrigin(0, 20, -6));

			Torso = BuildRotatedBox(MakeRotatedBoxBounds(-6, 12, -8, 6, 22, 10)

			                        .TexOrigin(18, 4));
			
			Udder = BuildRotatedBox(MakeRotatedBoxBounds(-2, 11, 4, 2, 12, 10)

			                        .TexOrigin(52, 0));

			LeftLegFront = BuildBox(MakeBoxBounds(-6, 0, -8, -2, 12, -4)

			                        .TexOrigin(0, 16)

			                        .RotOrigin(0, 12, -5));

			RightLegFront = BuildBox(MakeBoxBounds(2, 0, -8, 6, 12, -4)

			                         .TexOrigin(0, 16)

			                         .RotOrigin(0, 12, -5));

			LeftLegBack = BuildBox(MakeBoxBounds(-6, 0, 5, -2, 12, 9)

			                       .TexOrigin(0, 16)

			                       .RotOrigin(0, 12, 7));

			RightLegBack = BuildBox(MakeBoxBounds(2, 0, 5, 6, 12, 9)

			                        .TexOrigin(0, 16)

			                        .RotOrigin(0, 12, 7));

		}

		

		/// <inheritdoc/>

		public override float NameYOffset { get { return 1.5f; } }



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

			DrawRotate(-p.HeadXRadians, 0, 0, LeftHorn, true);

			DrawRotate(-p.HeadXRadians, 0, 0, RightHorn, true);

			DrawPart(Torso);

			DrawPart(Udder);

			DrawRotate(p.anim.leftLegX, 0, 0, LeftLegFront, false);

			DrawRotate(p.anim.rightLegX, 0, 0, RightLegFront, false);

			DrawRotate(p.anim.rightLegX, 0, 0, LeftLegBack, false);

			DrawRotate(p.anim.leftLegX, 0, 0, RightLegBack, false);

			UpdateVB();

		}

		

		ModelPart Head, RightHorn, LeftHorn, Torso, Udder, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;

	}

}
