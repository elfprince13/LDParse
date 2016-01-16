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

void err(std::string msg, std::string tok, bool fatal) {
	std::cerr << (fatal ? "Error: " : "Warning: ") << msg << "(" << tok << ")" << std::endl;
	if(fatal) exit(-1);
}

int main(int argc, const char * argv[]) {
	if(argc < 2){
		std::cerr << "Requires a filename to run" << std::endl;
		exit(-1);
	}
	std::string fileName = argv[1];
	std::ifstream file(fileName);
	LDParse::CallbackLexer lex(file, &err);
	
	std::string rootName = fileName;
	LDParse::ModelStream models;
	
	bool isMPD = lex.lexModelBoundaries(models, rootName);
	
	
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
	
    return 0;
}
