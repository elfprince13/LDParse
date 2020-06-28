/*
 *  Parse.hpp
 *  LDParse
 *
 *  Created by Thomas Dickerson on 1/13/16.
 *  Copyright © 2016 - 2020 StickFigure Graphic Productions. All rights reserved.
 *
 */
#pragma once

#include "Lex.hpp"
#include "Geom.hpp"
#include "ReadFs.hpp"
#include "Expect.hpp"

#include <functional>
#include <optional>

namespace LDParse{
	
	enum ActionKind {
		SwitchFile,
		SkipNLines,
		NoAction,
		StopParsing
	};
	
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
	
	template<typename ErrHandler>
	class ParserBase {
	protected:
		ErrHandler mErr;
	public:
		template<typename Out>
		using ReadF = ReadF<Out, ErrHandler>;
		
		using ExpectKeyword = Expect<ReadF<const TokenKind>, const TokenKind, 1, ExpectTokenStrings::strKW, ErrHandler>;
		ExpectKeyword expectKeyword;
		
		using ExpectEOL = Expect<void, TokenStream::const_iterator, 0, ExpectTokenStrings::strEOL, ErrHandler>;
		ExpectEOL expectEOL;
		
		using ExpectColor = Expect<ReadF<ColorRef>, ColorRef, 1, ExpectTokenStrings::strCOL, ErrHandler>;
		ExpectColor expectColor;
		
		using ExpectNumber = Expect<ReadF<float>, float, 1, ExpectTokenStrings::strNUM, ErrHandler>;
		ExpectNumber expectNumber;
		
		using ExpectIdent = Expect<ReadF<std::string>, std::string, 1, ExpectTokenStrings::strID, ErrHandler>;
		ExpectIdent expectIdent;
		
		using ExpectPosition = Expect<ReadF<Position>, Position, 3 * ExpectNumber::TokenCount, ExpectTokenStrings::strPOS, ErrHandler>;
		ExpectPosition expectPosition;
		
		using ExpectLine = Expect<ReadF<Line>, Line, 2 * ExpectPosition::TokenCount, ExpectTokenStrings::strLINE, ErrHandler>;
		ExpectLine expectLine;
		
		using ExpectTriangle = Expect<ReadF<Triangle>, Triangle, 3 * ExpectPosition::TokenCount, ExpectTokenStrings::strTRI, ErrHandler>;
		ExpectTriangle expectTriangle;
		
		using ExpectQuad = Expect<ReadF<Quad>, Quad, 4 * ExpectPosition::TokenCount, ExpectTokenStrings::strQUAD, ErrHandler>;
		ExpectQuad expectQuad;
		
		using ExpectOptLine = Expect<ReadF<OptLine>, OptLine, 2 * 2 * ExpectPosition::TokenCount, ExpectTokenStrings::strOPT, ErrHandler>;
		ExpectOptLine expectOptLine;
		
		using ExpectMat = Expect<ReadF<TransMatrix>, TransMatrix, ExpectPosition::TokenCount + 9 * ExpectNumber::TokenCount, ExpectTokenStrings::strMAT, ErrHandler>;
		ExpectMat expectMat;
		
		bool expectFileName(TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eol,
							std::string &o, const std::string &lineT){
			static std::locale efnLocale;
			o = lineT.substr(tokenIt->c);
			boost::trim_right(o, efnLocale);
			tokenIt = eol;
			return true;
		}
		
		ParserBase(ErrHandler e)
		: mErr(e),
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
		expectMat(mErr, readMat) {}
	};
		
	template<	typename MPDHandler, typename MetaHandler, typename IncludeHandler,
				typename LineHandler, typename TriangleHandler, typename QuadHandler,
				typename OptHandler, typename EOFHandler, typename ErrHandler>
	class Parser : public ParserBase<ErrHandler> {
		Winding winding;
		MPDHandler mMPD;
		MetaHandler mMeta;
		IncludeHandler mIncl;
		LineHandler mLine;
		TriangleHandler mTri;
		QuadHandler mQuad;
		OptHandler mOpt;
		EOFHandler mEOF;
		
	public:
		using SelfType = Parser<MPDHandler, MetaHandler, IncludeHandler, LineHandler, TriangleHandler, QuadHandler, OptHandler, EOFHandler, ErrHandler>;
		using SuperType = ParserBase<ErrHandler>;
		
		using SuperType::mErr;
		using SuperType::expectKeyword;
		using SuperType::expectEOL;
		using SuperType::expectColor;
		using SuperType::expectNumber;
		using SuperType::expectIdent;
		using SuperType::expectPosition;
		using SuperType::expectLine;
		using SuperType::expectTriangle;
		using SuperType::expectQuad;
		using SuperType::expectOptLine;
		using SuperType::expectMat;
		using SuperType::expectFileName;
		
		Parser(MPDHandler mpd, MetaHandler m, IncludeHandler i, LineHandler l, TriangleHandler t, QuadHandler q, OptHandler o, EOFHandler eof, ErrHandler e)
		:  SuperType(e), winding(CCW),
		mMPD(mpd), mMeta(m), mIncl(i), mLine(l), mTri(t), mQuad(q), mOpt(o), mEOF(eof) {}
		
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
									
									switch(auto heldToken = token++;
										   heldToken->k){
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
												nextAction = mMPD(std::nullopt);
											} else {  // Garbage has already been filtered at this point.
												break;
											}
										default:
											if(ret) nextAction = mMeta(heldToken, eol);
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
	
	using MPDF = Action (*)(std::optional<std::reference_wrapper<const std::string>> file);
	using MetaF = Action (*)(TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eolIt);
	using InclF = Action (*)(const ColorRef &c, const TransMatrix &t, const std::string &name);
	using LineF = Action (*)(const ColorRef &c, const Line &l);
	using TriF = Action (*)(const ColorRef &c, const Triangle &t);
	using QuadF = Action (*)(const ColorRef &c, const Quad &q);
	using OptF = Action (*)(const ColorRef &c, const OptLine &o);
	using EOFF = void (*)();
	
	namespace DummyImpl {
		static MPDF dummyMPD = [](std::optional<std::reference_wrapper<const std::string>>){ return Action(); };
		static MetaF dummyMeta = [](TokenStream::const_iterator &, const TokenStream::const_iterator &){ return Action(); };
		static InclF dummyIncl = [](const ColorRef &, const TransMatrix &, const std::string &){ return Action(); };
		static LineF dummyLine = [](const ColorRef &, const Line &){ return Action(); };
		static TriF dummyTri = [](const ColorRef &, const Triangle &){ return Action(); };
		static QuadF dummyQuad = [](const ColorRef &, const Quad &){ return Action(); };
		static OptF dummyOpt = [](const ColorRef &, const OptLine &){ return Action(); };
		static EOFF dummyEOF = [](){};
	};
	
	using CallbackParser = Parser<MPDF, MetaF, InclF, LineF, TriF, QuadF, OptF, EOFF, ErrF>;
	
	template<typename ...Args>
	auto makeParser(Args&& ...args) {
		return Parser<Args...>(std::forward<Args>(args)...);
	}
	
	
}
