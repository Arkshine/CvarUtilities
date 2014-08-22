
#include "cvarsManager.h"
#include "amxx_api.h"
#include "cvarsHooker.h"
#include <float.h>

#if defined __linux__
	#include <strings.h>
#endif

void print_srvconsole( const char *fmt, ... )
{ 
	va_list argptr;
	static char string[ 384 ];

	va_start( argptr, fmt );
	vsnprintf( string, sizeof( string ) - 1, fmt, argptr );
	string[ sizeof( string ) - 1 ] = '\0';
	va_end( argptr );

	SERVER_PRINT( string );
}

const char* getPluginName( int pluginId )
{
	return findPluginFast( MF_GetScriptAmx( pluginId + CSXModuleLoaded ) )->getName();
}

String getPluginFlags( int flags )
{
	String flagsName;

	if( flags & FCVAR_ARCHIVE       ) flagsName.append( "FCVAR_ARCHIVE " );
	if( flags & FCVAR_USERINFO      ) flagsName.append( "FCVAR_USERINFO " );
	if( flags & FCVAR_SERVER        ) flagsName.append( "FCVAR_SERVER "	);
	if( flags & FCVAR_EXTDLL        ) flagsName.append( "FCVAR_EXTDLL "	);
	if( flags & FCVAR_CLIENTDLL     ) flagsName.append( "FCVAR_CLIENTDLL " );
	if( flags & FCVAR_PROTECTED     ) flagsName.append( "FCVAR_PROTECTED " );
	if( flags & FCVAR_SPONLY        ) flagsName.append( "FCVAR_SPONLY "	);
	if( flags & FCVAR_PRINTABLEONLY ) flagsName.append( "FCVAR_PRINTABLEONLY " );
	if( flags & FCVAR_UNLOGGED      ) flagsName.append( "FCVAR_UNLOGGED " );

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

int CvarsFound = 0;

void CommandCvarUtil( void )
{
	const char *cmd = CMD_ARGV( 1 );

	if( !strcasecmp( cmd, "list" ) )  
	{		
		CvarsFound = 0;

		print_srvconsole("\nRegistered cvars :\n\n");
		print_srvconsole("       %-26.25s %-26.25s %-26.25s %-26.25s %-26.25s %-26.25s %-26.25s\n", "NAME", "VALUE", "REGISTERED", "HOOKED", "LOCKED", "BOUNDED", "CACHED" );	
		print_srvconsole(" - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n" );

		const char *pluginName    = NULL;
		size_t		pluginLen     = 0;
		bool		checkPlugin   = false;
		int			index		  = 0;
		
		bool		hasReferences = false;

		if( CMD_ARGC() > 2 )
		{
			pluginLen = strlen( pluginName = CMD_ARGV( 2 ) );
			checkPlugin = pluginName && pluginLen;
		}

		for( CVector< CvarInfo* >::iterator iter = CvarsManager::CvarsInfo.begin(); iter != CvarsManager::CvarsInfo.end(); ++iter )
		{
			CvarInfo* info = *iter;

			/*print_srvconsole(" %d %-24.23s %-24.23s\n", 
				info->pluginId,
				info->var->name, 
				info->var->string );*/

			if( info->status )
			{
				if( checkPlugin && strncasecmp( info->var->name, pluginName, pluginLen ) != 0 )
				{
					continue;
				}

				hasReferences = false;

				const char* registeredString = ( info->status & CvarStatus_NewCvar )		? getPluginName( info->pluginId )		: "no";
				const char* lockString		 = ( info->status & CvarStatus_LockRegistered ) ? getPluginName( info->lock->pluginId ) : "no";
				const char* boundedString	 = "no";
				const char* hookedString	 = "no";
				const char* cachedString     = "no";

				if( info->status & ( CvarStatus_HasMinValue | CvarStatus_HasMaxValue ) )
				{
					if( info->status & CvarStatus_HasMinValue && ~info->status & CvarStatus_HasMaxValue )
					{
						boundedString = getPluginName( info->bound->minPluginId );
					}
					else if( info->status & CvarStatus_HasMaxValue && ~info->status & CvarStatus_HasMinValue )
					{
						boundedString = getPluginName( info->bound->maxPluginId );
					}
					else if( !strcasecmp( getPluginName( info->bound->minPluginId ), getPluginName( info->bound->maxPluginId ) ) )
					{
						boundedString = getPluginName( info->bound->minPluginId );
					}
					else
					{
						boundedString = "yes*";
						hasReferences = true;
					}
				}

				if( info->status & CvarStatus_HookRegistered )
				{
					if( info->hook->plugin.size() > 1 )
					{
						hookedString = "yes*";
						hasReferences = true;
					}
					else
					{
						CVector< CvarPlugin* >::iterator iter = info->hook->plugin.begin();
						//printf( "PLUGIN ID = %d\n", ( *iter )->id );
						hookedString = getPluginName( ( *iter )->id );
					}
				}

				if( info->status & CvarStatus_Cached )
				{
					if( info->cache.size() > 1 )
					{
						cachedString = "yes*";
						hasReferences = true;
					}
					else
					{
						CVector< CvarCache* >::iterator iter = info->cache.begin();
						cachedString = getPluginName( ( *iter )->pluginId );
					}
				}

				/*if( checkPlugin )
				{
					bool foundMatchedPlugin = false;

					if( strncasecmp( info->var->name, pluginName, pluginLen ) == 0 )
					{
						foundMatchedPlugin = true;
					}

					if( info->status & ( CvarStatus_HasMinValue | CvarStatus_HasMaxValue ) )
					{
						if( strncasecmp( getPluginName( info->bound->minPluginId ), pluginName, pluginLen ) == 0 ||
							strncasecmp( getPluginName( info->bound->maxPluginId ), pluginName, pluginLen ) == 0 )
						{	
							foundMatchedPlugin = true;
						}
					}

					if( !foundMatchedPlugin && info->status & CvarStatus_HookRegistered )
					{
						CVector< CvarPlugin* > plugin = info->hook->plugin;

						for( CVector< CvarPlugin* >::iterator iter = plugin.begin(); iter != plugin.end(); ++iter )
						{
							if( strncasecmp( getPluginName( (*iter)->id ), pluginName, pluginLen ) == 0 )
							{
								foundMatchedPlugin = true;
								break;
							}
						}
					}

					if( !foundMatchedPlugin && info->status & CvarStatus_Cached )
					{
						CVector< CvarCache* > cache = info->cache;

						for( CVector< CvarCache* >::iterator iter = cache.begin(); iter != cache.end(); ++iter )
						{
							if( strncasecmp( getPluginName( (*iter)->pluginId ), pluginName, pluginLen ) == 0 )
							{
								foundMatchedPlugin = true;
								break;
							}
						}
					}*/

					/*if( !foundMatchedPlugin )
					{
						hasReferences = false;
					}

					if( !foundMatchedPlugin								       &&
						strncasecmp( registeredString, pluginName, pluginLen ) && 
						strncasecmp( lockString	     , pluginName, pluginLen ) &&
						strncasecmp( boundedString	 , pluginName, pluginLen ) &&
						strncasecmp( cachedString	 , pluginName, pluginLen ) )
					{
						continue;
					}
				}*/

				print_srvconsole(" %3d.  %-26.25s %-26.25s %-26.25s %-26.25s %-26.25s %-26.25s %-26.25s\n", 
									++index, 
									info->var->name, 
									info->var->string, 
									registeredString, 
									hookedString, 
									lockString, 
									boundedString,
									cachedString );

				info->commandIndex = index;
			}
		}

		CvarsFound = index;

		if( !CvarsFound )
		{
			if( checkPlugin )
				print_srvconsole( "\n No results found matching with \"%s\".\n", pluginName );
			else
				print_srvconsole( "\n No results found.\n" );
		}
		else if( hasReferences )
		{
			print_srvconsole( "\n * = There are two or more references of plugins.\n" );
			print_srvconsole( "     Use \"cvarutil info cvarname\" to get more informations.\n" );
		}

		if( CvarsFound )
			print_srvconsole( "\n > Found %d cvars.\n", CvarsFound );
			
		print_srvconsole( "\n" );
		return;
	}
	else if( !strcasecmp( cmd, "info" ) )  
	{
		if( CMD_ARGC() > 2 )
		{
			const char *cvarName = CMD_ARGV( 2 );

			int num = atoi( cvarName );
			int index = 0;
			bool useNumber = false;
			bool found = false;

			if( num > 0 && num <= CvarsFound )
			{
				useNumber = true;
			}

			//print_srvconsole( "num = %d, useNumber = %d, isdigit = %d, cvarsfound = %d\n", num, useNumber, isdigit( num ), CvarsFound );
			
			for( CVector< CvarInfo* >::iterator iter = CvarsManager::CvarsInfo.begin(); iter != CvarsManager::CvarsInfo.end(); ++iter )
			{
				CvarInfo* info = *iter;

				if( !info->status )
				{
					continue;
				}

				++index;

				if( useNumber )
				{
					//print_srvconsole( "%s - %d - %d\n", info->var->name, info->commandIndex, index );

					if( info->commandIndex == num )
					{
						found = true;
					}
				}
				else
				{
					if( !strcasecmp( cvarName, info->var->name ) )
					{
						found = true;
					}
				}

				if( found )
				{
					//printf( "info->status = %d\n", info->status );

					print_srvconsole( "\nCvar details :\n\n" );
					print_srvconsole( " Cvar name   : %s\n", info->var->name );
					print_srvconsole( " Value       : %s\n", info->var->string );
					print_srvconsole( " Def. value  : %s\n", info->origValue.c_str() );
					print_srvconsole( " Description : %s\n", info->description.c_str() );
					print_srvconsole( " Flags       : %s\n\n", getPluginFlags( info->var->flags ).c_str() );
		
					print_srvconsole( " %-12s  %-26.25s %-8s %s\n", "STATUS", "PLUGIN", "ACTIVE", "VALUE / CALLBACK" );
					print_srvconsole( " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n" );

					if( info->status & CvarStatus_NewCvar )
					{
						print_srvconsole( " Registered    %-26.25s %-8s %s\n", 
											getPluginName( info->pluginId ), 
											"yes",
											"-" );
					}

					if( info->status & CvarStatus_LockRegistered )
					{
						print_srvconsole( " Locked        %-26.25s %-8s %s\n", 
											getPluginName( info->lock->pluginId ), 
											info->lock->active ? "yes" : "no",
											info->lock->currValue.c_str() );
					}

					if( info->status & CvarStatus_HasMinValue )
					{
						print_srvconsole(  " Min Bounded   %-26.25s %-8s %f\n", 
											getPluginName( info->bound->minPluginId ), 
											info->lock->active ? "no" : "yes",
											info->bound->fMin );
					}

					if( info->status & CvarStatus_HasMaxValue )
					{
						print_srvconsole(  " Max Bounded   %-26.25s %-8s %f\n", 
											getPluginName( info->bound->maxPluginId ), 
											info->lock->active ? "no" : "yes",
											info->bound->fMax );
					}
					
					if( info->status & CvarStatus_HookRegistered )
					{
						CVector< CvarPlugin* > plugin = info->hook->plugin;

						for( CVector< CvarPlugin* >::iterator iter = plugin.begin(); iter != plugin.end(); ++iter )
						{
							print_srvconsole( " Hooked        %-26.25s %-8s %s\n", 
												getPluginName( (*iter)->id ), 
												(*iter)->forward.state == FSTATE_OK ? "yes" : "no",
												(*iter)->forward.callback.c_str() );
						}
					}

					if( info->status & CvarStatus_Cached )
					{
						CVector< CvarCache* > cache = info->cache;

						for( CVector< CvarCache* >::iterator iter = cache.begin(); iter != cache.end(); ++iter )
						{
							print_srvconsole( " Cached        %-26.25s %-8s %s\n", 
								getPluginName( (*iter)->pluginId ), 
								"-", "-" );
						}

						print_srvconsole( "\n" );
					}

					return;
				}
			}

			print_srvconsole( "\n Couldn't find plugin matching \"%s\".\n", cvarName );
			return;
		}
	}
	else if( !strcasecmp( cmd, "version" ) )  
	{
		print_srvconsole( "\n %s %s\n -\n", Plugin_info.name, Plugin_info.version );
		print_srvconsole( " Support  : %s\n", Plugin_info.url );
		print_srvconsole( " Author   : %s\n", Plugin_info.author );
		print_srvconsole( " Compiled : %s\n\n", __DATE__ ", " __TIME__ );
		return;
	}
	else if( !strcasecmp( cmd, "status" ) )
	{
		print_srvconsole( "%s", ModuleStatus.c_str() );
		return;
	}

	print_srvconsole( "\n Usage: cvarutil < command > [ argument ]\n" );
	print_srvconsole( " Commands:\n");
	print_srvconsole( "   version                - Display some informations about the module and where to get a support.\n");
	print_srvconsole( "   status                 - Display module status.\n");
	print_srvconsole( "   list [ plugin ]        - Display a list of registered cvars which have a status. Can be filtered by [partial] plugin.\n");
	print_srvconsole( "   info < [cvar|index] >  - Give detailed informations about a cvar.\n\n");
}
