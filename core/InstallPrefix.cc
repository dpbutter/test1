
#include "Config.hh"
#include "InstallPrefix.hh"
#if !defined(__OpenBSD__) && !defined(__FreeBSD__)
#include "whereami.h"
#endif
#include <stdexcept>

std::string cadabra::install_prefix()
	{
//#if defined(__FreeBSD__) || defined(__OpenBSD__)
// // We cannot use a hardcoded CMAKE_INSTALL_PREFIX as that
//	// breaks relocatability. Hopefully the 'whereami' support
// // for FreeBSD/OpenBSD is ok now.
//	std::string ret(CMAKE_INSTALL_PREFIX);
//	return ret;
// #else
	std::string ret;
	int dirname_length;
	auto length = wai_getExecutablePath(NULL, 0, &dirname_length);
	if(length > 0) {
		char *path = (char*)malloc(length + 1);
		if (!path)
			throw std::logic_error("Cannot determine installation path.");
		wai_getExecutablePath(path, length, &dirname_length);
		path[length] = '\0';
		path[dirname_length] = '\0';
		ret=std::string(path);
		free(path);
#if !defined(_WIN32)
		ret=ret.substr(0, ret.size()-4); // strip '/bin'
#endif
		}
	return ret;
// #endif
	}

//const char *cadabra::cmake_install_prefix()
//	{
//	static const char prefix[]=CMAKE_INSTALL_PREFIX;
//
//	return prefix;
//	}
