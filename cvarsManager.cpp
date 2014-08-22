
#include "cvarsManager.h"
#include "cvarsHooker.h"

namespace CvarsManager
{
	CVector< CvarInfo* > CvarsInfo;
	CVector< CvarInfoMin* > CvarsInfoMin;
	CVector< CvarPostponed* > CvarsPostponed;

	bool NewCvar;

	CvarInfo* getCvarInfo( cvar_t* var, bool dontAdd )
	{
		if( var != NULL )
		{
			for( CVector< CvarInfo* >::iterator iter = CvarsInfo.begin(); iter != CvarsInfo.end(); ++iter )
			{
				if( ( *iter )->var == var )
				{
					return *iter;
				}
			}
		}

		if( dontAdd )
		{
			return NULL;
		}

		NewCvar = true;

		return addCvar( var );
	}

	bool checkDuplicate( CvarInfo* cvarInfo, int pluginId, String callback )
	{
		CVector< CvarPlugin* > plugin = cvarInfo->hook->plugin;

		for( CVector< CvarPlugin* >::iterator iter = plugin.begin(); iter != plugin.end(); ++iter )
		{
			if( ( *iter )->id == pluginId && !strcmp( ( *iter )->forward.callback.c_str(), callback.c_str() ) )
			{
				return true;
			}
		}

		return false;
	}

	bool checkDuplicateCache( CvarInfo* cvarInfo, int pluginId )
	{
		CVector< CvarCache* > cache = cvarInfo->cache;

		for( CVector< CvarCache* >::iterator iter = cache.begin(); iter != cache.end(); ++iter )
		{
			if( ( *iter )->pluginId == pluginId )
			{
				return true;
			}
		}

		return false;
	}

	bool registerForward( AMX* amx, String callback, int &forwardId )
	{
		return ( forwardId = MF_RegisterSPForwardByName( amx, callback.c_str(), FP_CELL, FP_STRING, FP_STRING, FP_STRING, FP_STRING, FP_DONE ) ) != -1;
	}

	CvarInfo* addCvar( cvar_t* var )
	{
		CvarInfo* cvarInfo = new CvarInfo;

		if( var )
			cvarInfo->var = var;
		else
			cvarInfo->var = new cvar_t;

		cvarInfo->description = "";
		cvarInfo->origValue	  = "";
		cvarInfo->registered  = false;
		cvarInfo->pluginId	  = -1;
		cvarInfo->status      = 0;
		cvarInfo->isOOB		  = false;
		cvarInfo->postponed	  = false;
		cvarInfo->sentToGlobal= false;
		cvarInfo->commandIndex= 0;

		cvarInfo->bound = new CvarBound;
		cvarInfo->hook  = new CvarHook;
		cvarInfo->lock  = new CvarLock;

		cvarInfo->bound->minPluginId = -1;
		cvarInfo->bound->maxPluginId = -1;
		cvarInfo->bound->hasMin      = false;
		cvarInfo->bound->hasMax      = false;
		cvarInfo->bound->fMin	     = 0.0;
		cvarInfo->bound->fMax	     = 0.0;

		cvarInfo->hook->registered	= false;
		cvarInfo->hook->activeHooks = 0;

		cvarInfo->lock->registered	= false;
		cvarInfo->lock->active		= false;
		cvarInfo->lock->pluginId	= -1;
		cvarInfo->lock->origValue   = "";
		cvarInfo->lock->currValue   = "";

		CvarsInfo.push_back( cvarInfo );

		return cvarInfo;
	}

	void addPlugin( CvarInfo* cvarInfo, int pluginId, int forwardId, String callback )
	{
		CvarPlugin* plugin = new CvarPlugin;

		plugin->id = pluginId;
		plugin->forward.Set( forwardId, FSTATE_OK, callback );

		cvarInfo->hook->plugin.push_back( plugin );
	}

	int setHookState( fwdstate state, CVector< CvarPlugin* > plugin, int pluginId, String callback )
	{
		bool checkCallback = !callback.empty();
		int found = 0;

		for( CVector< CvarPlugin* >::iterator iter = plugin.begin(); iter != plugin.end(); ++iter )
		{
			CvarPlugin* plugin = *iter;

			if( plugin->id == pluginId )
			{
				if( checkCallback && strcmp( plugin->forward.callback.c_str(), callback.c_str() ) )
				{
					continue;
				}

				plugin->forward.state = state;
				found++;
			}
		}

		return found;
	}

	void setCvarStatus( CvarInfo* cvarInfo, bool state, int flags )
	{
		if( state )
		{
			cvarInfo->status |= flags;
		}
		else
		{
			cvarInfo->status &= ~flags;
		}
	}

