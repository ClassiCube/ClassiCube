// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;

namespace Launcher.Gui.Widgets {
	
	abstract class TableEntryComparer : IComparer<TableEntry> {
		
		public bool Invert = true;
		
		public abstract int Compare(TableEntry a, TableEntry b);
	}
	
	sealed class DefaultComparer : TableEntryComparer {
		
		public override int Compare(TableEntry a, TableEntry b) {
			long valX = Int64.Parse(a.Players.Substring(0, a.Players.IndexOf('/')));
			long valY = Int64.Parse(b.Players.Substring(0, b.Players.IndexOf('/')));
			int value = valY.CompareTo(valX);
			if (value != 0) return value;
			
			long timeX = Int64.Parse(a.RawUptime);
			long timeY = Int64.Parse(b.RawUptime);
			return timeY.CompareTo(timeX);
		}
	}
	
	sealed class NameComparer : TableEntryComparer {
		
		public override int Compare(TableEntry a, TableEntry b) {
			StringComparison comparison = StringComparison.CurrentCultureIgnoreCase;
			int value = String.Compare(a.Name, b.Name, comparison);
			return Invert ? -value : value;
		}
	}
	
	sealed class PlayersComparer : TableEntryComparer {
		
		public override int Compare(TableEntry a, TableEntry b) {
			long valX = Int64.Parse(a.Players.Substring(0, a.Players.IndexOf('/')));
			long valY = Int64.Parse(b.Players.Substring(0, b.Players.IndexOf('/')));
			int value = valX.CompareTo(valY);
			return Invert ? -value : value;
		}
	}
	
	sealed class UptimeComparer : TableEntryComparer {
		
		public override int Compare(TableEntry a, TableEntry b) {
			long timeX = Int64.Parse(a.RawUptime);
			long timeY = Int64.Parse(b.RawUptime);
			int value = timeX.CompareTo(timeY);
			return Invert ? -value : value;
		}
	}
	
	sealed class SoftwareComparer : TableEntryComparer {
		
		public override int Compare(TableEntry a, TableEntry b) {
			StringComparison comparison = StringComparison.CurrentCultureIgnoreCase;
			int value = String.Compare(a.Software, b.Software, comparison);
			return Invert ? -value : value;
		}
	}
}
