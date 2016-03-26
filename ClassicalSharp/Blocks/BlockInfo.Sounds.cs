// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		public SoundType[] DigSounds = new SoundType[BlocksCount];
		
		public SoundType[] StepSounds = new SoundType[BlocksCount];
		
		int curSoundBlock = (int)Block.Stone;
		public void InitSounds() {
			SetSound( 1, SoundType.Stone ); SetSound( 1, SoundType.Grass ); SetSound( 1, SoundType.Gravel );
			SetSound( 1, SoundType.Stone ); SetSound( 1, SoundType.Wood ); SetSound( 1, SoundType.Grass, SoundType.None );
			SetSound( 1, SoundType.Stone ); SetSound( 4, SoundType.None ); SetSound( 1, SoundType.Sand );
			SetSound( 1, SoundType.Gravel ); SetSound( 3, SoundType.Stone ); SetSound( 1, SoundType.Wood );
			SetSound( 2, SoundType.Grass ); SetSound( 1, SoundType.Glass, SoundType.Stone ); SetSound( 16, SoundType.Cloth );
			SetSound( 4, SoundType.Grass, SoundType.None ); SetSound( 5, SoundType.Stone ); SetSound( 1, SoundType.Grass );
			SetSound( 1, SoundType.Wood ); SetSound( 3, SoundType.Stone ); SetSound( 1, SoundType.Cloth );
			SetSound( 1, SoundType.Stone ); SetSound( 1, SoundType.Snow ); SetSound( 1, SoundType.Wood );
			SetSound( 5, SoundType.Cloth ); SetSound( 4, SoundType.Stone ); SetSound( 1, SoundType.Wood );
			SetSound( 1, SoundType.Stone );
			
			curSoundBlock = (int)Block.Fire;
			SetSound( 1, SoundType.Wood, SoundType.None );
		}
		
		void SetSound( int count, SoundType type ) {
			SetSound( count, type, type );
		}
		
		void SetSound( int count, SoundType digType, SoundType stepType ) {
			for( int i = 0; i < count; i++ ) {
				StepSounds[i + curSoundBlock] = stepType;
				DigSounds[i + curSoundBlock] = digType;
			}
			curSoundBlock += count;
		}
	}
	
	public enum SoundType : byte {
		None, Wood, Gravel, Grass, Stone, 
		Metal, Glass, Cloth, Sand, Snow,
	}
}