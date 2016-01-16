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
	
	
	
	typedef void(*FailF)(const TokenStream::const_iterator &, bool *);
	template<typename Out> using ReadF = bool (*)(TokenStream::const_iterator &tokenIt, Out &o, FailF fail);
	
	template<typename ReadF, typename Out, size_t tokenLen, const char * tokenName, typename ErrHandler> class Expect{
	private:
		void failed(const std::string &expect, const TokenStream::const_iterator &tokenIt, bool * expectSuccess = nullptr) const {
			failed(expect, tokenIt->textRepr(), expectSuccess);
		}
		void failed(const std::string &expect, const std::string &found, bool * expectSuccess = nullptr) const {
			mErr("Expected " + expect, found, true);
			if(expectSuccess != nullptr) *expectSuccess = false;
		}
		const FailF failF;
		
	public:
		
		typedef Expect<ReadF, Out, tokenLen, tokenName, ErrHandler> SelfType;
		
		ReadF &mRead;
		ErrHandler &mErr;
		Expect(ErrHandler &err, ReadF *read = nullptr) : mRead(*read), mErr(err) {
			const SelfType *e = this;
			failF = [](const TokenStream::const_iterator &tokenIt, bool * expectSuccess){
				e->failed(tokenName, tokenIt, expectSuccess);
			};
		}
		
		
		bool operator()(const TokenStream &tokens, TokenStream::const_iterator &tokenIt, Out &o){
			bool ret = std::distance(tokenIt, tokens.end()) >= 1;
			if(ret) {
				ret = mRead(tokenIt, o, this);
			} else {
				failed(tokenName, "EOL");
			}
			return ret;
		}
	};
	
	template<const char* tokenName, typename ErrHandler> class Expect<void, TokenStream::const_iterator, 0, tokenName, ErrHandler> {
		bool operator()(const TokenStream &tokens, TokenStream::const_iterator &tokenIt, TokenStream::const_iterator &compare){
			bool ret = tokenIt == compare;
			if(!ret) this->failed(tokenName, tokenIt);
			return ret;
		}
	};
	
	template<typename MPDHandler, typename MetaHandler, typename IncludeHandler, typename LineHandler, typename TriangleHandler, typename QuadHandler, typename OptHandler, typename ErrHandler> class Parser
	{
	private:
		constexpr static const char strEOL[] = "EOL";
		constexpr static const char strNUM[] = "number";
		constexpr static const char strCOL[] = "color reference";
		
		
		Winding winding;
		MPDHandler &mMPD;
		MetaHandler &mMeta;
		IncludeHandler &mIncl;
		LineHandler &mLine;
		TriangleHandler &mTri;
		QuadHandler &mQuad;
		OptHandler &mOpt;
		ErrHandler &mErr;
		
		bool readColor(TokenStream::const_iterator &tokenIt, ColorRef &color, FailF fail) const{
			bool ret = true;
			switch(tokenIt->k){
				case HexInt:
					color = {false, boost::get<int32_t>((tokenIt++)->v)};
					break;
				case DecInt:
					color = {true, boost::get<int32_t>((tokenIt++)->v)};
					break;
				default:
					fail(tokenIt, &ret);
			}
			return ret;
		}
		
		bool readNumber(TokenStream::const_iterator &tokenIt, float &num, FailF fail) const{
			bool ret = true;
			switch(tokenIt->k){
				case HexInt:
				case DecInt:
					num = boost::get<int32_t>((tokenIt++)->v);
					break;
				case Float:
					num = boost::get<float>((tokenIt++)->v);
					break;
				default:
					fail(tokenIt, &ret);
			}
			return ret;
		}
		
	public:
		
		typedef Parser<MPDHandler, MetaHandler, IncludeHandler, LineHandler, TriangleHandler, QuadHandler, OptHandler, ErrHandler> SelfType;
		
		typedef Expect<void, TokenStream::const_iterator, 0, strEOL, ErrHandler> ExpectEOL;
		const ExpectEOL expectEOL;
		
		typedef Expect<ReadF<ColorRef>, ColorRef, 1, strCOL, ErrHandler> ExpectColor;
		const ExpectColor expectColor;
		
		typedef Expect<ReadF<float>, float, 1, strNUM, ErrHandler> ExpectNumber;
		const ExpectNumber expectNumber;
		
		Parser(MPDHandler &mpd, MetaHandler &m, IncludeHandler &i, LineHandler &l, TriangleHandler &t, QuadHandler &q, OptHandler &o, ErrHandler &e)
		:  mMeta(m), mIncl(i), mLine(l), mTri(t), mQuad(q), mOpt(o), winding(CCW), expectEOL(mErr) {
			const SelfType *self = this;
			expectColor = ExpectColor(mErr,
									  [](TokenStream::const_iterator &tokenIt, ColorRef &color, FailF fail){
										  self->readColor(tokenIt, color, fail);
									  });
			expectNumber = ExpectNumber(mErr,
									  [](TokenStream::const_iterator &tokenIt, float &num, FailF fail){
										  self->readNumber(tokenIt, num, fail);
									  });
		}
		
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
						auto token = line.begin();
						switch((token++)->k){
							case Zero:
								break;
							case One: {
								ColorRef color; expectColor(line, token, color);
								
								//nextAction = mIncl();
								break;
							}
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
