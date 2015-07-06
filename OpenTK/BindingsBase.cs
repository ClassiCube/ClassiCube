#region License
//
// The Open Toolkit Library License
//
// Copyright (c) 2006 - 2009 the Open Toolkit library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights to 
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
#endregion

using System;
using System.Runtime.InteropServices;

namespace OpenTK {
	
    /// <summary>
    /// Provides a common foundation for all flat API bindings and implements the extension loading interface.
    /// </summary>
    public abstract class BindingsBase {
        protected readonly Type CoreClass;

        public BindingsBase( Type coreType ) {
            CoreClass = coreType;
        }

        protected abstract IntPtr GetAddress( string funcname );

        // Tries to load the specified core or extension function.
        protected Delegate LoadDelegate( string name, Type signature ) {
        	Delegate d = GetExtensionDelegate(name, signature);
        	if( d != null ) return d;
        	
        	try {
        		return Delegate.CreateDelegate( signature, CoreClass, name );
        	} catch( ArgumentException ) {
        		return null;
        	}
        }

        // Creates a System.Delegate that can be used to call a dynamically exported OpenGL function.
        Delegate GetExtensionDelegate( string name, Type signature ) {
            IntPtr address = GetAddress(name);          
            if (address == IntPtr.Zero || address == new IntPtr(1) || address == new IntPtr(2)) {
                return null; // Workaround for buggy nvidia drivers which return 1 or 2 instead of IntPtr.Zero for some extensions.
            }
            return Marshal.GetDelegateForFunctionPointer(address, signature);
        }
        
        protected internal T GetExtensionDelegate<T>( string name ) where T : class {
            IntPtr address = GetAddress( name );          
            if (address == IntPtr.Zero || address == new IntPtr(1) || address == new IntPtr(2)) {
                return null;
            }
            return (T)(object)Marshal.GetDelegateForFunctionPointer( address, typeof(T) );
        }
    }
}
