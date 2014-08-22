
#ifndef UTILS_H
#define UTILS_H

#include "amxxmodule.h"
#include "CString.h"
#include <time.h>

#if defined __linux__
	#include <sys/types.h>
	#include <dirent.h>
#endif

#define charsmax(a) ( sizeof( a ) - 1 )
#define EOS			'\0'

extern bool	FileExists( const char* file );

#endif // UTILS_H
