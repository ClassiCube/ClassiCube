// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

namespace ClassicalSharp.Gui.Widgets {
	public abstract class MenuInputValidator {
		
		public string Range;
		
		public abstract bool IsValidChar(char c);
		
		public abstract bool IsValidString(string s);
		
		public virtual bool IsValidValue(string s) {
			return IsValidString(s);
		}
		
		protected void MakeRange(string min, string max) {
			Range = "&7(" + min + " - " + max + ")";
		}
	}
	
	/// <summary> Ensures that the input conforms to a hex colour code. </summary>
	public sealed class HexColourValidator : MenuInputValidator {
		
		public HexColourValidator() {
			Range = "&7(#000000 - #FFFFFF)";
		}
		
		public override bool IsValidChar(char c) {
			return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')
				|| (c >= 'a' && c <= 'f');
		}
		
		public override bool IsValidString(string s) {
			return s.Length <= 6;
		}
		
		public override bool IsValidValue(string s) {
			FastColour col;
			return FastColour.TryParse(s, out col);
		}
	}

	/// <summary> Ensures that the input conforms to a whole number integer. </summary>	
	public class IntegerValidator : MenuInputValidator {
		
		int min, max;
		public IntegerValidator(int min, int max) {
			this.min = min;
			this.max = max;
			SetRange();
		}
		
		protected virtual void SetRange() {
			MakeRange(min.ToString(), max.ToString());
		}
		
		public override bool IsValidChar(char c) {
			return (c >= '0' && c <= '9') || c == '-';
		}
		
		public override bool IsValidString(string s) {
			int value;
			if (s.Length == 1 && s[0] == '-') return true; // input is just a minus sign
			return Int32.TryParse(s, out value);
		}
		
		public override bool IsValidValue(string s) {
			int value;
			if (!Int32.TryParse(s, out value)) return false;
			return min <= value && value <= max;
		}
	}
	
	public sealed class SeedValidator : IntegerValidator {
		
		public SeedValidator() : base(int.MinValue, int.MaxValue) { }
		
		protected override void SetRange() {
			Range = "&7(an integer)";
		}
	}
	
	public sealed class RealValidator : MenuInputValidator {
		
		float min, max;
		public RealValidator(float min, float max) {
			this.min = min;
			this.max = max;
			MakeRange(min.ToString(), max.ToString());
		}
		
		public override bool IsValidChar(char c) {
			return (c >= '0' && c <= '9') || c == '-' || c == '.' || c == ',';
		}
		
		public override bool IsValidString(string s) {
			float value;
			if (s.Length == 1 && IsValidChar(s[0])) return true;
			return Utils.TryParseDecimal(s, out value);
		}
		
		public override bool IsValidValue(string s) {
			float value;
			if (!Utils.TryParseDecimal(s, out value)) return false;
			return min <= value && value <= max;
		}
	}
	
	public sealed class PathValidator : MenuInputValidator {
		
		public PathValidator() {
			Range = "&7(Enter name)";
		}
		
		public override bool IsValidChar(char c) {
			return !(c == '/' || c == '\\' || c == '?' || c == '*' || c == ':'
			         || c == '<' || c == '>' || c == '|' || c == '"' || c == '.');
		}
		
		public override bool IsValidString(string s) { return true; }
	}
	
	public sealed class BooleanValidator : MenuInputValidator {
		
		public BooleanValidator() {
			Range = "&7(yes or no)";
		}
		
		public override bool IsValidChar(char c) { return true; }
		
		public override bool IsValidString(string s) { return true; }
		
		public override bool IsValidValue(string s) { return true; }
	}
	
	public sealed class EnumValidator : MenuInputValidator {
		
		public Type EnumType;
		public EnumValidator(Type type) {
			EnumType = type;
		}
		
		public override bool IsValidChar(char c) { return true; }
		
		public override bool IsValidString(string s) { return true; }
		
		public override bool IsValidValue(string s) { return true; }
	}
	
	public sealed class StringValidator : MenuInputValidator {
		
		public StringValidator() {
			Range = "&7(Enter text)";
		}
		
		public override bool IsValidChar(char c) {
			return c != '&' && Utils.IsValidInputChar(c, true);
		}
		
		public override bool IsValidString(string s) {
			return s.Length <= Utils.StringLength;
		}
	}
}
