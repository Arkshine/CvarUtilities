
// From Ham Sandwich, modified.

#ifndef FORWARD_H
#define FORWARD_H

#include "amxxmodule.h"
#include "CString.h"

enum fwdstate
{
	FSTATE_INVALID = 0,
	FSTATE_OK,
	FSTATE_PAUSE,
	FSTATE_STOP,
	FSTATE_DESTROY
};

class Forward
{
	public:

		int			id; 
		fwdstate	state;
		String		callback;

		~Forward()
		{
			MF_UnregisterSPForward( id );
		}

		inline void Set( int id_, fwdstate state_, String callback_ )
		{
			state	= state_;
			id		= id_;

			callback.assign( callback_ );
		};
};

#endif
