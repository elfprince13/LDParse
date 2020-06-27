//
//  Color.cpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/19/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#include <LDParse/Color.hpp>

#include <limits>
#include <stdexcept>

namespace LDParse {
	uint8_t& RGB::r() { return (*this)[0]; }
	const uint8_t& RGB::r() const  { return (*this)[0]; }
	uint8_t& RGB::g() { return (*this)[1]; }
	const uint8_t& RGB::g() const  { return (*this)[1]; }
	uint8_t& RGB::b() { return (*this)[2]; }
	const uint8_t& RGB::b() const { return (*this)[2]; }
	
	ColorScope::ColorScope()
	: ColorMapStack()
	, active_() {}
	
	ColorScope::ColorScope(const ColorMapStack& stack)
	: ColorMapStack(stack)
	, active_() {}
	
	ColorScope::ColorMapStack& ColorScope::contents() {
		return (ColorMapStack&)(*this);
	}
	
	const ColorScope::ColorMapStack& ColorScope::contents() const {
		return (ColorMapStack&)(*this);
	}
	
	void ColorScope::commit(){
		if(active_.size()) {
			contents() = push_back(ColorMap(std::move(active_)));
			active_.clear();
		}

	}
	
	void ColorScope::record(const Color& color) {
		active_.try_emplace(color.code, color);
	}
	
	std::optional<std::reference_wrapper<const ColorData>> ColorScope::find(uint16_t code, bool findEdge) const {
		if(active_.size()) {
			throw std::logic_error("You must not perform color lookups between record and commit");
		} else {
			// This doesn't map to std::find_if without duplicating work
			const auto oldestIt = contents().rend();
			const auto firstIt = contents().rbegin();
			for(auto mapIt = firstIt; mapIt != oldestIt; ++mapIt) {
				const auto& colorMap = *mapIt;
				const auto lastColor = colorMap.end();
				if(auto foundIt = colorMap.find(code);
				   foundIt == lastColor) {
					continue;
				} else {
					const Color& found = foundIt->second;
					if(findEdge) {
						if(auto& edge = found.edge;
						   std::holds_alternative<ColorData>(edge)) {
							return std::get<ColorData>(edge);
						} else {
							auto edgeCode = std::get<uint16_t>(edge);
							if(auto edgeIt = colorMap.find(edgeCode);
							   edgeIt == lastColor) {
								code = edgeCode;
								findEdge = false;
								continue;
							} else {
								const Color& edgeColor = edgeIt->second;
								return std::cref((const ColorData&)edgeColor);
							}
						}
					} else {
						return std::cref((const ColorData&)found);
					}
				}
			}
			return std::nullopt;
		}
	}
	
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
