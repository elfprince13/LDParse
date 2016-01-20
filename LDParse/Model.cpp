//
//  Model.cpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/18/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#include "Model.hpp"
namespace LDParse {
	Model::Model(std::string name, std::string srcLoc, SrcType srcType, ColorTable& colorTable,
				 const std::shared_ptr<const IndexType> subModelNames, std::shared_ptr<CacheType> subModels)
	: mName(name), mSrcLoc(srcLoc), mSrcType(srcType),
	mSubModelNames(subModelNames), mSubModels((subModels == nullptr) ? std::shared_ptr<CacheType>((mSrcType == MPDRootT) ? CacheType::makeRoot() : nullptr) : subModels),
	mColorTable(colorTable), mCertify(boost::logic::indeterminate),
	mpdCallback(this, &Model::handleMPDCommand),
	inclCallback(this, &Model::handleInclude),
	triCallback(this, &Model::handleTriangle),
	quadCallback(this, &Model::handleQuad),
	eofCallback(this, &Model::handleEOF){ mStepEnds.clear(); }
	
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
			Model * subModel = new Model(*file, mSrcLoc, MPDSubT, mColorTable, mSubModelNames, mSubModels);
			mSubModels->insert(*file, std::unique_ptr<Model>(subModel));
			recordTo(subModel);
		} else if(!file) {
			recordTo(nullptr);
		}
		return Action();
	}
	
	Action Model::handleInclude(const ColorRef &c, const TransMatrix &t, const std::string &name){
		boost::optional<const size_t&> subIndex;
		boost::optional<const Model&> subModel;
		Action ret = Action();
		// This may be a performance bottleneck and we'll have to combine the two trees, but that would require us to tackle the typing more directly.
		if(mSubModelNames && (subIndex = mSubModelNames->find(name)) && !(subModel = mSubModels->find(name))){
			std::cout << "Switching file to " << name << std::endl;
			ret = Action(SwitchFile, *subIndex);
		} else if (subModel) {
			std::cout << "Submodel " << name << " was present" << std::endl;
		} else {
			std::cout << "Fetching " << name << ", later" << std::endl;
		}
		return ret;
	}
	
	Action Model::handleTriangle(const ColorRef &c, const Triangle &t){
		return Action();
	}
	
	Action Model::handleQuad(const ColorRef &c, const Quad &q){
		return Action();
	}
	
	void Model::handleEOF(){
		std::cout << "Unswitching" << std::endl;
		recordTo(this);
	}
}
