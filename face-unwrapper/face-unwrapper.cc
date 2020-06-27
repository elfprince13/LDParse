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

static LDParse::MPDF mpdHandler = [](std::optional<std::reference_wrapper<const std::string>> file) {
	std::cout << "0 ";
	if(file) {
		std::cout << "FILE " << file->get();
	} else {
		std::cout << "NOFILE";
	}
	std::cout << std::endl;
	return LDParse::Action();
};

int main(int argc, const char * argv[]) {
	if(argc < 2){
		std::cerr << "Requires a filename to run" << std::endl;
		exit(-1);
	}
	std::string fileName = argv[1];
	std::ifstream file(fileName);
	
	LDParse::ColorTable colors;
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
