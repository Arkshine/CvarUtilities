
#ifndef _INCLUDE_CVARS_MANAGER_
#define _INCLUDE_CVARS_MANAGER_

#include "amxxmodule.h"
#include "forward.h"
#include "CVector.h"
#include "CString.h"

enum CvarReturn
{
	CvarReturn_DuplicatedCache = -5,
	CvarReturn_AlreadyBounded  = -4,
	CvarReturn_AlreadyLocked   = -3,
	CvarReturn_Duplicated	   = -2,
	CvarReturn_AlreadySet      = -1,
	CvarReturn_Error		   = 0,
	CvarReturn_NoError		   = 1
};

enum CvarStatus
{
	CvarStatus_HookRegistered = 1,
	CvarStatus_HookActive     = 2,
	CvarStatus_LockRegistered = 4,
	CvarStatus_LockActive	  = 8,
	CvarStatus_HasMinValue	  = 16,
	CvarStatus_HasMaxValue	  = 32,
	CvarStatus_NewCvar		  = 64,   
	CvarStatus_WasOutOfBound  = 128,
	CvarStatus_Cached		  = 256
};

enum CvarBoundType
{
	CvarBound_Lower = 0,
	CvarBound_Upper
};

enum CvarType
{
	CvarType_Int,
	CvarType_Float,
	CvarType_String,
	CvarType_Flags
};

enum CvarError
{
	CvarError_InvalidCvarPointer = 0,
	CvarError_InvalidFunction
};

struct CvarPlugin
{
    int			id;
    Forward		forward;
};

struct CvarHook
{
	bool					registered;
	int						activeHooks;
	CVector< CvarPlugin* >	plugin;
};

struct CvarLock
{
	bool	registered;
	bool	active;
	int		pluginId;
	String	origValue;
	String	currValue;
};

struct CvarBound
{
	int		minPluginId;
	bool	hasMin;
	REAL	fMin;
	int		maxPluginId;
	bool	hasMax;
	REAL	fMax;
	bool	forceInterval;
};

struct CvarCache
{
	int		pluginId;
	int		type;
	AMX*	amxx;
	cell	dataOfs;
	cell	stringLenOfs;
};

struct CvarInfoMin
{
	cvar_t*					var;
	String					description;
	String					origValue;
};

struct CvarInfo
{
	cvar_t*					var;
	String					description;
	String					origValue;
	bool					registered;
	int						pluginId;
	int						status;
	bool					isOOB;
	bool					postponed;
	bool					sentToGlobal;
	int						commandIndex;
	CVector< CvarCache* >	cache;
	CvarBound*				bound;
	CvarHook*				hook;
	CvarLock*				lock;
};

struct CvarPostponed
{
	cvar_t*					var;
	const char*				value;
};

namespace CvarsManager
{
	extern CVector< CvarInfo* > CvarsInfo;
	extern CVector< CvarInfoMin* > CvarsInfoMin;
	extern CVector< CvarPostponed* >CvarsPostponed;

	extern bool NewCvar;

	cvar_t* validCvarPointerOrDie( AMX* amx, cell param );
	CvarInfo* getCvarInfo( cvar_t* var, bool dontAdd = false );

	bool registerForward( AMX* amx, String callback, int &forwardId );
	bool checkDuplicate( CvarInfo* cvarInfo, int pluginId, String callback );
	bool checkDuplicateCache( CvarInfo* cvarInfo, int pluginId );

	CvarInfo* addCvar( cvar_t* var );
	void addPlugin( CvarInfo* cvarInfo, int pluginId, int forwardId, String callback );

	int setHookState( fwdstate state, CVector< CvarPlugin* > plugin, int pluginId, String callback );
	void setCvarStatus( CvarInfo* cvarInfo, bool state, int flags );

	void deleteCvarsInfos( void );
	void clearCvarsInfos( void );
	void copyAndDeleteCvarInfos( void );
	void setCopyCvarInfos( void );

	int UTIL_ReadFlags( const char* c );
	String getStatusFlags( int flags );

	void sendPostponedCvars( void );
}

#endif