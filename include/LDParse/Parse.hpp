/*
 *  Parse.hpp
 *  LDParse
 *
 *  Created by Thomas Dickerson on 1/13/16.
 *  Copyright © 2016 - 2020 StickFigure Graphic Productions. All rights reserved.
 *
 */

#ifndef Parse_hpp
#define Parse_hpp

#include "Lex.hpp"
#include "Geom.hpp"
#include "ReadFs.hpp"
#include "Expect.hpp"
#include <boost/optional.hpp>

namespace LDParse{
	
	typedef enum {
		SwitchFile,
		SkipNLines,
		NoAction,
		StopParsing
	} ActionKind;
	
	struct Action {
		ActionKind k;
		size_t v;
		Action() : k(NoAction), v(0) {}
		Action(ActionKind k, size_t v) : k(k), v(v) {}
	};
	
	namespace ExpectTokenStrings {
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
		constexpr static const char strKW[] = "specific keyword";
		
	}
		
	template<typename MPDHandler, typename MetaHandler, typename IncludeHandler, typename LineHandler, typename TriangleHandler, typename QuadHandler, typename OptHandler, typename EOFHandler, typename ErrHandler> class Parser
	{
	private:
		Winding winding;
		
		ErrHandler &mErr;
		
		MPDHandler &mMPD;
		MetaHandler &mMeta;
		IncludeHandler &mIncl;
		LineHandler &mLine;
		TriangleHandler &mTri;
		QuadHandler &mQuad;
		OptHandler &mOpt;
		EOFHandler &mEOF;
		
	public:
		template<typename Out> using ReadF = ReadF<Out, ErrHandler>;
		typedef Parser<MPDHandler, MetaHandler, IncludeHandler, LineHandler, TriangleHandler, QuadHandler, OptHandler, EOFHandler, ErrHandler> SelfType;
		
		typedef Expect<ReadF<const TokenKind>, const TokenKind, 1, ExpectTokenStrings::strKW, ErrHandler> ExpectKeyword;
		ExpectKeyword expectKeyword;
		
		typedef Expect<void, TokenStream::const_iterator, 0, ExpectTokenStrings::strEOL, ErrHandler> ExpectEOL;
		ExpectEOL expectEOL;
		
		typedef Expect<ReadF<ColorRef>, ColorRef, 1, ExpectTokenStrings::strCOL, ErrHandler> ExpectColor;
		ExpectColor expectColor;
		
		typedef Expect<ReadF<float>, float, 1, ExpectTokenStrings::strNUM, ErrHandler> ExpectNumber;
		ExpectNumber expectNumber;
		
		typedef Expect<ReadF<std::string>, std::string, 1, ExpectTokenStrings::strID, ErrHandler> ExpectIdent;
		ExpectIdent expectIdent;
		
		typedef Expect<ReadF<Position>, Position, 3 * ExpectNumber::TokenCount, ExpectTokenStrings::strPOS, ErrHandler> ExpectPosition;
		ExpectPosition expectPosition;
		
		typedef Expect<ReadF<Line>, Line, 2 * ExpectPosition::TokenCount, ExpectTokenStrings::strLINE, ErrHandler> ExpectLine;
		ExpectLine expectLine;
		
		typedef Expect<ReadF<Triangle>, Triangle, 3 * ExpectPosition::TokenCount, ExpectTokenStrings::strTRI, ErrHandler> ExpectTriangle;
		ExpectTriangle expectTriangle;
		
		typedef Expect<ReadF<Quad>, Quad, 4 * ExpectPosition::TokenCount, ExpectTokenStrings::strQUAD, ErrHandler> ExpectQuad;
		ExpectQuad expectQuad;
		
		typedef Expect<ReadF<OptLine>, OptLine, 2 * 2 * ExpectPosition::TokenCount, ExpectTokenStrings::strOPT, ErrHandler> ExpectOptLine;
		ExpectOptLine expectOptLine;
		
		typedef Expect<ReadF<TransMatrix>, TransMatrix, ExpectPosition::TokenCount + 9 * ExpectNumber::TokenCount, ExpectTokenStrings::strMAT, ErrHandler> ExpectMat;
		ExpectMat expectMat;
		
		bool expectFileName(TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eol,
							std::string &o, const std::string &lineT){
			static std::locale efnLocale;
			o = lineT.substr(tokenIt->c);
			boost::trim_right(o, efnLocale);
			tokenIt = eol;
			return true;
		}
		
		Parser(MPDHandler &mpd, MetaHandler &m, IncludeHandler &i, LineHandler &l, TriangleHandler &t, QuadHandler &q, OptHandler &o, EOFHandler &eof, ErrHandler &e)
		:  winding(CCW), mErr(e),
		mMPD(mpd), mMeta(m), mIncl(i), mLine(l), mTri(t), mQuad(q), mOpt(o), mEOF(eof),
		expectKeyword(mErr, readKeyword),
		expectEOL(mErr),
		expectColor(mErr, readColor),
		expectNumber(mErr, readNumber),
		expectIdent(mErr, readIdent),
		expectPosition(mErr, readPosition),
		expectLine(mErr, readLine),
		expectTriangle(mErr, readTriangle),
		expectQuad(mErr, readQuad),
		expectOptLine(mErr, readOptLine),
		expectMat(mErr, readMat)
		{}
		
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
					const std::string lineT = lineIt->first;
					const TokenStream &line = lineIt->second;
					if(line.size()){
						auto token = line.begin();
						const auto eol = line.end();
						switch((token++)->k){
							case Zero:
								if(token != eol){
									switch((token++)->k){
										case File:{
											std::string name;
											if(ret &= expectIdent(token, eol, name)){
												coalesceText(token, eol, name);
												if(ret &= expectEOL(token, eol)) nextAction = mMPD(name);
											}
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
									nextAction = mMeta(token, eol) /* nb token == eol, but the types are important */;
								}
								break;
							case One: {
								ColorRef color; TransMatrix mat; std::string name;
								if(ret &= expectColor(token, eol, color)
								   && expectMat(token, eol, mat)
								   && expectFileName(token, eol, name, lineT)
								   && expectEOL(token, eol)) nextAction = mIncl(color, mat, name);
								break;
							}
							case Two: {
								ColorRef color; Line l;
								if(ret &= expectColor(token, eol, color)
								   && expectLine(token, eol, l)
								   && expectEOL(token, eol)) nextAction = mLine(color, l);
								break;
							}
							case Three: {
								ColorRef color; Triangle t;
								if(ret &= expectColor(token, eol, color)
								   && expectTriangle(token, eol, t)
								   && expectEOL(token, eol)) nextAction = mTri(color, t);
								break;
							}
							case Four: {
								ColorRef color; Quad q;
								if(ret &= expectColor(token, eol, color)
								   && expectQuad(token, eol, q)
								   && expectEOL(token, eol)) nextAction = mQuad(color, q);
								break;
							}
							case Five: {
								ColorRef color; OptLine l;
								if(ret &= expectColor(token, eol, color)
								   && expectOptLine(token, eol, l)
								   && expectEOL(token, eol)) nextAction = mOpt(color, l);
								break;
							}
							default:
								// This is an error, but we can just ignore the whole line, and roll with it
								mErr("Invalid beginning of line. Expected one of: 0,1,2,3,4,5",line[0].textRepr(), strict);
								
						}
					}
					
					if(ret){
						switch(nextAction.k){
							case StopParsing:
								ret = nextAction.v;
								goto PARSING_MEGA_BREAK;
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
							mEOF();
							if(scanStack.size()){
								std::tie(modelIt, lineIt) = scanStack.back();
								scanStack.pop_back();
								//++lineIt; // We want to repeat the last line before the SwitchFile Action. This is easier than implementing a general deferral mechanism
							} else {
								break;
							}
						}
					}
				}
			}
		PARSING_MEGA_BREAK:
			return ret;
		}
		
