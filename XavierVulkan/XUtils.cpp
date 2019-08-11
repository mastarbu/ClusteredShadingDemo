#include "XUtils.h"

#include <fstream>
#include <iostream>
namespace Xavier
{
	std::vector<char> utils::m_getFileBinaryContent(const std::string& fileName)
	{
		std::vector<char> bytes;
		std::ifstream file(fileName, std::ios::binary);
		if (file.fail())
		{
			std::cout << "Open File: " << fileName.c_str() << "Failed !" << std::endl;
		}
		else
		{
			std::streampos begin, end;
			begin = file.tellg();
			file.seekg(0, std::ios::end);
			end = file.tellg();

			// Read the file from begin to end.
			bytes.resize(end - begin);
			file.seekg(0, std::ios::beg);
			file.read(bytes.data(), static_cast<size_t>(end - begin));
		}
		return bytes;
	}

	int utils::maxFlagBit(int flags)
	{
		int ret = 0;
		int next = 1;
		while (next & flags)
		{
			ret = next;
			next <<= 1;
		}
		return ret;
	}
}
