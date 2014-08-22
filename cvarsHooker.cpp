
#include "amxxmodule.h"
#include "cvarsHooker.h"
#include "chooker.h"
#include "utils.h"

CvarDirectSet CvarDirectSetOrig;

CHooker		HookerClass;
CHooker*	Hooker = &HookerClass;

CFunc* CvarDirectSetHook = NULL;

String ModuleStatus;

bool CreateHook( void* referenceAddress, void* callbackAddress )
{
	ModuleStatus.assign( UTIL_VarArgs( "\n %s v%s - by %s.\n -\n", MODULE_NAME, MODULE_VERSION, MODULE_AUTHOR ) );
	ModuleStatus.append( " Memory initialization started.\n\n" );

	unsigned char* baseAddress = ( unsigned char* )referenceAddress;
	void* functionAddress = NULL;

	#ifdef WIN32
		byte opcodeToSearch = 0xE8;
	#else
		byte opcodeToSearch = 0xE9;
	#endif

	const int maxBytesLimit = 20;
	int i;

	for( i = 0 ; i < maxBytesLimit; i++ )
	{
		if( baseAddress[ i ] == opcodeToSearch )
		{
			int displacement = *( ( long* )&baseAddress[ i + 1 ] );
			functionAddress = ( void* )( ( long )&baseAddress[ i + 5 ] + displacement );

			

			break;
		}
	}

	ModuleStatus.append( UTIL_VarArgs( "\t Searching Cvar_DirectSet address... %s\n", functionAddress ? "FOUND" : "NOT FOUND" ) );

	CvarDirectSetHook = Hooker->CreateHook( functionAddress, callbackAddress, TRUE );
	CvarDirectSetOrig = reinterpret_cast< CvarDirectSet>( CvarDirectSetHook->GetOriginal() );

	ModuleStatus.append( UTIL_VarArgs( "\t Creating hook... %s\n\n", CvarDirectSetHook ? "DONE" : "FAILED" ) );
	ModuleStatus.append( UTIL_VarArgs( " Memory initialization %s.\n\n", functionAddress && CvarDirectSetHook? "aborted" : "ended" ) );

	return functionAddress && CvarDirectSetHook;
}