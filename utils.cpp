
#include "utils.h"

bool FileExists( const char *file )
{
	#if defined WIN32 || defined _WIN32

		DWORD attr = GetFileAttributes( file );

		if( attr == INVALID_FILE_ATTRIBUTES || attr == FILE_ATTRIBUTE_DIRECTORY )
			return false;

		return true;

	#else

		struct stat s;

		if( stat( file, &s ) != 0 || S_ISDIR( s.st_mode ) )
			return false;

		return true;

#endif
}