using System;

namespace ClassicalSharp.Items {
	
	public class Item {
		
		public ItemId Id;
		
		public Item( short id ) {
			Id = (ItemId)id;
		}
		
		public byte MaxCount = 64;
		
		internal int texId;
		public virtual int Get2DTextureLoc( short damage ) {
			return texId;
		}
		
	}
}
