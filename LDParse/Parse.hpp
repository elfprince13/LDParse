/*
 *  LDParse.hpp
 *  LDParse
 *
 *  Created by Thomas Dickerson on 1/13/16.
 *  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
 *
 */

#ifndef Parse_hpp
#define Parse_hpp

#include "Lex.hpp"

namespace LDParse{
	
	typedef std::tuple<float, float, float> Position;
	typedef std::tuple<	Position,
						float, float, float, float,
						float, float, float, float,
						float, float, float, float> TransMatrix;
	
	
	class Parser
	{
	private:
		Winding winding;
	public:
		
		Parser() : winding(CCW) {}
		
		bool parseModels(ModelStream models, std::string root);
		
		
		inline std::string coalesceText(TokenStream::const_iterator start, TokenStream::const_iterator end){
			std::string ret("");
			for(auto it = start; it != end; ++it) ret += it->textRepr();
			return ret;
		}
		
	};
	
}
#endif
