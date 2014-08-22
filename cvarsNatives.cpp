
#include "amxx_api.h"
#include "cvarsNatives.h"
#include "cvarsManager.h"
#include "cvarsHooker.h"
#include <float.h>
#include "const.h"

using namespace CvarsManager;

cell AMX_NATIVE_CALL CvarNative::HookChange( AMX* amx, cell* params )
{
	//printf( "\nCvarNative::HookChange\n\n" );

	// params[1] = cvar pointer
	// params[2] = callback[]
	// params[3] = ignore first call

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error;
	}

	char* callback = MF_GetAmxString( amx, params[ 2 ], 0, NULL );

	int		  pluginId = findPluginFast( amx )->getId();
	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	//printf( "PLUGIN ID = %d\n",pluginId );

	if( checkDuplicate( cvarInfo, pluginId, callback ) )
	{
		return CvarReturn_Duplicated;
	}

	int forwardId;

	if( !registerForward( amx, callback, forwardId ) )
	{
		MF_LogError( amx, AMX_ERR_NOTFOUND, "Invalid hook callback \"%s\" specified for cvar \"%s\"", callback, paramCvar->name );
		return CvarReturn_Error;
	}

	bool ignoreFirstCall = !!params[ 3 ];

	addPlugin( cvarInfo, pluginId, forwardId, callback );

	cvarInfo->hook->registered = true;
	cvarInfo->hook->activeHooks++;

	setCvarStatus( cvarInfo, true, CvarStatus_HookRegistered | CvarStatus_HookActive );
	
	if( !ignoreFirstCall )
	{
		MF_ExecuteForward( forwardId, reinterpret_cast< cell >( cvarInfo->var ), 
			cvarInfo->sentToGlobal ? cvarInfo->origValue.c_str() : "", 
			cvarInfo->var->string, 
			cvarInfo->var->name, 
			cvarInfo->origValue.c_str() );
	}

	return cvarInfo->lock->active ? CvarReturn_AlreadyLocked : cvarInfo->hook->activeHooks;
}

cell AMX_NATIVE_CALL CvarNative::HookChangeAll( AMX* amx, cell* params )
{
	// params[1] = callback[]
	// params[2] = ignore first call

	char* callback = MF_GetAmxString( amx, params[ 1 ], 0, NULL );
	int	  pluginId = findPluginFast( amx )->getId();
	
	int forwardId = -1;
	int found     = 0;

	for( CVector< CvarInfo* >::iterator iter = CvarsInfo.begin(); iter != CvarsInfo.end(); ++iter )
	{
		CvarInfo* info = *iter;

		//printf( "ALL : %s : registered ? %s, saved pluginId = %d, this pluginId = %d \n", info->var->name, info->registered ? "yes" : "no", info->pluginId, pluginId  );

		if( info->registered && info->pluginId == pluginId )
		{
			if( checkDuplicate( info, pluginId, callback ) )
			{
				continue;
			}

			if( !registerForward( amx, callback, forwardId ) )
			{
				MF_LogError( amx, AMX_ERR_NOTFOUND, "Invalid hook callback \"%s\"", callback );
				return CvarReturn_Error;
			}

			bool ignoreFirstCall = !!params[ 2 ];

			addPlugin( info, pluginId, forwardId, callback );

			info->hook->registered = true;
			info->hook->activeHooks++;

			setCvarStatus( info, true, CvarStatus_HookRegistered | CvarStatus_HookActive );
			
			if( !ignoreFirstCall )
			{
				MF_ExecuteForward( forwardId, reinterpret_cast< cell >( info->var ), 
					info->sentToGlobal ? info->origValue.c_str() : "", 
					info->var->string, 
					info->var->name, 
					info->origValue.c_str() );
			}

			found++;
		}
	}

	return found;
}

