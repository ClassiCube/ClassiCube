using System;
using System.Collections;
using System.Windows.Forms;

namespace Launcher {
	
	public abstract class CustomListComparer : IComparer {
		
		public bool Invert = false;
		protected readonly int col;
		
		public CustomListComparer( int column ) {
			col = column;
		}
		
		public int Compare( object x, object y ) {
			return Compare( (ListViewItem)x, (ListViewItem)y );
		}
		
		protected abstract int Compare( ListViewItem itemX, ListViewItem itemY );
	}
	
	public class NameComparer : CustomListComparer {
		
		public NameComparer( int column ) : base( column ) { }
		
		protected override int Compare( ListViewItem x, ListViewItem y ) {
			StringComparison comparison = StringComparison.CurrentCultureIgnoreCase;
			int value = String.Compare( x.SubItems[col].Text, y.SubItems[col].Text, comparison );
			return Invert ? -value : value;
		}
	}
	
	public class NumericalComparer : CustomListComparer {
		
		public NumericalComparer( int column ) : base( column ) { }
		
		protected override int Compare( ListViewItem x, ListViewItem y ) {
			long valX = Int64.Parse( x.SubItems[col].Text );
			long valY = Int64.Parse( y.SubItems[col].Text );
			int value = valX.CompareTo( valY );
			return Invert ? -value : value;
		}
	}
	
	public class UptimeComparer : CustomListComparer {
		
		public UptimeComparer( int column ) : base( column ) { }
		
		protected override int Compare( ListViewItem x, ListViewItem y ) {
			TimeSpan valX = ParseUptimeString( x.SubItems[col].Text );
			TimeSpan valY = ParseUptimeString( y.SubItems[col].Text );
			int value = valX.CompareTo( valY );
			return Invert ? -value : value;
		}
		
		static TimeSpan ParseUptimeString( string s ) {
			int sum = 0;
			for( int i = 0; i < s.Length - 1; i++ ) {
				sum *= 10;
				sum += s[i] - '0';
			}
			
			char timeType = s[s.Length - 1];
			switch( timeType ) {
					case 'w' : return TimeSpan.FromDays( sum * 7 );
					case 'd' : return TimeSpan.FromDays( sum );					
					case 'h' : return TimeSpan.FromHours( sum );
					case 'm' : return TimeSpan.FromMinutes( sum );
					case 's' : return TimeSpan.FromSeconds( sum );
					default: throw new NotSupportedException( "unsupported uptime type: " + timeType );
			}
		}
	}
}