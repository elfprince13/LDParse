/*
 *  LDParse.hpp
 *  LDParse
 *
 *  Created by Thomas Dickerson on 1/13/16.
 *  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
 *
 */

#ifndef Parse_hpp
#define Parse_hpp

#include "Lex.hpp"
#include "Geom.hpp"

namespace LDParse{
	
	typedef enum {
		SwitchFile,
		SkipNLines,
		NoAction
	} ActionKind;
	
	struct Action {
		ActionKind k;
		size_t v;
		Action() : k(NoAction), v(0) {}
	};
	
	template<typename MPDHandler, typename MetaHandler, typename IncludeHandler, typename LineHandler, typename TriangleHandler, typename QuadHandler, typename OptHandler, typename ErrHandler> class Parser
	{
	private:
		Winding winding;
		MPDHandler &mMPD;
		MetaHandler &mMeta;
		IncludeHandler &mIncl;
		LineHandler &mLine;
		TriangleHandler &mTri;
		QuadHandler &mQuad;
		OptHandler &mOpt;
		ErrHandler &mErr;
	public:
		
		Parser(MPDHandler &mpd, MetaHandler &m, IncludeHandler &i, LineHandler &l, TriangleHandler &t, QuadHandler &q, OptHandler &o, ErrHandler &e) :  mMeta(m), mIncl(i), mLine(l), mTri(t), mQuad(q), mOpt(o), winding(CCW) {}
		
		void parseModels(const ModelStream &models){
			std::vector<bool> completed(models.size(), false);
			std::vector<std::pair<ModelStream::const_iterator, LineStream::const_iterator> > scanStack;
			scanStack.clear();
			
			for (auto modelIt = models.begin(); modelIt != models.end(); ++modelIt) {
				const LineStream &model = modelIt->second;
				for(auto lineIt = model.begin(); lineIt != model.end(); ++lineIt){
					const TokenStream &line = *lineIt;
					if(line.size()){
						Action nextAction;
						switch(line[0].k){
							case Zero:
								break;
							case One:
								//ColorRef color = expectColor(line[1]);
								
								//nextAction = mIncl();
								break;
							case Two:
							case Three:
							case Four:
							case Five:
							default:
								mErr("Invalid beginning of line. Expected one of: 0,1,2,3,4,5",line[0].textRepr(), false);
								
						}
					}
				}
			}
			
		}
		
		
		static inline std::string coalesceText(TokenStream::const_iterator start, TokenStream::const_iterator end){
			std::string ret("");
			for(auto it = start; it != end; ++it) ret += it->textRepr();
			return ret;
		}
		
	};
	
	typedef Action (*MPDF)(const std::string&);
	typedef Action (*MetaF)(const TokenStream &restOfLine);
	typedef Action (*InclF)(const ColorRef &c, const TransMatrix &t, const std::string &name);
	typedef Action (*LineF)(const ColorRef &c, const Line &l);
	typedef Action (*TriF)(const ColorRef &c, const Triangle &t);
	typedef Action (*QuadF)(const ColorRef &c, const Quad &q);
	typedef Action (*OptF)(const ColorRef &c, const OptLine &o);
	
	typedef Parser<MPDF, MetaF, InclF, LineF, TriF, QuadF, OptF, ErrF> CallbackParser;
	
	
}
#endif