cell AMX_NATIVE_CALL CvarNative::EnableHook( AMX* amx, cell* params )
{
	// params[1] = cvar pointer
	// params[2] = callback[] (optional)

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error;
	}

	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	if( !cvarInfo->hook->registered )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Cvar \"%s\" has no active hook", paramCvar->name );
		return CvarReturn_Error;
	}

	char* callback = MF_GetAmxString( amx, params[ 2 ], 0, NULL );
	int	  pluginId = findPluginFast( amx )->getId();

	int	found = setHookState( FSTATE_OK, cvarInfo->hook->plugin, pluginId, callback );

	if( !found && callback )
	{
		MF_LogError( amx, AMX_ERR_NOTFOUND, "Invalid hook callback \"%s\" specified for cvar \"%s\"", callback, paramCvar->name );
		return CvarReturn_Error;
	}

	cvarInfo->hook->activeHooks += found;
	setCvarStatus( cvarInfo, true, CvarStatus_HookActive );

	return found;
} 

cell AMX_NATIVE_CALL CvarNative::EnableHookAll( AMX* amx, cell* params )
{
	// params[1] = cvar pointer

	int	pluginId = findPluginFast( amx )->getId();

	int	numFoundCvars = 0;
	int numHookSet	  = 0;

	for( CVector< CvarInfo* >::iterator iter = CvarsInfo.begin(); iter != CvarsInfo.end(); ++iter )
	{
		CvarInfo* info = *iter;

		if( info->registered && info->pluginId == pluginId )
		{
			numHookSet = setHookState( FSTATE_OK, info->hook->plugin, pluginId, NULL );

			info->hook->activeHooks += numHookSet;
			setCvarStatus( info, true, CvarStatus_HookActive );

			numFoundCvars++;
		}
	}

	return numFoundCvars;
}

cell AMX_NATIVE_CALL CvarNative::DisableHook( AMX* amx, cell* params )
{
	// params[1] = cvar pointer
	// params[2] = callback[] (optional)
	
	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error;
	}

	char* callback = MF_GetAmxString( amx, params[ 2 ], 0, NULL );

	int		  pluginId = findPluginFast( amx )->getId();
	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	if( !cvarInfo->hook->registered )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Cvar \"%s\" has no active hook", paramCvar->name );
		return CvarReturn_Error;
	}

	int	found = setHookState( FSTATE_STOP, cvarInfo->hook->plugin, pluginId, callback );

	if( !found && callback )
	{
		MF_LogError( amx, AMX_ERR_NOTFOUND, "Invalid hook callback \"%s\" specified for cvar \"%s\"", callback, paramCvar->name );
		return CvarReturn_Error;
	}

	cvarInfo->hook->activeHooks -= found;
	setCvarStatus( cvarInfo, false, CvarStatus_HookActive );

	return found;
}

cell AMX_NATIVE_CALL CvarNative::DisableHookAll( AMX* amx, cell* params )
{
	// params[1] = cvar pointer

	int	pluginId = findPluginFast( amx )->getId();

	int	numFoundCvars = 0;
	int numHookSet	  = 0;

	for( CVector< CvarInfo* >::iterator iter = CvarsInfo.begin(); iter != CvarsInfo.end(); ++iter )
	{
		CvarInfo* info = *iter;

		if( info->registered && info->pluginId == pluginId )
		{
			numHookSet = setHookState( FSTATE_STOP, info->hook->plugin, pluginId, NULL );

			info->hook->activeHooks -= numHookSet;
			setCvarStatus( info, false, CvarStatus_HookActive );

			numFoundCvars++;
		}
	}

	return numFoundCvars;
}

cell AMX_NATIVE_CALL CvarNative::HookNum( AMX* amx, cell* params )
{
	// param[1] = cvar pointer

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error;
	}

	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	if( !cvarInfo->hook->registered )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Cvar \"%s\" has no active hook", paramCvar->name );
		return CvarReturn_Error;
	}

	return cvarInfo->hook->plugin.size();
}

