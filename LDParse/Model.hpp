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

		LDMesh mData;
		size_t mColor;
		std::vector<std::tuple<size_t, BFCStatus, TransMatrix, const Model*> > mChildren;
		std::unordered_map<const Model*, std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>> > mChildOffsets;
		std::vector<size_t> mStepEnds;
		boost::logic::tribool mCertify;

		
		template<typename R, typename ...ArgTs> struct CallbackMethod{
		public:
			typedef R(Model::*CallbackM)(ArgTs... args);
			Model * mSelf;
			const CallbackM mCallbackM;
			CallbackMethod(Model * self, const CallbackM callbackM) : mSelf(self), mCallbackM(callbackM) {
				assert((callbackM != nullptr) && "Can't call a null method");
			}
			R operator()(ArgTs... args){
				assert((mSelf != nullptr) && "Invalid target");
				return (mSelf->*mCallbackM)(args ...);
			}
			
			Model * retarget(Model * self){
				std::swap(mSelf, self);
				return self; // Return the old value
			}
		};
		
		Action handleMPDCommand(boost::optional<const std::string&> file);
		CallbackMethod<Action, boost::optional<const std::string&> > mpdCallback;
		
		Action handleInclude(const ColorRef &c, const TransMatrix &t, const std::string &name);
		CallbackMethod<Action, const ColorRef &, const TransMatrix &, const std::string &> inclCallback;
		
		Action handleTriangle(const ColorRef &c, const Triangle &t);
		CallbackMethod<Action, const ColorRef &, const Triangle &> triCallback;
		
		Action handleQuad(const ColorRef &c, const Quad &q);
		CallbackMethod<Action, const ColorRef &, const Quad &> quadCallback;
		
		void handleEOF();
		CallbackMethod<void> eofCallback;
		
		void recordTo(Model * subModel);
		
	public:
		Model(std::string name, std::string srcLoc, SrcType srcType, ColorTable& colorTable, const std::shared_ptr<const IndexType> subModelNames = nullptr, std::shared_ptr<CacheType> subModels = nullptr);
		
		
		template<typename ErrF> static Model* construct(std::string srcLoc, std::string modelName, std::istream &fileContents, ColorTable& colorTable, ErrF errF, SrcType srcType = UnknownT){
			Model* ret = nullptr;
			
			Lexer<ErrF> lexer(fileContents, errF);
			ModelStream models;
			std::string rootName = modelName;
			bool isMPD = lexer.lexModelBoundaries(models, rootName);
			
			std::shared_ptr<IndexType> subModelNames = nullptr;
			
			if(isMPD){
				if(srcType < ModelT) {
					errF("Was told this file was not an MPD, but MPD commands were found", modelName, false);
				}
				if(srcType != MPDSubT){
					srcType = MPDRootT;
					srcLoc += "/";
					srcLoc += modelName;
					modelName = rootName;
				}
				subModelNames = std::shared_ptr<IndexType>(IndexType::makeRoot());
				size_t modelCount = models.size();
				for(size_t i = 0; i < modelCount; i++){
					size_t *dst = new size_t(i);
					subModelNames->insert(models[i].first, std::unique_ptr<size_t>(dst));
				}
			}
			
			ret = new Model(modelName, srcLoc, srcType, colorTable, subModelNames);
			
			typedef Parser<decltype(mpdCallback), MetaF, decltype(inclCallback), LineF, decltype(triCallback), decltype(quadCallback), OptF, decltype(eofCallback), ErrF > ModelParser;
			ModelParser parser(ret->mpdCallback,
							   LDParse::DummyImpl::dummyMeta,
							   ret->inclCallback,
							   LDParse::DummyImpl::dummyLine,
							   ret->triCallback,
							   ret->quadCallback,
							   LDParse::DummyImpl::dummyOpt,
							   ret->eofCallback,
							   errF);
			
			if(!parser.parseModels(models)){
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
			
			return ret;
		}
		
	};
	
}

#endif /* Model_h */
