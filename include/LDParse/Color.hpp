//
//  Color.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/19/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#ifndef Color_hpp
#define Color_hpp

#include <stddef.h>
#include <vector>
#include <boost/optional.hpp>

namespace LDParse {
	struct ColorTable {
		uint16_t mNextFree;
		std::vector<uint32_t> mColors;
		std::vector<uint16_t> mComplements;
		
		ColorTable();
		boost::optional<uint32_t> getColour(uint16_t code) const;
		bool setColour(uint16_t code, uint32_t color);
		bool setComplement(uint16_t code, uint16_t cCode);
		boost::optional<uint16_t> addLocalColour(uint32_t);
		
	};
	
}


#endif /* Color_hpp */
