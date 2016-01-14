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
	std::ifstream file(argv[1]);
	LDParse::Lexer lex(file, &err);
	std::vector<LDParse::Token> line;
	while(lex.lexLine(line)){
		for(size_t i = 0; i < line.size(); i++){
			std::cout << line[i];
		}
		std::cout << std::endl;
	}
	
	
    return 0;
}
