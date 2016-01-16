//
//  Lex.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/13/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#ifndef Lex_hpp
#define Lex_hpp

#include <stdio.h>
#include <stddef.h>
#include <iostream>
#include <vector>
#include <map>
#include <limits>

#include <boost/variant.hpp>

namespace LDParse {
	
	typedef enum : int32_t {
		Zero = 0, One = 1, Two = 2, Three = 3, Four = 4, Five = 5,
		Step, Pause, Write /*Print*/, Clear, Save,
		Colour, Code, Value, Edge, Alpha, Luminance,
		Chrome, Pearlescent, Rubber, MatteMetallic, Metal, Material,
		File, NoFile,
		BFC, Certify, NoCertify, Clip, NoClip, InvertNext, Orientation,
		HexInt = -6, DecInt = -5, Float = -4, Ident = -3, Garbage = -2, T_EOF = -1
	} TokenKind;
	
	typedef enum : bool {
		CCW = false,
		CW = true
	} Winding;
	
	
	struct Token;
	
	std::ostream &operator<<(std::ostream &out, const Token &o);
	
	typedef boost::variant<const char *, std::string, Winding, float, int32_t> TokenValue;
	struct Token {
		TokenKind k;
		TokenValue v;
		const std::string textRepr() const;
		friend std::ostream &operator<<(std::ostream &out, const Token &o);
		
		
		inline bool isNumber(Token &t){
			switch(k){
				case Zero...Five:
				case HexInt:
				case DecInt:
				case Float:
					return true;
				default:
					return false;
			}
		}
		inline float getNumber(Token &t){
			float ret;
			switch(k){
				case Zero...Five:
				case DecInt:
				case HexInt:
					ret = boost::get<int32_t>(v);
					break;
				case Float:
					ret = boost::get<float>(v);
					break;
				default:
					ret = std::numeric_limits<float>::quiet_NaN();
			}
			return ret;
		}
		
	};
	
	typedef std::vector<Token> TokenStream;
	typedef std::vector<TokenStream> LineStream;
	typedef std::vector<std::pair<std::string, LineStream> > ModelStream;
	
	class Lexer {
	public:
		typedef void (*ErrFType)(std::string msg, std::string tokText, bool fatal);
	private:
		std::istream& mInput;
		const std::streampos mBOF;
		ErrFType mErrHandler;
	public:
		typedef enum {
			Lex, String, Discard
		} LexState;
		
		static const std::map<std::string, TokenKind> keywordMap;
		
		
		Lexer(std::istream &input, ErrFType errHandler) : mInput(input), mErrHandler(errHandler), mBOF(mInput.tellg()) {mInput >> std::noskipws;}
		bool lexLine(TokenStream &line, LexState start = Lex);
		bool lexModelBoundaries(ModelStream &models, std::string &root, bool rewind = true);
		
	private:
		
		bool parseHexFromOffset(std::string &src, size_t ofs, int32_t &dst, bool expect = false){
			bool ret;
			ptrdiff_t charCt = src.length() - ofs;
			const char* src_ptr = src.c_str() + ofs;
			char* end_ptr;
			long lVal = strtol(src_ptr, &end_ptr, 16);
			if(charCt != (end_ptr - src_ptr)){
				if(expect) mErrHandler("Couldn't parse hex int", src, true);
				ret = false;
			} else {
				dst = (int32_t)lVal;
				if(dst != lVal) mErrHandler("Parsed a long, but it had to be truncated", src, false);
				ret = true;
			}
			return ret;
		}
		
		
		
		bool parseFloat(std::string &src, float &dst, bool expect = false){
			bool ret;
			ptrdiff_t charCt = src.length();
			const char* src_ptr = src.c_str();
			char* end_ptr;
			dst = strtof(src_ptr, &end_ptr);
			if(charCt != (end_ptr - src_ptr)){
				if(expect) mErrHandler("Couldn't parse float", src, true);
				ret = false;
			} else {
				ret = true;
			}
			return ret;
		}
		
		template<bool discard = false> std::istream& safeGetline(std::istream& is, std::string& t)
		{
			t.clear();
			
			// The characters in the stream are read one-by-one using a std::streambuf.
			// That is faster than reading them one-by-one using the std::istream.
			// Code that uses streambuf this way must be guarded by a sentry object.
			// The sentry object performs various tasks,
			// such as thread synchronization and updating the stream state.
			
			std::istream::sentry se(is, true);
			std::streambuf* sb = is.rdbuf();
			
			for(;;) {
				int c = sb->sbumpc();
				switch (c) {
					case '\n':
						return is;
					case '\r':
						if(sb->sgetc() == '\n')
							sb->sbumpc();
						return is;
					case EOF:
						// Also handle the case when the last line has no line ending
						if(t.empty())
							is.setstate(std::ios::eofbit);
						return is;
					default:
						if(!discard) t += (char)c;
				}
			}
		}
	};
}

#endif /* Lex_hpp */
