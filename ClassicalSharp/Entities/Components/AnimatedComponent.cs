﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs model animation depending on movement speed and time. </summary>
	public sealed class AnimatedComponent {
		
		Game game;
		Entity entity;
		public AnimatedComponent(Game game, Entity entity) {
			this.game = game;
			this.entity = entity;
		}
		
		public float legXRot, armXRot, armZRot;
		public float bobbingHor, bobbingVer, bobbingModel, tiltX, tiltY;
		public float walkTime, swing, bobStrength = 1, velTiltStrength = 1;
		
		internal float walkTimeO, walkTimeN, swingO, swingN;
		internal float leftXRot, leftZRot, rightXRot, rightZRot;
		
		/// <summary> Calculates the next animation state based on old and new position. </summary>
		public void UpdateAnimState(Vector3 oldPos, Vector3 newPos, double delta) {
			walkTimeO = walkTimeN;
			swingO = swingN;
			float dx = newPos.X - oldPos.X;
			float dz = newPos.Z - oldPos.Z;
			double distance = Math.Sqrt(dx * dx + dz * dz);
			
			if (distance > 0.05) {
				float walkDelta = (float)distance * 2 * (float)(20 * delta);
				walkTimeN += walkDelta;
				swingN += (float)delta * 3;
			} else {
				swingN -= (float)delta * 3;
			}
			Utils.Clamp(ref swingN, 0, 1);
		}
		
		const float armMax = 60 * Utils.Deg2Rad;
		const float legMax = 80 * Utils.Deg2Rad;
		const float idleMax = 3 * Utils.Deg2Rad;
		const float idleXPeriod = (float)(2 * Math.PI / 5.0f);
		const float idleZPeriod = (float)(2 * Math.PI / 3.5f);
		
		/// <summary> Calculates the interpolated state between the last and next animation state. </summary>
		public void GetCurrentAnimState(float t) {
			swing = Utils.Lerp(swingO, swingN, t);
			walkTime = Utils.Lerp(walkTimeO, walkTimeN, t);
			float idleTime = (float)game.accumulator;
			float idleXRot = (float)(Math.Sin(idleTime * idleXPeriod) * idleMax);
			float idleZRot = (float)(idleMax + Math.Cos(idleTime * idleZPeriod) * idleMax);
			
			armXRot = (float)(Math.Cos(walkTime) * swing * armMax) - idleXRot;
			legXRot = -(float)(Math.Cos(walkTime) * swing * legMax);
			armZRot = -idleZRot;
			
			bobbingHor = (float)(Math.Cos(walkTime) * swing * (2.5f/16f));
			bobbingVer = (float)(Math.Abs(Math.Sin(walkTime)) * swing * (2.5f/16f));
			bobbingModel = (float)(Math.Abs(Math.Cos(walkTime)) * swing * (4.0f/16f));
			
			DoTilt(ref bobStrength, !game.ViewBobbing || !entity.onGround);
			if (entity is LocalPlayer) {
				LocalPlayer p = (LocalPlayer)entity;
				DoTilt(ref velTiltStrength, p.Hacks.Noclip || p.Hacks.Flying);
				
				tiltX = (float)Math.Cos(walkTime) * swing * (0.15f * Utils.Deg2Rad);
				tiltY = (float)Math.Sin(walkTime) * swing * (0.15f * Utils.Deg2Rad);
			}

			if (entity.Model is HumanoidModel)
				CalcHumanAnim(idleXRot, idleZRot);
		}
		
		static void DoTilt(ref float tilt, bool reduce) {
			if (reduce) tilt *= 0.84f;
			else tilt += 0.1f;
			Utils.Clamp(ref tilt, 0, 1);
		}
		
		void CalcHumanAnim(float idleXRot, float idleZRot) {
			if (game.SimpleArmsAnim) {
				leftXRot = armXRot; leftZRot = armZRot;
				rightXRot = -armXRot; rightZRot = -armZRot;
			} else {
				PerpendicularAnim(0.23f, idleXRot, idleZRot, out leftXRot, out leftZRot);
				PerpendicularAnim(0.28f, idleXRot, idleZRot, out rightXRot, out rightZRot);
				rightXRot = -rightXRot; rightZRot = -rightZRot;
			}
		}
		
		const float maxAngle = 110 * Utils.Deg2Rad;
		void PerpendicularAnim(float flapSpeed, float idleXRot, float idleZRot,
		                       out float xRot, out float zRot) {
			float verAngle = (float)(0.5 + 0.5 * Math.Sin(walkTime * flapSpeed));
			zRot = -idleZRot - verAngle * swing * maxAngle;
			float horAngle = (float)(Math.Cos(walkTime) * swing * armMax * 1.5f);
			xRot = idleXRot + horAngle;
		}
	}
}