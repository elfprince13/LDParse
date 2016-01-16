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
	
	const std::string Token::textRepr() const {
		std::ostringstream out("");
		switch(k){
			case Zero...Five:
			case DecInt:
			case HexInt:
				out << boost::get<int32_t>(v);
				break;
			case Float:
				out << boost::get<float>(v);
				break;
			case Orientation:
				out << (boost::get<Winding>(v) ? "CW" : "CCW");
				break;
			case Ident:
			case Garbage:
				out << boost::get<std::string>(v);
				break;
			default:
				out << boost::get<const char *>(v);
		}
		return out.str();
	}
	
	std::ostream &operator<<(std::ostream &out, const Token &o) {
		out << "(" << o.k << " " << o.textRepr() << ")\t";
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

	
	bool Lexer::lexLine(TokenStream &lineV, LexState start) {
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
						if(lineStream.fail()){
							break; // We weren't EOL yet, but only whitespace was left.
						} else {
							bool push = true;
							auto kindIt = keywordMap.find(tokText);
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
											} else {
												goto LEXED_GARBAGE;
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
											goto LEXED_GARBAGE;
										}
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
										cur.v = kindIt->first.c_str() /* This should persist, because it's in the table */;	break;
									case Orientation:
										cur.v = (tokText.length() - 2) ? CCW : CW;	break;
									default:
										push = false;
										mErrHandler("Found keyword entry in Lexer::keywordMap, but unknown keyword", tokText, true);
								}
							}
							if(push) lineV.push_back(cur);
						}
				}
			}
		}
		return ret;
	}
	
	bool Lexer::lexModelBoundaries(ModelStream &models, std::string &root, bool rewind){
		typedef enum : uint8_t {
			First = 0,
			Second = 1,
			YTail = 2,
			ITail = 3,
			NTail = 4
		} LineState;
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
		
		auto storeFile = [&](bool cleanup = false){
			models.push_back(std::make_pair(fileName, fileContents));
			if(fileCt == 1 || (cleanup && fileCt == 0)) root = fileName;
			fileName = "";
			fileContents.clear();
		};
		
		while(lexLine(lineContents, inFile ? Lex : Discard)){
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
					case YTail:
						if(fileName.length()) fileName += " ";
						fileName += it->textRepr();
						break;
					default:
						mErrHandler("Loop invariant failed. Something is wrong,", it->textRepr(), false);
				}
			}
			if(inFile || state == NTail){
				fileContents.push_back(lineContents);
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
};