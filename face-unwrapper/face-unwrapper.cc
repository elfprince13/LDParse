//
//  face-unwrapper.cc
//  face-unwrapper
//
//  Created by Thomas Dickerson on 6/27/20.
//  Copyright © 2020 StickFigure Graphic Productions. All rights reserved.
//

#include <algorithm>
#include <iostream>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif

#include <fstream>
#include <iomanip>
#include <stack>
#include <utility>

#include <LDParse/Lex.hpp>
#include <LDParse/Parse.hpp>
#include <LDParse/Color.hpp>

#include <BidirectionalCoroutine.hpp>

#include <fmt/printf.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
						yieldColor.color.setFromHex(v.second);
						if(e.first) {
							yieldColor.edge = e.second;
						} else {
							ColorData edgeData;
							edgeData.color.setFromHex(e.second);
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
	
	
	std::optional<ColorData> bindColor(const ColorRef& color, const ColorScope& colors,
						const ColorData& face, const ColorData& edge, bool complement = false) {
		
		if(color.first) {
			if(color.second == 16) {
				return complement ? edge : face;
			} else if (color.second == 24) {
				if(complement) {
					std::cerr << "Warning: complement of complement is undefined" << std::endl;
					return std::nullopt;
				} else {
					return edge;
				}
			} else {
				auto out = colors.find(color.second, complement);
				return out;
			}
		} else {
			if(complement) {
				std::cerr << "Warning: complement of direct is undefined" << std::endl;
				return std::nullopt;
			} else {
				ColorData outColor;
				outColor.color.setFromHex(color.second);
				return outColor;
			}
		}
	}
}

#define RADIUS 13
#define X_OFF (RADIUS * 2 * M_PI)
static glm::vec2 translater(X_OFF, 0);

glm::vec2 unwrap_cylinder(glm::vec4 pos) {
	float theta = (pos.z == 0 && pos.x == 0) ? 0 : glm::atan(pos.z, pos.x);
	theta += M_PI;
	
	return glm::vec2(theta * RADIUS, pos.y);
}

std::string hexColor(const LDParse::RGB &rgb) {
	return fmt::sprintf("#%02x%02x%02x",rgb.r(), rgb.g(), rgb.b());
}

template<size_t N>
class SeqFmt {
	template<size_t ...I>
	static constexpr std::array<char,3*N> applyHelper(std::index_sequence<I...>){
		return {((I % 3) ? (((I % 3) == 2) ? ' ' : 's' ) : '%') ..., '\0' };
	}
public:
	static constexpr std::array<char,3*N> apply() {
		return applyHelper(std::make_index_sequence<3*N-1>());
	}
};
std::string fmtPoint(const glm::vec2& point) {
	return fmt::sprintf("%f,%f",point.x,point.y);
}

template<typename ...Args>
void polygon(const LDParse::ColorData &color, const Args& ... args) {
	auto metafmt = fmt::sprintf("<polygon points=\"%s\" fill=\"%%s\" stroke=\"none\" fill-opacity=\"%%d%%%%\" />\n",
								SeqFmt<sizeof...(args)>::apply().data());
	
	fmt::printf(metafmt, fmtPoint(args)...,
				hexColor(color.color), (int)(100*(color.alpha.value_or(255) / 255.)));
}

