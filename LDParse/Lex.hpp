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

#include <boost/variant/variant.hpp>

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
	} OrientationT;
	
	typedef struct TVU { // A lot more memory intensive than need be. Investigate boost::variant options.
		const char * kw ;
		std::string s;
		OrientationT o;
		float f;
		int32_t i;
		TVU() { clear(); }
		void clear() {  kw = nullptr; s = ""; o = CCW; f = 0; i = 0; }
	} TokenValue;
	
	struct Token;
	
	std::ostream &operator<<(std::ostream &out, const Token &o);
	
	struct Token {
		TokenKind k;
		TokenValue v;
		friend std::ostream &operator<<(std::ostream &out, const Token &o);
	};
	
	class Lexer {
	public:
		
		typedef enum {
			Lex, String, Discard
		} LexState;
		
		static const std::map<std::string, TokenKind> keywordMap;
		
		typedef void (*ErrFType)(std::string msg, std::string tokText, bool fatal);
		
		std::istream& mInput;
		ErrFType mErrHandler;
		Lexer(std::istream &input, ErrFType errHandler) : mInput(input), mErrHandler(errHandler) {mInput >> std::noskipws;}
		std::vector<Token> lexLine(LexState start = Lex);
		
	private:
		
		bool parseHexFromOffset(std::string &src, size_t ofs, int32_t &dst){
			bool ret;
			int ct = sscanf(src.c_str() + ofs, "%x", &dst);
			if(ct != 1){
				mErrHandler("Couldn't parse hex int", src, true);
				ret = false;
			} else {
				ret = true;
			}
			return ret;
		}
		
		
		
		bool parseFloat(std::string &src, float &dst){
			bool ret;
			int ct = sscanf(src.c_str(), "%f", &dst);
			if(ct != 1){
				mErrHandler("Couldn't parse float", src, true);
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
