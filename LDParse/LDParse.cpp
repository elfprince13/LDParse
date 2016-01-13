/*
 *  LDParse.cpp
 *  LDParse
 *
 *  Created by Thomas Dickerson on 1/13/16.
 *  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
 *
 */

#include <iostream>
#include "LDParse.hpp"
#include "LDParsePriv.hpp"

void LDParse::HelloWorld(const char * s)
{
	 LDParsePriv *theObj = new LDParsePriv;
	 theObj->HelloWorldPriv(s);
	 delete theObj;
};

void LDParsePriv::HelloWorldPriv(const char * s) 
{
	std::cout << s << std::endl;
};

