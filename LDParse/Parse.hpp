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
#include <boost/optional.hpp>

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
		Action(ActionKind k, size_t v) : k(k), v(v) {}
	};
	
	
	
	typedef void(*FailF)(const TokenStream::const_iterator &, bool *);
	template<typename Out> using ReadF = bool (*)(TokenStream::const_iterator &tokenIt, Out &o, FailF fail);
	template<typename Class, typename Out> using ReadM = bool (Class::*)(TokenStream::const_iterator &tokenIt, Out &o, FailF fail);
	
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
		
		constexpr static const size_t TokenCount = tokenLen;
		
		typedef Expect<ReadF, Out, tokenLen, tokenName, ErrHandler> SelfType;
		
		ReadF &mRead;
		ErrHandler &mErr;
		Expect(ErrHandler &err, ReadF *read = nullptr) : mRead(*read), mErr(err) {
			const SelfType *e = this;
			failF = [](const TokenStream::const_iterator &tokenIt, bool * expectSuccess){
				e->failed(tokenName, tokenIt, expectSuccess);
			};
		}
		
		
		bool operator()(TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eol, Out &o){
			bool ret = std::distance(tokenIt, eol) >= 1;
			if(ret) {
				ret = mRead(tokenIt, eol, o, this);
			} else {
				failed(tokenName, "EOL");
			}
			return ret;
		}
	};
	
	template<const char* tokenName, typename ErrHandler> class Expect<void, TokenStream::const_iterator, 0, tokenName, ErrHandler> {
		
		bool operator()(TokenStream::const_iterator &tokenIt, TokenStream::const_iterator &compare){
			return this(tokenIt, compare);
		}
		
		bool operator()(TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &compare, TokenStream::const_iterator &){
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
		constexpr static const char strPOS[] = "position";
		constexpr static const char strLINE[] = "line";
		constexpr static const char strTRI[] = "triangle";
		constexpr static const char strQUAD[] = "quadrilateral";
		constexpr static const char strOPT[] = "optional line";
		constexpr static const char strCOL[] = "color reference";
		constexpr static const char strMAT[] = "transformation matrix";
		constexpr static const char strID[] = "identifier";
	public:
		typedef Parser<MPDHandler, MetaHandler, IncludeHandler, LineHandler, TriangleHandler, QuadHandler, OptHandler, ErrHandler> SelfType;
		
		typedef Expect<void, TokenStream::const_iterator, 0, strEOL, ErrHandler> ExpectEOL;
		const ExpectEOL expectEOL;
		
		typedef Expect<ReadF<ColorRef>, ColorRef, 1, strCOL, ErrHandler> ExpectColor;
		const ExpectColor expectColor;
		
		typedef Expect<ReadF<float>, float, 1, strNUM, ErrHandler> ExpectNumber;
		const ExpectNumber expectNumber;
		
		typedef Expect<ReadF<std::string>, std::string, 1, strID, ErrHandler> ExpectIdent;
		const ExpectIdent expectIdent;
		
		typedef Expect<ReadF<Position>, Position, 3 * ExpectNumber::TokenCount, strPOS, ErrHandler> ExpectPosition;
		const ExpectPosition expectPosition;
		
		typedef Expect<ReadF<Line>, Line, 2 * ExpectPosition::TokenCount, strLINE, ErrHandler> ExpectLine;
		const ExpectLine expectLine;
		
		typedef Expect<ReadF<Triangle>, Triangle, 3 * ExpectPosition::TokenCount, strTRI, ErrHandler> ExpectTriangle;
		const ExpectTriangle expectTriangle;
		
		typedef Expect<ReadF<Quad>, Quad, 4 * ExpectPosition::TokenCount, strQUAD, ErrHandler> ExpectQuad;
		const ExpectQuad expectQuad;
		
		typedef Expect<ReadF<OptLine>, OptLine, 2 * 2 * ExpectPosition::TokenCount, strOPT, ErrHandler> ExpectOptLine;
		const ExpectOptLine expectOptLine;
		
		typedef Expect<ReadF<TransMatrix>, TransMatrix, ExpectPosition::TokenCount + 9 * ExpectNumber::TokenCount, strMAT, ErrHandler> ExpectMat;
		const ExpectMat expectMat;
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
		
		bool readIdent(TokenStream::const_iterator &tokenIt, std::string &id, FailF fail) const {
			bool ret = true;
			switch(tokenIt->k){
				case Ident:
					id = boost::get<std::string>((tokenIt++)->v);
					break;
				default:
					fail(tokenIt, &ret);
			}
			return ret;
		}
		
		bool readPosition(TokenStream::const_iterator &tokenIt, Position &p, FailF fail) const{
			const TokenStream::const_iterator start = tokenIt;
			bool ret = true;
			ret = readNumber(tokenIt, std::get<0>(p), fail)
				&&readNumber(tokenIt, std::get<1>(p), fail)
				&&readNumber(tokenIt, std::get<2>(p), fail);
			if(!ret) fail(start, &ret);
 			return ret;
		}
		
		bool readLine(TokenStream::const_iterator &tokenIt, Line &l, FailF fail) const{
			const TokenStream::const_iterator start = tokenIt;
			bool ret = true;
			ret = readPosition(tokenIt, std::get<0>(l), fail)
				&&readPosition(tokenIt, std::get<1>(l), fail);
			if(!ret) fail(start, &ret);
			return ret;
		}
		
		bool readTriangle(TokenStream::const_iterator &tokenIt, Triangle &t, FailF fail) const{
			const TokenStream::const_iterator start = tokenIt;
			bool ret = true;
			ret = readPosition(tokenIt, std::get<0>(t), fail)
				&&readPosition(tokenIt, std::get<1>(t), fail)
				&&readPosition(tokenIt, std::get<2>(t), fail);
			if(!ret) fail(start, &ret);
			return ret;
		}
		
		bool readQuad(TokenStream::const_iterator &tokenIt, Quad &q, FailF fail) const{
			const TokenStream::const_iterator start = tokenIt;
			bool ret = true;
			ret = readPosition(tokenIt, std::get<0>(q), fail)
				&&readPosition(tokenIt, std::get<1>(q), fail)
				&&readPosition(tokenIt, std::get<2>(q), fail)
				&&readPosition(tokenIt, std::get<3>(q), fail);
			if(!ret) fail(start, &ret);
			return ret;
		}
		
		bool readOptLine(TokenStream::const_iterator &tokenIt, OptLine &o, FailF fail) const{
			const TokenStream::const_iterator start = tokenIt;
			bool ret = true;
			ret = readLine(tokenIt, std::get<0>(o), fail)
				&&readLine(tokenIt, std::get<1>(o), fail);
			if(!ret) fail(start, &ret);
			return ret;
		}
		
		bool readMat(TokenStream::const_iterator &tokenIt, TransMatrix &m, FailF fail) const{
			const TokenStream::const_iterator start = tokenIt;
			bool ret = true;
			ret = readPosition(tokenIt, std::get<0>(m), fail)
			&&readNumber(tokenIt, std::get<1>(m), fail)
			&&readNumber(tokenIt, std::get<2>(m), fail)
			&&readNumber(tokenIt, std::get<3>(m), fail)
			&&readNumber(tokenIt, std::get<4>(m), fail)
			&&readNumber(tokenIt, std::get<5>(m), fail)
			&&readNumber(tokenIt, std::get<6>(m), fail)
			&&readNumber(tokenIt, std::get<7>(m), fail)
			&&readNumber(tokenIt, std::get<8>(m), fail)
			&&readNumber(tokenIt, std::get<9>(m), fail);
			if(!ret) fail(start, &ret);
			return ret;
		}
		
		template<typename Out> ReadF<Out> makeReadF(const ReadM<SelfType, Out> readM) const{
			const SelfType *self = this;
			return [](TokenStream::const_iterator &tokenIt, Out &o, FailF fail){
				self->readM(tokenIt, o, fail);
			};
		}
		
	public:
		Parser(MPDHandler &mpd, MetaHandler &m, IncludeHandler &i, LineHandler &l, TriangleHandler &t, QuadHandler &q, OptHandler &o, ErrHandler &e)
		:  mMeta(m), mIncl(i), mLine(l), mTri(t), mQuad(q), mOpt(o), winding(CCW), expectEOL(mErr) {
			const SelfType *self = this;
			expectColor = ExpectColor(mErr, makeReadF(&SelfType::readColor));
			expectNumber = ExpectNumber(mErr, makeReadF(&SelfType::readNumber));
			expectIdent = ExpectIdent(mErr, makeReadF(&SelfType::readIdent));
			expectPosition = ExpectPosition(mErr, makeReadF(&SelfType::readPosition));
			expectLine = ExpectLine(mErr,makeReadF(&SelfType::readLine));
			expectTriangle = ExpectTriangle(mErr,makeReadF(&SelfType::readTriangle));
			expectQuad = ExpectQuad(mErr,makeReadF(&SelfType::readQuad));
			expectOptLine = ExpectOptLine(mErr,makeReadF(&SelfType::readOptLine));
			expectMat = ExpectMat(mErr,makeReadF(&SelfType::readMat));
		}
		
		bool parseModels(const ModelStream &models, bool strict = false){
			bool ret = true;
			std::vector<bool> completed(models.size(), false);
			std::vector<std::pair<ModelStream::const_iterator, LineStream::const_iterator> > scanStack;
			scanStack.reserve(models.size()); // This is the worst case scenario for a sane client and poor file construction
			scanStack.clear();
			
			for (auto modelIt = models.begin(); modelIt != models.end() && ret; ++modelIt) {
				if(completed[std::distance(models.begin(), modelIt)]){
					continue;
				}
				auto lineIt = modelIt->second.begin();
				while(lineIt != modelIt->second.end() && ret){
					Action nextAction = {NoAction, 0};
					const TokenStream &line = *lineIt;
					if(line.size()){
						auto token = line.begin();
						const auto eol = line.end();
						switch((token++)->k){
							case Zero:
								if(token != eol){
									switch((token++)->k){
										case File:{
											std::string name;
											ret &= expectIdent(token, eol, name);
											if(ret) nextAction = mMPD(name);
											break;
										}
										case NoFile:
											if(strict){
												nextAction = mMPD(boost::none);
											} else {  // Garbage has already been filtered at this point.
												break;
											}
										default:
											if(ret) nextAction = mMeta(token, eol);
									}
								} else if(strict){
									nextAction = mMeta(eol, eol);
								}
								break;
							case One: {
								ColorRef color; TransMatrix mat; std::string name;
								ret &= expectColor(token, eol, color)
									&& expectMat(token, eol, mat)
									&& expectIdent(token, eol, name)
									&& expectEOL(token, eol);
								if(ret) nextAction = mIncl(color, mat, name);
								break;
							}
							case Two: {
								ColorRef color; Line l;
								ret &= expectColor(token, eol, color)
									&& expectLine(token, eol, l)
									&& expectEOL(token, eol);
								if(ret) nextAction = mLine(color, l);
								break;
							}
							case Three: {
								ColorRef color; Triangle t;
								ret &= expectColor(token, eol, color)
									&& expectTriangle(token, eol, t)
									&& expectEOL(token, eol);
								if(ret) nextAction = mTri(color, t);
								break;
							}
							case Four: {
								ColorRef color; Quad q;
								ret &= expectColor(token, eol, color)
									&& expectLine(token, eol, q)
									&& expectEOL(token, eol);
								if(ret) nextAction = mQuad(color, q);
								break;
							}
							case Five: {
								ColorRef color; OptLine l;
								ret &= expectColor(token, eol, color)
									&& expectOptLine(token, eol, l)
									&& expectEOL(token, eol);
								if(ret) nextAction = mOpt(color, l);
								break;
							}
							default:
								// This is an error, but we can just ignore the whole line, and roll with it
								mErr("Invalid beginning of line. Expected one of: 0,1,2,3,4,5",line[0].textRepr(), strict);
								
						}
					}
					
					if(ret){
						switch(nextAction.k){
							case SwitchFile:
								scanStack.push_back(std::make_pair(modelIt, lineIt));
								std::advance((modelIt = models.begin()), nextAction.v);
								lineIt = modelIt->second.begin();
								break;
							case SkipNLines:
								std::advance(lineIt, nextAction.v);
							default:
								++lineIt;
						}
						
						while(lineIt == modelIt->second.end()){
							completed[std::distance(models.begin(), modelIt)] = true;
							if(scanStack.size()){
								std::tie(modelIt, lineIt) = scanStack.back();
								scanStack.pop_back();
								++lineIt;
							} else {
								break;
							}
						}
					}
				}
			}
			return ret;
		}
		
		static inline std::string coalesceText(TokenStream::const_iterator start, TokenStream::const_iterator end){
			std::string ret("");
			for(auto it = start; it != end; ++it) ret += it->textRepr();
			return ret;
		}
		
	};
	
	typedef Action (*MPDF)(boost::optional<const std::string&>);
	typedef Action (*MetaF)(TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eolIt);
	typedef Action (*InclF)(const ColorRef &c, const TransMatrix &t, const std::string &name);
	typedef Action (*LineF)(const ColorRef &c, const Line &l);
	typedef Action (*TriF)(const ColorRef &c, const Triangle &t);
	typedef Action (*QuadF)(const ColorRef &c, const Quad &q);
	typedef Action (*OptF)(const ColorRef &c, const OptLine &o);
	
	typedef Parser<MPDF, MetaF, InclF, LineF, TriF, QuadF, OptF, ErrF> CallbackParser;
	
	
}
#endif
