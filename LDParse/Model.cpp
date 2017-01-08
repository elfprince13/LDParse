//
//  Model.cpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/18/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#include <LDParse/Model.hpp>
namespace LDParse {
	Model::Model(std::string name, std::string srcLoc, SrcType srcType, ColorTable& colorTable,
				 const std::shared_ptr<const IndexType> subModelNames, std::shared_ptr<CacheType> subModels)
	: mName(name), mSrcLoc(srcLoc), mSrcType(srcType),
	mSubModelNames(subModelNames), mSubModels((subModels == nullptr) ? std::shared_ptr<CacheType>((mSrcType == MPDRootT) ? CacheType::makeRoot() : nullptr) : subModels),
	mColorTable(colorTable), mCertify(boost::logic::indeterminate), mWinding(BFCOff)
	{ mStepEnds.clear(); mLocalColors.clear(); mLocalComplements.clear(); }

}
