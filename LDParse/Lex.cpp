//
//  Lex.cpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/13/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#include "Lex.hpp"
#include <sstream>


namespace LDParse {
	/*
	 
	Zero, One, Two, Three, Four, Five,
	Step, Pause, Write, Print, Clear, Save,
	Colour, Code, Value, Edge, Alpha, Luminance,
	Chrome, Pearlescent, Rubber, MatteMetallic, Metal, Material, 
	 File, NoFile,
	BFC, Certify, NoCertify, Clip, NoClip, Orientation, InvertNext,
	HexInt, DecInt, Float, Ident, Garbage, T_EOF
	
	*/
	
	std::ostream &operator<<(std::ostream &out, const Token &o) {
		out << "(" << o.k << " ";
		switch(o.k){
			case Zero...Five:
			case DecInt:
			case HexInt:
				out << o.v.i;
				break;
			case Float:
				out << o.v.f;
				break;
			case Orientation:
				out << (bool)(o.v.o);
				break;
			case Ident:
			case Garbage:
				out << o.v.s;
				break;
			default:
				out << o.v.kw;
		}
		out << ")\t";
		return out;
		}
	
	const std::map<std::string, TokenKind> Lexer::keywordMap = {
		{"0", Zero}, {"1", One}, {"2", Two}, {"3", Three}, {"4", Four}, {"5",Five},
		{"STEP", Step}, {"PAUSE", Pause}, {"WRITE", Write}, {"PRINT", Write}, {"CLEAR", Clear}, {"SAVE", Save},
		{"!COLOUR", Colour}, {"CODE", Code}, {"VALUE", Value}, {"EDGE", Edge}, {"ALPHA", Alpha}, {"LUMINANCE", Luminance},
		{"CHROME", Chrome}, {"PEARLESCENT", Pearlescent}, {"RUBBER", Rubber}, {"MATTE_METALLIC", MatteMetallic}, {"METAL", Metal}, {"MATERIAL", Material},
		{"FILE", File}, {"NOFILE", NoFile},
		{"BFC", BFC}, {"CERTIFY", Certify}, {"NOCERTIFY", NoCertify}, {"CLIP", Clip}, {"NOCLIP", NoClip},
		{"CW", Orientation}, {"CCW", Orientation}, {"INVERTNEXT", InvertNext}
	};

	
	std::vector<Token> Lexer::lexLine(LexState start) {
		LexState state = start;
		std::vector<Token> ret;
		Token cur;
		ret.clear();
		if(mInput.eof()){
			cur.k = T_EOF;
			ret.push_back(cur);
		} else {
			std::string line;
			std::string tokText("");
			safeGetline(mInput, line);
			std::istringstream lineStream(line);
			const std::streampos BOL = lineStream.tellg();
			while(!(lineStream.peek() == EOF || lineStream.eof())){
				switch (state) {
					case String:
						char next;	lineStream.get(next);
						switch(next){
							case '"':
								cur.k = Ident;
								cur.v.s = std::string(tokText);
								lineStream >> std::skipws;
								ret.push_back(cur);
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
						safeGetline(lineStream, tokText);
						cur.v.s = std::string(tokText);
						cur.k = Garbage;
						ret.push_back(cur);
						break;
						
					default:
						const std::streampos posHere = lineStream.tellg();
						lineStream >> tokText;
						auto kindIt = keywordMap.find(tokText);
						bool push = true;
						if(kindIt == keywordMap.end()) {
							switch(tokText[0]) {
								case '"':
									lineStream.clear();
									lineStream.seekg(posHere);
									lineStream >> std::noskipws;
									lineStream.ignore(1,'"');
									if(lineStream.gcount() == 1) {
										state = String;
										tokText = "";
									} else {
										mErrHandler("Something has gone very wrong while parsing a string", tokText, true);
									}
									push = false;
									break;
								case '#':
									cur.k = HexInt;
									push = parseHexFromOffset(tokText, 1, cur.v.i);
									break;
								case '0':
									// We know this can't be a single 0, or else we would have recognized it as a key word!
									if(tokText[1] == 'x'){
										cur.k = HexInt;
										push = parseHexFromOffset(tokText, 2, cur.v.i);
										break;
									}
								case '1'...'9':
								case '+':
								case '-':
								case '.': {
									cur.k = Float;
									bool parsed = parseFloat(tokText, cur.v.f);
									if(parsed){
										uint32_t intVal = cur.v.f;
										if(intVal == cur.v.f){
											cur.k = DecInt;
											cur.v.i = intVal;
										}
									} else {
										goto LEXED_GARBAGE;
									}
									break;
								}
								case 'a'...'z':
								case 'A'...'Z':
									if(find_if(tokText.begin(), tokText.end(),
											   [](char c) { return !isalnum(c); }) == tokText.end()) {
										cur.k = Ident;
										cur.v.s = std::string(tokText);
										break;
									}
								default:
								LEXED_GARBAGE:
									cur.k = Garbage;
									cur.v.s = std::string(tokText);
							}
						} else {
							const TokenKind kind = kindIt->second;
							cur.k = kind;
							switch(kind){
								case Zero: case One: case Two: case Three: case Four: case Five:
									cur.v.i = kind;
								case Step: case Pause: case Write /*Print*/: case Clear: case Save:
								case Colour: case Code: case Value: case Edge: case Alpha: case Luminance:
								case Chrome: case Pearlescent: case Rubber: case MatteMetallic: case Metal: case Material:
								case File: case NoFile:
								case BFC: case Certify: case NoCertify: case Clip: case NoClip: case InvertNext:
									cur.v.kw = kindIt->first.c_str(); // This should persist, because it's in the table
									break;
								case Orientation:
									cur.v.o = (tokText.length() - 2) ? CCW : CW;
									break;
								default:
									push = false;
									mErrHandler("Found keyword entry in Lexer::keywordMap, but unknown keyword", tokText, true);
							}
						}
						if(push) ret.push_back(cur);
				}
			}
			cur.v.clear();
		}
		
		return ret;
	}
};