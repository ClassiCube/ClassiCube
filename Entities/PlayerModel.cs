using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.Entities {

	public class PlayerModel {
		
		public ModelSet Set64x32, Set64x64, Set64x64Slim;
		static readonly FastColour col = new FastColour( 178, 178, 178 );
		
		public PlayerModel( IGraphicsApi graphics ) {
			VertexPos3fTex2fCol4b[] buffer = new VertexPos3fTex2fCol4b[6 * 6];
			int index = 0;
			Set64x32 = new ModelSet();
			Make64x32Parts( ref index, buffer, graphics );
			index = 0;
			Set64x64 = new ModelSet();
			Make64x64Parts( ref index, buffer, graphics );
			index = 0;
			Set64x64Slim = new ModelSet();
			Make64x64SlimParts( ref index, buffer, graphics );
		}
		
		void Make64x32Parts( ref int index, VertexPos3fTex2fCol4b[] buffer, IGraphicsApi graphics ) {
			// Head
			YPlane64x32( 8, 0, 8, 8, 0.25f, -0.25f, 0.25f, -0.25f, 2.125f, ref index, buffer ); // head top
			YPlane64x32( 16, 0, 8, 8, 0.25f, -0.25f, -0.25f, 0.25f, 1.625f, ref index, buffer ); // head bottom
			ZPlane64x32( 8, 8, 8, 8, 0.25f, -0.25f, 1.625f, 2.125f, -0.25f, ref index, buffer ); // head front
			ZPlane64x32( 24, 8, 8, 8, -0.25f, 0.25f, 1.625f, 2.125f, 0.25f, ref index, buffer ); // head back
			XPlane64x32( 16, 8, 8, 8, -0.25f, 0.25f, 1.625f, 2.125f, -0.25f, ref index, buffer ); // head left
			XPlane64x32( 0, 8, 8, 8, 0.25f, -0.25f, 1.625f, 2.125f, 0.25f, ref index, buffer ); // head right
			Set64x32.Head = new ModelPart( buffer, graphics );
			// Torso
			index = 0;
			YPlane64x32( 20, 16, 8, 4, 0.25f, -0.25f, 0.125f, -0.125f, 1.625f, ref index, buffer ); // torso top
			YPlane64x32( 28, 16, 8, 4, 0.25f, -0.25f, -0.125f, 0.125f, 0.875f, ref index, buffer ); // torso bottom
			ZPlane64x32( 20, 20, 8, 12, 0.25f, -0.25f, 0.875f, 1.625f, -0.125f, ref index, buffer ); // torso front
			ZPlane64x32( 32, 20, 8, 12, -0.25f, 0.25f, 0.875f, 1.625f, 0.125f, ref index, buffer ); // torso back
			XPlane64x32( 28, 20, 4, 12, -0.125f, 0.125f, 0.875f, 1.625f, -0.25f, ref index, buffer ); // torso left
			XPlane64x32( 16, 20, 4, 12, 0.125f, -0.125f, 0.875f, 1.625f, 0.25f, ref index, buffer ); // torso right
			Set64x32.Torso = new ModelPart( buffer, graphics );
			// Left leg
			index = 0;
			YPlane64x32( 4, 16, 4, 4, -0.25f, 0f, 0.125f, -0.125f, 0.875f, ref index, buffer ); // left leg top
			YPlane64x32( 8, 16, 4, 4, -0.25f, 0f, -0.125f, 0.125f, 0f, ref index, buffer ); // left leg bottom
			ZPlane64x32( 4, 20, 4, 12, -0.25f, 0f, 0f, 0.875f, -0.125f, ref index, buffer ); // left leg front
			ZPlane64x32( 12, 20, 4, 12, 0f, -0.25f, 0f, 0.875f, 0.125f, ref index, buffer ); // left leg back
			XPlane64x32( 0, 20, 4, 12, 0.125f, -0.125f, 0f, 0.875f, -0.25f, ref index, buffer ); // left leg left
			XPlane64x32( 8, 20, 4, 12, -0.125f, 0.125f, 0f, 0.875f, 0f, ref index, buffer ); // left leg right
			Set64x32.LeftLeg = new ModelPart( buffer, graphics );
			// Right leg
			index = 0;
			YPlane64x32( 4, 16, 4, 4, 0.25f, 0f, 0.125f, -0.125f, 0.875f, ref index, buffer ); // right leg top
			YPlane64x32( 8, 16, 4, 4, 0.25f, 0f, -0.125f, 0.125f, 0f, ref index, buffer ); // right leg bottom
			ZPlane64x32( 4, 20, 4, 12, 0.25f, 0f, 0f, 0.875f, -0.125f, ref index, buffer ); // right leg front
			ZPlane64x32( 12, 20, 4, 12, 0f, 0.25f, 0f, 0.875f, 0.125f, ref index, buffer ); // right leg back
			XPlane64x32( 8, 20, 4, 12, -0.125f, 0.125f, 0f, 0.875f, 0f, ref index, buffer ); // right leg left
			XPlane64x32( 0, 20, 4, 12, 0.125f, -0.125f, 0f, 0.875f, 0.25f, ref index, buffer ); // right leg right
			Set64x32.RightLeg = new ModelPart( buffer, graphics );
			// Left arm
			index = 0;
			YPlane64x32( 44, 16, 4, 4, -0.5f, -0.25f, 0.125f, -0.125f, 1.625f, ref index, buffer ); // left arm top
			YPlane64x32( 48, 16, 4, 4, -0.5f, -0.25f, -0.125f, 0.125f, 0.875f, ref index, buffer ); // left arm bottom
			ZPlane64x32( 44, 20, 4, 12, -0.5f, -0.25f, 0.875f, 1.625f, -0.125f, ref index, buffer ); // left arm front
			ZPlane64x32( 52, 20, 4, 12, -0.25f, -0.5f, 0.875f, 1.625f, 0.125f, ref index, buffer ); // left arm back
			XPlane64x32( 40, 20, 4, 12, 0.125f, -0.125f, 0.875f, 1.625f, -0.5f, ref index, buffer ); // left arm left
			XPlane64x32( 48, 20, 4, 12, -0.125f, 0.125f, 0.875f, 1.625f, -0.25f, ref index, buffer ); // left arm right
			Set64x32.LeftArm = new ModelPart( buffer, graphics );
			// Right arm
			index = 0;
			YPlane64x32( 44, 16, 4, 4, 0.5f, 0.25f, 0.125f, -0.125f, 1.625f, ref index, buffer ); // right arm top
			YPlane64x32( 48, 16, 4, 4, 0.5f, 0.25f, -0.125f, 0.125f, 0.875f, ref index, buffer ); // right arm bottom
			ZPlane64x32( 44, 20, 4, 12, 0.5f, 0.25f, 0.875f, 1.625f, -0.125f, ref index, buffer ); // right arm front
			ZPlane64x32( 52, 20, 4, 12, 0.25f, 0.5f, 0.875f, 1.625f, 0.125f, ref index, buffer ); // right arm back
			XPlane64x32( 48, 20, 4, 12, -0.125f, 0.125f, 0.875f, 1.625f, 0.25f, ref index, buffer ); // right arm left
			XPlane64x32( 40, 20, 4, 12, 0.125f, -0.125f, 0.875f, 1.625f, 0.5f, ref index, buffer ); // right arm right
			Set64x32.RightArm = new ModelPart( buffer, graphics );
			// Hat
			index = 0;
			YPlane64x32( 40, 0, 8, 8, 0.3125f, -0.3125f, 0.3125f, -0.3125f, 2.1875f, ref index, buffer ); // hat top
			YPlane64x32( 48, 0, 8, 8, 0.3125f, -0.3125f, -0.3125f, 0.3125f, 1.5625f, ref index, buffer ); // hat bottom
			ZPlane64x32( 40, 8, 8, 8, 0.3125f, -0.3125f, 1.5625f, 2.1875f, -0.3125f, ref index, buffer ); // hat front
			ZPlane64x32( 56, 8, 8, 8, -0.3125f, 0.3125f, 1.5625f, 2.1875f, 0.3125f, ref index, buffer ); // hat back
			XPlane64x32( 48, 8, 8, 8, -0.3125f, 0.3125f, 1.5625f, 2.1875f, -0.3125f, ref index, buffer ); // hat left
			XPlane64x32( 32, 8, 8, 8, 0.3125f, -0.3125f, 1.5625f, 2.1875f, 0.3125f, ref index, buffer ); // hat right
			Set64x32.Hat = new ModelPart( buffer, graphics );
		}
				
		void Make64x64Parts( ref int index, VertexPos3fTex2fCol4b[] buffer, IGraphicsApi graphics ) {
			// Head
			YPlane64x64( 8, 0, 8, 8, 0.25f, -0.25f, 0.25f, -0.25f, 2.125f, ref index, buffer ); // head top
			YPlane64x64( 16, 0, 8, 8, 0.25f, -0.25f, 00.25f, 0.25f, 1.625f, ref index, buffer ); // head bottom
			ZPlane64x64( 8, 8, 8, 8, 0.25f, -0.25f, 1.625f, 2.125f, -0.25f, ref index, buffer ); // head front
			ZPlane64x64( 24, 8, 8, 8, -0.25f, 0.25f, 1.625f, 2.125f, 0.25f, ref index, buffer ); // head back
			XPlane64x64( 16, 8, 8, 8, -0.25f, 0.25f, 1.625f, 2.125f, -0.25f, ref index, buffer ); // head left
			XPlane64x64( 0, 8, 8, 8, 0.25f, -0.25f, 1.625f, 2.125f, 0.25f, ref index, buffer ); // head right
			Set64x64.Head = new ModelPart( buffer, graphics );
			// Torso
			index = 0;
			YPlane64x64( 20, 16, 8, 4, 0.25f, -0.25f, 0.125f, -0.125f, 1.625f, ref index, buffer ); // torso top
			YPlane64x64( 28, 16, 8, 4, 0.25f, -0.25f, -0.125f, 0.125f, 0.875f, ref index, buffer ); // torso bottom
			ZPlane64x64( 20, 20, 8, 12, 0.25f, -0.25f, 0.875f, 1.625f, -0.125f, ref index, buffer ); // torso front
			ZPlane64x64( 32, 20, 8, 12, -0.25f, 0.25f, 0.875f, 1.625f, 0.125f, ref index, buffer ); // torso back
			XPlane64x64( 28, 20, 4, 12, -0.125f, 0.125f, 0.875f, 1.625f, -0.25f, ref index, buffer ); // torso left
			XPlane64x64( 16, 20, 4, 12, 0.125f, -0.125f, 0.875f, 1.625f, 0.25f, ref index, buffer ); // torso right
			Set64x64.Torso = new ModelPart( buffer, graphics );
			// Left leg
			index = 0;
			YPlane64x64( 20, 48, 4, 4, 0f, -0.25f, 0.125f, -0.125f, 0.875f, ref index, buffer ); // left leg top
			YPlane64x64( 24, 48, 4, 4, 0f, -0.25f, -0.125f, 0.125f, 0f, ref index, buffer ); // left leg bottom
			ZPlane64x64( 20, 52, 4, 12, 0f, -0.25f, 0f, 0.875f, -0.125f, ref index, buffer ); // left leg front
			ZPlane64x64( 28, 52, 4, 12, -0.25f, 0f, 0f, 0.875f, 0.125f, ref index, buffer ); // left leg back
			XPlane64x64( 24, 52, 4, 12, -0.125f, 0.125f, 0f, 0.875f, -0.25f, ref index, buffer ); // left leg left
			XPlane64x64( 16, 52, 4, 12, 0.125f, -0.125f, 0f, 0.875f, 0f, ref index, buffer ); // left leg right
			Set64x64.LeftLeg = new ModelPart( buffer, graphics );
			// Right leg
			index = 0;
			YPlane64x64( 4, 16, 4, 4, 0.25f, 0f, 0.125f, -0.125f, 0.875f, ref index, buffer ); // right leg top
			YPlane64x64( 8, 16, 4, 4, 0.25f, 0f, -0.125f, 0.125f, 0f, ref index, buffer ); // right leg bottom
			ZPlane64x64( 4, 20, 4, 12, 0.25f, 0f, 0f, 0.875f, -0.125f, ref index, buffer ); // right leg front
			ZPlane64x64( 12, 20, 4, 12, 0f, 0.25f, 0f, 0.875f, 0.125f, ref index, buffer ); // right leg back
			XPlane64x64( 8, 20, 4, 12, -0.125f, 0.125f, 0f, 0.875f, 0f, ref index, buffer ); // right leg left
			XPlane64x64( 0, 20, 4, 12, 0.125f, -0.125f, 0f, 0.875f, 0.25f, ref index, buffer ); // right leg right
			Set64x64.RightLeg = new ModelPart( buffer, graphics );
			// Left arm
			index = 0;
			YPlane64x64( 36, 48, 4, 4, -0.25f, -0.5f, 0.125f, -0.125f, 1.625f, ref index, buffer ); // left arm top
			YPlane64x64( 40, 48, 4, 4, -0.25f, -0.5f, -0.125f, 0.125f, 0.875f, ref index, buffer ); // left arm bottom
			ZPlane64x64( 36, 52, 4, 12, -0.25f, -0.5f, 0.875f, 1.625f, -0.125f, ref index, buffer ); // left arm front
			ZPlane64x64( 44, 52, 4, 12, -0.5f, -0.25f, 0.875f, 1.625f, 0.125f, ref index, buffer ); // left arm back
			XPlane64x64( 40, 52, 4, 12, -0.125f, 0.125f, 0.875f, 1.625f, -0.5f, ref index, buffer ); // left arm left 
			XPlane64x64( 32, 52, 4, 12, 0.125f, -0.125f, 0.875f, 1.625f, -0.25f, ref index, buffer ); // left arm right
			Set64x64.LeftArm = new ModelPart( buffer, graphics );
			// Right arm
			index = 0;
			YPlane64x64( 44, 16, 4, 4, 0.5f, 0.25f, 0.125f, -0.125f, 1.625f, ref index, buffer ); // right arm top
			YPlane64x64( 48, 16, 4, 4, 0.5f, 0.25f, -0.125f, 0.125f, 0.875f, ref index, buffer ); // right arm bottom
			ZPlane64x64( 44, 20, 4, 12, 0.5f, 0.25f, 0.875f, 1.625f, -0.125f, ref index, buffer ); // right arm front
			ZPlane64x64( 52, 20, 4, 12, 0.25f, 0.5f, 0.875f, 1.625f, 0.125f, ref index, buffer ); // right arm back
			XPlane64x64( 48, 20, 4, 12, -0.125f, 0.125f, 0.875f, 1.625f, 0.25f, ref index, buffer ); // right arm left
			XPlane64x64( 40, 20, 4, 12, 0.125f, -0.125f, 0.875f, 1.625f, 0.5f, ref index, buffer ); // right arm right
			Set64x64.RightArm = new ModelPart( buffer, graphics );
			// Hat
			index = 0;
			YPlane64x64( 40, 0, 8, 8, 0.3125f, -0.3125f, 0.3125f, -0.3125f, 2.1875f, ref index, buffer ); // hat top
			YPlane64x64( 48, 0, 8, 8, 0.3125f, -0.3125f, -0.3125f, 0.3125f, 1.5625f, ref index, buffer ); // hat bottom
			ZPlane64x64( 40, 8, 8, 8, 0.3125f, -0.3125f, 1.5625f, 2.1875f, -0.3125f, ref index, buffer ); // hat front
			ZPlane64x64( 56, 8, 8, 8, -0.3125f, 0.3125f, 1.5625f, 2.1875f, 0.3125f, ref index, buffer ); // hat back
			XPlane64x64( 48, 8, 8, 8, -0.3125f, 0.3125f, 1.5625f, 2.1875f, -0.3125f, ref index, buffer ); // hat left
			XPlane64x64( 32, 8, 8, 8, 0.3125f, -0.3125f, 1.5625f, 2.1875f, 0.3125f, ref index, buffer ); // hat right
			Set64x64.Hat = new ModelPart( buffer, graphics );
		}
		
		void Make64x64SlimParts( ref int index, VertexPos3fTex2fCol4b[] buffer, IGraphicsApi graphics ) {
			// Head
			YPlane64x64( 8, 0, 8, 8, 0.25f, -0.25f, 0.25f, -0.25f, 2.125f, ref index, buffer ); // head top
			YPlane64x64( 16, 0, 8, 8, 0.25f, -0.25f, -0.25f, 0.25f, 1.625f, ref index, buffer ); // head bottom
			ZPlane64x64( 8, 8, 8, 8, 0.25f, -0.25f, 1.625f, 2.125f, -0.25f, ref index, buffer ); // head front
			ZPlane64x64( 24, 8, 8, 8, -0.25f, 0.25f, 1.625f, 2.125f, 0.25f, ref index, buffer ); // head back
			XPlane64x64( 16, 8, 8, 8, -0.25f, 0.25f, 1.625f, 2.125f, -0.25f, ref index, buffer ); // head left
			XPlane64x64( 0, 8, 8, 8, 0.25f, -0.25f, 1.625f, 2.125f, 0.25f, ref index, buffer ); // head right
			Set64x64Slim.Head = new ModelPart( buffer, graphics );
			// Torso
			index = 0;
			YPlane64x64( 20, 16, 8, 4, 0.25f, -0.25f, 0.125f, -0.125f, 1.625f, ref index, buffer ); // torso top
			YPlane64x64( 28, 16, 8, 4, 0.25f, -0.25f, -0.125f, 0.125f, 0.875f, ref index, buffer ); // torso bottom
			ZPlane64x64( 20, 20, 8, 12, 0.25f, -0.25f, 0.875f, 1.625f, -0.125f, ref index, buffer ); // torso front
			ZPlane64x64( 32, 20, 8, 12, -0.25f, 0.25f, 0.875f, 1.625f, 0.125f, ref index, buffer ); // torso back
			XPlane64x64( 28, 20, 4, 12, -0.125f, 0.125f, 0.875f, 1.625f, -0.25f, ref index, buffer ); // torso left
			XPlane64x64( 16, 20, 4, 12, 0.125f, -0.125f, 0.875f, 1.625f, 0.25f, ref index, buffer ); // torso right
			Set64x64Slim.Torso = new ModelPart( buffer, graphics );
			// Left leg
			index = 0;
			YPlane64x64( 20, 48, 4, 4, 0f, -0.25f, 0.125f, -0.125f, 0.875f, ref index, buffer ); // left leg top
			YPlane64x64( 24, 48, 4, 4, 0f, -0.25f, -0.125f, 0.125f, 0f, ref index, buffer ); // left leg bottom
			ZPlane64x64( 20, 52, 4, 12, 0f, -0.25f, 0f, 0.875f, -0.125f, ref index, buffer ); // left leg front
			ZPlane64x64( 28, 52, 4, 12, -0.25f, 0f, 0f, 0.875f, 0.125f, ref index, buffer ); // left leg back
			XPlane64x64( 24, 52, 4, 12, -0.125f, 0.125f, 0f, 0.875f, -0.25f, ref index, buffer ); // left leg left
			XPlane64x64( 16, 52, 4, 12, 0.125f, -0.125f, 0f, 0.875f, 0f, ref index, buffer ); // left leg right
			Set64x64Slim.LeftLeg = new ModelPart( buffer, graphics );
			// Right leg
			index = 0;
			YPlane64x64( 4, 16, 4, 4, 0.25f, 0f, 0.125f, -0.125f, 0.875f, ref index, buffer ); // right leg top
			YPlane64x64( 8, 16, 4, 4, 0.25f, 0f, -0.125f, 0.125f, 0f, ref index, buffer ); // right leg bottom
			ZPlane64x64( 4, 20, 4, 12, 0.25f, 0f, 0f, 0.875f, -0.125f, ref index, buffer ); // right leg front
			ZPlane64x64( 12, 20, 4, 12, 0f, 0.25f, 0f, 0.875f, 0.125f, ref index, buffer ); // right leg back
			XPlane64x64( 8, 20, 4, 12, -0.125f, 0.125f, 0f, 0.875f, 0f, ref index, buffer ); // right leg left
			XPlane64x64( 0, 20, 4, 12, 0.125f, -0.125f, 0f, 0.875f, 0.25f, ref index, buffer ); // right leg right
			Set64x64Slim.RightLeg = new ModelPart( buffer, graphics );		
			// Left arm
			index = 0;
			YPlane64x64( 36, 48, 3, 4, -0.25f, -0.4375f, 0.125f, -0.125f, 1.625f, ref index, buffer ); // left arm top
			YPlane64x64( 39, 48, 3, 4, -0.25f, -0.4375f, -0.125f, 0.125f, 0.875f, ref index, buffer ); // left arm bottom
			ZPlane64x64( 36, 52, 3, 12, -0.25f, -0.4375f, 0.875f, 1.625f, -0.125f, ref index, buffer ); // left arm front
			ZPlane64x64( 43, 52, 3, 12, -0.4375f, -0.25f, 0.875f, 1.625f, 0.125f, ref index, buffer ); // left arm back
			XPlane64x64( 39, 52, 4, 12, -0.125f, 0.125f, 0.875f, 1.625f, -0.4375f, ref index, buffer ); // left arm left 
			XPlane64x64( 32, 52, 4, 12, 0.125f, -0.125f, 0.875f, 1.625f, -0.25f, ref index, buffer ); // left arm right
			Set64x64Slim.LeftArm = new ModelPart( buffer, graphics );
			// Right arm
			index = 0;
			YPlane64x64( 44, 16, 3, 4, 0.4375f, 0.25f, 0.125f, -0.125f, 1.625f, ref index, buffer ); // right arm top
			YPlane64x64( 47, 16, 3, 4, 0.4375f, 0.25f, -0.125f, 0.125f, 0.875f, ref index, buffer ); // right arm bottom
			ZPlane64x64( 44, 20, 3, 12, 0.4375f, 0.25f, 0.875f, 1.625f, -0.125f, ref index, buffer ); // right arm front
			ZPlane64x64( 51, 20, 3, 12, 0.25f, 0.4375f, 0.875f, 1.625f, 0.125f, ref index, buffer ); // right arm back
			XPlane64x64( 47, 20, 4, 12, -0.125f, 0.125f, 0.875f, 1.625f, 0.25f, ref index, buffer ); // right arm left
			XPlane64x64( 40, 20, 4, 12, 0.125f, -0.125f, 0.875f, 1.625f, 0.4375f, ref index, buffer ); // right arm right
			Set64x64Slim.RightArm = new ModelPart( buffer, graphics );
			// Hat
			index = 0;
			YPlane64x64( 40, 0, 8, 8, 0.3125f, -0.3125f, 0.3125f, -0.3125f, 2.1875f, ref index, buffer ); // hat top
			YPlane64x64( 48, 0, 8, 8, 0.3125f, -0.3125f, -0.3125f, 0.3125f, 1.5625f, ref index, buffer ); // hat bottom
			ZPlane64x64( 40, 8, 8, 8, 0.3125f, -0.3125f, 1.5625f, 2.1875f, -0.3125f, ref index, buffer ); // hat front
			ZPlane64x64( 56, 8, 8, 8, -0.3125f, 0.3125f, 1.5625f, 2.1875f, 0.3125f, ref index, buffer ); // hat back
			XPlane64x64( 48, 8, 8, 8, -0.3125f, 0.3125f, 1.5625f, 2.1875f, -0.3125f, ref index, buffer ); // hat left
			XPlane64x64( 32, 8, 8, 8, 0.3125f, -0.3125f, 1.5625f, 2.1875f, 0.3125f, ref index, buffer ); // hat right
			Set64x64Slim.Hat = new ModelPart( buffer, graphics );
		}
		
		public void Dispose() {
			Set64x32.Dispose();
			Set64x64.Dispose();
			Set64x64Slim.Dispose();
		}	
		
		static TextureRectangle GetSkinTexCoords64x32( int x, int y, int width, int height ) {
			TextureRectangle rec;
			rec.U1 = x / 64f;
			rec.U2 = ( x + width ) / 64f;
			rec.V1 = y / 32f;
			rec.V2 = ( y + height ) / 32f;
			return rec;
		}
		
		static TextureRectangle GetSkinTexCoords64x64( int x, int y, int width, int height ) {
			TextureRectangle rec;
			rec.U1 = x / 64f;
			rec.U2 = ( x + width ) / 64f;
			rec.V1 = y / 64f;
			rec.V2 = ( y + height ) / 64f;
			return rec;
		}
		
		static void XPlane64x32( int texX, int texY, int texWidth, int texHeight,
		                   float z1, float z2, float y1, float y2, float x,
		                   ref int index, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = GetSkinTexCoords64x32( texX, texY, texWidth, texHeight );
			XPlane( rec, z1, z2, y1, y2, x, ref index, vertices );
		}
		
		static void YPlane64x32( int texX, int texY, int texWidth, int texHeight,
		                   float x1, float x2, float z1, float z2, float y,
		                   ref int index, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = GetSkinTexCoords64x32( texX, texY, texWidth, texHeight );
			YPlane( rec, x1, x2, z1, z2, y, ref index, vertices );
		}
		
		static void ZPlane64x32( int texX, int texY, int texWidth, int texHeight,
		                   float x1, float x2, float y1, float y2, float z,
		                   ref int index, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = GetSkinTexCoords64x32( texX, texY, texWidth, texHeight );
			ZPlane( rec, x1, x2, y1, y2, z, ref index, vertices );
		}
		
		static void XPlane64x64( int texX, int texY, int texWidth, int texHeight,
		                   float z1, float z2, float y1, float y2, float x,
		                   ref int index, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = GetSkinTexCoords64x64( texX, texY, texWidth, texHeight );
			XPlane( rec, z1, z2, y1, y2, x, ref index, vertices );
		}
		
		static void YPlane64x64( int texX, int texY, int texWidth, int texHeight,
		                   float x1, float x2, float z1, float z2, float y,
		                   ref int index, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = GetSkinTexCoords64x64( texX, texY, texWidth, texHeight );
			YPlane( rec, x1, x2, z1, z2, y, ref index, vertices );
		}
		
		static void ZPlane64x64( int texX, int texY, int texWidth, int texHeight,
		                   float x1, float x2, float y1, float y2, float z,
		                   ref int index, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = GetSkinTexCoords64x64( texX, texY, texWidth, texHeight );
			ZPlane( rec, x1, x2, y1, y2, z, ref index, vertices );
		}
		
		static void XPlane( TextureRectangle rec,
		                   float z1, float z2, float y1, float y2, float x,
		                   ref int index, VertexPos3fTex2fCol4b[] vertices ) {
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z2, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V2, col );
		}
		
		static void YPlane( TextureRectangle rec,
		                   float x1, float x2, float z1, float z2, float y,
		                   ref int index, VertexPos3fTex2fCol4b[] vertices ) {
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z1, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z2, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
		}
		
		static void ZPlane( TextureRectangle rec,
		                   float x1, float x2, float y1, float y2, float z,
		                   ref int index, VertexPos3fTex2fCol4b[] vertices ) {
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y1, z, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y1, z, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y2, z, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y2, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y2, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y1, z, rec.U1, rec.V2, col );
		}
	}
}