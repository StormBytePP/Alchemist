#pragma once

#include "visibility.h"

#include <string>

namespace Alchemist::Media {
	struct DLL_PUBLIC Info {
		unsigned short id;
		std::string short_name;
		std::string long_name;
	};
}