cell AMX_NATIVE_CALL CvarNative::HookInfo( AMX* amx, cell* params )
{
	// param[1] = index
	// param[2] = cvar pointer
	// param[3] = plugin id
	// param[4] = forward id (optional)
	// param[5] = callback[] (optional)
	// param[6] = callback length

	int index = params[ 1 ];
	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 2 ] );

	if( params[ 2 ] <= 0 || !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error;
	}

	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	if( !cvarInfo->hook->registered )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Cvar \"%s\" has no active hook", paramCvar->name );
		return CvarReturn_Error;
	}

	CVector< CvarPlugin* > plugin = cvarInfo->hook->plugin;
	int size = static_cast< int >( plugin.size() );

	if( index < 0 && index > size )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid index %d, expected value between 0 and %d", size );
		return CvarReturn_Error;
	}

	int iterIndex = 0;
	int state = 0;

	for( CVector< CvarPlugin* >::iterator iter = plugin.begin(); iter != plugin.end(); ++iter )
	{
		if( iterIndex++ == index )
		{	
			CvarPlugin* plugin = ( *iter );

			*MF_GetAmxAddr( amx, params[ 3 ] ) = plugin->id;
			*MF_GetAmxAddr( amx, params[ 4 ] ) = plugin->forward.id;

			MF_SetAmxString( amx, params[ 5 ], plugin->forward.callback.c_str(), params[ 6 ] );

			state = plugin->forward.state == FSTATE_OK ? 1 : 0;
		}
	}

	return state;
}

cell AMX_NATIVE_CALL CvarNative::LockValue( AMX* amx, cell* params )
{
	// param[1] = cvar pointer
	// param[2] = value[]
	// param[3] = fvalue

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error;
	}

	int	  valueLength;
	char* valueToLock = MF_GetAmxString( amx, params[ 2 ], 0, &valueLength );

	if( !valueLength )
	{
		sprintf( valueToLock, "%f", amx_ctof( params[ 3 ] ) );
	}

	int		  pluginId = findPluginFast( amx )->getId();
	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	if( cvarInfo->lock->registered && cvarInfo->lock->pluginId != pluginId )
	{
		return !strcmp( valueToLock, cvarInfo->lock->origValue.c_str() ) ? CvarReturn_AlreadySet : CvarReturn_AlreadyLocked;
	}

	//::Cvar_DirectSet( paramCvar, valueToLock );

	//CvarsHooker::FuncCvarDirectSet->Restore();
	//CvarsHooker::Func_Cvar_DirectSet( paramCvar, valueToLock );
	//CvarsHooker::FuncCvarDirectSet->Patch();

	g_engfuncs.pfnCvar_DirectSet( paramCvar, valueToLock );

	cvarInfo->lock->pluginId   = pluginId;
	cvarInfo->lock->registered = true;
	cvarInfo->lock->active     = true;

	cvarInfo->lock->origValue.assign( paramCvar->string );
	cvarInfo->lock->currValue.assign( valueToLock );

	setCvarStatus( cvarInfo, true, CvarStatus_LockRegistered | CvarStatus_LockActive );

	return CvarReturn_NoError;
}

cell AMX_NATIVE_CALL CvarNative::EnableLock( AMX* amx, cell* params )
{
	// param[1] = cvar pointer

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error;
	}

	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	if( !cvarInfo->lock->registered )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Cvar \"%s\" has no registered lock", paramCvar->name );
		return CvarReturn_Error;
	}

	if( cvarInfo->lock->active )
	{
		return CvarReturn_AlreadySet;
	}

	//::Cvar_DirectSet( paramCvar, cvarInfo->lock->currValue.c_str() );

	//CvarsHooker::FuncCvarDirectSet->Restore();
	//CvarsHooker::Func_Cvar_DirectSet( paramCvar, cvarInfo->lock->currValue.c_str() );
	//CvarsHooker::FuncCvarDirectSet->Patch();

	g_engfuncs.pfnCvar_DirectSet( paramCvar, ( char* )cvarInfo->lock->currValue.c_str() );

	cvarInfo->lock->active = true;
	setCvarStatus( cvarInfo, true, CvarStatus_LockActive );

	return CvarReturn_NoError;
}