		/*
		 * Does not clear context of `text` prior to coalescing. This is the caller's responsibility.
		 */
		static inline void coalesceText(TokenStream::const_iterator &start, const TokenStream::const_iterator &end, std::string &text){
			while(start != end){
				if(text.length()) text += " ";
				text += (start++)->textRepr();
			}
		}
		
	};
	
	typedef Action (*MPDF)(boost::optional<const std::string&> file);
	typedef Action (*MetaF)(TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eolIt);
	typedef Action (*InclF)(const ColorRef &c, const TransMatrix &t, const std::string &name);
	typedef Action (*LineF)(const ColorRef &c, const Line &l);
	typedef Action (*TriF)(const ColorRef &c, const Triangle &t);
	typedef Action (*QuadF)(const ColorRef &c, const Quad &q);
	typedef Action (*OptF)(const ColorRef &c, const OptLine &o);
	typedef void (*EOFF)();
	
	namespace DummyImpl {
		static MPDF dummyMPD = [](boost::optional<const std::string&>){ return Action(); };
		static MetaF dummyMeta = [](TokenStream::const_iterator &, const TokenStream::const_iterator &){ return Action(); };
		static InclF dummyIncl = [](const ColorRef &, const TransMatrix &, const std::string &){ return Action(); };
		static LineF dummyLine = [](const ColorRef &, const Line &){ return Action(); };
		static TriF dummyTri = [](const ColorRef &, const Triangle &){ return Action(); };
		static QuadF dummyQuad = [](const ColorRef &, const Quad &){ return Action(); };
		static OptF dummyOpt = [](const ColorRef &, const OptLine &){ return Action(); };
		static EOFF dummyEOF = [](){};
	};
	
	typedef Parser<MPDF, MetaF, InclF, LineF, TriF, QuadF, OptF, EOFF, ErrF> CallbackParser;
	
	
}
#endif
