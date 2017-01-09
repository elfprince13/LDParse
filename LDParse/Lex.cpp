//
//  Lex.cpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/13/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
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
};