cell AMX_NATIVE_CALL CvarNative::DisableLock( AMX* amx, cell* params )
{
	// param[1] = cvar pointer

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error;
	}

	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	if( !cvarInfo->lock->registered )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Cvar \"%s\" has no registered lock", paramCvar->name );
		return CvarReturn_Error;
	}

	if( !cvarInfo->lock->active )
	{
		return CvarReturn_AlreadySet;
	}

	cvarInfo->lock->active = false;
	setCvarStatus( cvarInfo, false, CvarStatus_LockActive );

	//::Cvar_DirectSet( paramCvar, cvarInfo->lock->origValue.c_str() );

	//CvarsHooker::FuncCvarDirectSet->Restore();
	//CvarsHooker::Func_Cvar_DirectSet( paramCvar, cvarInfo->lock->origValue.c_str() );
	//CvarsHooker::FuncCvarDirectSet->Patch();

	g_engfuncs.pfnCvar_DirectSet( paramCvar, ( char* )cvarInfo->lock->origValue.c_str() );

	return CvarReturn_NoError;
}

cell AMX_NATIVE_CALL CvarNative::LockInfo( AMX* amx, cell* params )
{
	// param[1] = cvar pointer
	// param[2] = plugin id
	// param[3] = value[] (optional)
	// param[4] = value[] length

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error;
	}

	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	if( !cvarInfo->lock->registered )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Cvar \"%s\" has no registered lock", paramCvar->name );
		return CvarReturn_Error;
	}

	*MF_GetAmxAddr( amx, params[ 2 ] ) = cvarInfo->lock->pluginId;
	MF_SetAmxString( amx, params[ 3 ], cvarInfo->var->string, params[ 4 ] );

	return cvarInfo->lock->active;
}

cell AMX_NATIVE_CALL CvarNative::SetBounds( AMX* amx, cell* params )
{
	// param[1] = cvar pointer
	// param[2] = bounds type
	// param[3] = set type
	// param[4] = bound value
	// param[5] = fixedBounds

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error;
	}

	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	int  type = params[ 2 ];
	bool set  = !!params[ 3 ];

	cvarInfo->bound->forceInterval = !!params[ 9 ];

	int  pluginId = findPluginFast( amx )->getId();

	float newValue	 = 0.0;
	bool  outOfBound = false;

	switch( type )
	{
		case CvarBound_Lower : 
		{
			if( cvarInfo->bound->hasMin )
			{
				if( cvarInfo->bound->minPluginId != pluginId )
					return CvarReturn_AlreadyBounded;

				if( set )
					return CvarReturn_AlreadySet;
			}

			setCvarStatus( cvarInfo, set, CvarStatus_HasMinValue );
			cvarInfo->bound->hasMin = set;

			if( set )
			{
				cvarInfo->bound->minPluginId = pluginId;
				cvarInfo->bound->fMin		 = newValue = amx_ctof( params[ 4 ] );

				outOfBound = paramCvar->value < newValue;
			}

			break;
		}
		case CvarBound_Upper : 
		{
			if( cvarInfo->bound->hasMax )
			{
				if( cvarInfo->bound->maxPluginId != pluginId )
					return CvarReturn_AlreadyBounded;

				if( set )
					return CvarReturn_AlreadySet;
			}

			setCvarStatus( cvarInfo, set, CvarStatus_HasMaxValue );
			cvarInfo->bound->hasMax = set;

			if( set )
			{
				cvarInfo->bound->maxPluginId = pluginId;
				cvarInfo->bound->fMax		 = newValue = amx_ctof( params[ 4 ] );

				outOfBound = paramCvar->value > newValue;
			}

			break;
		}
	}

	if( !outOfBound && cvarInfo->bound->forceInterval && cvarInfo->bound->hasMin && cvarInfo->bound->hasMax )
	{
		if( paramCvar->value > cvarInfo->bound->fMin && paramCvar->value < cvarInfo->bound->fMax )
		{
			outOfBound = true;
			newValue = cvarInfo->bound->fMax;
		}
	}

	if( !cvarInfo->lock->active && outOfBound )
	{
		cvarInfo->isOOB = true;
		CVAR_SET_FLOAT( paramCvar->name, newValue );
		cvarInfo->origValue.assign( cvarInfo->var->string );
	}

	return !cvarInfo->lock->active;
}

