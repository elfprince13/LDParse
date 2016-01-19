//
//  Model.cpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/18/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#include "Model.hpp"
namespace LDParse {
	Model::Model(std::string name, std::string srcLoc, SrcType srcType, ColorTable& colorTable, boost::optional<std::unordered_set<std::string> > subModelNames)
	: mName(name), mSrcLoc(srcLoc), mSrcType(srcType), mSubModelNames(subModelNames), mSubModels(mSrcType == MPDRootT ? CacheType::makeRoot() : nullptr),
	mColorTable(colorTable),
	mpdCallback(this, &Model::handleMPDCommand),
	inclCallback(this, &Model::handleInclude),
	triCallback(this, &Model::handleTriangle),
	quadCallback(this, &Model::handleQuad)  {}
	
	void Model::recordTo(Model * subModel){
		inclCallback.retarget(subModel);
		triCallback.retarget(subModel);
		quadCallback.retarget(subModel);
	}
	
	Action Model::handleMPDCommand(boost::optional<const std::string&> file){
		std::cout << "0 ";
		if(file) {
			std::cout << "FILE " << *file;
		} else {
			std::cout << "NOFILE";
		}
		std::cout << std::endl;
		
		if(file && file->compare(mName)){
			Model * subModel = new Model(*file, mSrcLoc, MPDSubT, mColorTable);
			mSubModels->insert(*file, std::unique_ptr<Model>(subModel));
			recordTo(subModel);
		} else if(!file) {
			recordTo(nullptr);
		}
		return Action();
	}
	
	Action Model::handleInclude(const ColorRef &c, const TransMatrix &t, const std::string &name){
		return Action();
	}
	
	Action Model::handleTriangle(const ColorRef &c, const Triangle &t){
		return Action();
	}
	
	Action Model::handleQuad(const ColorRef &c, const Quad &q){
		return Action();
	}
}