int main(int argc, const char * argv[]) {
	if(argc < 4){
		std::cerr << "face-unwrapper ldpath configpath facefile" << std::endl;
		exit(-1);
	}
	fs::path ldPath(argv[1]);
	fs::path configPath(argv[2]);
	fs::path facePath(argv[3]);
	
	auto partsPath = ldPath / "parts";
	if(!fs::exists(partsPath)) {
		partsPath = ldPath / "PARTS";
	}
	
	auto primsPath = ldPath / "p";
	if(!fs::exists(primsPath)) {
		primsPath = ldPath / "P";
	}
	
	auto modelsPath = ldPath / "models";
	if(!fs::exists(modelsPath)) {
		modelsPath = ldPath / "MODELS";
	}
	
	using namespace LDParse;
	ColorScope colors;
	
	{
		ModelStream models;
		std::string rootName = configPath.string();
		
		{
			std::ifstream configFile(configPath.native());
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
			auto parser = makeParser(dummyMPD, colorYielder(yield, pBase),
									 dummyIncl, dummyLine, dummyTri, dummyQuad, dummyOpt,
									 dummyEOF, errF);
			yield();
			parser.parseModels(models);
		});
		
		size_t numColors = 0;
		for(auto color = std::cref(configReader()); configReader; (color = configReader(), ++numColors)) {
			colors.record(color);
		}
		std::cerr << configPath << " contained " << numColors << " color definitions" << std::endl;
		colors.commit();
	}
	
	auto parse = [](const fs::path & modelPath) {
		std::string rootName = modelPath.string();
		std::ifstream modelFile(modelPath.native());
		Lexer<ErrF> lexer(modelFile, errF);
		
		ModelStream models;
		bool isMPD = lexer.lexModelBoundaries(models, rootName);
		
		if(isMPD) {
			std::cerr << "We are not supporting mpds at this time" << std::endl;
			exit(-2);
		}
		
		return [models = std::move(models)](SVGCoroutine::Yield & yield) -> void {
			using namespace DummyImpl;
			ParserBase<ErrF> pBase(errF);
			auto parser = makeParser(dummyMPD, colorYielder(yield, pBase),
									 inclYielder(yield), lineYielder(yield), triYielder(yield), quadYielder(yield),
									 dummyOpt, dummyEOF, errF);
			yield();
			parser.parseModels(models);
		};
	};
	
	using ParserState = std::tuple<SVGCoroutine,glm::mat4,ColorScope,ColorData,ColorData>;
	const ColorData DEFAULT_COLOR{{{0, 0, 0}},{0},std::nullopt,std::nullopt};
	
	std::stack<ParserState> parseStack;
	// Docs say mat4(1.0) is the identity
	parseStack.emplace(SVGCoroutine(parse(facePath)), glm::mat4(1.0), colors, DEFAULT_COLOR, DEFAULT_COLOR);
	
	const float svgWidth = 2 * X_OFF;
	const float svgHeight = 21;
	fmt::printf("<?xml version=\"1.0\" standalone=\"no\"?>\n");
	fmt::printf("<svg width=\"%f\" height=\"%f\" version=\"1.1\""
				" xmlns=\"http://www.w3.org/2000/svg\">\n",svgWidth,svgHeight);
	while(parseStack.size()) {
		auto& [parser, transform, colors, faceColor, edgeColor] = parseStack.top();
		if(auto& command = parser();
		   parser) {
			if(!std::holds_alternative<Color>(command) && colors.hasUncommitted()) {
				colors.commit();
			}
			switch(command.index()) {
				case 0: {
					colors.record(std::get<0>(command));
				} break;
				case 1: {
					auto& [colorRef, relative, rawName] = std::get<1>(command);
					ColorData newFaceColor = bindColor(colorRef, colors, faceColor, edgeColor).value_or(DEFAULT_COLOR);
					ColorData newEdgeColor = bindColor(colorRef, colors, faceColor, edgeColor, true).value_or(DEFAULT_COLOR);
					
					const auto & [xyz, a, b, c, d, e, f, g, h, i] = relative;
					const auto & [x, y, z] = xyz;
					// OpenGL storage convention is column major, so read this transposed.
					float data[16] = {	a, d, g, 0,
										b, e, h, 0,
										c, f, i, 0,
										x, y, z, 1};
					glm::mat4 localTransform(glm::make_mat4(data));
					
					std::replace(rawName.begin(), rawName.end(), '\\', '/');
					
					fs::path name(rawName);
					name.make_preferred();
					fs::path includedPath = (primsPath / name);
					if(!fs::exists(includedPath)) {
						includedPath = (partsPath / name);
						
						if(!fs::exists(includedPath)) {
							includedPath = (modelsPath / name);
							
							if(!fs::exists(includedPath)) {
								std::cerr << "Warning: couldn't find " << name << " in " << ldPath.native() << std::endl;
								break;
							}
						}
						
					}
					
					// colors should be getting copy-constructed
					parseStack.emplace(SVGCoroutine(parse(includedPath)), transform * localTransform, colors, newFaceColor, newEdgeColor);
				} break;
				case 2: {
					auto& [colorRef, line] = std::get<2>(command);
					ColorData color = bindColor(colorRef, colors, faceColor, edgeColor).value_or(DEFAULT_COLOR);
					
					auto& [source, term] = line;
					
					glm::vec4 s(std::make_from_tuple<glm::vec3>(source), 1.);
					glm::vec4 t(std::make_from_tuple<glm::vec3>(term), 1.);
					
					s = transform * s;
					t = transform * t;
					
					auto sProj = unwrap_cylinder(s);
					auto tProj = unwrap_cylinder(t);
					
					for(size_t i = 0; i < 2; ++i) {
						fmt::printf("<line x1=\"%f\" x2=\"%f\" y1=\"%f\" y2=\"%f\""
									" stroke=\"%s\" stroke-opacity=\"%d%%\" stroke-width=\"%f\" />\n",
									sProj.x, tProj.x, sProj.y, tProj.y,
									hexColor(color.color), (int)(100*(color.alpha.value_or(255)/255.)), 1.);
						sProj += translater;
						tProj += translater;
					}
				} break;
				case 3: {
					auto& [colorRef, tri] = std::get<3>(command);
					ColorData color = bindColor(colorRef, colors, faceColor, edgeColor).value_or(DEFAULT_COLOR);
					
					auto& [p0, p1, p2] = tri;
					
					glm::vec4 v0(std::make_from_tuple<glm::vec3>(p0), 1.);
					glm::vec4 v1(std::make_from_tuple<glm::vec3>(p1), 1.);
					glm::vec4 v2(std::make_from_tuple<glm::vec3>(p2), 1.);
					
					glm::vec2 pr0 = unwrap_cylinder(v0);
					glm::vec2 pr1 = unwrap_cylinder(v1);
					glm::vec2 pr2 = unwrap_cylinder(v2);
					
					for(size_t i = 0; i < 2; ++i) {
						polygon(color, pr0, pr1, pr2);
						pr0 += translater;
						pr1 += translater;
						pr2 += translater;
					}
				} break;
				case 4: {
					auto& [colorRef, quad] = std::get<4>(command);
					ColorData color = bindColor(colorRef, colors, faceColor, edgeColor).value_or(DEFAULT_COLOR);
					
					auto& [p0, p1, p2, p3] = quad;
					
					glm::vec4 v0(std::make_from_tuple<glm::vec3>(p0), 1.);
					glm::vec4 v1(std::make_from_tuple<glm::vec3>(p1), 1.);
					glm::vec4 v2(std::make_from_tuple<glm::vec3>(p2), 1.);
					glm::vec4 v3(std::make_from_tuple<glm::vec3>(p3), 1.);
					
					glm::vec2 pr0 = unwrap_cylinder(v0);
					glm::vec2 pr1 = unwrap_cylinder(v1);
					glm::vec2 pr2 = unwrap_cylinder(v2);
					glm::vec2 pr3 = unwrap_cylinder(v3);
					
					for(size_t i = 0; i < 2; ++i) {
						polygon(color, pr0, pr1, pr2, pr3);
						pr0 += translater;
						pr1 += translater;
						pr2 += translater;
						pr3 += translater;
					}
					
				} break;
				default:
					std::cerr << "This should be unreachable" << std::endl;
					std::abort();
			}
		} else {
			parseStack.pop();
		}
	}
	
	fmt::printf("</svg>\n");
	
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