	void deleteCvarsInfos( void )
	{
		for( CVector< CvarInfo* >::iterator cvarIter = CvarsInfo.begin(); cvarIter != CvarsInfo.end(); ++cvarIter )
		{
			CvarInfo* cvarInfo = *cvarIter;

			for( CVector< CvarPlugin* >::iterator pluginIter = cvarInfo->hook->plugin.begin(); pluginIter != cvarInfo->hook->plugin.end(); ++pluginIter )
			{
				delete ( *pluginIter );
			}

			cvarInfo->hook->plugin.clear();

			for( CVector< CvarCache* >::iterator cacheIter = cvarInfo->cache.begin(); cacheIter != cvarInfo->cache.end(); ++cacheIter )
			{
				delete ( *cacheIter );
			}

			cvarInfo->cache.clear();

			delete ( cvarInfo->hook );
			delete ( cvarInfo->lock );
			delete ( cvarInfo->bound );
		}

		CvarsInfo.clear();
		CvarsPostponed.clear();
	}

	void clearCvarsInfos( void )
	{
		for( CVector< CvarInfo* >::iterator cvarIter = CvarsInfo.begin(); cvarIter != CvarsInfo.end(); ++cvarIter )
		{
			CvarInfo* cvarInfo = *cvarIter;

			cvarInfo->registered  = false;
			cvarInfo->pluginId	  = -1;
			cvarInfo->status      = 0;
			cvarInfo->isOOB		  = false;
			cvarInfo->postponed	  = false;
			cvarInfo->sentToGlobal= false;

			for( CVector< CvarPlugin* >::iterator pluginIter = cvarInfo->hook->plugin.begin(); pluginIter != cvarInfo->hook->plugin.end(); ++pluginIter )
			{
				delete ( *pluginIter );
			}

			cvarInfo->hook->plugin.clear();
			cvarInfo->hook->registered	= false;
			cvarInfo->hook->activeHooks = 0;

			for( CVector< CvarCache* >::iterator cacheIter = cvarInfo->cache.begin(); cacheIter != cvarInfo->cache.end(); ++cacheIter )
			{
				delete ( *cacheIter );
			}

			cvarInfo->cache.clear();

			delete ( cvarInfo->hook );
			delete ( cvarInfo->lock );
			delete ( cvarInfo->bound );
		}

		CvarsPostponed.clear();
	}

	void copyAndDeleteCvarInfos( void )
	{
		CvarsInfoMin.clear();

		for( CVector< CvarInfo* >::iterator cvarIter = CvarsInfo.begin(); cvarIter != CvarsInfo.end(); ++cvarIter )
		{
			CvarInfo* cvarInfo = *cvarIter;

			CvarInfoMin* cvarInfoMin = new CvarInfoMin;

			cvarInfoMin->var			= cvarInfo->var;
			cvarInfoMin->description	= cvarInfo->description;
			cvarInfoMin->origValue		= cvarInfo->origValue;

			CvarsInfoMin.push_back( cvarInfoMin );

			//printf( "BACKUP : %s - %s - %s\n", cvarInfoMin->var->name, cvarInfoMin->description.c_str(), cvarInfoMin->origValue.c_str() );
		}

		deleteCvarsInfos();
	}

	void setCopyCvarInfos( void )
	{
		for( CVector< CvarInfoMin* >::iterator cvarIter = CvarsInfoMin.begin(); cvarIter != CvarsInfoMin.end(); ++cvarIter )
		{
			CvarInfoMin* cvarInfo = *cvarIter;

			CvarInfo* info = addCvar( cvarInfo->var );

			info->description.assign( cvarInfo->description );
			info->origValue.assign( cvarInfo->origValue );

			//printf( "SET : %s - %s - %s\n", info->var->name, info->description.c_str(), info->origValue.c_str() );
		}
	}

	int UTIL_ReadFlags( const char* c ) 
	{
		int flags = 0;

		while( *c )
		{
			flags |= ( 1 << ( *c++ - 'a' ) );
		}

		return flags;
	}

	String getStatusFlags( int flags )
	{
		String flagsName;

		if( flags & CvarStatus_HookRegistered	) flagsName.append( "HookRegistered " );
		if( flags & CvarStatus_HookActive		) flagsName.append( "HookActive " );
		if( flags & CvarStatus_LockRegistered	) flagsName.append( "LockRegistered "	);
		if( flags & CvarStatus_LockActive		) flagsName.append( "LockActive "	);
		if( flags & CvarStatus_HasMinValue		) flagsName.append( "HasMinValue " );
		if( flags & CvarStatus_HasMaxValue		) flagsName.append( "HasMaxValue " );
		if( flags & CvarStatus_NewCvar			) flagsName.append( "NewCvar "	);
		if( flags & CvarStatus_WasOutOfBound	) flagsName.append( "WasOutOfBound " );

		if( !flagsName.size() )
		{
			flagsName.assign( "No Flags" );
		}
		else
		{
			flagsName.trim();

			for( size_t i = 0; i < flagsName.size(); i++ )
			{
				if( flagsName[ i ] == ' ' )
				{
					flagsName.at( i, '|' );
				}
			}
		}

		return flagsName;
	} 

	void sendPostponedCvars( void )
	{
		if( CvarsPostponed.size() )
		{
			for( CVector< CvarPostponed* >::iterator iter = CvarsPostponed.begin(); iter != CvarsPostponed.end(); ++iter )
			{
				g_engfuncs.pfnCvar_DirectSet( ( *iter )->var, ( char* )( *iter )->value );
			}

			CvarsPostponed.clear();
		}	
	}
}