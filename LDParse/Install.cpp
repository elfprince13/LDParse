//
//  Install.cc
//  LDParse
//
//  Created by Thomas Dickerson on 7/5/20.
//  Copyright © 2020 StickFigure Graphic Productions. All rights reserved.
//

#include <LDParse/Install.hpp>

#include <algorithm>
#include <iostream>
#include <cstring>
#include <memory>

namespace LDParse{
Install::Install(const std::vector<std::string>& installDirs)
	{
		std::transform(installDirs.begin(), installDirs.end(), std::back_inserter(installDirs_),
					   [](const std::string &ldDir) -> std::array<fs::path, 3> {
						   fs::path ldPath(ldDir);
						   
						   auto primsPath = ldPath / "p";
						   if(!fs::exists(primsPath)) {
							   primsPath = ldPath / "P";
						   }
						   
						   auto partsPath = ldPath / "parts";
						   if(!fs::exists(partsPath)) {
							   partsPath = ldPath / "PARTS";
						   }
						   
						   auto modelsPath = ldPath / "models";
						   if(!fs::exists(modelsPath)) {
							   modelsPath = ldPath / "MODELS";
						   }
						   
						   return {primsPath, partsPath, modelsPath};
					   });
	}

	std::optional<std::tuple<SrcType, fs::path>> Install::find(const fs::path& name) const {
		constexpr static const char subPartPrefix[3] = {'s', fs::path::preferred_separator, '\0'};
		for(const auto& [primsPath, partsPath, modelsPath] : installDirs_){
			std::cerr << "Scanning for " << name << " in " << primsPath << ", " << partsPath << ", " << modelsPath << std::endl;
			if(fs::path includedPath = (primsPath / name);
			   fs::exists(includedPath)) {
				return std::make_tuple(PrimitiveT, includedPath);
			} else if (includedPath = (partsPath / name);
					   fs::exists(includedPath)) {
				std::string pathName(name.native());
				
				return std::make_tuple((0 == pathName.find(subPartPrefix)) ? SubPartT : PartT, includedPath);
			} else if(includedPath = (modelsPath / name);
					  fs::exists(includedPath)) {
				// TODO : Does ModelT need to be conditional on extension?
				return std::make_tuple(ModelT, includedPath);
			}
		}
		return std::nullopt;
	}
	
	std::vector<std::string> Install::splitPaths(const std::string& paths) {
		std::unique_ptr<char[]> pathBuffer(std::make_unique<char[]>(paths.size() + 1));
		std::strcpy(pathBuffer.get(), paths.c_str());
		
		std::vector<std::string> output;
		for(const char* path = std::strtok(pathBuffer.get(), ":"); path != nullptr; path = std::strtok(nullptr, ":")) {
			output.push_back(path);
		}
		
		return output;
	}
}
