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
#include "Geom.hpp"

namespace LDParse{
	
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
