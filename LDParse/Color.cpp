//
//  Color.cpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/19/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#include <LDParse/Color.hpp>

#include <limits>

namespace LDParse {
	ColorTable::ColorTable() : mNextFree(512), mColors(mNextFree, 0), mComplements(mNextFree, 0) { }
	
	std::optional<uint32_t> ColorTable::getColour(uint16_t code) const{
		std::optional<uint32_t> ret = std::nullopt;
		if(code < mNextFree) ret = mColors[code];
		return ret;
	}
	
	bool ColorTable::setColour(uint16_t code, uint32_t color){
		bool ret = true;
		if(code < mNextFree) mColors[code] = color;
		else ret = false;
		return ret;
	}
	
	bool ColorTable::setComplement(uint16_t code, uint16_t cCode){
		bool ret = true;
		if(code < mNextFree) mComplements[code] = cCode;
		else ret = false;
		return ret;
	}
	
	std::optional<uint16_t> ColorTable::addLocalColour(uint32_t color){
		std::optional<uint16_t> ret = std::nullopt;
		if(mNextFree < std::numeric_limits<uint16_t>::max()){
			mColors.push_back(color);
			mComplements.push_back(0);
			ret = mNextFree++;
		}
		return ret;
	}
}
