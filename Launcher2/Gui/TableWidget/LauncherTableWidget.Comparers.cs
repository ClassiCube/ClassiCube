// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;

namespace Launcher {

	public partial class LauncherTableWidget : LauncherWidget {
		
		abstract class TableEntryComparer : IComparer<TableEntry> {
			
			public bool Invert = true;
			
			public abstract int Compare( TableEntry a, TableEntry b );
		}
		
		sealed class NameComparer : TableEntryComparer {
			
			public override int Compare( TableEntry a, TableEntry b ) {
				StringComparison comparison = StringComparison.CurrentCultureIgnoreCase;
				int value = String.Compare( a.Name, b.Name, comparison );
				return Invert ? -value : value;
			}
		}
		
		sealed class PlayersComparer : TableEntryComparer {
			
			public override int Compare( TableEntry a, TableEntry b ) {
				long valX = Int64.Parse( a.Players.Substring( 0, a.Players.IndexOf( '/' ) ) );
				long valY = Int64.Parse( b.Players.Substring( 0, b.Players.IndexOf( '/' ) ) );
				int value = valX.CompareTo( valY );
				return Invert ? -value : value;
			}
		}
		
		sealed class UptimeComparer : TableEntryComparer {
			
			public override int Compare( TableEntry a, TableEntry b ) {
				TimeSpan valX = ParseUptimeString( a.Uptime );
				TimeSpan valY = ParseUptimeString( b.Uptime );
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
		
		sealed class SoftwareComparer : TableEntryComparer {
			
			public override int Compare( TableEntry a, TableEntry b ) {
				StringComparison comparison = StringComparison.CurrentCultureIgnoreCase;
				int value = String.Compare( a.Software, b.Software, comparison );
				return Invert ? -value : value;
			}
		}
	}
}
