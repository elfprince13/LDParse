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
#include "Lex.hpp"
#include "Parse.hpp"

#include <iostream>

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
		typedef Cache::CacheNode<Model> CacheType;
	private:
		std::string mName;
		std::string mSrcLoc;
		SrcType mSrcType;
		std::unique_ptr<CacheType> mSubModels;

		LDMesh data;
		size_t color;
		std::vector<std::tuple<size_t, BFCStatus, TransMatrix, const Model*> > children;
		std::map<const Model*, std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>> > childOffsets;

		
		/*
		 typedef Action (*MPDF)(boost::optional<const std::string&> file);
		 typedef Action (*MetaF)(TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eolIt);
		 typedef Action (*InclF)(const ColorRef &c, const TransMatrix &t, const std::string &name);
		 typedef Action (*LineF)(const ColorRef &c, const Line &l);
		 typedef Action (*TriF)(const ColorRef &c, const Triangle &t);
		 typedef Action (*QuadF)(const ColorRef &c, const Quad &q);
		 typedef Action (*OptF)(const ColorRef &c, const OptLine &o);
		 */
		Action handleMPDCommand(boost::optional<const std::string&> file){
			std::cout << "0 ";
			if(file) {
				std::cout << "FILE " << *file;
			} else {
				std::cout << "NOFILE";
			}
			std::cout << std::endl;
			
			if(file && file->compare(mName)){
				std::unique_ptr<Model> subModel(new Model(*file, mSrcLoc, MPDSubT));
				mSubModels->insert(*file, subModel);
			}
			return Action();
		}
		
		template<typename ...ArgTs> struct CallbackMethod{
			typedef Action(Model::*CallbackM)(ArgTs... args);
			Model * mSelf;
			CallbackM mCallbackM;
			CallbackMethod(Model * self, CallbackM callbackM) : mSelf(self), mCallbackM(callbackM) {}
			Action operator()(ArgTs... args){
				return (mSelf->*mCallbackM)(args ...);
			}
		};
		
		// This causes Xcode to blow a gasket
		//template<typename ...ArgTs> auto makeClosure(Action (Model::*handlerM)(ArgTs...)) -> decltype([&](ArgTs... args){ (this->*handlerM)(args ...) }) {
		//	return [&](ArgTs... args){ (this->*handlerM)(args ...) };
		//}
		
	public:
		Model(std::string name, std::string srcLoc, SrcType srcType)
		: mName(name), mSrcLoc(srcLoc), mSrcType(srcType), mSubModels(mSrcType == MPDRootT ? CacheType::makeRoot() : nullptr) {}
		
		
		template<typename ErrF> static Model* construct(std::string srcLoc, std::string modelName, std::istream &fileContents, ErrF errF, SrcType srcType = UnknownT){
			Model* ret = nullptr;
			
			Lexer<ErrF> lexer(fileContents, errF);
			ModelStream models;
			std::string rootName = modelName;
			bool isMPD = lexer.lexModelBoundaries(models, rootName);
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
			}
			
			ret = new Model(modelName, srcLoc, srcType);
			
			typedef CallbackMethod<boost::optional<const std::string&> > MPDCallback;
			MPDCallback mpdCallback(ret, &Model::handleMPDCommand);
			typedef Parser<MPDCallback, MetaF, InclF, LineF, TriF, QuadF, OptF, ErrF > ModelParser;
			ModelParser parser(mpdCallback,
							   LDParse::DummyImpl::dummyMeta,
							   LDParse::DummyImpl::dummyIncl,
							   LDParse::DummyImpl::dummyLine,
							   LDParse::DummyImpl::dummyTri,
							   LDParse::DummyImpl::dummyQuad,
							   LDParse::DummyImpl::dummyOpt,
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
