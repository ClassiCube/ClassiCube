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
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;

namespace OpenTK
{
    /// <summary>
    /// Provides a common foundation for all flat API bindings and implements the extension loading interface.
    /// </summary>
    public abstract class BindingsBase
    {
        #region Fields

        readonly protected Type DelegatesClass; // nested type for function delegates
        readonly protected Type CoreClass; // nested type that contains core functions (i.e. not extensions)      
        readonly int NamePrefixSize; // two for 'gl', three for 'wgl'

        #endregion

        #region Constructors

        public BindingsBase( Type delegatesType, Type coreType, int prefixSize ) {
            DelegatesClass = delegatesType;
            CoreClass = coreType;
            NamePrefixSize = prefixSize;
        }

        #endregion

        #region Protected Members

        /// <summary>
        /// Retrieves an unmanaged function pointer to the specified function.
        /// </summary>
        /// <param name="funcname">
        /// A <see cref="System.String"/> that defines the name of the function.
        /// </param>
        /// <returns>
        /// A <see cref="IntPtr"/> that contains the address of funcname or IntPtr.Zero,
        /// if the function is not supported by the drivers.
        /// </returns>
        /// <remarks>
        /// Note: some drivers are known to return non-zero values for unsupported functions.
        /// Typical values include 1 and 2 - inheritors are advised to check for and ignore these
        /// values.
        /// </remarks>
        protected abstract IntPtr GetAddress(string funcname);

        /// <summary>
        /// Gets an object that can be used to synchronize access to the bindings implementation.
        /// </summary>
        /// <remarks>This object should be unique across bindings but consistent between bindings
        /// of the same type. For example, ES10.GL, OpenGL.GL and CL10.CL should all return 
        /// unique objects, but all instances of ES10.GL should return the same object.</remarks>
        protected abstract object SyncRoot { get; }

        #endregion

        #region Internal Members

        #region LoadEntryPoints

        internal void LoadEntryPoints()
        {
            // Using reflection is more than 3 times faster than directly loading delegates on the first
            // run, probably due to code generation overhead. Subsequent runs are faster with direct loading
            // than with reflection, but the first time is more significant.

            int supported = 0;

            FieldInfo[] delegates = DelegatesClass.GetFields(BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
            if (delegates == null)
                throw new InvalidOperationException("The specified type does not have any loadable extensions.");

            Debug.Write("Loading extensions for " + this.GetType().FullName + "... ");

            Stopwatch time = new Stopwatch();
            time.Reset();
            time.Start();

            foreach (FieldInfo f in delegates) {
                Delegate d = LoadDelegate( f.Name, f.FieldType );
                if (d != null) supported++;

                lock (SyncRoot) {
                    f.SetValue(null, d);
                }
            }

            time.Stop();
            Debug.Print("{0} extensions loaded in {1} ms.", supported, time.Elapsed.TotalMilliseconds);
            time.Reset();
        }

        #endregion

        #endregion

        #region Private Members

        // Tries to load the specified core or extension function.
        Delegate LoadDelegate(string name, Type signature)
        {
        	Delegate d = GetExtensionDelegate(name, signature);
        	if( d != null ) return d;
        	
        	return Delegate.CreateDelegate( signature, CoreClass, name.Substring( NamePrefixSize ) );
        }

        // Creates a System.Delegate that can be used to call a dynamically exported OpenGL function.
        internal Delegate GetExtensionDelegate(string name, Type signature) {
            IntPtr address = GetAddress(name);          
            if (address == IntPtr.Zero || address == new IntPtr(1) || address == new IntPtr(2)) {
                return null; // Workaround for buggy nvidia drivers which return 1 or 2 instead of IntPtr.Zero for some extensions.
            }
            return Marshal.GetDelegateForFunctionPointer(address, signature);
        }

        #endregion
    }
}
