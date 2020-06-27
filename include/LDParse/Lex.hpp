//
//  Lex.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/13/16.
//  Copyright © 2016 - 2020 StickFigure Graphic Productions. All rights reserved.
//

#pragma once

#include <cstddef>
#include <iostream>
#include <limits>
#include <locale>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include <boost/algorithm/string/trim.hpp>

namespace LDParse {
	
	enum TokenKind : int32_t {
		Zero = 0, One = 1, Two = 2, Three = 3, Four = 4, Five = 5,
		Step, Pause, Write /*Print*/, Clear, Save,
		Colour, Code, Value, Edge, Alpha, Luminance,
		// Finishes are assumed to be consecutive, between Chrome and Material
		Chrome, Pearlescent, Rubber, MatteMetallic, Metal, Material,
		File, NoFile,
		BFC, Certify, NoCertify, Clip, NoClip, InvertNext, Orientation,
		HexInt = -6, DecInt = -5, Float = -4, Ident = -3, Garbage = -2, T_EOF = -1
	};
	
	enum Winding : bool {
		CCW = false,
		CW = true
	};
		
		
	struct Token;
	
	std::ostream &operator<<(std::ostream &out, const Token &o);
	
	using TokenValue = std::variant<const char *, std::string, Winding, float, int32_t>;
	
	struct Token {
		TokenKind k;
		TokenValue v;
		size_t l;
		size_t c;
		const std::string textRepr(bool showPos = false) const;
		friend std::ostream &operator<<(std::ostream &out, const Token &o);
		
		
		inline bool isNumber() const {
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
		
		inline float getNumber() const {
			float ret;
			switch(k){
				case Zero...Five:
				case DecInt:
				case HexInt:
					ret = std::get<int32_t>(v);
					break;
				case Float:
					ret = std::get<float>(v);
					break;
				default:
					ret = std::numeric_limits<float>::quiet_NaN();
			}
			return ret;
		}
		
	};
	
	using TokenStream = std::vector<Token>;
	using LineStream = std::vector<std::pair<std::string, TokenStream> >;
	using ModelStream = std::vector<std::pair<std::string, LineStream> >;
	
	namespace detail {
		std::unordered_multimap<TokenKind, std::string, std::hash<uint32_t> > invertMap(const std::unordered_map<std::string, TokenKind> &src);
	}
		
	const std::unordered_map<std::string, TokenKind>& keywordMap();
	const std::unordered_multimap<TokenKind, std::string, std::hash<uint32_t> >& keywordRevMap();
		
	template<typename ErrFType>
	class Lexer {
		std::istream& mInput;
		const std::streampos mBOF;
		size_t mLineNo;
		ErrFType mErrHandler;
		
	public:
		enum LexState {
			Lex, String, Discard
		};

		
		
		Lexer(std::istream &input, ErrFType errHandler)
		: mInput(input), mBOF(mInput.tellg()),
		mLineNo(0), mErrHandler(errHandler) {mInput >> std::noskipws;}
		
		bool lexLine(std::string &lineT, TokenStream &line, LexState start = Lex);
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
		
		std::istream& skipWSCounting(std::istream& is, size_t& colNo){
			std::istream::sentry(is, true);
			std::streambuf* sb = is.rdbuf();
			const std::locale loc = sb->getloc();
			char c;
			while((c = sb->sgetc()) != EOF && isspace(c, loc)){
				sb->sbumpc();
				colNo++;
			}
			if(c == EOF) is.setstate(std::ios::eofbit);
			return is;
		}
		
