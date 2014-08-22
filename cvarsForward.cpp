
#include "cvarsHooker.h"
#include "cvarsManager.h"
#include <float.h>
#include "chooker.h"

int ForwardCvarHookChanged;

extern bool AmxxPluginsLoaded;

bool isFloat( const char* string )
{
	do
	{
		if( !isdigit( *string ) && *string != '.' )
		{
			return false;
		}
	}
	while( *++string );

	return true;
}

bool SendindForwards = false;

bool Cvar_DirectSet_Custom( cvar_s* var, const char* value )
{
	if( SendindForwards )
	{
		return true;
	}

	//printf( "\t::Cvar_DirectSet - %s -> %s\n", var->name, value );

	int		lastResult		= 0;
	bool	outOfBound		= false;
	float	newBoundValue	= 0.0;
	float	flValue			= atof( value );

	CvarsManager::NewCvar = false;

	CvarInfo* cvarInfo = CvarsManager::getCvarInfo( var );

	if( !CvarsManager::NewCvar )
	{
		if( cvarInfo->postponed )
		{
			//printf( "\t%s should be postponed !\n", cvarInfo->var->name );
			goto sendToForward;
		}
		else if ( cvarInfo->lock->active )
		{
			return false;
		}

		/*printf( "\t\tOLD CVAR : %-24.23s %-24.23s %-24.23s %-4d %-4d %-4d %s\n", 
			cvarInfo->var->name, 
			strlen( cvarInfo->var->string ) ? cvarInfo->var->string : "-", 
			value, 
			cvarInfo->bound->hasMin, 
			cvarInfo->bound->hasMax, 
			cvarInfo->isOOB,
			CvarsManager::getStatusFlags( cvarInfo->status ).c_str() );*/

		if( !cvarInfo->isOOB )
		{
			if( cvarInfo->bound->hasMin && flValue < cvarInfo->bound->fMin )
			{
				outOfBound = true;
				newBoundValue = cvarInfo->bound->fMin;
			}	
			else if( cvarInfo->bound->hasMax && flValue > cvarInfo->bound->fMax )
			{
				outOfBound = true;
				newBoundValue = cvarInfo->bound->fMax;
			}
			else if( !cvarInfo->bound->forceInterval && cvarInfo->bound->hasMin && cvarInfo->bound->hasMax )
			{
				if( flValue > cvarInfo->bound->fMin && flValue < cvarInfo->bound->fMax )
				{
					outOfBound = true;
					newBoundValue = cvarInfo->bound->fMax;
				}
			}

			if( outOfBound )
			{
				cvarInfo->isOOB = true;
				CVAR_SET_FLOAT( var->name, newBoundValue );

				return false;
			}
		}	
		else if( cvarInfo->isOOB )
		{
			cvarInfo->isOOB = false;
			CvarsManager::setCvarStatus( cvarInfo, true, CvarStatus_WasOutOfBound );
		}
		else if( cvarInfo->status & CvarStatus_WasOutOfBound )
		{
			CvarsManager::setCvarStatus( cvarInfo, false, CvarStatus_WasOutOfBound );
		}
	}
	else
	{
		if( !AmxxPluginsLoaded )
		{
			//printf( "\tFOUND : %-24.23s %-24.23s %-24.23s\n", var->name, var->string, value );
			cvarInfo->postponed = true;

			CvarPostponed* info = new CvarPostponed;

			info->var = var;
			info->value = value;

			CvarsManager::CvarsPostponed.push_back( info );
		}

		if( strlen( var->string ) )
			cvarInfo->origValue.assign( var->string );
		else
			cvarInfo->origValue.assign( value );

		//printf( "\t\tNEW CVAR: %-24.23s %-24.23s %-24.23s %-24.23s\n", var->name, var->string, value, cvarInfo->origValue.c_str() );

		cvarInfo->sentToGlobal = true;

		goto sendToForward;
	}

	if( strcmp( var->string, value ) )
	{
		if( isFloat( value ) && isFloat( var->string ) && fabsf( var->value - atof( value ) ) < FLT_EPSILON )
		{
			return false;
		}

sendToForward:

		String* oldValue = new String;
		oldValue->assign( value );

		SendindForwards = true;

		const char* newvalue;
		const char* oldvalue;

		if( strlen( var->string ) )
			oldvalue = var->string;
		else
			oldvalue = NULL;

		if( strlen( value ) )
			newvalue = value;
		else
			newvalue = NULL;

		if( cvarInfo->hook->registered && cvarInfo->hook->plugin.size() )
		{
			for( CVector< CvarPlugin* >::iterator iterPlugin = cvarInfo->hook->plugin.begin(); iterPlugin != cvarInfo->hook->plugin.end(); ++iterPlugin )
			{
				CvarPlugin* plugin = ( *iterPlugin );

				if( plugin->forward.state == FSTATE_OK )
				{
					//printf( "FORWARD : %-24.23s %-24.23s %-24.23s %-24.23s\n", var->name, var->string, value, cvarInfo->origValue.c_str() );
					
					int result = MF_ExecuteForward( plugin->forward.id, reinterpret_cast< cell >( var ), oldvalue, newvalue, var->name, cvarInfo->origValue.c_str() );

					if( result >= lastResult )
					{
						lastResult = result;
					}
				}
			}
		}

		int result = MF_ExecuteForward( ForwardCvarHookChanged, reinterpret_cast< cell >( var ), oldvalue, newvalue, var->name, cvarInfo->origValue.c_str() );

		SendindForwards = false;

		char* newValue;
		newValue = ( char* )malloc( sizeof( char ) * ( oldValue->size() + 1 ) );
		strcpy( newValue, oldValue->c_str() );
		value = newValue;

		delete oldValue;

		if( result >= lastResult )
		{
			lastResult = result;
		}

		if( lastResult && !cvarInfo->postponed )
		{
			return false;
		}

		if( cvarInfo->cache.size() )
		{
			for( CVector< CvarCache* >::iterator iterCache = cvarInfo->cache.begin(); iterCache != cvarInfo->cache.end(); ++iterCache )
			{
				CvarCache* cache = ( *iterCache );

				switch( cache->type )
				{
					case CvarType_Int :
					{
						*MF_GetAmxAddr( cache->amxx, cache->dataOfs ) = atoi( value );
						break;
					}
					case CvarType_Float :
					{
						*MF_GetAmxAddr( cache->amxx, cache->dataOfs ) = amx_ftoc( flValue );
						break;
					}
					case CvarType_String :
					{
						MF_SetAmxString( cache->amxx, cache->dataOfs, value, cache->stringLenOfs );
						break;
					}
					case CvarType_Flags :
					{
						*MF_GetAmxAddr( cache->amxx, cache->dataOfs ) = CvarsManager::UTIL_ReadFlags( value );
						break;
					}
				}
			}
		}
	}

	if( cvarInfo->postponed )
	{
		cvarInfo->postponed = false;

		if( !lastResult )
		{
			return false;
		}
	}

	return true;
}

void Cvar_DirectSet( cvar_s* var, const char* value )
{
	if( Cvar_DirectSet_Custom( var, value ) )
	{
		if( CvarDirectSetHook->Restore() )
		{
			CvarDirectSetOrig( var, value );
			CvarDirectSetHook->Patch();
		}
	}
}
