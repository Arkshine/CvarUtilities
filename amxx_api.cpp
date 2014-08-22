
#include "amxx_api.h"
#include "cvarsHooker.h"
#include "cvarsManager.h"

extern int ForwardCvarHookChanged;
extern AMX_NATIVE_INFO CvarUtilNatives[];

void CommandCvarUtil( void );

bool ConfigReady = false;
bool AmxxPluginsLoaded = false;
int CSXModuleLoaded = 0;

void OnAmxxAttach( void )
{
	REG_SVR_COMMAND( "cvarutil", CommandCvarUtil );
	REG_SVR_COMMAND( "cu", CommandCvarUtil );

	ConfigReady = CreateHook( ( void* )g_engfuncs.pfnCvar_DirectSet, ( void* )Cvar_DirectSet );

	if( ConfigReady )
	{	
		MF_AddNatives( CvarUtilNatives );
	}	
	else
	{
		MF_Log( " Module failed to load. Contact author...\n" );
		ModuleStatus.append( " -> Module failed to load. Contact author...\n\n" );
	}
}

void OnPluginsLoaded( void )
{
	if( ConfigReady ) 
	{
		const char* path = MF_GetLocalInfo( "csstats_score", "addons/amxmodx/data/csstats.amxx" );

		if( path && *path )
		{
			CSXModuleLoaded = MF_FindScriptByName( MF_BuildPathname( "%s", path ) ) == 0 ? 1 : 0;
		}

		AmxxPluginsLoaded = true;

		ForwardCvarHookChanged = MF_RegisterForward( "CvarHookChanged", ET_STOP, FP_CELL, FP_STRING, FP_STRING, FP_STRING, FP_STRING, FP_DONE );

		CvarsManager::sendPostponedCvars();
	}
}

void OnPluginsUnloaded( void )
{
	if( ConfigReady ) 
	{
		CvarsManager::copyAndDeleteCvarInfos();
		CvarsManager::setCopyCvarInfos();
	}
}

void OnMetaDetach( void )
{
	if( ConfigReady ) 
	{
		CvarsManager::deleteCvarsInfos();
		CvarsManager::CvarsInfoMin.clear();
	}
}

