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
using Mono.Collections.Generic;

namespace InteropPatcher {
	
	public static class Patcher {

		public static int Main(string[] args) {
			try {
				if( args.Length == 0 ) {
					Console.WriteLine( "Expecting single argument specifying the file to patch" );
					return 2;
				}
				// Some older IDEs seem to like splitting the path when it has spaces.
				// So we undo this and treat the arguments as a single path.
				string path = String.Join( " ", args );
				PatchFile( path );
				return 0;
			} catch( Exception ex ) {
				Console.WriteLine( ex );
				return 1;
			}
		}
		
		static void PatchMethod(MethodDefinition method) {
			if (!method.HasBody) return;
			ILProcessor ilProcessor = method.Body.GetILProcessor();

			Collection<Instruction> instructions = method.Body.Instructions;
			for (int i = 0; i < instructions.Count; i++) {
				Instruction instruction = instructions[i];
				if (instruction.OpCode == OpCodes.Call && instruction.Operand is MethodReference) {
					
					MethodReference desc = (MethodReference)instruction.Operand;
					if (desc.DeclaringType.Name != "Interop") continue;
					if (!desc.Name.StartsWith("Calli")) continue;
					
					CallSite callSite = new CallSite(desc.ReturnType);
					callSite.CallingConvention = MethodCallingConvention.StdCall;
					// Last parameter is the function ptr, so we don't add it as a parameter for calli
					// as it is already an implicit parameter for calli
					for (int j = 0; j < desc.Parameters.Count - 1; j++) {
						callSite.Parameters.Add(desc.Parameters[j]);
					}

					Instruction calli_ins = ilProcessor.Create(OpCodes.Calli, callSite);
					ilProcessor.Replace(instruction, calli_ins);
				}
			}
		}

		static void PatchType(TypeDefinition type) {
			if (type.Name == "Interop") return;
			if (!NeedsToBePatched(type)) return;
			
			Console.WriteLine("Patching type: " + type);
			foreach (MethodDefinition method in type.Methods)
				PatchMethod(method);
			foreach (TypeDefinition nestedType in type.NestedTypes)
				PatchType(nestedType);
		}
		
		static bool NeedsToBePatched(TypeDefinition type) {
			foreach (CustomAttribute attrib in type.CustomAttributes) {
				if (attrib.AttributeType.Name == "InteropPatchAttribute")
					return true;
			}
			return false;
		}

		public static void PatchFile(string file) {			
			ReaderParameters rParams = new ReaderParameters();
			rParams.AssemblyResolver = new DefaultAssemblyResolver();
			WriterParameters wParams = new WriterParameters();
			
			// Copy PDB from input assembly to output assembly if any
			if (File.Exists(Path.ChangeExtension(file, "pdb"))) {
				rParams.SymbolReaderProvider = new PdbReaderProvider();
				rParams.ReadSymbols = true;
				wParams.WriteSymbols = true;
			}
			
			AssemblyDefinition assembly = AssemblyDefinition.ReadAssembly(file, rParams);
			TypeDefinition interopTypeDefinition = null;
			Console.WriteLine("Interop patching: " + file);
			
			foreach (TypeDefinition type in assembly.MainModule.Types) {
				if (type.Name == "Interop") {
					interopTypeDefinition = type; break;
				}
			}
			if (interopTypeDefinition == null) {
				Console.WriteLine("Nothing to do, already patched.");
				return;
			}
			
			foreach (TypeDefinition type in assembly.MainModule.Types) {
				PatchType(type);
			}

			assembly.MainModule.Types.Remove(interopTypeDefinition);
			assembly.Write(file, wParams);
			Console.WriteLine("Done patching.");
		}
	}
}