//
//  face-unwrapper.cc
//  face-unwrapper
//
//  Created by Thomas Dickerson on 6/27/20.
//  Copyright © 2020 StickFigure Graphic Productions. All rights reserved.
//

#include <algorithm>
#include <iostream>
#include <fstream>
#include <LDParse/Lex.hpp>
#include <LDParse/Parse.hpp>
#include <LDParse/Color.hpp>

#include <BidirectionalCoroutine.hpp>

void err(std::string msg, std::string tok, bool fatal) {
	std::cerr << (fatal ? "Error: " : "Warning: ") << msg << " ( " << tok << " )" << std::endl;
	if(fatal) exit(-1);
}


static LDParse::ErrF errF = &err;

/*
 using MPDF = Action (*)(std::optional<std::reference_wrapper<const std::string>> file);
 using MetaF = Action (*)(TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eolIt);
 using InclF = Action (*)(const ColorRef &c, const TransMatrix &t, const std::string &name);
 using LineF = Action (*)(const ColorRef &c, const Line &l);
 using TriF = Action (*)(const ColorRef &c, const Triangle &t);
 using QuadF = Action (*)(const ColorRef &c, const Quad &q);
 using OptF = Action (*)(const ColorRef &c, const OptLine &o);
 using EOFF = void (*)();
 */

template<typename R, typename ...Args>
using BidirectionalCoroutine = com::geopipe::functional::CoroutineContext<>::BidirectionalCoroutine<R, Args...>;
namespace LDParse {
	template<class Yield, class ErrF>
	auto colorYielder(Yield& yield, ParserBase<ErrF>& parser) {
		return [&](TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eolIt) -> Action {
			bool success = true;
			auto& oldToken = *tokenIt;
			switch ((tokenIt++)->k) {
				default: break;
				case Colour: {
					Color yieldColor;
					std::string name;
					ColorRef code;
					ColorRef v;
					ColorRef e;
					if(success &= parser.expectIdent(tokenIt, eolIt, yieldColor.name)
					   && parser.expectKeyword(tokenIt, eolIt, Code)
					   && parser.expectColor(tokenIt, eolIt, code) && code.first
					   && parser.expectKeyword(tokenIt, eolIt, Value)
					   && parser.expectColor(tokenIt, eolIt, v) && !v.first
					   && parser.expectKeyword(tokenIt, eolIt, Edge)
					   && parser.expectColor(tokenIt, eolIt, e)){
						
						yieldColor.code = code.second;
						yieldColor.color.r() = v.second & 0x00FF0000;
						yieldColor.color.g() = v.second & 0x0000FF00;
						yieldColor.color.b() = v.second & 0x000000FF;
						if(e.first) {
							yieldColor.edge = e.second;
						} else {
							ColorData edgeData;
							edgeData.color.r() = e.second & 0x00FF0000;
							edgeData.color.g() = e.second & 0x0000FF00;
							edgeData.color.b() = e.second & 0x000000FF;
							yieldColor.edge = edgeData;
						}
						
						if(tokenIt != eolIt && tokenIt->k == Alpha){
							ColorRef alpha;
							if(success &= parser.expectColor(++tokenIt, eolIt, alpha)){
								yieldColor.alpha = alpha.second;
							}
						}
						
						if(success && tokenIt != eolIt && tokenIt->k == Luminance){
							ColorRef luminance;
							if(success &= parser.expectColor(++tokenIt, eolIt, luminance)){
								yieldColor.luminance = luminance.second;
							}
						}
						
						if(success && tokenIt != eolIt){
							switch(tokenIt->k) {
								default: success = false; break;
								case Chrome...Material: {
									TokenStream finish;
									std::copy(tokenIt, eolIt, std::back_inserter(finish));
									yieldColor.finish = std::move(finish);
								}
							}
						}
					}
					
					if(success){
						yield(yieldColor);
					}
				}
			}
			return success ? Action() : Action(StopParsing, false);
		};
	}
	
