
#ifndef _INCLUDE_CVARS_NATIVES_
#define _INCLUDE_CVARS_NATIVES_

#include "amxxmodule.h"

class CvarNative
{	
	public:

		static cell AMX_NATIVE_CALL HookChange		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL HookChangeAll	( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL EnableHook		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL EnableHookAll	( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL DisableHook		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL DisableHookAll	( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL HookNum			( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL HookInfo		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL LockValue		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL EnableLock		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL DisableLock		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL LockInfo		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL SetBounds		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL GetBounds		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL Register		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL Num				( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL Info			( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL GetStatus		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL GetName			( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL SetDescription	( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL GetDescription	( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL PluginInfo		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL Cache			( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL GetDefault		( AMX* amx, cell* params );
		static cell AMX_NATIVE_CALL Reset			( AMX* amx, cell* params );
};

#endif