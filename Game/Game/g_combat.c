/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// g_combat.c
#include "g_local.h"
qboolean CanDamage(gentity_t* targ,vec3_t origin){
	vec3_t	dest;
	trace_t	tr;
	vec3_t	midpoint;
	VectorAdd(targ->r.absmin,targ->r.absmax,midpoint);
	VectorScale(midpoint,0.5,midpoint);
	VectorCopy(midpoint,dest);
	trap_Trace(&tr,origin,vec3_origin,vec3_origin,dest,ENTITYNUM_NONE,MASK_SOLID);
	if(tr.fraction == 1.0 || tr.entityNum == targ->s.number){return qtrue;}
	VectorCopy(midpoint,dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trap_Trace(&tr,origin,vec3_origin,vec3_origin,dest,ENTITYNUM_NONE,MASK_SOLID);
	if(tr.fraction == 1.0){return qtrue;}
	VectorCopy(midpoint,dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trap_Trace(&tr,origin,vec3_origin,vec3_origin,dest,ENTITYNUM_NONE,MASK_SOLID);
	if(tr.fraction == 1.0){return qtrue;}
	VectorCopy(midpoint,dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trap_Trace(&tr,origin,vec3_origin,vec3_origin,dest,ENTITYNUM_NONE,MASK_SOLID);
	if(tr.fraction == 1.0){return qtrue;}
	VectorCopy(midpoint,dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trap_Trace(&tr,origin,vec3_origin,vec3_origin,dest,ENTITYNUM_NONE,MASK_SOLID);
	if(tr.fraction == 1.0){return qtrue;}
	return qfalse;
}
