#pragma once
#include <vector>
#include <string>


namespace Xavier
{	
	namespace utils
	{
		std::vector<char> m_getFileBinaryContent(const std::string& fileName);
		int maxFlagBit(int flags);
	}
}