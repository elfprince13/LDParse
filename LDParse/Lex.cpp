//
//  Lex.cpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/13/16.
//  Copyright © 2016 - 2020 StickFigure Graphic Productions. All rights reserved.
//

#include <LDParse/Lex.hpp>

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
	
	const std::string Token::textRepr(bool showPos) const {
		std::ostringstream out("");
		if(showPos){
			out << (l+1) << ":" << (c+1) << " : ";
		}
		switch(k){
			case Zero...Five:
			case DecInt:
			case HexInt:
				out << std::get<int32_t>(v);
				break;
			case Float:
				out << std::get<float>(v);
				break;
			case Orientation:
				out << (std::get<Winding>(v) ? "CW" : "CCW");
				break;
			case Ident:
			case Garbage:
				out << std::get<std::string>(v);
				break;
			default:
				out << std::get<const char *>(v);
		}
		return out.str();
	}
	
	std::ostream &operator<<(std::ostream &out, const Token &o) {
		out << "(" << o.k << " " << o.textRepr() << ")\t";
		return out;
	}
	
	namespace detail {
		std::unordered_multimap<TokenKind, std::string, std::hash<uint32_t> > invertMap(const std::unordered_map<std::string, TokenKind> &src){
			std::unordered_multimap<TokenKind, std::string, std::hash<uint32_t> > dst;
			for(auto it = src.begin(); it != src.end(); ++it){
				dst.insert(std::make_pair(it->second, it->first));
			}
			return dst;
		}
	}
	
	const std::unordered_map<std::string, TokenKind>& keywordMap() {
		const static std::unordered_map<std::string, TokenKind> keywordMap_ = {
			{"0", Zero}, {"1", One}, {"2", Two}, {"3", Three}, {"4", Four}, {"5",Five},
			{"STEP", Step}, {"PAUSE", Pause}, {"WRITE", Write}, {"PRINT", Write}, {"CLEAR", Clear}, {"SAVE", Save},
			{"!COLOUR", Colour}, {"CODE", Code}, {"VALUE", Value}, {"EDGE", Edge}, {"ALPHA", Alpha}, {"LUMINANCE", Luminance},
			{"CHROME", Chrome}, {"PEARLESCENT", Pearlescent}, {"RUBBER", Rubber}, {"MATTE_METALLIC", MatteMetallic}, {"METAL", Metal}, {"MATERIAL", Material},
			{"FILE", File}, {"NOFILE", NoFile},
			{"BFC", BFC}, {"CERTIFY", Certify}, {"NOCERTIFY", NoCertify}, {"CLIP", Clip}, {"NOCLIP", NoClip},
			{"CW", Orientation}, {"CCW", Orientation}, {"INVERTNEXT", InvertNext}
		};
		return keywordMap_;
	}
	
	const std::unordered_multimap<TokenKind, std::string, std::hash<uint32_t> >& keywordRevMap() {
		const static std::unordered_multimap<TokenKind, std::string, std::hash<uint32_t> > keywordRevMap_ = detail::invertMap(keywordMap());
		return keywordRevMap_;
	}
};