cell AMX_NATIVE_CALL CvarNative::GetBounds( AMX* amx, cell* params )
{
	// param[1] = cvar pointer
	// param[2] = bounds type
	// param[3] = bound value
	// param[4] = plugin id (optional)

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error;
	}

	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	switch( params[ 2 ] )
	{
		case CvarBound_Lower : 
		{
			if( !cvarInfo->bound->hasMin )
			{
				MF_LogError( amx, AMX_ERR_NATIVE, "Cvar \"%s\" has no active minimum bound", paramCvar->name );
				return CvarReturn_Error;
			}

			*MF_GetAmxAddr( amx, params[ 3 ] ) = amx_ftoc( cvarInfo->bound->fMin );
			*MF_GetAmxAddr( amx, params[ 4 ] ) = cvarInfo->bound->minPluginId;
			*MF_GetAmxAddr( amx, params[ 5 ] ) = cvarInfo->bound->forceInterval;

			break;
		}
		case CvarBound_Upper : 
		{
			if( !cvarInfo->bound->hasMax )
			{
				MF_LogError( amx, AMX_ERR_NATIVE, "Cvar \"%s\" has no active maximum bound", paramCvar->name );
				return CvarReturn_Error;
			}

			*MF_GetAmxAddr( amx, params[ 3 ] ) = amx_ftoc( cvarInfo->bound->fMax );
			*MF_GetAmxAddr( amx, params[ 4 ] ) = cvarInfo->bound->maxPluginId;
			*MF_GetAmxAddr( amx, params[ 5 ] ) = cvarInfo->bound->forceInterval;

			break;
		}
	}

	return CvarReturn_NoError;
}

cell AMX_NATIVE_CALL CvarNative::Register( AMX* amx, cell* params )
{
	//printf( "\nCvarNative::Register\n\n" );

	// param[1] = cvar name[]
	// param[2] = value[]
	// param[3] = description[] (optional)
	// param[4] = flags			(optional)
	// param[5] = hasMin		(optional)
	// param[6] = minValue		(optional)
	// param[7] = hasMax		(optional)
	// param[8] = maxValue		(optional)
	// param[9] = forceInterval (optional)

	char*	name	= MF_GetAmxString( amx, params[ 1 ], 0, NULL );
	cvar_t* pointer = CVAR_GET_POINTER( name );

	CvarInfo* cvarInfo = getCvarInfo( pointer, true );
	bool notRegistered = false;

	//printf("REGISTER: name = %s, pointer = %d, cvarInfo = %d\n", name, pointer, cvarInfo );

	if( cvarInfo == NULL )
	{
		cvarInfo = addCvar( pointer );
		notRegistered = true;
	}

	if( !cvarInfo->registered )
	{
		bool  outOfBound = false;
		float newValue   = 0.0; 

		cvarInfo->registered = true;
		cvarInfo->pluginId	 = findPluginFast( amx )->getId();
		
		setCvarStatus( cvarInfo, true, CvarStatus_NewCvar );

		if( notRegistered )
		{
			char* description = MF_GetAmxString( amx, params[ 3 ], 1, NULL );
			cvarInfo->description.assign( description );

			if( !pointer )
			{
				cvarInfo->var->name   = name;
				cvarInfo->var->flags  = params[ 4 ];
				cvarInfo->var->string = "";
				cvarInfo->var->value  = 0.0;

				CVAR_REGISTER( cvarInfo->var );

				delete ( cvarInfo->var );
			}

			cvarInfo->var = CVAR_GET_POINTER( name );
		}

		char* string = MF_GetAmxString( amx, params[ 2 ], 2, NULL );

		cvarInfo->bound->forceInterval = !!params[ 9 ];

		if( params[ 5 ] )
		{
			float minValue = amx_ctof( params[ 6 ] );

			cvarInfo->bound->hasMin	     = true;
			cvarInfo->bound->fMin	     = minValue;
			cvarInfo->bound->minPluginId = cvarInfo->pluginId;

			if( atof( string ) < minValue )
			{
				outOfBound = true;
				newValue = minValue;
			}

			setCvarStatus( cvarInfo, true, CvarStatus_HasMinValue );
		}

		if( params[ 7 ] )
		{
			float maxValue = amx_ctof( params[ 8 ] );

			cvarInfo->bound->hasMax      = true;
			cvarInfo->bound->fMax	     = maxValue;
			cvarInfo->bound->maxPluginId = cvarInfo->pluginId;			

			if( !outOfBound && atof( string ) > maxValue )
			{
				outOfBound = true;
				newValue = maxValue;
			}
			setCvarStatus( cvarInfo, true, CvarStatus_HasMaxValue );
		}

		if( notRegistered || outOfBound )
		{
			if( outOfBound )
			{	
				CVAR_SET_FLOAT( name, newValue );
				cvarInfo->origValue.assign( cvarInfo->var->string );
			}
			else
			{
				//::Cvar_DirectSet( cvarInfo->var, string );

				//CvarsHooker::FuncCvarDirectSet->Restore();
				//CvarsHooker::Func_Cvar_DirectSet( cvarInfo->var, string );
				//CvarsHooker::FuncCvarDirectSet->Patch();

				g_engfuncs.pfnCvar_DirectSet( cvarInfo->var, string );
			}
		}
	}

	return reinterpret_cast< cell >( cvarInfo->var );
}

