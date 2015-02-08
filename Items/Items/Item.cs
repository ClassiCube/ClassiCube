using System;

namespace ClassicalSharp.Items {
	
	public class Item {
		
		public ItemId Id;
		
		public Item( ItemId id ) {
			Id = id;
		}
		
		public byte MaxCount = 64;
		
		internal int texId;
		public virtual int Get2DTextureLoc( short damage ) {
			return texId;
		}
		
		public virtual bool CanHarvest( BlockId block ) {
			return false;
		}
		
		public virtual float GetEffectiveness( BlockId block ) {
			return 1;
		}
	}
}
