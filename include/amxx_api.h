
#ifndef _INCLUDE_AMXX_API_
#define _INCLUDE_AMXX_API_

#include "amxxmodule.h"
#include "CString.h"

extern int CSXModuleLoaded;

class CPlugin
{
	AMX			 amx;
	void*		 code;

	String		 name;
	String		 version;
	String		 title;
	String		 author;
	String		 errorMsg;

	unsigned int failcounter;
	int			 m_PauseFwd;
	int			 m_UnpauseFwd;
	int			 paused_fun;
	int			 status;
	CPlugin*	 next;
	int			 id;

	bool		 m_Debug;

public:

	inline const char* getName()	   { return name.c_str();	  }
	inline const char* getVersion()    { return version.c_str();  }
	inline const char* getTitle()	   { return title.c_str();	  }
	inline const char* getAuthor()	   { return author.c_str();	  }
	inline const char* getError()	   { return errorMsg.c_str(); }
	inline int		   getStatusCode() { return status;			  }
	inline int		   getId()	 const { return id; 			  }
	inline AMX*		   getAMX()		   { return &amx;			  }
	inline const AMX*  getAMX()  const { return &amx;			  }
};

#define UD_FINDPLUGIN  3
inline CPlugin* findPluginFast( AMX* amx ) { return ( CPlugin* )( amx->userdata[ UD_FINDPLUGIN ] ); }

void initMemory( void );

#endif