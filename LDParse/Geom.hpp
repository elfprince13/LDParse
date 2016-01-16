//
//  Geom.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/15/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#ifndef Geom_hpp
#define Geom_hpp

#include <stdio.h>
#include <tuple>
#include <vector>
#include <string>
#include <map>

namespace LDParse{
	
	typedef std::tuple<bool, uint32_t> ColorRef;
	typedef std::tuple<float, float, float> Position;
	typedef std::tuple<Position, Position, Position> Line;
	typedef std::tuple<Position, Position, Position> Triangle;
	typedef std::tuple<Position, Position, Position> Quad;
	typedef std::tuple<Line, Line> OptLine;
	typedef std::tuple<Position,
	float, float, float,
	float, float, float,
	float, float, float> TransMatrix;
	
	
	template<typename ...AttrTypes> class Mesh {
	public:
		typedef Mesh<AttrTypes...> SelfType;
		typedef std::tuple<std::vector<AttrTypes> ...> AttrsType;
		template<size_t i> using AttrType = typename std::tuple_element<i, AttrsType>::type;
		AttrsType attributes;
		std::vector<uint32_t> indices;
		
		std::pair<size_t, size_t> mergeMesh(const SelfType &merge, std::tuple<void(AttrTypes&) ...> txformFs = std::make_tuple(identity<AttrTypes>() ...)){
			const size_t offset = vertexCount();
			size_t indexCt = indices.size();
			mergeHelper(merge, txformFs);
			appendVector(indices, merge.indices, [](uint32_t &index){ index += offset; });
			size_t niCt = indices.size();
			/*for(size_t i = indexCt; i < niCt; ++i){
				indices[i] += offset;
			}*/
			return std::make_pair(indexCt, niCt);
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
		template<size_t N = std::tuple_size<AttrsType>::value> void mergeHelper(const SelfType &merge, std::tuple<void(AttrTypes&) ...> txformFs, int_<N> elemCt = int_<N>()){
			constexpr size_t i = std::tuple_size<AttrsType>::value - N;
			AttrType<i> &thisAttr = std::get<i>(attributes);
			const AttrType<i> &mergeAttr = std::get<i>(merge.attributes);
			
			appendVector(thisAttr, mergeAttr);
			
			mergeHelper(merge, txformFs, int_<N-1>());
		}
		void mergeHelper(const SelfType &merge, std::tuple<void(AttrTypes&) ...> txformFs, int_<0>){}
		
		template<typename T> void appendVector(std::vector<T> &l, const std::vector<T> &r, void(*txformF)(T&)){
			size_t begin = l.size();
			l.insert(l.end(), r.begin(), r.end());
			size_t end = l.size();
			transform(l, txformF, std::make_pair(begin, end));
		}
		
		
	};
	
	typedef Mesh<Position, Position, uint32_t> LDMesh;
	
	typedef enum : uint8_t {
		Primitive,
		Part,
		Model,
		MPDRoot,
		MPDSub
	} SrcType;
	
	typedef enum : int8_t {
		BFCOff = -1,
		Standard = 0,
		Invert = 1
	} BFCStatus;
	
	class Model {
		std::string name;
		std::string srcLoc;
		SrcType srcType;
		
		
		LDMesh data;
		size_t color;
		std::vector<std::tuple<size_t, BFCStatus, TransMatrix,Model*> > children;
		std::map<Model*, std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>> > childOffsets;
		
	};
	
}

#endif /* Geom_hpp */
