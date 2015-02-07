using System;

namespace ClassicalSharp.Items {
	
	public class Item {
		
		public ItemId Id;
		
		public Item( short id ) {
			Id = (ItemId)id;
		}
		
		public byte MaxCount = 64;
		
	}
}
