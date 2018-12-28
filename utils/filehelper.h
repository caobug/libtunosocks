#pragma once

#include "singleton.h"
#include <string>
#include <boost/filesystem.hpp>

class FileHelper
{

public:

	static std::string GetCurrentDir()
	{
		boost::filesystem::path full_path(boost::filesystem::current_path());
		return full_path.string() + std::string{ "\\" };
	}




};