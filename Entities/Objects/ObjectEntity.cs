using System;

namespace ClassicalSharp {

	public abstract class ObjectEntity : Entity {
		
		public int Data;
		
		public ObjectEntityType EntityType;
		
		public ObjectEntity( ObjectEntityType type, int data, Game window ) : base( window ) {
			EntityType = type;
			Data = data;
		}
	}
	
	public enum ObjectEntityType {
		Boats = 1,
		Minecart = 10,
		StorageCart = 11,
		PoweredCart = 12,
		ActivatedTNT = 50,
		Arrow = 60,
		ThrownSnowball = 61,
		ThrownEgg = 62,
		FallingSand = 70,
		FallingGravel = 71,
		FishingFloat = 90,
	}
}