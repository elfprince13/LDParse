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
				out << boost::get<int32_t>(o.v);
				break;
			case Float:
				out << boost::get<float>(o.v);
				break;
			case Orientation:
				out << (bool)boost::get<OrientationT>(o.v);
				break;
			case Ident:
			case Garbage:
				out << boost::get<std::string>(o.v);
				break;
			default:
				out << boost::get<const char *>(o.v);
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

	
	bool Lexer::lexLine(std::vector<Token> &lineV, LexState start) {
		LexState state = start;
		bool ret = true;
		Token cur;
		lineV.clear();
		if(mInput.eof()){
			cur.k = T_EOF;
			lineV.push_back(cur);
			ret = false;
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
								cur.v = tokText;
								lineStream >> std::skipws;
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
						safeGetline(lineStream, tokText);
						cur.v = tokText;
						cur.k = Garbage;
						lineV.push_back(cur);
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
								case '#': {
									cur.k = HexInt;
									int32_t i;
									push = parseHexFromOffset(tokText, 1, i);
									cur.v = i;
									break;
								}
								case '0':
									// We know this can't be a single 0, or else we would have recognized it as a key word!
									if(tokText[1] == 'x'){
										cur.k = HexInt;
										int32_t i;
										push = parseHexFromOffset(tokText, 2, i);
										cur.v = i;
										break;
									}
								case '1'...'9':
								case '+':
								case '-':
								case '.': {
									float fVal;
									bool parsed = parseFloat(tokText, fVal);
									if(parsed){
										int32_t intVal = fVal;
										if(intVal == fVal){
											cur.k = DecInt;
											cur.v = intVal;
										} else {
											cur.k = Float;
											cur.v = fVal;
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
										cur.v = tokText;
										break;
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
									cur.v = kindIt->first.c_str(); // This should persist, because it's in the table
									break;
								case Orientation:
									cur.v = (tokText.length() - 2) ? CCW : CW;
									break;
								default:
									push = false;
									mErrHandler("Found keyword entry in Lexer::keywordMap, but unknown keyword", tokText, true);
							}
						}
						if(push) lineV.push_back(cur);
				}
			}
		}
		
		return ret;
	}
};