cell AMX_NATIVE_CALL CvarNative::Num( AMX* amx, cell* params )
{
	return static_cast< int >( CvarsInfo.size() );
}

cell AMX_NATIVE_CALL CvarNative::Info( AMX* amx, cell* params )
{
	// params[1]  = index
	// params[2]  = cvar pointer
	// params[3]  = plugin id
	// params[4]  = cvar name
	// params[5]  = cvar name length
	// params[6]  = cvar string
	// params[7]  = cvar string length
	// params[8]  = cvar description
	// params[9] = cvar description length
	// params[10] = cvar flags
	// params[11] = cvar default value
	// params[12] = cvar default value length

	int index = params[ 1 ];
	int size = static_cast< int >( CvarsInfo.size() );

	if( index < 0 && index > size )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid index %d, expected value between 0 and %d", index, size );
		return CvarReturn_Error;
	}

	int iterIndex = 0;

	for( CVector< CvarInfo* >::iterator iter = CvarsInfo.begin(); iter != CvarsInfo.end(); ++iter )
	{
		if( iterIndex++ == index )
		{	
			CvarInfo* info = *iter;

			*MF_GetAmxAddr( amx, params[ 2 ] ) = reinterpret_cast< cell >( info->var );
			*MF_GetAmxAddr( amx, params[ 3 ] ) = info->pluginId;

			MF_SetAmxString( amx, params[ 4 ], info->var->name, params[ 5 ] );
			MF_SetAmxString( amx, params[ 6 ], info->var->string, params[ 7 ] );
			MF_SetAmxString( amx, params[ 8 ], info->description.c_str(), params[ 9 ] );
			MF_SetAmxString( amx, params[ 11 ], info->origValue.c_str(), params[ 12 ] );

			*MF_GetAmxAddr( amx, params[ 10 ] ) = info->var->flags;

			return info->status;
		}
	}

	return 1;
}

cell AMX_NATIVE_CALL CvarNative::GetStatus( AMX* amx, cell* params )
{
	// param[1] = cvar pointer

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error; 
	}

	return getCvarInfo( paramCvar )->status;
}

cell AMX_NATIVE_CALL CvarNative::GetName( AMX* amx, cell* params )
{
	// param[1] = cvar pointer
	// param[2] = buffer[]
	// param[3] = buffer length

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error; 
	}

	return MF_SetAmxString( amx, params[ 2 ], paramCvar->name, params[ 3 ] );
}

cell AMX_NATIVE_CALL CvarNative::SetDescription( AMX* amx, cell* params )
{
	// param[1] = cvar pointer
	// param[2] = description[]

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error; 
	}

	int   descriptionLen = 0;
	char* description = MF_GetAmxString( amx, params[ 2 ], 0, &descriptionLen );

	getCvarInfo( paramCvar )->description.assign( description );

	return descriptionLen;
}

