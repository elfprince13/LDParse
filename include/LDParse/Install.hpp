//
//  Install.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 7/5/20.
//  Copyright © 2020 StickFigure Graphic Productions. All rights reserved.
//

#pragma once

#include <array>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif
#include <optional>
#include <tuple>
#include <vector>

namespace LDParse {
	
	enum SrcType : uint8_t {
		ConfigT = 0,
		PrimitiveT = 1,
		SubPartT = 2,
		PartT = 3,
		ModelT = 4,
		MPDRootT = 5,
		MPDSubT = 6,
		UnknownT = 7
	};
	
	class Install {
		std::vector<std::array<fs::path, 3> > installDirs_;
	public:
		Install(const std::vector<std::string>& installDirs);
		std::optional<std::tuple<SrcType, fs::path>> find(const fs::path& name) const;
		
		static std::vector<std::string> splitPaths(const std::string& paths);
	};
}
