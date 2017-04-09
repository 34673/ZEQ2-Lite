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
// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities
#include "cg_local.h"
extern char *eventnames[];
//=======
//A respawn happened this snapshot
void CG_Respawn(void){
	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;
	cg.resetValues = qtrue;
	// display weapons available
	cg.weaponSelectTime = cg.time;
	// select the weapon the server says we are using
	cg.weaponSelect = cg.snap->ps.weapon;
	cg.tierCurrent = 0;
	cg.tierSelect = -1;
	cg.weaponDesired = 1;
	cg.weaponChanged = 0;
	cg.weaponSelectionMode = 0;
}
void CG_CheckPlayerstateEvents(playerState_t *ps, playerState_t *ops){
	centity_t	*cent;
	int			i;
	int			event;
	if(ps->externalEvent && ps->externalEvent != ops->externalEvent){
		cent = &cg_entities[ps->clientNum];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent(cent, cent->lerpOrigin);
	}
	cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];
	// go through the predictable events buffer
	for(i = ps->eventSequence -MAX_PS_EVENTS; i < ps->eventSequence; ++i){
		// if we have a new predictable event or the server told us to play another event instead of a predicted event we already issued
		// or something the server told us changed our prediction causing a different event
		if(i >= ops->eventSequence || (i > ops->eventSequence -MAX_PS_EVENTS && ps->events[i&(MAX_PS_EVENTS-1)] != ops->events[i&(MAX_PS_EVENTS-1)])){
			event = ps->events[i&(MAX_PS_EVENTS-1)];
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[i&(MAX_PS_EVENTS-1)];
			CG_EntityEvent(cent, cent->lerpOrigin);
			cg.predictableEvents[i&(MAX_PREDICTED_EVENTS-1)] = event;
			cg.eventSequence++;
		}
	}
}
void CG_CheckChangedPredictableEvents(playerState_t *ps){
	centity_t	*cent;
	int			i;
	int			event;
	cent = &cg.predictedPlayerEntity;
	for(i = ps->eventSequence -MAX_PS_EVENTS; i < ps->eventSequence; ++i){
		if(i >= cg.eventSequence) continue;
		// if this event is not further back in than the maximum predictable events we remember
		if(i > cg.eventSequence -MAX_PREDICTED_EVENTS){
			// if the new playerstate event is different from a previously predicted one
			if(ps->events[i &(MAX_PS_EVENTS-1)] != cg.predictableEvents[i&(MAX_PREDICTED_EVENTS-1)]){
				event = ps->events[i &(MAX_PS_EVENTS-1)];
				cent->currentState.event = event;
				cent->currentState.eventParm = ps->eventParms[i &(MAX_PS_EVENTS-1)];
				CG_EntityEvent(cent, cent->lerpOrigin);
				cg.predictableEvents[i&(MAX_PREDICTED_EVENTS-1)] = event;
				if(cg_showmiss.integer){CG_Printf("WARNING: changed predicted event\n");}
			}
		}
	}
}
void CG_CheckLocalSounds(playerState_t *ps, playerState_t *ops){
	// don't play the sounds if the player just changed teams or
	// if we are going into the intermission, don't start any voices
	if((ps->persistant[PERS_TEAM] != ops->persistant[PERS_TEAM]) || cg.intermissionStarted){return;}
}
void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops){
	if(ps->clientNum != ops->clientNum){
		cg.thisFrameTeleport = qtrue;
		*ops = *ps;
	}
	if(ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT])
		CG_Respawn();
	if(cg.mapRestart){
		CG_Respawn();
		cg.mapRestart = qfalse;
	}
	if(cg.snap->ps.pm_type != PM_INTERMISSION && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR)
		CG_CheckLocalSounds(ps,ops);
	CG_CheckPlayerstateEvents(ps,ops);
	cg.tierSelectionMode = 0;
	cg.tierSelect = -1;
	if(ps->powerLevel[plTierChanged] == 1 && ops->powerLevel[plTierChanged] != 1){
		cg.tierCurrent = ps->powerLevel[plTierCurrent];
		cg.weaponDesired = -1;
		cg.weaponSelectionMode = 0;
	}
	if(ps->currentSkill[WPSTAT_CHANGED]){
		//If the server validated a weapon change, changes it
		cg.weaponSelectionMode = 0;
		cg.weaponDesired = -1;
		cg.weaponSelect = ps->weapon;
		if(ps->currentSkill[WPSTAT_CHANGED] != -1){cg.drawWeaponBar = 1;}
	}
}
