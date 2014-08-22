
#ifndef _INCLUDE_CVARS_HOOKER_
#define _INCLUDE_CVARS_HOOKER_

#include "amxxmodule.h"
#include "CString.h"
#include "chooker.h"

typedef void ( *CvarDirectSet )( cvar_t*, const char* );

extern void Cvar_DirectSet( cvar_t* var, const char* value );
extern CvarDirectSet CvarDirectSetOrig;
extern CFunc* CvarDirectSetHook;

extern String ModuleStatus;

bool CreateHook( void* referenceAddress, void* callbackAddress );


#endif