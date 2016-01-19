//
//  Model.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/18/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#ifndef Model_h
#define Model_h

#include "Cache.hpp"
#include "Geom.hpp"
#include "Parse.hpp"

namespace LDParse{

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

#endif /* Model_h */
