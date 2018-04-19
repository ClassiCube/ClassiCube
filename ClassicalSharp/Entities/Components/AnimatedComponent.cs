// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
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
		
		public float bobbingHor, bobbingVer, bobbingModel;
		public float walkTime, swing, bobStrength = 1;
		internal float walkTimeO, walkTimeN, swingO, swingN, bobStrengthO = 1, bobStrengthN = 1;
		
		public float leftLegX, leftLegZ, rightLegX, rightLegZ;
		public float leftArmX, leftArmZ, rightArmX, rightArmZ;
		
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
			
			// TODO: the Tilt code was designed for 60 ticks/second, fix it up for 20 ticks/second
			bobStrengthO = bobStrengthN;
			for (int i = 0; i < 3; i++) {
				DoTilt(ref bobStrengthN, !game.ViewBobbing || !entity.onGround);
			}
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
			bobStrength = Utils.Lerp(bobStrengthO, bobStrengthN, t);
			
			float idleTime = (float)game.accumulator;
			float idleXRot = (float)(Math.Sin(idleTime * idleXPeriod) * idleMax);
			float idleZRot = (float)(idleMax + Math.Cos(idleTime * idleZPeriod) * idleMax);
			
			leftArmX = (float)(Math.Cos(walkTime) * swing * armMax) - idleXRot;
			leftArmZ = -idleZRot;
			leftLegX = -(float)(Math.Cos(walkTime) * swing * legMax);
			leftLegZ = 0;
			
			rightLegX = -leftLegX; rightLegZ = -leftLegZ;
			rightArmX = -leftArmX; rightArmZ = -leftArmZ;
			
			bobbingHor = (float)(Math.Cos(walkTime) * swing * (2.5f/16f));
			bobbingVer = (float)(Math.Abs(Math.Sin(walkTime)) * swing * (2.5f/16f));
			bobbingModel = (float)(Math.Abs(Math.Cos(walkTime)) * swing * (4.0f/16f));
			
			if (entity.Model.CalcHumanAnims && !game.SimpleArmsAnim)
				CalcHumanAnim(idleXRot, idleZRot);
		}
		
		internal static void DoTilt(ref float tilt, bool reduce) {
			if (reduce) tilt *= 0.84f;
			else tilt += 0.1f;
			Utils.Clamp(ref tilt, 0, 1);
		}
		
		void CalcHumanAnim(float idleXRot, float idleZRot) {
			PerpendicularAnim(0.23f, idleXRot, idleZRot, out leftArmX, out leftArmZ);
			PerpendicularAnim(0.28f, idleXRot, idleZRot, out rightArmX, out rightArmZ);
			rightArmX = -rightArmX; rightArmZ = -rightArmZ;
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
	
	
	/// <summary> Entity component that performs tilt animation depending on movement speed and time. </summary>
	public sealed class TiltComponent {
		
		Game game;
		public TiltComponent(Game game) { this.game = game; }
		
		public float tiltX, tiltY, velTiltStrength = 1;
		internal float velTiltStrengthO = 1, velTiltStrengthN = 1;
		
		/// <summary> Calculates the next animation state based on old and new position. </summary>
		public void UpdateAnimState(double delta) {		
			velTiltStrengthO = velTiltStrengthN;
			LocalPlayer p = game.LocalPlayer;
			
			// TODO: the Tilt code was designed for 60 ticks/second, fix it up for 20 ticks/second
			for (int i = 0; i < 3; i++) {
				AnimatedComponent.DoTilt(ref velTiltStrengthN, p.Hacks.Floating);
			}
		}
		
		/// <summary> Calculates the interpolated state between the last and next animation state. </summary>
		public void GetCurrentAnimState(float t) {
			LocalPlayer p = game.LocalPlayer;
			velTiltStrength = Utils.Lerp(velTiltStrengthO, velTiltStrengthN, t);
			
			tiltX = (float)Math.Cos(p.anim.walkTime) * p.anim.swing * (0.15f * Utils.Deg2Rad);
			tiltY = (float)Math.Sin(p.anim.walkTime) * p.anim.swing * (0.15f * Utils.Deg2Rad);
		}
	}
}