	template<class Yield>
	auto inclYielder(Yield& yield) {
		return [&](const ColorRef& color, const TransMatrix& transform, const std::string& file) -> Action {
			yield(std::make_tuple(color, transform, file));
			return Action();
		};
	}
	
	template<class Yield>
	auto lineYielder(Yield& yield) {
		return [&](const ColorRef& color, const Line& line) -> Action {
			yield(std::make_tuple(color, line));
			return Action();
		};
	}
	
	template<class Yield>
	auto triYielder(Yield& yield) {
		return [&](const ColorRef& color, const Triangle& tri) -> Action {
			yield(std::make_tuple(color, tri));
			return Action();
		};
	}
	
	template<class Yield>
	auto quadYielder(Yield& yield) {
		return [&](const ColorRef& color, const Quad& quad) -> Action {
			yield(std::make_tuple(color, quad));
			return Action();
		};
	}
	
	using ConfigCoroutine = BidirectionalCoroutine<Color>;
	using SVGCoroutine = BidirectionalCoroutine<
		std::variant<
			Color,
			std::tuple<ColorRef,TransMatrix,std::string>,
			std::tuple<ColorRef,Line>,
			std::tuple<ColorRef,Triangle>,
			std::tuple<ColorRef,Quad>
		>
	>;
}

int main(int argc, const char * argv[]) {
	if(argc < 4){
		std::cerr << "face-unwrapper ldpath configpath facefile" << std::endl;
		exit(-1);
	}
	std::string ldPath = argv[1];
	std::string configPath = argv[2];
	std::string facePath = argv[3];
	//std::ifstream faceFile(facePath);
	
	using namespace LDParse;
	ColorScope colors;
	
	
	ModelStream models;
	std::string rootName = configPath;
	
	{
		std::ifstream configFile(configPath);
		Lexer<ErrF> lexer(configFile, errF);
		bool isMPD = lexer.lexModelBoundaries(models, rootName);
		
		if(isMPD) {
			std::cerr << "ldconfig.ldr must not be an mpd" << std::endl;
			exit(-2);
		}
	}
	
	ConfigCoroutine configReader([&](ConfigCoroutine::Yield & yield) -> void {
		using namespace DummyImpl;
		ParserBase<ErrF> pBase(errF);
		auto parser = makeParser(dummyMPD, colorYielder(yield, pBase), dummyIncl, dummyLine
								 , dummyTri, dummyQuad, dummyOpt, dummyEOF, errF);
		yield();
		parser.parseModels(models);
	});
	
	size_t numColors = 0;
	for(std::reference_wrapper<const Color> color = configReader(); configReader; (color = configReader(), ++numColors)) {
		colors.record(color);
	}
	std::cout << configPath << " contained " << numColors << " color definitions" << std::endl;
	colors.commit();
	
	//LDParse::ModelBuilder<LDParse::ErrF> modelBuilder(errF);
	//LDParse::Model * model = modelBuilder.construct(fileName, fileName, file, colors);
	
	/*
	 
	 std::cout << "Model " << fileName << " is " << (isMPD ? "" : "not ") << " an MPD" << std::endl;
	 if(isMPD) std::cout << "\t" << "root model is " << rootName << std::endl << std::endl;
	 
	 for(auto it = models.begin(); it != models.end(); ++it){
	 if(isMPD) std::cout << "// " << (it->first.compare(rootName) ? "sub" : "root") << "-model, " << it->first << std::endl << std::endl;
	 for(auto fIt = it->second.begin(); fIt != it->second.end(); ++fIt){
	 for(size_t i = 0; i < fIt->size(); i++){
	 std::cout << (*fIt)[i];
	 }
	 std::cout << std::endl;
	 }
	 std::cout << std::endl << std::endl;
	 }
	 
	 */
	
	
	
	return 0;
}
