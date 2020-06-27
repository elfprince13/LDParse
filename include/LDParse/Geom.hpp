//
//  Geom.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/15/16.
//  Copyright © 2016 - 2020 StickFigure Graphic Productions. All rights reserved.
//

#ifndef Geom_hpp
#define Geom_hpp

#include <stdio.h>
#include <tuple>
#include <vector>
#include <string>
#include <map>

namespace LDParse{
	
	using ColorRef = std::pair<bool, uint32_t>;
	using Position = std::tuple<float, float, float>;
	using Line = std::tuple<Position, Position, Position>;
	using Triangle = std::tuple<Position, Position, Position>;
	using Quad = std::tuple<Position, Position, Position, Position>;
	using OptLine = std::tuple<Line, Line>;
	using TransMatrix = std::tuple<Position,
	float, float, float,
	float, float, float,
	float, float, float>;
	
	
	template<typename ...AttrTypes> class Mesh {
	public:
		using SelfType = Mesh<AttrTypes...>;
		using AttrsType = std::tuple<std::vector<AttrTypes> ...>;
		template<size_t i> using AttrType = typename std::tuple_element<i, AttrsType>::type;
		AttrsType attributes;
		std::vector<uint32_t> indices;
		std::vector<uint32_t> bfIndices; // We may store reverse copies of certain triangles
		
		template<typename ...TxFormFs> void mergeMesh(const SelfType &merge, std::tuple<TxFormFs ...> txformFs = std::make_tuple(identity<AttrTypes>() ...)){
			const size_t offset = vertexCount();
			//size_t indexCt = indices.size();
			mergeHelper(merge, txformFs);
			const auto offsetF = [](uint32_t &index){ index += offset; };
			appendVector(indices, merge.indices, offsetF);
			appendVector(bfIndices, merge.bfIndices, offsetF);
			//size_t niCt = indices.size();
			/*for(size_t i = indexCt; i < niCt; ++i){
				indices[i] += offset;
			}*/
		}
		
		size_t vertexCount() const {
			return std::get<0>(attributes).size();
		}
		
		size_t rangeAll() const {
			return std::make_pair(0, vertexCount());
		}
		
		template<typename T> using IdentityF = void(*)(const T&);
		template<typename T> static const IdentityF<T> identity(){ return nullptr; }
		
		template<size_t i> inline void transform(void (*txformF)(AttrType<i>&)){ transform(txformF, rangeAll()); }
		template<size_t i> void transform(void (*txformF)(AttrType<i>&), const std::pair<size_t, size_t> &range){
			transform(std::get<i>(attributes), txformF, range);
		}
		
		template<typename T> inline void transform(std::vector<T> &v, void (*txformF)(T&), const std::pair<size_t, size_t> &range){
			if(txformF != nullptr) for(size_t j = range.first; j < range.second; ++j) txFormF(v[j]);
		}
		
	private:
		template<std::size_t> struct int_{};
		template<size_t N = std::tuple_size<AttrsType>::value> void mergeHelper(const SelfType &merge, std::tuple<void(AttrTypes&) ...> txformFs, int_<N> = int_<N>()){
			constexpr size_t i = std::tuple_size<AttrsType>::value - N;
			AttrType<i> &thisAttr = std::get<i>(attributes);
			const AttrType<i> &mergeAttr = std::get<i>(merge.attributes);
			
			appendVector(thisAttr, mergeAttr);
			
			mergeHelper(merge, txformFs, int_<N-1>());
		}
		
		void mergeHelper(const SelfType &, std::tuple<void(AttrTypes&) ...>, int_<0>){}
		
		template<typename T> void appendVector(std::vector<T> &l, const std::vector<T> &r, void(*txformF)(T&)){
			size_t begin = l.size();
			l.insert(l.end(), r.begin(), r.end());
			size_t end = l.size();
			transform(l, txformF, std::make_pair(begin, end));
		}
	};
}

#endif /* Geom_hpp */
