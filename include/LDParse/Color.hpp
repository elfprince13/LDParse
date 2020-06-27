//
//  Color.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/19/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <LDParse/Lex.hpp>

#include <immer/vector.hpp>

namespace LDParse {
	struct RGB : std::array<uint8_t, 3>
	{
		using std::array<uint8_t, 3>::operator[];
		
		uint8_t& r();
		const uint8_t& r() const;
		uint8_t& g();
		const uint8_t& g() const;
		uint8_t& b();
		const uint8_t& b() const;
	};
	
	struct ColorData {
		RGB color;
		std::optional<uint8_t> alpha;
		std::optional<uint8_t> luminance;
		std::optional<TokenStream> finish;
	};
	
	struct Color : ColorData {
		std::string name;
		uint16_t code;
		std::variant<uint16_t, ColorData> edge;
		using ColorData::color;
		using ColorData::alpha;
		using ColorData::luminance;
		using ColorData::finish;
		
	};
	
	struct ColorScope : protected immer::vector<std::map<uint16_t,Color>> {
		using ColorMap = std::map<uint16_t, Color>;
		using ColorMapStack = immer::vector<ColorMap>;
	protected:
		using ColorMapStack::push_back;
		ColorMap active_;
		ColorMapStack& contents();
		const ColorMapStack& contents() const;
	public:
		ColorScope();
		ColorScope(const ColorMapStack& stack);
		ColorScope(const ColorScope&) = default;
		ColorScope(ColorScope&&) = default;
		ColorScope& operator=(const ColorScope&) = default;
		ColorScope& operator=(ColorScope&&) = default;
		
		void commit();
		void record(const Color&);
		std::optional<std::reference_wrapper<const ColorData>> find(uint16_t code, bool findEdge = false) const;
	};
	
	struct ColorTable {
		uint16_t mNextFree;
		std::vector<uint32_t> mColors;
		std::vector<uint16_t> mComplements;
		
		ColorTable();
		std::optional<uint32_t> getColour(uint16_t code) const;
		bool setColour(uint16_t code, uint32_t color);
		bool setComplement(uint16_t code, uint16_t cCode);
		std::optional<uint16_t> addLocalColour(uint32_t);
		
	};
	
}

