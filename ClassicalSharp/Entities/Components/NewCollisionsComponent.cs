// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

#if false
namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs collision detection. </summary>
	public sealed class NewCollisionsComponent {
		
		Game game;
		Entity entity;
		BlockInfo info;
		static AABB bb;
		static RayTracer tracerP1Y1 = new RayTracer();
		static RayTracer tracerP2Y1 = new RayTracer();
		static RayTracer tracerP1Y2 = new RayTracer();
		static RayTracer tracerP2Y2 = new RayTracer();
		
		public NewCollisionsComponent( Game game, Entity entity ) {
			this.game = game;
			this.entity = entity;
			info = game.BlockInfo;
		}
		
		internal unsafe void MoveAndWallSlide() {
			if( entity.Velocity == Vector3.Zero ) return;
			int* minY = stackalloc int[4096];
			int* maxY = stackalloc int[4096];
			int* minX = stackalloc int[4096];
			int* maxX = stackalloc int[4096];
			
			for( int i = 0; i < 4096; i++ ) {
				minX[i] = int.MaxValue; minY[i] = int.MaxValue;
				maxX[i] = int.MinValue; maxY[i] = int.MinValue;
			}
			bb = entity.Bounds;
			int count = 0;
			
			// First raytrace out from the origin to find approximate blocks
			CalcRayDirection();
			while( true ) {
				UpdateCoords( tracerP1Y1, minX, maxX, minY, maxY );
				UpdateCoords( tracerP1Y2, minX, maxX, minY, maxY );
				UpdateCoords( tracerP2Y1, minX, maxX, minY, maxY );
				UpdateCoords( tracerP2Y2, minX, maxX, minY, maxY );
			}
			
			// Now precisely which blocks we really intersect with
			
			// .. then perform collision
		}
		
		void UpdateCoords( RayTracer tracer, int* minX, int* maxX, 
		                  int* minY, int* maxY, ref int count ) {
			tracer.Step();
			minX[tracer.Z] = Math.Min( tracer.X, minX[tracer.Z] );
			minY[tracer.Z] = Math.Min( tracer.Y, minY[tracer.Z] );
			maxX[tracer.Z] = Math.Max( tracer.X, maxX[tracer.Z] );
			maxY[tracer.Z] = Math.Max( tracer.Y, maxY[tracer.Z] );
			count = Math.Max( count, tracer.Z );
		}
		
		void CalcRayDirection() {
			Vector3 dir = entity.Velocity;
			Vector3 P = new Vector3( bb.Min.X, bb.Min.Y, bb.Max.Z );
			tracerP1Y1.SetRayData( P, dir );
			P.Y = bb.Max.Y;
			tracerP1Y1.SetRayData( P, dir );
			
			P = new Vector3( bb.Max.X, bb.Min.Y, bb.Min.Z );
			tracerP2Y1.SetRayData( P, dir );
			P.Y = bb.Max.Y;
			tracerP2Y2.SetRayData( P, dir );
		}
	}
}
#endif