		template<bool discard = false> std::istream& safeGetline(std::istream& is, size_t& n, std::string& t, bool needsInc = true)
		{
			t.clear();
			
			// The characters in the stream are read one-by-one using a std::streambuf.
			// That is faster than reading them one-by-one using the std::istream.
			// Code that uses streambuf this way must be guarded by a sentry object.
			// The sentry object performs various tasks,
			// such as thread synchronization and updating the stream state.
			
			std::istream::sentry se(is, true);
			std::streambuf* sb = is.rdbuf();
			n = mLineNo;
			if(needsInc) mLineNo += 1;
			
			for(;;) {
				int c = sb->sbumpc();
				switch (c) {
					case '\r':
						if(sb->sgetc() == '\n')
							sb->sbumpc();
					case '\n':
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
	
	
	template<typename ErrFType>
	bool Lexer<ErrFType>::lexLine(std::string &line, TokenStream &lineV, LexState start) {
		constexpr static const std::string_view AllowedSpecialChars("._,-~/\\#:()[]");
		LexState state = start;
		bool ret = true;
		Token cur;
		lineV.clear();
		if(mInput.eof()){
			cur.l = mLineNo;
			cur.c = 0;
			cur.k = T_EOF;
			lineV.push_back(cur);
			ret = false;
		} else {
			std::string tokText("");
			size_t lineNo;
			size_t colNo = 0;
			safeGetline(mInput, lineNo, line);
			std::istringstream lineStream(line);
			const std::streampos BOL = lineStream.tellg();
			lineStream >> std::noskipws;
			while(!(lineStream.peek() == EOF || lineStream.eof())){
				switch (state) {
					case String:
						char next;	lineStream.get(next);
						switch(next){
							case '"':
								cur = {Ident, tokText, lineNo, colNo};
								colNo += tokText.length() + 2;
								lineV.push_back(cur);
								break;
							default:
								tokText += next;
						}
						break;
					case Discard:
						if(BOL == lineStream.tellg()){
							char linePref[7];
							lineStream.read(linePref, 6);
							lineStream.clear();
							lineStream.seekg(BOL);
							if(lineStream.gcount() == 6 && strcmp("0 FILE", linePref) == 0){
								state = Lex;
								break;
							}
						}
						safeGetline(lineStream, lineNo, tokText, false);
						cur = {Garbage, tokText, lineNo, colNo};
						colNo += tokText.length();
						lineV.push_back(cur);
						break;
						
					default:
						const std::streampos posHere = lineStream.tellg();
						skipWSCounting(lineStream, colNo);
						lineStream >> tokText;
						if(lineStream.fail()){
							break; // We weren't EOL yet, but only whitespace was left.
						} else {
							bool push = true;
							auto kindIt = keywordMap().find(tokText);
							if(kindIt == keywordMap().end()) {
								switch(tokText[0]) {
									case '"':
										lineStream.clear();
										lineStream.seekg(posHere);
										lineStream.ignore(1,'"');
										if(lineStream.gcount() == 1) {
											state = String;
											tokText = "";
										} else {
											mErrHandler("Something has gone very wrong while parsing a string", tokText, true);
										}
										push = false;
										break;
									case '#': {
										int32_t i;
										if (parseHexFromOffset(tokText, 1, i)){
											cur.k = HexInt;
											cur.v = i;
											break;
										} else {
											goto LEXED_GARBAGE;
										}
									}
									case '0':
										// We know this can't be a single 0, or else we would have recognized it as a key word!
										if(tokText[1] == 'x'){
											int32_t i;
											if (parseHexFromOffset(tokText, 2, i)){
												cur.k = HexInt;
												cur.v = i;
												break;
											}
										}
									case '1'...'9':
									case '+':
									case '-':
									case '.': {
										float fVal;
										if(parseFloat(tokText, fVal)){
											int32_t intVal = fVal;
											if(intVal == fVal){
												cur.k = DecInt;
												cur.v = intVal;
											} else {
												cur.k = Float;
												cur.v = fVal;
											}
											break;
										} else {
											switch(tokText[0]){
												case '0'...'9':
													break;
												case '+':
												case '-':
												case '.':
													goto LEXED_GARBAGE;
												default:
													mErrHandler("Lexer has experienced disallowed fall-through", tokText, true);
											}
										}
									}
									case 'a'...'z':
									case 'A'...'Z': {
										const std::string::const_iterator garbage_pos = find_if_not(tokText.begin(), tokText.end(),
																	 [](char c) { return isalnum(c) || (AllowedSpecialChars.find(c) != std::string::npos); });
										if(garbage_pos == tokText.end()) {
											cur.k = Ident;
											cur.v = tokText;
											break;
										} /* 
										else {
											mErrHandler("Started lexing ident, found apparent garbage instead", std::string(1, *garbage_pos), false);
										}
										//*/
									}
									default:
									LEXED_GARBAGE:
										cur.k = Garbage;
										cur.v = tokText;
								}
							} else {
								const TokenKind kind = kindIt->second;
								cur.k = kind;
								switch(kind){
									case Zero: case One: case Two: case Three: case Four: case Five:
										cur.v = kind; break;
									case Step: case Pause: case Write /*Print*/: case Clear: case Save:
									case Colour: case Code: case Value: case Edge: case Alpha: case Luminance:
									case Chrome: case Pearlescent: case Rubber: case MatteMetallic: case Metal: case Material:
									case File: case NoFile:
									case BFC: case Certify: case NoCertify: case Clip: case NoClip: case InvertNext:
										cur.v = kindIt->first.c_str() /* This should persist, because it's in the table */;	break;
									case Orientation:
										cur.v = (tokText.length() - 2) ? CCW : CW;	break;
									default:
										push = false;
										mErrHandler("Found keyword entry in Lexer::keywordMap, but unknown keyword", tokText, true);
								}
							}
							if(push){
								cur.l = lineNo;
								cur.c = colNo;
								colNo += tokText.length();
								lineV.push_back(cur);
							}
						}
				}
			}
		}
		return ret;
	}
	
	template<typename ErrFType>
	bool Lexer<ErrFType>::lexModelBoundaries(ModelStream &models, std::string &root, bool rewind){
		enum LineState : uint8_t {
			First = 0,
			Second = 1,
			YTail = 2,
			ITail = 3,
			NTail = 4
		};
		static std::locale lmbLocale;
		if(!(mInput.tellg() == mBOF || rewind)){
			mErrHandler("Lexing model boundaries, but not at beginning of file and not asked to rewind", "", false);
		} else if (rewind) {
			mInput.clear(); mInput.seekg(mBOF);
		}
		LineStream fileContents;
		TokenStream lineContents;
		models.clear();
		fileContents.clear();
		lineContents.clear();
		std::string fileName(root);
		bool inFile = true;
		size_t fileCt = 0;
		std::string lineT("");
		
		auto storeFile = [&](bool cleanup = false){
			models.push_back(std::make_pair(fileName, fileContents));
			if(fileCt == 1 || (cleanup && fileCt == 0)) root = fileName;
			fileName = "";
			fileContents.clear();
		};
		
		while(lexLine(lineT, lineContents, inFile ? Lex : Discard)){
			LineState state = First;
			for(auto it = lineContents.begin(); state < ITail && it != lineContents.end(); ++it){
				switch(state){
					case First:
						switch(it->k){
							case Zero:
								state = Second;
								break;
							default:
								state = /*N*/ITail;
						}
						break;
					case Second:
						switch(it->k){
							case File:
								if(fileCt && inFile) storeFile();
								if(!(fileCt++) && fileContents.size()){
									mErrHandler("LDR commands found before first model. Previous commands will be discarded", it->textRepr(), false);
									fileContents.clear();
								}
								fileName = "";
								inFile = true;
								state = YTail;
								break;
							case NoFile: {
								bool pInFile = inFile;
								inFile = false;
								if(fileCt){
									if(pInFile){
										state = NTail;
										break;
									}
								} else {
									mErrHandler("MPD command found before first model. Previous commands will be discarded",it->textRepr(), false);
									fileContents.clear();
								}
							}
							default:
								state = ITail;
						}
						break;
					case YTail: {
						fileName = lineT.substr(it->c);
						boost::trim_right(fileName, lmbLocale);
						it = lineContents.end() - 1; // ++ off the deep end.
						break;
					}
					default:
						mErrHandler("Loop invariant failed. Something is wrong,", it->textRepr(), false);
				}
			}
			if(inFile || state == NTail){
				fileContents.push_back(std::make_pair(lineT,lineContents));
			}
			
			if(fileContents.size() && !inFile){
				storeFile();
			}
		}
		
		if (inFile) {
			storeFile(true);
		}
		
		return fileCt;
	}
	
	using ErrF = void (*)(std::string msg, std::string tokText, bool fatal);
	
	using CallbackLexer = Lexer<ErrF>;
}