cell AMX_NATIVE_CALL CvarNative::GetDescription( AMX* amx, cell* params )
{
	// param[1] = cvar pointer
	// param[2] = description[]
	// param[3] = description length

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error; 
	}

	return MF_SetAmxString( amx, params[ 2 ], getCvarInfo( paramCvar )->description.c_str(), params[ 3 ] );
}

cell AMX_NATIVE_CALL CvarNative::PluginInfo( AMX* amx, cell* params )
{
	// param[1] = plugin id
	// param[2] = plugin[] name
	// param[3] = plugin name length
	// param[4] = plugin[] title
	// param[5] = plugin title length

	int pluginId = params[ 1 ];

	if( pluginId < 0 )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid index %d, expected value between >= 0", pluginId );
		return CvarReturn_Error;
	}
	
	CPlugin* plugin = findPluginFast( MF_GetScriptAmx( pluginId + CSXModuleLoaded ) );

	if( !plugin )
	{
		return CvarReturn_Error;
	}

	MF_SetAmxString( amx, params[ 2 ], plugin->getName(), params[ 3 ] );
	MF_SetAmxString( amx, params[ 4 ], plugin->getTitle(), params[ 5 ] );

	return CvarReturn_NoError;
}

cell AMX_NATIVE_CALL CvarNative::Cache( AMX* amx, cell* params )
{
	//printf( "\nCvarNative::Cache\n\n" );

	// param[1] = cvar pointer
	// param[2] = cvar value type
	// param[3] = variable by ref

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error; 
	}
	
	CvarInfo* cvarInfo = getCvarInfo( paramCvar );
	int pluginId = findPluginFast( amx )->getId();

	if( checkDuplicateCache( cvarInfo, pluginId ) )
	{
		return CvarReturn_DuplicatedCache;
	}

	int  type		= params[ 2 ];
	cell data		= params[ 3 ];
	int  bufferSize = 0;

	unsigned int numParam = *params / sizeof( cell );

	if( numParam == 4 )
	{
		bufferSize = *MF_GetAmxAddr( amx, params[ 4 ] );
	}

	if( type == CvarType_String && ( numParam == 3 || bufferSize <= 0 ) )
	{
		if( numParam == 3 )
		{
			MF_LogError( amx, AMX_ERR_NATIVE, "Expected 4 arguments, got 3. Missing buffer size argument." );
		}
		else if( bufferSize <= 0 )
		{
			MF_LogError( amx, AMX_ERR_NATIVE, "Invalid buffer size." );
		}

		return CvarReturn_Error; 
	}

	switch( type )
	{
		case CvarType_Int :
		{
			*MF_GetAmxAddr( amx, data ) = atoi( paramCvar->string );
			break;
		}
		case CvarType_Float :
		{
			*MF_GetAmxAddr( amx, data ) = amx_ftoc( paramCvar->value );
			break;
		}
		case CvarType_String :
		{
			MF_SetAmxString( amx, data, paramCvar->string, bufferSize );
			break;
		}
		case CvarType_Flags :
		{
			*MF_GetAmxAddr( amx, data ) = UTIL_ReadFlags( paramCvar->string );
			break;
		}
	}

	CvarCache *cache = new CvarCache;

	cache->pluginId     = pluginId;
	cache->type	        = type;
	cache->amxx		    = amx;
	cache->dataOfs		= data;

	if( type == CvarType_String )
	{
		cache->stringLenOfs = params[ 4 ];
	}

	cvarInfo->cache.push_back( cache );

	setCvarStatus( cvarInfo, true, CvarStatus_Cached );

	return CvarReturn_NoError;
}

