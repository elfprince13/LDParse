//
//  main.cpp
//  ParseTest
//
//  Created by Thomas Dickerson on 1/13/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#include <iostream>
#include <fstream>
#include "Lex.hpp"
#include "Parse.hpp"
#include "Model.hpp"
#include "ModelBuilder.hpp"

void err(std::string msg, std::string tok, bool fatal) {
	std::cerr << (fatal ? "Error: " : "Warning: ") << msg << " ( " << tok << " )" << std::endl;
	if(fatal) exit(-1);
}


static LDParse::ErrF errF = &err;

/*
typedef Action (*MPDF)(boost::optional<const std::string&>);
typedef Action (*MetaF)(TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eolIt);
typedef Action (*InclF)(const ColorRef &c, const TransMatrix &t, const std::string &name);
typedef Action (*LineF)(const ColorRef &c, const Line &l);
typedef Action (*TriF)(const ColorRef &c, const Triangle &t);
typedef Action (*QuadF)(const ColorRef &c, const Quad &q);
typedef Action (*OptF)(const ColorRef &c, const OptLine &o);
*/

static LDParse::MPDF mpdHandler = [](boost::optional<const std::string &> file) {
	std::cout << "0 ";
	if(file) {
		std::cout << "FILE " << *file;
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
	LDParse::ModelBuilder<LDParse::ErrF> modelBuilder(errF);
	LDParse::Model * model = modelBuilder.construct(fileName, fileName, file, colors);
	
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
