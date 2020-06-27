//
//  ModelBuilderImpl.h
//  LDParse
//
//  Created by Thomas Dickerson on 1/20/16.
//  Copyright © 2016 - 2020 StickFigure Graphic Productions. All rights reserved.
//

#pragma once

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
		bool success = true;
		switch ((tokenIt++)->k) {
			case Colour: {
				std::string name;
				ColorRef code;
				ColorRef v;
				ColorRef e;
				if(success &= mParser.expectIdent(tokenIt, eolIt, name)
				   && mParser.expectKeyword(tokenIt, eolIt, Code)
				   && mParser.expectColor(tokenIt, eolIt, code) && code.first
				   && mParser.expectKeyword(tokenIt, eolIt, Value)
				   && mParser.expectColor(tokenIt, eolIt, v) && !v.first
				   && mParser.expectKeyword(tokenIt, eolIt, Edge)
				   && mParser.expectColor(tokenIt, eolIt, e)){
					v.second <<= 8; // Make room for alpha
					if(!e.first){
						e.second <<= 8;
						e.second |= 0xff;
					}
					
					if(tokenIt != eolIt && tokenIt->k == Alpha){
						ColorRef alpha;
						if(success &= mParser.expectColor(++tokenIt, eolIt, alpha)){
							v.second |= (alpha.second & 0xff);
						}
					} else {
						v.second |= 0xff;
					}
					// We don't care about other properties for now;
				}
				
				if(success){
					boost::optional<uint16_t> lVal;
					if(target.mSrcType == ConfigT){
						target.mColorTable.setColour(code.second, v.second);
						target.mColorTable.setComplement(code.second, (e.first || !(lVal = target.mColorTable.addLocalColour(e.second))) ? e.second : *lVal);
						success &= e.first || lVal; // We have garbage data if the !lVal condition above was true
					} else {
						if(success &= (bool)(lVal = target.mColorTable.addLocalColour(v.second))){
							target.mLocalColors[code.second] = *lVal;
							target.mColorTable.setComplement(code.second, (e.first || !(lVal = target.mColorTable.addLocalColour(e.second))) ? e.second : *lVal);
							if((success &= e.first || lVal) && e.first) { // We have garbage data if the !lVal condition above was true
								target.mLocalComplements[code.second] = e.second;
							}
						}
					}
					
				}
				break;
			}
			case Step:
				target.mStepEnds.push_back(target.mData.indices.size());
				break;
			case BFC:
				if(success &= (tokenIt != eolIt)){
					bool leadingOrient = true;
					switch (tokenIt->k) {
						case InvertNext:
							leadingOrient = false;
							if(target.mCertify) {
								mInvertNext.insert(&target);
							} break;
						case NoCertify:
							leadingOrient = false;
							target.mCertify = false;
							break;
						case NoClip:
							leadingOrient = false;
							if(target.mCertify){
								mClipping.erase(&target);
							} break;
						case Certify:
							leadingOrient = false;
							if(success &= (indeterminate(target.mCertify) || (bool)target.mCertify)){
								target.mCertify = true;
								mClipping.insert(&target);
								mWindings[&target] = CCW;
								if (++tokenIt != eolIt && (success &= tokenIt->k == Orientation)) goto META_PARSE_ORIENTATION;
								break;
							}
						META_PARSE_CLIP:
						case Clip:
							if(target.mCertify){
								mClipping.insert(&target);
								if (++tokenIt != eolIt && (success &= tokenIt->k == Orientation && !leadingOrient)){
									leadingOrient = false;
									goto META_PARSE_ORIENTATION;
								} else {
									leadingOrient = false;
								}
							} break;
						META_PARSE_ORIENTATION:
						case Orientation:
							if(target.mCertify){
								mWindings[&target] = std::get<Winding>(tokenIt->v);
								if(tokenIt != eolIt && (success &= tokenIt->k == Clip && leadingOrient))
									goto META_PARSE_CLIP;
							} break;
						default:
							success = false;
							break;
					}
				}
				break;
			default:
				break;
		}
		return success ? Action() : Action(StopParsing, false);
	}
	
	template<typename ErrF>
	Action ModelBuilder<ErrF>::handleInclude(Model& target, const ColorRef &, const TransMatrix &, const std::string &name){
		if(indeterminate(target.mCertify)) target.mCertify = false;
		Action ret = Action();
		boost::optional<const size_t&> subIndex;
		boost::optional<const Model&> subModel;
		// This may be a performance bottleneck and we'll have to combine the two trees, but that would require us to tackle the typing more directly.
		if(target.mSubModelNames && (subIndex = target.mSubModelNames->find(name)) && !(subModel = target.mSubModels->find(name))){
			std::cout << "Switching file to " << name << std::endl;
			ret = Action(SwitchFile, *subIndex);
		} else if (subModel) {
			std::cout << "Submodel " << name << " was present" << std::endl;
			
			
		} else {
			std::cout << "Fetching " << name << ", later" << std::endl;
		}
		if(ret.k != SwitchFile) mInvertNext.erase(&target);
		return ret;
	}
	
	template<typename ErrF>
	Action ModelBuilder<ErrF>::handleTriangle(Model& target, const ColorRef &, const Triangle &){
		if(indeterminate(target.mCertify)) target.mCertify = false;
		return Action();
	}
	
	template<typename ErrF>
	Action ModelBuilder<ErrF>::handleQuad(Model& target, const ColorRef &, const Quad &){
		if(indeterminate(target.mCertify)) target.mCertify = false;
		return Action();
	}
	
	template<typename ErrF>	void ModelBuilder<ErrF>::handleEOF(Model& target){
		std::cout << "Unswitching" << std::endl;
		recordTo(&target);
	}
	
	
	// These two routines have nothing to do but enforce Certification rules w.r.t. "operational command lines" - http://www.ldraw.org/article/415.html
	template<typename ErrF>	Action ModelBuilder<ErrF>::handleLine(Model& target, const ColorRef &, const Line &){
		if(indeterminate(target.mCertify)) target.mCertify = false;
		return Action();
	}
	
	template<typename ErrF>	Action ModelBuilder<ErrF>::handleOptLine(Model& target, const ColorRef &, const OptLine &){
		if(indeterminate(target.mCertify)) target.mCertify = false;
		return Action();
	}
	
	
	template<typename ErrF>	ModelBuilder<ErrF>::ModelBuilder(ErrF &errF)
	: mErr(errF),
	mpdCallback(this, &ModelBuilder::handleMPDCommand),
	metaCallback(this, &ModelBuilder::handleMetaCommand),
	inclCallback(this, &ModelBuilder::handleInclude),
	lineCallback(this, &ModelBuilder::handleLine),
	triCallback(this, &ModelBuilder::handleTriangle),
	quadCallback(this, &ModelBuilder::handleQuad),
	optLineCallback(this, &ModelBuilder::handleOptLine),
	eofCallback(this, &ModelBuilder::handleEOF),
	mParser(mpdCallback,
			metaCallback,
			inclCallback,
			lineCallback,
			triCallback,
			quadCallback,
			optLineCallback,
			eofCallback,
			mErr) { mWindings.clear(); mInvertNext.clear(); }
	
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
			/*if(ret->mSubModels){
				(ret->mSubModels)->dump();
				for(auto it = models.begin() + 1; it < models.end(); ++it){
					std::cout << "Find? " << ((ret->mSubModels)->find(it->first) != boost::none) << std::endl;
				}
			}*/
		}
		
		recordTo(nullptr);
		
		return ret;
	}
}