cell AMX_NATIVE_CALL CvarNative::GetDefault( AMX* amx, cell* params )
{
	// param[1] = cvar pointer
	// param[2] = cvar value type

	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error; 
	}

	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	unsigned int numParam = *params / sizeof( cell );

	switch( params[ 2 ] )
	{
		case CvarType_Int : 
		{
			return atoi( cvarInfo->origValue.c_str() );
			break;
		}
		case CvarType_Float :
		{
			if( numParam == 3 )
			{
				*MF_GetAmxAddr( amx, params[ 3 ] ) = amx_ftoc( atof( cvarInfo->origValue.c_str() ) );
				return atoi( cvarInfo->origValue.c_str() );
			}

			return amx_ftoc( atof( cvarInfo->origValue.c_str() ) ); 
			break;
		}
		case CvarType_Flags :
		{
			return UTIL_ReadFlags( cvarInfo->origValue.c_str() );
			break;
		}
		case CvarType_String :
		{
			int bufferSize = 0;

			if( numParam == 4 )
			{
				bufferSize = *MF_GetAmxAddr( amx, params[ 4 ] );
			}

			if( numParam == 3 || bufferSize <= 0 )
			{
				if( numParam == 3 )
				{
					MF_LogError( amx, AMX_ERR_NATIVE, "Expected 4 arguments, got 3. Missing buffer size argument." );
				}
				else if( bufferSize <= 0 )
				{
					MF_LogError( amx, AMX_ERR_NATIVE, "Invalid buffer size." );
				}

				return CvarReturn_Error;
			}

			return MF_SetAmxString( amx, params[ 3 ], cvarInfo->origValue.c_str(), params[ 4 ] );
		}
	}

	return CvarReturn_Error;
}

cell AMX_NATIVE_CALL CvarNative::Reset( AMX* amx, cell* params )
{
	cvar_t* paramCvar = reinterpret_cast< cvar_t* >( params[ 1 ] );

	if( !paramCvar )
	{
		MF_LogError( amx, AMX_ERR_NATIVE, "Invalid cvar pointer %x", paramCvar );
		return CvarReturn_Error; 
	}

	CvarInfo* cvarInfo = getCvarInfo( paramCvar );

	if( cvarInfo->lock->active )
	{
		return CvarReturn_AlreadyLocked;
	}

	//CvarsHooker::FuncCvarDirectSet->Restore();
	//CvarsHooker::Func_Cvar_DirectSet( cvarInfo->var, cvarInfo->origValue.c_str() );
	//CvarsHooker::FuncCvarDirectSet->Patch();

	g_engfuncs.pfnCvar_DirectSet( cvarInfo->var, ( char* )cvarInfo->origValue.c_str() );

	return CvarReturn_NoError;
}


AMX_NATIVE_INFO CvarUtilNatives[] = 
{
	{ "CvarHookChange"    , CvarNative::HookChange     },
	{ "CvarHookChangeAll" , CvarNative::HookChangeAll  },
	{ "CvarEnableHook"    , CvarNative::EnableHook     },
	{ "CvarEnableHookAll" , CvarNative::EnableHookAll  },
	{ "CvarDisableHook"   , CvarNative::DisableHook    },
	{ "CvarDisableHookAll", CvarNative::DisableHookAll },
	{ "CvarHookNum"       , CvarNative::HookNum        },
	{ "CvarHookInfo"      , CvarNative::HookInfo       },
	{ "CvarLockValue"     , CvarNative::LockValue      },
	{ "CvarEnableLock"    , CvarNative::EnableLock     },
	{ "CvarDisableLock"   , CvarNative::DisableLock    },
	{ "CvarLockInfo"      , CvarNative::LockInfo       },
	{ "CvarSetBounds"     , CvarNative::SetBounds      },
	{ "CvarGetBounds"     , CvarNative::GetBounds      },
	{ "CvarRegister"      , CvarNative::Register       },
	{ "CvarNum"			  , CvarNative::Num			   },
	{ "CvarInfo"          , CvarNative::Info	       },
	{ "CvarGetStatus"     , CvarNative::GetStatus      },
	{ "CvarGetName"		  , CvarNative::GetName	       },
	{ "CvarSetDescription", CvarNative::SetDescription },
	{ "CvarGetDescription", CvarNative::GetDescription },
	{ "CvarPluginInfo"    , CvarNative::PluginInfo	   },
	{ "CvarCache"		  , CvarNative::Cache		   },
	{ "CvarGetDefault"	  , CvarNative::GetDefault	   },
	{ "CvarReset"		  , CvarNative::Reset		   },

	{ NULL			      , NULL                       }
};