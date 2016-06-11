// Copyright (c) 2010-2014 SharpDX - Alexandre Mutel
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
using System;
using System.IO;
using Mono.Cecil;
using Mono.Cecil.Cil;
using Mono.Cecil.Pdb;

namespace InteropPatcher {
	
	public static class Patcher {
		
		static void ReplaceFixedStatement(MethodDefinition method, ILProcessor ilProcessor, Instruction fixedtoPatch) {
			var paramT = ((GenericInstanceMethod)fixedtoPatch.Operand).GenericArguments[0];
			// Preparing locals
			// local(0) T*
			method.Body.Variables.Add(new VariableDefinition("pin", new PinnedType(new ByReferenceType(paramT))));

			int index = method.Body.Variables.Count - 1;
			Instruction ldlocFixed, stlocFixed;
			switch (index) {
				case 0:
					stlocFixed = ilProcessor.Create(OpCodes.Stloc_0);
					ldlocFixed = ilProcessor.Create(OpCodes.Ldloc_0);
					break;
				case 1:
					stlocFixed = ilProcessor.Create(OpCodes.Stloc_1);
					ldlocFixed = ilProcessor.Create(OpCodes.Ldloc_1);
					break;
				case 2:
					stlocFixed = ilProcessor.Create(OpCodes.Stloc_2);
					ldlocFixed = ilProcessor.Create(OpCodes.Ldloc_2);
					break;
				case 3:
					stlocFixed = ilProcessor.Create(OpCodes.Stloc_3);
					ldlocFixed = ilProcessor.Create(OpCodes.Ldloc_3);
					break;
				default:
					stlocFixed = ilProcessor.Create(OpCodes.Stloc, index);
					ldlocFixed = ilProcessor.Create(OpCodes.Ldloc, index);
					break;
			}
			
			ilProcessor.InsertBefore(fixedtoPatch, stlocFixed);
			ilProcessor.Replace(fixedtoPatch, ldlocFixed);
		}

		static void PatchMethod(MethodDefinition method) {
			if( !method.HasBody ) return;
			var ilProcessor = method.Body.GetILProcessor();

			var instructions = method.Body.Instructions;
			for (int i = 0; i < instructions.Count; i++)
			{
				Instruction instruction = instructions[i];
				if (instruction.OpCode == OpCodes.Call && instruction.Operand is MethodReference)
				{
					var desc = (MethodReference)instruction.Operand;
					if( desc.DeclaringType.Name != "Interop" ) continue;
					
					if( desc.Name.StartsWith( "Calli" ) ) {
						var callSite = new CallSite(desc.ReturnType) { CallingConvention = MethodCallingConvention.StdCall };
						// Last parameter is the function ptr, so we don't add it as a parameter for calli
						// as it is already an implicit parameter for calli
						for (int j = 0; j < desc.Parameters.Count - 1; j++) {
							callSite.Parameters.Add(desc.Parameters[j]);
						}

						var callIInstruction = ilProcessor.Create(OpCodes.Calli, callSite);
						ilProcessor.Replace(instruction, callIInstruction);
					} else if( desc.Name.StartsWith( "Fixed" ) ) {
						ReplaceFixedStatement(method, ilProcessor, instruction);
					}
				}
			}
		}

		static void PatchType( TypeDefinition type ) {
			if( type.Name == "Interop" ) return;
			
			if( NeedsToBePatched( type ) ) {
				Console.WriteLine( "Patching type: " + type );
				foreach( var method in type.Methods )
					PatchMethod( method );
				foreach( var nestedType in type.NestedTypes )
					PatchType( nestedType );
			}
		}
		
		static bool NeedsToBePatched( TypeDefinition type ) {
			foreach( CustomAttribute attrib in type.CustomAttributes ) {
				if( attrib.AttributeType.Name == "InteropPatchAttribute" )
					return true;
			}
			return false;
		}

		public static void PatchFile( string file ) {
			// Copy PDB from input assembly to output assembly if any
			var readerParameters = new ReaderParameters();
			readerParameters.AssemblyResolver = new DefaultAssemblyResolver();
			var writerParameters = new WriterParameters();
			string pdbName = Path.ChangeExtension( file, "pdb" );
			
			if( File.Exists( pdbName ) ) {
				readerParameters.SymbolReaderProvider = new PdbReaderProvider();
				readerParameters.ReadSymbols = true;
				writerParameters.WriteSymbols = true;
			}
			AssemblyDefinition assembly = AssemblyDefinition.ReadAssembly(file, readerParameters);
			TypeDefinition interopTypeDefinition = null;

			Console.WriteLine( "SharpDX interop patch for: " + file );
			foreach( var type in assembly.MainModule.Types ) {
				if( type.Name == "Interop" ) {
					interopTypeDefinition = type;
					break;
				}
			}
			if( interopTypeDefinition == null ) {
				Console.WriteLine( "Nothing to do, already patched." );
				return;
			}
			
			foreach( var type in assembly.MainModule.Types ) {
				PatchType( type );
			}

			assembly.MainModule.Types.Remove( interopTypeDefinition );
			assembly.Write( file, writerParameters );
			Console.WriteLine( "Done patching." );
		}
	}
}