// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that plays block step sounds. </summary>
	public sealed class SoundComponent {

		LocalPlayer p;
		Game game;		
		Predicate<byte> checkSoundNonSolid, checkSoundSolid;
		
		public SoundComponent( Game game, Entity entity ) {
			this.game = game;
			p = (LocalPlayer)entity;
			checkSoundNonSolid = CheckSoundNonSolid;
			checkSoundSolid = CheckSoundSolid;
		}
		
		Vector3 lastSoundPos = new Vector3( float.PositiveInfinity );
		public void Tick( bool wasOnGround ) {
			Vector3 soundPos = p.nextPos;
			GetSound();
			if( !anyNonAir ) soundPos = new Vector3( -100000 );
			
			if( p.onGround && (DoPlaySound( soundPos ) || !wasOnGround) ) {
				game.AudioPlayer.PlayStepSound( sndType );
				lastSoundPos = soundPos;
			}
		}
		
		bool DoPlaySound( Vector3 soundPos ) {
			float distSq = (lastSoundPos - soundPos).LengthSquared;
			bool enoughDist = distSq > 1.75f * 1.75f;
			// just play every certain block interval when not animating
			if( p.curSwing < 0.999f ) return enoughDist;
			
			// have our legs just crossed over the '0' point?
			float oldLegRot = (float)Math.Sin( p.anim.walkTimeO );
			float newLegRot = (float)Math.Sin( p.anim.walkTimeN );
			return Math.Sign( oldLegRot ) != Math.Sign( newLegRot );
		}
		
		bool anyNonAir = false;
		SoundType sndType = SoundType.None;
		void GetSound() {
			Vector3 pos = p.nextPos, size = p.CollisionSize;
			BoundingBox bounds = new BoundingBox( pos - size / 2, pos + size / 2 );
			sndType = SoundType.None;
			anyNonAir = false;
			
			// first check surrounding liquids/gas for sounds
			p.TouchesAny( bounds, checkSoundNonSolid );
			if( sndType != SoundType.None ) return;
			
			// then check block standing on
			byte blockUnder = (byte)p.BlockUnderFeet;
			SoundType typeUnder = game.BlockInfo.StepSounds[blockUnder];
			CollideType collideType = game.BlockInfo.Collide[blockUnder];
			if( collideType == CollideType.Solid && typeUnder != SoundType.None ) {
				anyNonAir = true; sndType = typeUnder; return;
			}
			
			// then check all solid blocks at feet
			pos.Y -= 0.01f;
			bounds.Max.Y = bounds.Min.Y = pos.Y;
			p.TouchesAny( bounds, checkSoundSolid );
		}
		
		bool CheckSoundNonSolid( byte b ) {
			SoundType newType = game.BlockInfo.StepSounds[b];
			CollideType collide = game.BlockInfo.Collide[b];
			if( newType != SoundType.None && collide != CollideType.Solid )
				sndType = newType;
			if( b != 0 ) anyNonAir = true;
			return false;
		}
		
		bool CheckSoundSolid( byte b ) {
			SoundType newType = game.BlockInfo.StepSounds[b];
			if( newType != SoundType.None ) sndType = newType;
			if( b != 0 ) anyNonAir = true;
			return false;
		}
	}
}