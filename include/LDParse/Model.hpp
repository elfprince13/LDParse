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
#include "Color.hpp"
#include "Geom.hpp"
#include "Lex.hpp"
#include "Parse.hpp"

#include <iostream>
#include <unordered_set>
#include <forward_list>
#include <boost/logic/tribool.hpp>

namespace LDParse{

	typedef Mesh<Position, Position, uint32_t> LDMesh;
	
	typedef enum : uint8_t {
		ConfigT = 0,
		PrimitiveT = 1,
		PartT = 2,
		ModelT = 3,
		MPDRootT = 4,
		MPDSubT = 5,
		UnknownT = 6
	} SrcType;
	
	typedef enum : int8_t {
		BFCOff = -1,
		Standard = 0,
		Invert = 1
	} BFCStatus;
	
	class Model {
		template<typename ErrF> friend class ModelBuilder;
	public:
		typedef Cache::CacheNode<const Model> CacheType;
		typedef Cache::CacheNode<const size_t> IndexType;
	private:
		std::string mName;
		std::string mSrcLoc;
		SrcType mSrcType;
		const std::shared_ptr<const IndexType> mSubModelNames;
		std::shared_ptr<CacheType> mSubModels; // This is shared across the whole MPD
		ColorTable &mColorTable;
		std::unordered_map<uint16_t, uint16_t> mLocalColors;
		std::unordered_map<uint16_t, uint16_t> mLocalComplements; // This is a bit of an abomination,

		LDMesh mData;
		size_t mColor;
		std::vector<std::tuple<size_t, BFCStatus, TransMatrix, const Model*> > mChildren;
		std::unordered_map<const Model*, std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>> > mChildOffsets;
		std::vector<size_t> mStepEnds;
		boost::logic::tribool mCertify;
		BFCStatus mWinding;

		
	public:
		Model(std::string name, std::string srcLoc, SrcType srcType, ColorTable& colorTable, const std::shared_ptr<const IndexType> subModelNames = nullptr, std::shared_ptr<CacheType> subModels = nullptr);
	};
}

#endif /* Model_h */
