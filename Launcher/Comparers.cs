﻿using System;
using System.Collections;
using System.Windows.Forms;

namespace Launcher {
	
	public abstract class CustomListComparer : IComparer {
		
		public bool Invert = false;
		protected readonly int Col;

	    protected CustomListComparer( int column ) {
			Col = column;
		}
		
		public int Compare( object x, object y ) {
			return Compare( (ListViewItem)x, (ListViewItem)y );
		}
		
		protected abstract int Compare( ListViewItem itemX, ListViewItem itemY );
	}
	
	public class NameComparer : CustomListComparer {
		
		public NameComparer( int column ) : base( column ) { }
		
		protected override int Compare( ListViewItem x, ListViewItem y ) {
			var value = String.Compare( x.SubItems[Col].Text, y.SubItems[Col].Text, true );
			if( Invert ) value = -value;
			return value;
		}
	}
	
	public class NumericalComparer : CustomListComparer {
		
		public NumericalComparer( int column ) : base( column ) { }
		
		protected override int Compare( ListViewItem x, ListViewItem y ) {
			var valX = Int64.Parse( x.SubItems[Col].Text );
			var valY = Int64.Parse( y.SubItems[Col].Text );
			var value = valX.CompareTo( valY );
			if( Invert ) value = -value;
			return value;
		}
	}
	
	public class UptimeComparer : CustomListComparer {
		
		public UptimeComparer( int column ) : base( column ) { }
		
		protected override int Compare( ListViewItem x, ListViewItem y ) {
			var valX = ParseUptimeString( x.SubItems[Col].Text );
			var valY = ParseUptimeString( y.SubItems[Col].Text );
			var value = valX.CompareTo( valY );
			if( Invert ) value = -value;
			return value;
		}
		
		static TimeSpan ParseUptimeString( string s ) {
			var sum = 0;
			for( var i = 0; i < s.Length - 1; i++ ) {
				sum *= 10;
				sum += s[i] - '0';
			}
			
			var timeType = s[s.Length - 1];
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