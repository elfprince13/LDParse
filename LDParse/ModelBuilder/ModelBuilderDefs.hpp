//
//  ModelBuilderDefs.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/20/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#ifndef ModelBuilderDefs_h
#define ModelBuilderDefs_h

#include "Model.hpp"

namespace LDParse {
	template<typename ErrF> class ModelBuilder{
	private:
		template<typename R, typename ...ArgTs> struct CallbackMethod{
			typedef R(ModelBuilder::*CallbackM)(Model&, ArgTs... args);
			ModelBuilder *mSelf;
			const CallbackM mCallbackM;
			Model * mTarget;
			CallbackMethod(ModelBuilder * self, const CallbackM callbackM) : mSelf(self), mCallbackM(callbackM), mTarget(nullptr) {
				assert((callbackM != nullptr) && "Can't call a null method");
				assert((self != nullptr) && "Can't call method on a null object");
			}
			R operator()(ArgTs... args){
				assert((mTarget != nullptr) && "Invalid target");
				return (mSelf->*mCallbackM)(*mTarget, args ...);
			}
			
			Model * retarget(Model * target){
				std::swap(mTarget, target);
				return target; // Return the old value
			}
		};
		
		Action handleMPDCommand(Model& target, boost::optional<const std::string&> file);
		CallbackMethod<Action, boost::optional<const std::string&> > mpdCallback;
		
		Action handleMetaCommand(Model& target, TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eolIt);
		CallbackMethod<Action, TokenStream::const_iterator &, const TokenStream::const_iterator &> metaCallback;
		
		Action handleInclude(Model& target, const ColorRef &c, const TransMatrix &t, const std::string &name);
		CallbackMethod<Action, const ColorRef &, const TransMatrix &, const std::string &> inclCallback;
		
		Action handleTriangle(Model& target, const ColorRef &c, const Triangle &t);
		CallbackMethod<Action, const ColorRef &, const Triangle &> triCallback;
		
		Action handleQuad(Model& target, const ColorRef &c, const Quad &q);
		CallbackMethod<Action, const ColorRef &, const Quad &> quadCallback;
		
		void handleEOF(Model& target);
		CallbackMethod<void> eofCallback;
		
		ErrF &mErr;
		
		template<bool root=false> void recordTo(Model * model){
			metaCallback.retarget(model);
			inclCallback.retarget(model);
			triCallback.retarget(model);
			quadCallback.retarget(model);
			if(root){
				mpdCallback.retarget(model);
				eofCallback.retarget(model);
			}
		}
		
		typedef Parser<decltype(mpdCallback), decltype(metaCallback), decltype(inclCallback),
		LineF, decltype(triCallback), decltype(quadCallback), OptF,
		decltype(eofCallback), ErrF > ModelParser;
		ModelParser mParser;
	public:
		ModelBuilder(ErrF &errF);
		Model* construct(std::string srcLoc, std::string modelName, std::istream &fileContents, ColorTable& colorTable, SrcType srcType = UnknownT);
	};
}

#endif /* ModelBuilderDefs_h */
