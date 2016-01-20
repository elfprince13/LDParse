//
//  ModelBuilderImpl.h
//  LDParse
//
//  Created by Thomas Dickerson on 1/20/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#ifndef ModelBuilderImpl_h
#define ModelBuilderImpl_h

#include "ModelBuilderDefs.hpp"

namespace LDParse {
	
	
	template<typename ErrF>	Action ModelBuilder<ErrF>::handleMPDCommand(Model& target, boost::optional<const std::string&> file){
		std::cout << "0 ";
		if(file) {
			std::cout << "FILE " << *file;
		} else {
			std::cout << "NOFILE";
		}
		std::cout << std::endl;
		
		if(file && file->compare(target.mName)){
			Model * subModel = new Model(*file, target.mSrcLoc, MPDSubT, target.mColorTable, target.mSubModelNames, target.mSubModels);
			target.mSubModels->insert(*file, std::unique_ptr<Model>(subModel));
			recordTo(subModel);
		} else if(!file) {
			recordTo(nullptr);
		}
		return Action();
	}
	
	template<typename ErrF>	Action ModelBuilder<ErrF>::handleMetaCommand(Model& target, TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eolIt){
		switch ((tokenIt++)->k) {
			case Colour:
				break;
			case Step:
				break;
			case BFC:
				break;
			default:
				break;
		}
		return Action();
	}
	
	template<typename ErrF>	Action ModelBuilder<ErrF>::handleInclude(Model& target, const ColorRef &c, const TransMatrix &t, const std::string &name){
		boost::optional<const size_t&> subIndex;
		boost::optional<const Model&> subModel;
		Action ret = Action();
		// This may be a performance bottleneck and we'll have to combine the two trees, but that would require us to tackle the typing more directly.
		if(target.mSubModelNames && (subIndex = target.mSubModelNames->find(name)) && !(subModel = target.mSubModels->find(name))){
			std::cout << "Switching file to " << name << std::endl;
			ret = Action(SwitchFile, *subIndex);
		} else if (subModel) {
			std::cout << "Submodel " << name << " was present" << std::endl;
		} else {
			std::cout << "Fetching " << name << ", later" << std::endl;
		}
		return ret;
	}
	
	template<typename ErrF>	Action ModelBuilder<ErrF>::handleTriangle(Model& target, const ColorRef &c, const Triangle &t){
		return Action();
	}
	
	template<typename ErrF>	Action ModelBuilder<ErrF>::handleQuad(Model& target, const ColorRef &c, const Quad &q){
		return Action();
	}
	
	template<typename ErrF>	void ModelBuilder<ErrF>::handleEOF(Model& target){
		std::cout << "Unswitching" << std::endl;
		recordTo(&target);
	}
	
	
	template<typename ErrF>	ModelBuilder<ErrF>::ModelBuilder(ErrF &errF)
	: mErr(errF),
	mpdCallback(this, &ModelBuilder::handleMPDCommand),
	metaCallback(this, &ModelBuilder::handleMetaCommand),
	inclCallback(this, &ModelBuilder::handleInclude),
	triCallback(this, &ModelBuilder::handleTriangle),
	quadCallback(this, &ModelBuilder::handleQuad),
	eofCallback(this, &ModelBuilder::handleEOF),
	mParser(mpdCallback,
			metaCallback,
			inclCallback,
			LDParse::DummyImpl::dummyLine,
			triCallback,
			quadCallback,
			LDParse::DummyImpl::dummyOpt,
			eofCallback,
			mErr) {}
	
	template<typename ErrF>	Model* ModelBuilder<ErrF>::construct(std::string srcLoc, std::string modelName, std::istream &fileContents, ColorTable& colorTable, SrcType srcType){
		Model* ret = nullptr;
		
		Lexer<ErrF> lexer(fileContents, mErr);
		ModelStream models;
		std::string rootName = modelName;
		bool isMPD = lexer.lexModelBoundaries(models, rootName);
		
		std::shared_ptr<Model::IndexType> subModelNames = nullptr;
		
		if(isMPD){
			if(srcType < ModelT) {
				mErr("Was told this file was not an MPD, but MPD commands were found", modelName, false);
			}
			if(srcType != MPDSubT){
				srcType = MPDRootT;
				srcLoc += "/";
				srcLoc += modelName;
				modelName = rootName;
			}
			subModelNames = std::shared_ptr<Model::IndexType>(Model::IndexType::makeRoot());
			size_t modelCount = models.size();
			for(size_t i = 0; i < modelCount; i++){
				size_t *dst = new size_t(i);
				subModelNames->insert(models[i].first, std::unique_ptr<size_t>(dst));
			}
		}
		
		ret = new Model(modelName, srcLoc, srcType, colorTable, subModelNames);
		
		recordTo<true>(ret);
		
		
		if(!mParser.parseModels(models)){
			delete ret;
			ret = nullptr;
		} else {
			if(ret->mSubModels){
				(ret->mSubModels)->dump();
				for(auto it = models.begin() + 1; it < models.end(); ++it){
					std::cout << "Find? " << ((ret->mSubModels)->find(it->first) != boost::none) << std::endl;
				}
			}
		}
		
		recordTo(nullptr);
		
		return ret;
	}
}


#endif /* ModelBuilderImpl_h */
