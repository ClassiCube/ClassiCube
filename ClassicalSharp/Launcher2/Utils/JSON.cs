// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Text;
using ClassicalSharp;
using JsonObject = System.Collections.Generic.Dictionary<string, object>;
using JsonArray = System.Collections.Generic.List<object>;

namespace Launcher {

	public class JsonContext {
		public string Val; public int Idx; public bool Success = true;
		public char Cur { get { return Val[Idx]; } }
		internal StringBuilder strBuffer = new StringBuilder(96);
	}
	
	public static class Json {
		const int T_NONE = 0, T_NUM = 1, T_TRUE = 2, T_FALSE = 3, T_NULL = 4;
		
		static bool IsWhitespace(char c) {
			return c == '\r' || c == '\n' || c == '\t' || c == ' ';
		}
		
		static bool NextConstant(JsonContext ctx, string value) {
			if (ctx.Idx + value.Length > ctx.Val.Length) return false;
			
			for (int i = 0; i < value.Length; i++) {
				if (ctx.Val[ctx.Idx + i] != value[i]) return false;
			}
			
			ctx.Idx += value.Length; return true;
		}
		
		static int NextToken(JsonContext ctx) {
			for (; ctx.Idx < ctx.Val.Length && IsWhitespace(ctx.Cur); ctx.Idx++);
			if (ctx.Idx >= ctx.Val.Length) return T_NONE;
			
			char c = ctx.Cur; ctx.Idx++;
			if (c == '{' || c == '}') return c;
			if (c == '[' || c == ']') return c;
			if (c == ',' || c == '"' || c == ':') return c;
			
			if (IsNumber(c)) return T_NUM;			
			ctx.Idx--;
			
			if (NextConstant(ctx, "true"))  return T_TRUE;
			if (NextConstant(ctx, "false")) return T_FALSE;
			if (NextConstant(ctx, "null"))  return T_NULL;
			
			// invalid token
			ctx.Idx++; return T_NONE;
		}
		
		public static object ParseStream(JsonContext ctx) {
			return ParseValue(NextToken(ctx), ctx);
		}
		
		static object ParseValue(int token, JsonContext ctx) {
			switch (token) {
				case '{': return ParseObject(ctx);
				case '[': return ParseArray(ctx);
				case '"': return ParseString(ctx);
					
				case T_NUM:   return ParseNumber(ctx);
				case T_TRUE:  return true;
				case T_FALSE: return false;
				case T_NULL:  return null;
					
				default: return null;
			}
		}
		
		static JsonObject ParseObject(JsonContext ctx) {
			JsonObject members = new JsonObject();
			while (true) {
				int token = NextToken(ctx);
				if (token == ',') continue;
				if (token == '}') return members;
				
				if (token != '"') { ctx.Success = false; return null; }
				string key = ParseString(ctx);
				
				token = NextToken(ctx);
				if (token != ':') { ctx.Success = false; return null; }
				
				token = NextToken(ctx);
				if (token == T_NONE) { ctx.Success = false; return null; }
				
				members[key] = ParseValue(token, ctx);
			}
		}
		
		static JsonArray ParseArray(JsonContext ctx) {
			JsonArray elements = new JsonArray();
			while (true) {
				int token = NextToken(ctx);
				if (token == ',') continue;
				if (token == ']') return elements;
				
				if (token == T_NONE) { ctx.Success = false; return null; }
				elements.Add(ParseValue(token, ctx));
			}
		}
		
		static string ParseString(JsonContext ctx) {
			StringBuilder s = ctx.strBuffer; s.Length = 0;
			
			for (; ctx.Idx < ctx.Val.Length;) {
				char c = ctx.Cur; ctx.Idx++;
				if (c == '"') return s.ToString();
				if (c != '\\') { s.Append(c); continue; }
				
				if (ctx.Idx >= ctx.Val.Length) break;
				c = ctx.Cur; ctx.Idx++;
				if (c == '/' || c == '\\' || c == '"') { s.Append(c); continue; }
				
				if (c != 'u') break;
				if (ctx.Idx + 4 > ctx.Val.Length) break;
				
				// form of \uYYYY
				int aH, aL, bH, bL;
				if (!PackedCol.UnHex(ctx.Val[ctx.Idx + 0], out aH)) break;
				if (!PackedCol.UnHex(ctx.Val[ctx.Idx + 1], out aL)) break;
				if (!PackedCol.UnHex(ctx.Val[ctx.Idx + 2], out bH)) break;
				if (!PackedCol.UnHex(ctx.Val[ctx.Idx + 3], out bL)) break;
								
				int codePoint = (aH << 12) | (aL << 8) | (bH << 4) | bL;
				// don't want control characters in names/software
				if (codePoint >= 32) s.Append((char)codePoint);
				ctx.Idx += 4;
			}
			
			ctx.Success = false; return null;
		}
		
		static bool IsNumber(char c) {
			return c == '-' || c == '.' || (c >= '0' && c <= '9');
		}
		
		static string ParseNumber(JsonContext ctx) {
			int start = ctx.Idx - 1;
			for (; ctx.Idx < ctx.Val.Length && IsNumber(ctx.Cur); ctx.Idx++);
			return ctx.Val.Substring(start, ctx.Idx - start);
		}
	}
}