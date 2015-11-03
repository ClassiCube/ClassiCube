//
//
//  xCSCarbon
//
//  Created by Erik Ylvisaker on 3/17/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
//

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace OpenTK.Platform.MacOS.Carbon
{
	internal struct EventInfo {
		
		internal EventInfo(IntPtr eventRef) {
			EventClass = API.GetEventClass(eventRef);
			EventKind = API.GetEventKind(eventRef);
		}

		public uint EventKind;
		public EventClass EventClass;

		public override string ToString() {
			return "Event class " + EventClass + ", kind: " + EventKind;
		}
	}
}
