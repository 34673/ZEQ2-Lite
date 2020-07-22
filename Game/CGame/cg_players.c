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
//
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"

char	*cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"death",
	"jump",
	"highjump",
	"ballFlip",
	"pain",
	"falling",
	"gasp",
	"drown",
	"fall",
	"fallSoft",
	"fallHard",
	"taunt",
	"idleBanter",
	"idleWinningBanter",
	"idleLosingBanter",
	"struggleBanter",
};

// HACK: We have to copy the entire playerEntity_t information
//       because otherwise Q3 finds that all previously set information
//       regarding frames and refEntities are nulled out on certain occassions.
//       WTF is up with this stupid bug anyway?! Same thing happened when adding
//       certain fields to centity_t...
static playerEntity_t	playerInfoDuplicate[MAX_GENTITIES];
/*================
CG_CustomSound
================*/
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName ) {
	clientInfo_t *ci;
	int	i;
	int nextIndex;
	nextIndex = (fabs(crandom()) * 8)+1;
	if(nextIndex > 9){nextIndex = 9;}
	if(nextIndex < 1){nextIndex = 1;}
	if(clientNum < 0 || clientNum >= MAX_CLIENTS){clientNum = 0;}
	ci = &cgs.clientinfo[clientNum];
	if(!ci->infoValid){return cgs.media.nullSound;}
	for(i = 0 ; i < MAX_CUSTOM_SOUNDS; i++){
		if(!strcmp(soundName,cg_customSoundNames[i]) && ci->sounds[ci->tierCurrent][(i*9)+nextIndex]){
			return ci->sounds[ci->tierCurrent][(i*9)+nextIndex];
		}
	}
	CG_Printf("Client %i [%s] could not find sound : %s%i\n",clientNum,ci->name,soundName,nextIndex);
	return cgs.media.nullSound;
}
/*=============================================================================
CLIENT INFO
=============================================================================*/

/*
==========================
CG_FileExists
==========================
*/
static qboolean	CG_FileExists(const char *filename) {
	int len;

	len = trap_FS_FOpenFile( filename, NULL, FS_READ );
	if (len>0) {
		return qtrue;
	}
	return qfalse;
}

/*
==========================
CG_FindClientModelFile
==========================
*/
static qboolean	CG_FindClientModelFile( char *filename, int length, clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *base, const char *ext ) {
	char *team, *charactersFolder;
	int i;
	if ( cgs.gametype >= GT_TEAM ) {
		switch ( ci->team ) {
			case TEAM_BLUE: {
				team = "blue";
				break;
			}
			default: {
				team = "red";
				break;
			}
		}
	}
	else {
		team = "default";
	}
	charactersFolder = "";
	while(1) {
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 && teamName && *teamName ) {
				//								"players//characters/james/stroggs/lower_lily_red.skin"
				Com_sprintf( filename, length, "players//%s%s/%s%s_%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, team, ext );
			}
			else {
				//								"players//characters/james/lower_lily_red.skin"
				Com_sprintf( filename, length, "players//%s%s/%s_%s_%s.%s", charactersFolder, modelName, base, skinName, team, ext );
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( cgs.gametype >= GT_TEAM ) {
				if ( i == 0 && teamName && *teamName ) {
					//								"players//characters/james/stroggs/lower_red.skin"
					Com_sprintf( filename, length, "players//%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, team, ext );
				}
				else {
					//								"players//characters/james/lower_red.skin"
					Com_sprintf( filename, length, "players//%s%s/%s_%s.%s", charactersFolder, modelName, base, team, ext );
				}
			}
			else {
				if ( i == 0 && teamName && *teamName ) {
					//								"players//characters/james/stroggs/lower_lily.skin"
					Com_sprintf( filename, length, "players//%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, ext );
				}
				else {
					//								"players//characters/james/lower_lily.skin"
					Com_sprintf( filename, length, "players//%s%s/%s_%s.%s", charactersFolder, modelName, base, skinName, ext );
				}
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( !teamName || !*teamName ) {
				break;
			}
		}
		// if tried the heads folder first
		if ( charactersFolder[0] ) {
			break;
		}
		charactersFolder = "characters/";
	}

	return qfalse;
}

/*
==========================
CG_FindClientHeadFile
==========================
*/
static qboolean	CG_FindClientHeadFile( char *filename, int length, clientInfo_t *ci, const char *teamName, const char *headModelName, const char *headSkinName, const char *base, const char *ext ) {
	return qfalse;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static qboolean	CG_RegisterClientSkin( clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName ) {
	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName, const char *teamName ) {
	if (!CG_RegisterClientModelnameWithTiers( ci, modelName, skinName ) ) {
		return qfalse;
	}
	return qtrue;
}

/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString( const char *v, vec3_t color ) {
	int val;

	VectorClear( color );

	val = atoi( v );

	if ( val < 1 || val > 7 ) {
		VectorSet( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 ) {
		color[2] = 1.0f;
	}
	if ( val & 2 ) {
		color[1] = 1.0f;
	}
	if ( val & 4 ) {
		color[0] = 1.0f;
	}
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo( clientInfo_t *ci ) {
	const char	*dir, *fallback;
	int			i,tier,count,currentIndex,soundIndex,loopIndex,modelloaded;
	char		soundPath[MAX_QPATH];
	char		singlePath[MAX_QPATH];
	char		loopPath[MAX_QPATH];
	int			clientNum;
	char		teamname[MAX_QPATH];
	teamname[0] = 0;

	modelloaded = qtrue;
	//Com_Printf("LoadClientInfo pre CG_RegisterClientModelname\n");
	if (!CG_RegisterClientModelname(ci,ci->modelName,ci->skinName, ci->headModelName,ci->headSkinName,teamname)){
		strcpy(ci->modelName,DEFAULT_MODEL);
		strcpy(ci->headModelName,DEFAULT_MODEL);
		strcpy(ci->skinName,"default");
		strcpy(ci->headSkinName,"default");
		if ( cg_buildScript.integer ) {
			CG_Error( "CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci, ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName,teamname);
		}
		if (!CG_RegisterClientModelname(ci,DEFAULT_MODEL,"default",DEFAULT_MODEL,"default",teamname) ) {
			CG_Error("DEFAULT_MODEL (%s) failed to register",DEFAULT_MODEL);
		}
		modelloaded = qfalse;
	}
	ci->newAnims = qfalse;
	// sounds
	dir = ci->modelName;
	fallback = DEFAULT_MODEL;
	for (tier = 0 ; tier < 9 ; tier++ ){
		for(i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++){
			ci->sounds[tier][i] = cgs.media.nullSound;
		}
	}
	for (tier = 0 ; tier < 9 ; tier++ ){
		for(i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++){
			if(!cg_customSoundNames[i]){continue;}
			loopIndex = 0;
			currentIndex = 1;
			for(count = 1;count <= 9;count++){
				soundIndex = (i*9)+(count-1);
				if(tier == 0){
					Com_sprintf(singlePath,sizeof(singlePath),"players/%s/%s.ogg",dir,cg_customSoundNames[i]);
					Com_sprintf(soundPath,sizeof(soundPath),"players/%s/%s%i.ogg",dir,cg_customSoundNames[i],count);
					Com_sprintf(loopPath,sizeof(loopPath),"players/%s/%s%i.ogg",dir,cg_customSoundNames[i],currentIndex);
				}
				else{
					Com_sprintf(singlePath,sizeof(singlePath),"players/%s/tier%i/%s.ogg",dir,tier,cg_customSoundNames[i]);
					Com_sprintf(soundPath,sizeof(soundPath),"players/%s/tier%i/%s.ogg",dir,tier,cg_customSoundNames[i],count);
					Com_sprintf(loopPath,sizeof(loopPath),"players/%s/tier%i/%s.ogg",dir,tier,cg_customSoundNames[i],currentIndex);
				}
				ci->sounds[tier][soundIndex] = 0;
				if(trap_FS_FOpenFile(singlePath,0,FS_READ)>0){
					ci->sounds[tier][soundIndex] = trap_S_RegisterSound(singlePath,qfalse);
					continue;
				}
				else if(trap_FS_FOpenFile(soundPath,0,FS_READ)>0){
					ci->sounds[tier][soundIndex] = trap_S_RegisterSound(soundPath,qfalse);
					loopIndex = count;
				}
				else if(loopIndex && trap_FS_FOpenFile(loopPath,0,FS_READ)>0){
					ci->sounds[tier][soundIndex] = trap_S_RegisterSound(loopPath,qfalse);
					currentIndex += 1;
					if(currentIndex > loopIndex){currentIndex = 1;}
				}
				else if(ci->sounds[0][soundIndex]){
					ci->sounds[tier][soundIndex] = ci->sounds[0][soundIndex];
				}
			}
		}
	}
	ci->deferred = qfalse;
	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	clientNum = ci - cgs.clientinfo;
	for ( i = 0 ; i < MAX_GENTITIES ; i++ ) {
		if ( cg_entities[i].currentState.clientNum == clientNum
			&& (cg_entities[i].currentState.eType == ET_INVISIBLE
			|| cg_entities[i].currentState.eType == ET_PLAYER) ) {
			CG_ResetPlayerEntity( &cg_entities[i] );
		}
	}
	// REFPOINT: Load the additional tiers' clientInfo here
	CG_RegisterClientAura(clientNum,ci);
	{
		char	filename[MAX_QPATH];
		
		Com_sprintf( filename, sizeof( filename ), "players/%s/%s.grfx", ci->modelName, ci->skinName );
		CG_weapGfx_Parse( filename, clientNum );
	}
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to ) {
	// ADDING FOR ZEQ2
	int fromIndex, toIndex, i;

	fromIndex = from - cgs.clientinfo;
	toIndex = to - cgs.clientinfo;
	// END ADDING

	VectorCopy( from->headOffset, to->headOffset );
	to->footsteps = from->footsteps;

	for ( i = 0; i < 8; i++ ) {
		to->legsModel[i] = from->legsModel[i];
		to->legsSkin[i] = from->legsSkin[i];
		to->torsoModel[i] = from->torsoModel[i];
		to->torsoSkin[i] = from->torsoSkin[i];
		to->headModel[i] = from->headModel[i];
		to->headSkin[i] = from->headSkin[i];
	}
	to->modelIcon = from->modelIcon;

	to->newAnims = from->newAnims;

	// ADDING FOR ZEQ2
	// Copy over the weapon graphics settings!
	CG_CopyUserWeaponGraphics( fromIndex, toIndex );
	// END ADDING

	memcpy( to->animations, from->animations, sizeof( to->animations ) );
	memcpy( to->camAnimations, from->camAnimations, sizeof( to->camAnimations ) );
	memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );

	
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo( clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}
		if ( match->deferred ) {
			continue;
		}
		if ( !Q_stricmp( ci->modelName, match->modelName )
			&& !Q_stricmp( ci->skinName, match->skinName )
			&& !Q_stricmp( ci->legsModelName, match->legsModelName )
			&& !Q_stricmp( ci->legsSkinName, match->legsSkinName )
			&& !Q_stricmp( ci->headModelName, match->headModelName )
			&& !Q_stricmp( ci->cameraModelName, match->cameraModelName )
			&& !Q_stricmp( ci->headSkinName, match->headSkinName ) 
			&& !Q_stricmp( ci->blueTeam, match->blueTeam ) 
			&& !Q_stricmp( ci->redTeam, match->redTeam )
			&& (cgs.gametype < GT_TEAM || ci->team == match->team) ) {
			// this clientinfo is identical, so use its handles

			ci->deferred = qfalse;

			CG_CopyClientInfoModel( match, ci );

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo( clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	// JUHOX: don't care about client models in lens flare editor
#if MAPLENSFLARES
	if (cgs.editMode == EM_mlf) return;
#endif

	// if someone else is already the same models and skins we
	// can just load the client info
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid || match->deferred ) {
			continue;
		}
		if ( Q_stricmp( ci->skinName, match->skinName ) ||
			 Q_stricmp( ci->modelName, match->modelName ) ||
			 Q_stricmp( ci->legsModelName, match->legsModelName ) ||
			 Q_stricmp( ci->legsSkinName, match->legsSkinName ) ||
			 Q_stricmp( ci->headModelName, match->headModelName ) ||
			 Q_stricmp( ci->headSkinName, match->headSkinName ) ||
 			 Q_stricmp( ci->cameraModelName, match->cameraModelName ) ||
			 (cgs.gametype >= GT_TEAM && ci->team != match->team) ) {
			continue;
		}
		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo( ci );
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if ( cgs.gametype >= GT_TEAM ) {
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			match = &cgs.clientinfo[ i ];
			if ( !match->infoValid || match->deferred ) {
				continue;
			}
			if ( Q_stricmp( ci->skinName, match->skinName ) ||
				(cgs.gametype >= GT_TEAM && ci->team != match->team) ) {
				continue;
			}
			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo( ci );
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel( match, ci );
		return;
	}

	// we should never get here...
	CG_Printf( "CG_SetDeferredClientInfo: no valid clients!\n" );

	CG_LoadClientInfo( ci );
}


/*======================
CG_NewClientInfo
======================*/
void CG_NewClientInfo( int clientNum ) {
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char	*configstring;
	const char	*v,*model;
	char *slash;
	char *skin;
	ci = &cgs.clientinfo[clientNum];
	configstring = CG_ConfigString( clientNum + CS_PLAYERS );
	if ( !configstring[0] ) {
		memset( ci, 0, sizeof( *ci ) );
		return;
	}
	memset( &newInfo, 0, sizeof( newInfo ) );
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );
	v = model = Info_ValueForKey(configstring, "model");
	if(( skin = strchr( v,'/'))== NULL){skin = "default";}else{*skin++ = 0;}
	Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
	Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );
	v = Info_ValueForKey(configstring, "hmodel");
	if(!v){v = model;}
	if(( skin = strchr( v,'/'))== NULL){skin = "default";}else{*skin++ = 0;}
	Q_strncpyz( newInfo.headSkinName, skin, sizeof( newInfo.headSkinName ) );
	Q_strncpyz( newInfo.headModelName, v, sizeof( newInfo.headModelName ) );
	v = Info_ValueForKey(configstring, "lmodel");
	if(!v){v = model;}
	if(( skin = strchr( v,'/'))== NULL){skin = "default";}else{*skin++ = 0;}
	Q_strncpyz( newInfo.legsSkinName, skin, sizeof( newInfo.legsSkinName ) );
	Q_strncpyz( newInfo.legsModelName, v, sizeof( newInfo.legsModelName ) );
	v = Info_ValueForKey(configstring, "cmodel");
	if(!v){v = model;}
	if(( skin = strchr( v,'/'))== NULL){skin = "default";}else{*skin++ = 0;}
	Q_strncpyz( newInfo.cameraModelName, v, sizeof( newInfo.cameraModelName ) );
	newInfo.infoValid = qtrue;
	*ci = newInfo;
	CG_LoadClientInfo( ci );
}



/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers( void ) {
	int		i;
	clientInfo_t	*ci;

	// scan for a deferred player to load
	for ( i = 0, ci = cgs.clientinfo ; i < cgs.maxclients ; i++, ci++ ) {
		if ( ci->infoValid && ci->deferred ) {
			// if we are low on memory, leave it deferred
			if ( trap_MemoryRemaining() < 4000000 ) {
				CG_Printf( "Memory is low. Using deferred model.\n" );
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo( ci );
//			break;
		}
	}
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles( float destination, float swingTolerance, float clampTolerance,
					float speed, float *angle, qboolean *swinging ) {
	float	swing;
	float	move;
	float	scale;

	if ( !*swinging ) {
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		if ( swing > swingTolerance || swing < -swingTolerance ) {
			*swinging = qtrue;
		}
	}

	if ( !*swinging ) {
		return;
	}
	
	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabs( swing );
	if ( scale < swingTolerance * 0.5 ) {
		scale = 0.5;
	} else if ( scale < swingTolerance ) {
		scale = 1.0;
	} else {
		scale = 2.0;
	}

	// swing towards the destination angle
	if ( swing >= 0 ) {
		move = cg.frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0 ) {
		move = cg.frametime * scale * -speed;
		if ( move <= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if ( swing > clampTolerance ) {
		*angle = AngleMod( destination - (clampTolerance - 1) );
	} else if ( swing < -clampTolerance ) {
		*angle = AngleMod( destination + (clampTolerance - 1) );
	}
}

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch( centity_t *cent, vec3_t torsoAngles ) {
	int		t;
	float	f;

	t = cg.time - cent->pe.painTime;
	if ( t >= PAIN_TWITCH_TIME ) {
		return;
	}

	f = 1.0 - (float)t / PAIN_TWITCH_TIME;

	if ( cent->pe.painDirection ) {
		torsoAngles[ROLL] += 20 * f;
	} else {
		torsoAngles[ROLL] -= 20 * f;
	}
}


/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3], vec3_t camera[3] ) {
	vec3_t		legsAngles, torsoAngles, headAngles, cameraAngles;
	float		dest;
	static	int	movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
//	vec3_t		velocity;
//	float		speed;
	int			dir, clientNum;
	clientInfo_t	*ci;
	
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum > MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
		return;
	}
	ci = &cgs.clientinfo[ clientNum ];
	
	VectorCopy( cent->lerpAngles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );
	VectorClear( cameraAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != ANIM_IDLE
		|| ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) != ANIM_IDLE
		|| ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != ANIM_IDLE_LOCKED
		|| ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) != ANIM_IDLE_LOCKED ) {
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;	// always center
		cent->pe.torso.pitching = qtrue;	// always center
		cent->pe.legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	if ( cent->currentState.eFlags & EF_DEAD ) {
		// don't let dead bodies twitch
		dir = 0;
	} else if ( (( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) >= ANIM_DASH_RIGHT ) &&
				(( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) <= ANIM_DASH_BACKWARD ) ) {
		// don't let these animations adjust angles.
		dir = 0;
	} else {
		dir = cent->currentState.angles2[YAW];
		if ( dir < 0 || dir > 7 ) {
			CG_Error( "Bad player movement angle" );
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[ dir ];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];

	switch ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) ){
		default:
			CG_SwingAngles( torsoAngles[YAW], 25, 90, 1, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
			CG_SwingAngles( legsAngles[YAW], 40, 90, 1, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
			break;
		case ANIM_IDLE:
		case ANIM_FLY_IDLE:
		case ANIM_FLY_UP:
		case ANIM_FLY_DOWN:
			break;
		case ANIM_SWIM_IDLE:
			if(((&cg.predictedPlayerState)->weaponstate != WEAPON_READY || cent->currentState.weaponstate != WEAPON_READY)){
				CG_SwingAngles( torsoAngles[YAW], 25, 90, 1, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
				CG_SwingAngles( legsAngles[YAW], 40, 90, 1, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
			}
			break;
	}

	headAngles[YAW] = cent->pe.torso.yawAngle;
	torsoAngles[YAW] = cent->pe.torso.yawAngle;
	legsAngles[YAW] = cent->pe.legs.yawAngle;
	if ( headAngles[PITCH] > 180 ) {
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	} else {
		dest = headAngles[PITCH] * 0.75f;
	}
	CG_SwingAngles( dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	//torsoAngles[PITCH] = cent->pe.torso.pitchAngle;
	if ( ci->fixedtorso || cent->currentState.weaponstate == WEAPON_CHARGING || cent->currentState.weaponstate == WEAPON_ALTCHARGING || 
				( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_DEATH_GROUND || 
				( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_DEATH_AIR || 
				( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_DEATH_AIR_LAND ||
				( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_KNOCKBACK_HIT_WALL ||
				( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_FLOOR_RECOVER ) {
		headAngles[PITCH] = 0.0f;
		torsoAngles[PITCH] = 0.0f;
	}
	if ( ci->fixedlegs ) {
		legsAngles[YAW] = torsoAngles[YAW];
		legsAngles[PITCH] = 0.0f;
		legsAngles[ROLL] = 0.0f;
	}
	// ADDING FOR ZEQ2
	// We're flying, so we change the entire body's directions altogether.
	if(cg_advancedFlight.value 
		|| !cg_thirdPersonCamera.value
		|| (&cg.predictedPlayerState)->bitFlags & usingSoar 
		|| cent->currentState.playerBitFlags & usingSoar){
		VectorCopy( cent->lerpAngles, headAngles );
		VectorCopy( cent->lerpAngles, torsoAngles );
		VectorCopy( cent->lerpAngles, legsAngles );
	}
	else if(((&cg.predictedPlayerState)->weaponstate != WEAPON_READY || cent->currentState.weaponstate != WEAPON_READY)){
		VectorCopy( cent->lerpAngles, headAngles );
	}

	VectorCopy( cent->lerpAngles, cameraAngles );
	// END ADDING

	// pain twitch
	CG_AddPainTwitch( cent, torsoAngles );

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( cameraAngles, torsoAngles, cameraAngles );
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
	AnglesToAxis( cameraAngles, camera );
}


//==========================================================================

/*
===============
CG_InnerAuraSpikes
===============
*/
static void CG_InnerAuraSpikes( centity_t *cent, refEntity_t *head) {
	vec3_t up, origin;
	float xyzspeed;
	int r1,r2,r3,r4,r5,r6;
	clientInfo_t *ci;
	ci = &cgs.clientinfo[ cent->currentState.number ];
	if ( ci->bubbleTrailTime > cg.time ) {return;}
	xyzspeed = sqrt( cent->currentState.pos.trDelta[0] * cent->currentState.pos.trDelta[0] +
					cent->currentState.pos.trDelta[1] * cent->currentState.pos.trDelta[1] + 
					cent->currentState.pos.trDelta[2] * cent->currentState.pos.trDelta[2]);
	if ( cent->currentState.eFlags & EF_AURA && !xyzspeed){
			r1 = random() * 32;
			r2 = random() * 32;
			r3 = random() * 32;
			r4 = random() * 32;
			r5 = random() * 32;
			r6 = random() * 2;
			VectorSet( up, 0, 0, 64 ); // <-- Type 1
			//VectorMA(head->origin, 0, head->axis[0], up); // <-- Type 2
			//VectorMA(up, 64, head->axis[2], up); // <-- Type 2
			VectorMA(head->origin, 0, head->axis[0], origin);
			VectorMA(origin, -40, head->axis[2], origin);
			origin[0] += r1;
			origin[1] += r2;
			origin[2] += r3;
			origin[0] -= r4;
			origin[1] -= r5;
			origin[2] -= r6;
			//up[0] += r1; // <-- Type 2
			//up[1] += r2; // <-- Type 2
			//up[2] += r3; // <-- Type 2
			//up[0] -= r4; // <-- Type 2
			//up[1] -= r5; // <-- Type 2
			//up[2] -= r6; // <-- Type 2
		CG_AuraSpike( origin, up, 10, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cent); // <-- Type 1
		//CG_Aura_DrawInnerSpike (origin, up, 25.0f, cent); // <-- Type 2
		ci->bubbleTrailTime = cg.time + 50;
	}
}

/*
===============
CG_BubblesTrail
===============
*/
static void CG_BubblesTrail( centity_t *cent, refEntity_t *head) {
	vec3_t up, origin;
	int contents;
	entityState_t	*s1;
	float			xyzspeed;
	int r1,r2,r3,r4,r5,r6;
	if ( cent->currentState.eFlags & EF_DEAD ) {return;}
	s1 = &cent->currentState;
	// Measure the objects velocity
	xyzspeed = sqrt( cent->currentState.pos.trDelta[0] * cent->currentState.pos.trDelta[0] +
					cent->currentState.pos.trDelta[1] * cent->currentState.pos.trDelta[1] + 
					cent->currentState.pos.trDelta[2] * cent->currentState.pos.trDelta[2]);
	contents = trap_CM_PointContents( head->origin, 0 );
	if ( (cent->currentState.eFlags & EF_AURA) || xyzspeed ){
		if ( contents & ( CONTENTS_WATER ) ) {
			r1 = random() * 32;
			r2 = random() * 32;
			r3 = random() * 32;
			r4 = random() * 32;
			r5 = random() * 32;
			r6 = random() * 48;
			VectorSet( up, 0, 0, 8 );
			VectorMA(head->origin, 0, head->axis[0], origin);
			VectorMA(origin, -8, head->axis[2], origin);
			origin[0] += r1;
			origin[1] += r2;
			origin[2] += r3;
			origin[0] -= r4;
			origin[1] -= r5;
			origin[2] -= r6;
			if (cent->currentState.eFlags & EF_AURA) {
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 750, cg.time, cg.time + 375, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleMediumShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 750, cg.time, cg.time + 375, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleMediumShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 750, cg.time, cg.time + 375, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleMediumShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleLargeShader );
				CG_WaterBubble( origin, up, 1, 1, 1, 1, 1.0f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleLargeShader );
			} else {
				CG_WaterBubble( origin, up, 2, 1, 1, 1, 1.0f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
				CG_WaterBubble( origin, up, 2, 1, 1, 1, 1.0f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
				CG_WaterBubble( origin, up, 2, 1, 1, 1, 1.0f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
				CG_WaterBubble( origin, up, 2, 1, 1, 1, 1.0f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
				CG_WaterBubble( origin, up, 2, 1, 1, 1, 1.0f, 750, cg.time, cg.time + 375, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader );
				CG_WaterBubble( origin, up, 2, 1, 1, 1, 1.0f, 750, cg.time, cg.time + 375, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader );
				CG_WaterBubble( origin, up, 2, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleMediumShader );
				CG_WaterBubble( origin, up, 2, 1, 1, 1, 1.0f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleLargeShader );
			}
		}
	}
}

/*
===============
CG_BreathPuffs
Used only for player breathing bubbles under water
===============
*/
static void CG_BreathPuffs( centity_t *cent, refEntity_t *head) {
	clientInfo_t *ci;
	vec3_t up, origin;
	int contents;
	ci = &cgs.clientinfo[ cent->currentState.number ];
	contents = trap_CM_PointContents( head->origin, 0 );
	if ( cent->currentState.eFlags & EF_DEAD ) {return;}
	if ( ci->breathPuffTime > cg.time ) {return;}
	if ( contents & ( CONTENTS_WATER ) ) {
		VectorSet( up, 0, 0, 8 );
		VectorMA(head->origin, 6, head->axis[0], origin);
		VectorMA(origin, -2, head->axis[2], origin);
		CG_WaterBubble( origin, up, 3, 1, 1, 1, 0.66f, 3000, cg.time, cg.time + 2500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
		CG_WaterBubble( origin, up, 3, 1, 1, 1, 0.66f, 3000, cg.time, cg.time + 2500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
		CG_WaterBubble( origin, up, 3, 1, 1, 1, 0.66f, 3000, cg.time, cg.time + 2500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
		CG_WaterBubble( origin, up, 3, 1, 1, 1, 0.66f, 3000, cg.time, cg.time + 2500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader );
		CG_WaterBubble( origin, up, 3, 1, 1, 1, 0.66f, 2500, cg.time, cg.time + 2000, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader );
		CG_WaterBubble( origin, up, 3, 1, 1, 1, 0.66f, 2500, cg.time, cg.time + 2000, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader );
		CG_WaterBubble( origin, up, 3, 1, 1, 1, 0.66f, 2000, cg.time, cg.time + 1500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleMediumShader );
		CG_WaterBubble( origin, up, 3, 1, 1, 1, 0.66f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleLargeShader );
		ci->breathPuffTime = cg.time + 2500;
	}
}

/*
===============
CG_TrailItem
===============
*/
static void CG_TrailItem( centity_t *cent, qhandle_t hModel ) {
	refEntity_t		ent;
	vec3_t			angles;
	vec3_t			axis[3];

	VectorCopy( cent->lerpAngles, angles );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis( angles, axis );

	memset( &ent, 0, sizeof( ent ) );
	VectorMA( cent->lerpOrigin, -16, axis[0], ent.origin );
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis( angles, ent.axis );

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene( &ent );
}


/*
===============
CG_PlayerFlag
===============
*/
static void CG_PlayerFlag( centity_t *cent, qhandle_t hSkin, refEntity_t *torso ) {}


/*===============
CG_PlayerPowerups
===============*/
static void CG_PlayerPowerups( centity_t *cent, refEntity_t *torso ) {
	int		powerups;
	clientInfo_t	*ci;
	powerups = cent->currentState.powerups;
	if ( !powerups ) {
		return;
	}
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
}


/*===============
CG_PlayerFloatSprite
Float a sprite over the player's head
===============*/
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader ) {
	int				rf;
	refEntity_t		ent;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
}



/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent ) {
	int		team;
	if(cent->currentState.eFlags & EF_CONNECTION){
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader );
		return;
	}
	if ( cent->currentState.eFlags & EF_TALK ) {
		CG_PlayerFloatSprite( cent, cgs.media.chatBubble );
		return;
	}
	team = cgs.clientinfo[ cent->currentState.clientNum ].team;
	if ( !(cent->currentState.eFlags & EF_DEAD) && 
		cg.snap->ps.persistant[PERS_TEAM] == team &&
		cgs.gametype >= GT_TEAM &&
		cent->currentState.number != cg.snap->ps.clientNum) {
		if (cg_drawFriend.integer) {
			CG_PlayerFloatSprite( cent, cgs.media.friendShader );
		}
		return;
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define	SHADOW_DISTANCE		128
static qboolean CG_PlayerShadow( centity_t *cent, float *shadowPlane ) {
	vec3_t		end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t		trace;
	float		alpha;
	*shadowPlane = 0;
	if ( cg_shadows.integer == 0 ) {
		return qfalse;
	}
	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid ) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	if ( cg_shadows.integer != 1 ) {	// no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f ) 

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, 
		cent->pe.legs.yawAngle, alpha,alpha,alpha,1, qfalse, 24, qtrue );

	return qtrue;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
void CG_PlayerSplash( centity_t *cent, int scale ) {
	vec3_t			start, end;
	vec3_t			origin;
	trace_t			trace;
	polyVert_t		verts[4];
	entityState_t	*s1;
	clientInfo_t	*ci;
	int				contents;
	int				powerBoost;
	float			xyzspeed;
	s1 = &cent->currentState;

	// Measure the objects velocity
	xyzspeed = sqrt( cent->currentState.pos.trDelta[0] * cent->currentState.pos.trDelta[0] +
	cent->currentState.pos.trDelta[1] * cent->currentState.pos.trDelta[1] + 
	cent->currentState.pos.trDelta[2] * cent->currentState.pos.trDelta[2]);

	// Get the objects content value and position.
	BG_EvaluateTrajectory( s1, &s1->pos, cg.time, origin );
	contents = trap_CM_PointContents( origin, 0 );

	VectorCopy( cent->lerpOrigin, end );
	VectorCopy( cent->lerpOrigin, start );

	if (xyzspeed && cent->currentState.eFlags & EF_AURA) {
		powerBoost = 1;
		start[2] += 128;
		end[2] -= 128;
	} else if (!xyzspeed && cent->currentState.eFlags & EF_AURA) {
		powerBoost = 6;
		start[2] += 512;
		end[2] -= 512;
	} else if (xyzspeed) {
		powerBoost = 1;
		start[2] += 64;
		end[2] -= 48;
	} else {
		powerBoost = 1;
		start[2] += 32;
		end[2] -= 24;
	}

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = CG_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return;
	}

	// if the head isn't out of liquid, don't make a mark
	contents = CG_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

	if ( trace.fraction == 1.0 ) {
		return;
	}

	if (xyzspeed || (cent->currentState.eFlags & EF_AURA)) {

		if (powerBoost == 1) {
			CG_WaterSplash(trace.endpos,powerBoost+(xyzspeed/scale));
		}

		if ( cent->trailTime > cg.time ) {
			return;
		}

		cent->trailTime += 250;

		if ( cent->trailTime < cg.time ) {
			cent->trailTime = cg.time;
		}

		if (!xyzspeed && cent->currentState.eFlags & EF_AURA) {
			CG_WaterRipple(trace.endpos,scale/20,qtrue);
		}else{
			CG_WaterRipple(trace.endpos,powerBoost+(xyzspeed/scale),qfalse);
		}
	}

	// create a mark polygon
	VectorCopy( trace.endpos, verts[0].xyz );
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[1].xyz );
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[2].xyz );
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.wakeMarkShader, 4, verts );
}

/*
===============
CG_PlayerDirtPush
===============
*/
void CG_PlayerDirtPush( centity_t *cent, int scale, qboolean once ) {
	vec3_t			start, end;
	trace_t			trace;
	playerState_t	*ps;
	ps = &cg.snap->ps;

	if (!once){
		if ( cent->dustTrailTime > cg.time ) {
			return;
		}

		cent->dustTrailTime += 250;

		if ( cent->dustTrailTime < cg.time ) {
			cent->dustTrailTime = cg.time;
		}
	}

	VectorCopy( cent->lerpOrigin, end );
	VectorCopy( cent->lerpOrigin, start );

	if(once){
		end[2] -= 4096;
	}else{
		end[2] -= 512;
	}

	CG_Trace( &trace, cent->currentState.pos.trBase, NULL, NULL, end, cent->currentState.number, MASK_PLAYERSOLID );

	if (trace.fraction == 1.0f){
		return;
	}

	VectorCopy( trace.endpos, end );
	end[2] += 8;

	CG_DirtPush(end,trace.plane.normal,scale);
}

/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team, qboolean auraAlways ) {
	trap_R_AddRefEntityToScene( ent );
	if(!auraAlways){
		ent->customShader = cgs.media.globalCelLighting;
		trap_R_AddRefEntityToScene( ent );
	}
}


/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts( vec3_t normal, int numVerts, polyVert_t *verts )
{
	int				i, j;
	float			incoming;
	vec3_t			ambientLight;
	vec3_t			lightDir;
	vec3_t			directedLight;

	trap_R_LightForPoint( verts[0].xyz, ambientLight, directedLight, lightDir );

	for (i = 0; i < numVerts; i++) {
		incoming = DotProduct (normal, lightDir);
		if ( incoming <= 0 ) {
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		j = ( ambientLight[0] + incoming * directedLight[0] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = ( ambientLight[1] + incoming * directedLight[1] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = ( ambientLight[2] + incoming * directedLight[2] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

/*
===============
CG_Player
===============
*/
void CG_Player( centity_t *cent ) {
	clientInfo_t	*ci;
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	refEntity_t		camera;
	playerState_t	*ps;
	int				clientNum;
	int				renderfx;
	qboolean		shadow;
	float			shadowPlane;
	float			meshScale;
	qboolean		onBodyQue;
	int				tier,health,damageState,damageTextureState,damageModelState;
	int				state,enemyState;
	int				scale;
	float			xyzspeed;
	vec3_t			mins = {-15, -15, -24};
	vec3_t			maxs = {15, 15, 32};
	ps = &cg.snap->ps;
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ){CG_Error( "Bad clientNum on player entity" );}
	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid){return;}
	if(cg.resetValues){
		ci->damageModelState = 0;
		ci->damageTextureState = 0;
		cg.resetValues = qfalse;
	}
	onBodyQue = (cent->currentState.number != cent->currentState.clientNum);
	if(onBodyQue){tier = 0;}
	else{
		tier = cent->currentState.tier;
		if ( ci->tierCurrent != tier ) {
			ci->tierCurrent = tier;
			CG_AddEarthquake(cent->lerpOrigin, 400, 1, 0, 1, 400);
		}
	}
	if(ci->tierCurrent > ci->tierMax){
		ci->tierMax = ci->tierCurrent;
	}
	renderfx = 0;
	// Measure the players velocity
	xyzspeed = sqrt( cent->currentState.pos.trDelta[0] * cent->currentState.pos.trDelta[0] +
	cent->currentState.pos.trDelta[1] * cent->currentState.pos.trDelta[1] + 
	cent->currentState.pos.trDelta[2] * cent->currentState.pos.trDelta[2]);
	if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 1000){scale = 5;
	}else if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 5462){scale = 6;
	}else if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 10924){scale = 7;
	}else if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 16386){scale = 8;
	}else if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 21848){scale = 9;
	}else if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 27310){scale = 10;
	}else{scale = 11;
	}
	CG_PlayerSplash(cent,10*scale);
	//if(!cg.renderingThirdPerson){renderfx |= RF_THIRD_PERSON;}
	//else if(cg_cameraMode.integer){return;}
	if(cent->currentState.playerBitFlags & isBlinking){return;}
	if((cent->currentState.playerBitFlags & usingZanzoken) || ((cent->currentState.number == ps->clientNum) && (ps->bitFlags & usingZanzoken) && !(ps->bitFlags & isUnconcious) && !(ps->bitFlags & isDead) && !(ps->bitFlags & isCrashed))) {
		return;
	}
	health = ((float)cent->currentState.attackPowerCurrent / (float)cent->currentState.attackPowerTotal) * 100;
	if(health <= 100){damageState = 9;}
	if(health <= 90){damageState = 8;}
	if(health <= 80){damageState = 7;}
	if(health <= 70){damageState = 6;}
	if(health <= 60){damageState = 5;}
	if(health <= 50){damageState = 4;}
	if(health <= 40){damageState = 3;}
	if(health <= 30){damageState = 2;}
	if(health <= 20){damageState = 1;}
	if(health <= 10){damageState = 0;}
	damageModelState = damageTextureState = damageState;
	if(ci->damageModelState && damageModelState > (ci->damageModelState-1) && !ci->tierConfig[tier].damageModelsRevertHealed){
		damageModelState = ci->damageModelState - 1;
	}
	if(ci->damageTextureState && damageTextureState > (ci->damageTextureState-1) && !ci->tierConfig[tier].damageTexturesRevertHealed){
		damageTextureState = ci->damageTextureState - 1;
	}
	ci->damageTextureState = damageTextureState + 1;
	ci->damageModelState = damageModelState + 1;
	CG_PlayerSprites(cent);
	memset( &legs, 0, sizeof(legs) );
	memset( &torso, 0, sizeof(torso) );
	memset( &head, 0, sizeof(head) );
	memset( &camera, 0, sizeof(camera) );
	CG_PlayerAngles( cent, legs.axis, torso.axis, head.axis, camera.axis );
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
							  &torso.oldframe, &torso.frame, &torso.backlerp,
							  &head.oldframe, &head.frame, &head.backlerp,
							  &camera.oldframe, &camera.frame, &camera.backlerp);
	shadow = CG_PlayerShadow(cent,&shadowPlane);
	if (cg_shadows.integer == 3 && shadow){
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all
	legs.hModel = ci->modelDamageState[tier][2][damageModelState];
	legs.customSkin = ci->skinDamageState[tier][2][damageTextureState];
	VectorCopy(cent->lerpOrigin,legs.origin);
	VectorCopy(cent->lerpOrigin,legs.lightingOrigin);
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy (legs.origin, legs.oldorigin);	// don't positionally lerp at all
	if (!legs.hModel){return;}
	meshScale = ci->tierConfig[tier].meshScale;
	torso.hModel = ci->modelDamageState[tier][1][damageModelState];
	torso.customSkin = ci->skinDamageState[tier][1][damageTextureState];
	if(!torso.hModel){return;}
	VectorCopy(cent->lerpOrigin,torso.lightingOrigin);
	legs.origin[2] += ci->tierConfig[tier].meshOffset;
	VectorScale(torso.axis[0],meshScale,torso.axis[0]);
	VectorScale(torso.axis[1],meshScale,torso.axis[1]);
	VectorScale(torso.axis[2],meshScale,torso.axis[2]);
	VectorScale(legs.axis[0],meshScale,legs.axis[0]);
	VectorScale(legs.axis[1],meshScale,legs.axis[1]);
	VectorScale(legs.axis[2],meshScale,legs.axis[2]);
	CG_PositionRotatedEntityOnTag( &torso, &legs, legs.hModel, "tag_torso");
	VectorScale(torso.axis[0],1/meshScale,torso.axis[0]);
	VectorScale(torso.axis[1],1/meshScale,torso.axis[1]);
	VectorScale(torso.axis[2],1/meshScale,torso.axis[2]);
	//VectorScale(cent->mins,2.0,cent->mins);
	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;
	head.hModel = ci->modelDamageState[tier][0][damageModelState];
	head.customSkin = ci->skinDamageState[tier][0][damageTextureState];
	if(!head.hModel){return;}
	VectorCopy( cent->lerpOrigin, head.lightingOrigin );
	CG_PositionRotatedEntityOnTag( &head, &torso, torso.hModel, "tag_head");
	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;
	VectorCopy(cent->lerpOrigin,camera.origin);
	VectorCopy(cent->lerpOrigin,camera.lightingOrigin);
	camera.hModel = ci->cameraModel[tier];
	if(!camera.hModel){return;}
	camera.shadowPlane = shadowPlane;
	camera.renderfx = renderfx;
	VectorCopy (camera.origin, camera.oldorigin);	// don't positionally lerp at all
	CG_PositionRotatedEntityOnTag( &camera, &torso, torso.hModel, "tag_torso");
	CG_AddRefEntityWithPowerups( &legs, &cent->currentState, ci->team, ci->auraConfig[tier]->auraAlways );
	CG_AddRefEntityWithPowerups( &torso, &cent->currentState, ci->team, ci->auraConfig[tier]->auraAlways );
	CG_AddRefEntityWithPowerups( &head, &cent->currentState, ci->team, ci->auraConfig[tier]->auraAlways );
	CG_BreathPuffs(cent,&head);
	CG_BubblesTrail(cent,&head);
	CG_InnerAuraSpikes(cent,&head);
	memcpy( &(cent->pe.cameraRef ), &camera , sizeof(refEntity_t));
	memcpy( &(cent->pe.headRef ), &head , sizeof(refEntity_t));
	memcpy( &(cent->pe.torsoRef), &torso, sizeof(refEntity_t));
	memcpy( &(cent->pe.legsRef ), &legs , sizeof(refEntity_t));
	memcpy( &playerInfoDuplicate[cent->currentState.number], &cent->pe, sizeof(playerEntity_t));
	if(onBodyQue){return;}
	CG_Camera(cent);
	CG_AddPlayerWeapon(&torso,NULL,cent,ci->team);
	CG_PlayerPowerups(cent,&torso);
	if((cent->currentState.eFlags & EF_AURA) || ci->auraConfig[tier]->auraAlways){
		CG_AuraStart(cent);
		if(!xyzspeed){CG_PlayerDirtPush(cent,scale,qfalse);}
		if(ps->powerLevel[plCurrent] == ps->powerLevel[plMaximum] && ps->bitFlags & usingAlter){
			CG_AddEarthquake(cent->lerpOrigin, 400, 1, 1, 1, 25);
		}
	}
	else{CG_AuraEnd(cent);}
	CG_AddAuraToScene(cent);
	if(cg_drawBBox.value){
		trap_R_ModelBounds( head.hModel, mins, maxs, head.frame);
		CG_DrawBoundingBox( head.origin, mins, maxs);
		trap_R_ModelBounds( torso.hModel, mins, maxs, torso.frame);
		CG_DrawBoundingBox( torso.origin, mins, maxs);
		trap_R_ModelBounds( legs.hModel, mins, maxs,legs.frame);
		CG_DrawBoundingBox( legs.origin, mins, maxs);
	}
	if(ps->timers[tmKnockback]){
		if(ps->timers[tmKnockback] > 0){
			trap_S_StartSound(cent->lerpOrigin,ENTITYNUM_NONE,CHAN_BODY,cgs.media.knockbackSound);
			trap_S_AddLoopingSound(cent->currentState.number,cent->lerpOrigin,vec3_origin,cgs.media.knockbackLoopSound);
		}
		return;
	}
	if(ci->auraConfig[tier]->showLightning){CG_LightningEffect(cent->lerpOrigin, ci, tier);}
	if(ci->auraConfig[tier]->showLightning && ps->bitFlags & usingMelee){CG_BigLightningEffect(cent->lerpOrigin);}
}
qboolean CG_GetTagOrientationFromPlayerEntityHeadModel( centity_t *cent, char *tagName, orientation_t *tagOrient ) {
	int				i, clientNum;
	orientation_t	lerped;
	vec3_t			tempAxis[3];
	playerEntity_t	*pe;

	if ( cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}

	if ( !tagName[0] ) {
		return qfalse;
	}

	// The client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}
	
	// It is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !cgs.clientinfo[clientNum].infoValid ) {
		return qfalse;
	}

	// HACK: Use this copy, which is sure to be stored correctly, unlike
	//       reading it from cg_entities, which tends to clear out its
	//       fields every now and then. WTF?!
	pe = &playerInfoDuplicate[clientNum];
	
	// Prepare the destination orientation_t
	AxisClear( tagOrient->axis );
	/*
	if(cgs.clientinfo[clientNum].usingMD4) {
		Com_Printf("Trying to get an zMesh Tag called %s!\n", tagName);
	}
	*/

	// Try to find the tag and return its coordinates
	if ( trap_R_LerpTag( &lerped, pe->headRef.hModel, pe->head.oldFrame, pe->head.frame, 1.0 - pe->head.backlerp, tagName ) ) {
		VectorCopy( pe->headRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->headRef.axis[i], tagOrient->origin );
		}
		//Com_Printf("Found tag %s\n", tagName);
		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->headRef.axis, tagOrient->axis );

		return qtrue;
	}

	// If we didn't find the tag, return false
	return qfalse;
}


qboolean CG_GetTagOrientationFromPlayerEntityTorsoModel( centity_t *cent, char *tagName, orientation_t *tagOrient ) {
	int				i, clientNum;
	orientation_t	lerped;
	vec3_t			tempAxis[3];
	playerEntity_t	*pe;

	if ( cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}

	if ( !tagName[0] ) {
		return qfalse;
	}

	// The client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}
	
	// It is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !cgs.clientinfo[clientNum].infoValid ) {
		return qfalse;
	}

	// HACK: Use this copy, which is sure to be stored correctly, unlike
	//       reading it from cg_entities, which tends to clear out its
	//       fields every now and then. WTF?!
	pe = &playerInfoDuplicate[clientNum];
	
	// Prepare the destination orientation_t
	AxisClear( tagOrient->axis );

	// Try to find the tag and return its coordinates
	if ( trap_R_LerpTag( &lerped, pe->torsoRef.hModel, pe->torso.oldFrame, pe->torso.frame, 1.0 - pe->torso.backlerp, tagName ) ) {
		VectorCopy( pe->torsoRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->torsoRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->torsoRef.axis, tagOrient->axis );

		return qtrue;
	}/* else if(pe->torsoRef.renderfx & RF_SKEL) {
		for(i=0;i<pe->torsoRef.skel.numBones;i++)
		{
			if(!strcmp(pe->torsoRef.skel.bones[i].name,tagName))
			{
				VectorCopy(pe->torsoRef.skel.bones[i].origin,lerped.origin);
				VectorCopy(pe->torsoRef.skel.bones[i].axis[0],lerped.axis[0]);
				VectorCopy(pe->torsoRef.skel.bones[i].axis[1],lerped.axis[1]);
				VectorCopy(pe->torsoRef.skel.bones[i].axis[2],lerped.axis[2]);
			}
		}
		VectorCopy( pe->headRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->headRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->headRef.axis, tagOrient->axis );
		return qtrue;
	}*/

	// If we didn't find the tag, return false
	return qfalse;
}

qboolean CG_GetTagOrientationFromPlayerEntityLegsModel( centity_t *cent, char *tagName, orientation_t *tagOrient ) {
	int				i, clientNum;
	orientation_t	lerped;
	vec3_t			tempAxis[3];
	playerEntity_t	*pe;

	if ( cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}

	if ( !tagName[0] ) {
		return qfalse;
	}

	// The client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}
	
	// It is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !cgs.clientinfo[clientNum].infoValid ) {
		return qfalse;
	}

	// HACK: Use this copy, which is sure to be stored correctly, unlike
	//       reading it from cg_entities, which tends to clear out its
	//       fields every now and then. WTF?!
	pe = &playerInfoDuplicate[clientNum];
	
	// Prepare the destination orientation_t
	AxisClear( tagOrient->axis );

	// Try to find the tag and return its coordinates
	if ( trap_R_LerpTag( &lerped, pe->legsRef.hModel, pe->legs.oldFrame, pe->legs.frame, 1.0 - pe->legs.backlerp, tagName ) ) {
		VectorCopy( pe->legsRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->legsRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->legsRef.axis, tagOrient->axis );

		return qtrue;
	}/* else if(pe->legsRef.renderfx & RF_SKEL) {
		for(i=0;i<pe->legsRef.skel.numBones;i++)
		{
			if(!strcmp(pe->legsRef.skel.bones[i].name,tagName))
			{
				VectorCopy(pe->legsRef.skel.bones[i].origin,lerped.origin);
				VectorCopy(pe->legsRef.skel.bones[i].axis[0],lerped.axis[0]);
				VectorCopy(pe->legsRef.skel.bones[i].axis[1],lerped.axis[1]);
				VectorCopy(pe->legsRef.skel.bones[i].axis[2],lerped.axis[2]);
			}
		}
		VectorCopy( pe->headRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->headRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->headRef.axis, tagOrient->axis );
		return qtrue;
	}*/

	// If we didn't find the tag, return false
	return qfalse;
}

qboolean CG_GetTagOrientationFromPlayerEntityCameraModel( centity_t *cent, char *tagName, orientation_t *tagOrient ) {
	int				i, clientNum;
	orientation_t	lerped;
	vec3_t			tempAxis[3];
	playerEntity_t	*pe;

	if ( cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}

	if ( !tagName[0] ) {
		return qfalse;
	}

	// The client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}
	
	// It is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !cgs.clientinfo[clientNum].infoValid ) {
		return qfalse;
	}

	// HACK: Use this copy, which is sure to be stored correctly, unlike
	//       reading it from cg_entities, which tends to clear out its
	//       fields every now and then. WTF?!
	pe = &playerInfoDuplicate[clientNum];
	
	// Prepare the destination orientation_t
	AxisClear( tagOrient->axis );

	// Try to find the tag and return its coordinates
	if ( trap_R_LerpTag( &lerped, pe->cameraRef.hModel, pe->camera.oldFrame, pe->camera.frame, 1.0 - pe->camera.backlerp, tagName ) ) {
		VectorCopy( pe->cameraRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->cameraRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->cameraRef.axis, tagOrient->axis );

		return qtrue;
	}/* else if(pe->cameraRef.renderfx & RF_SKEL) {
		for(i=0;i<pe->cameraRef.skel.numBones;i++)
		{
			if(!strcmp(pe->cameraRef.skel.bones[i].name,tagName))
			{
				VectorCopy(pe->cameraRef.skel.bones[i].origin,lerped.origin);
				VectorCopy(pe->cameraRef.skel.bones[i].axis[0],lerped.axis[0]);
				VectorCopy(pe->cameraRef.skel.bones[i].axis[1],lerped.axis[1]);
				VectorCopy(pe->cameraRef.skel.bones[i].axis[2],lerped.axis[2]);
			}
		}
		VectorCopy( pe->cameraRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->cameraRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->cameraRef.axis, tagOrient->axis );
		return qtrue;
	}*/

	// If we didn't find the tag, return false
	return qfalse;
}

/*
===============
CG_GetTagOrientationFromPlayerEntity

If the entity in question is of the type CT_PLAYER, this gets the orientation of a tag on the player's model
and stores it in tagOrient. If the entity is of a different type, the tag is not found, or the model is on
the bodyQue and belongs to a disconnected client with invalid clientInfo, false is returned. In other cases
true is returned.
===============
*/
qboolean CG_GetTagOrientationFromPlayerEntity( centity_t *cent, char *tagName, orientation_t *tagOrient ) {
	if ( CG_GetTagOrientationFromPlayerEntityTorsoModel( cent, tagName, tagOrient ))
		return qtrue;
	if ( CG_GetTagOrientationFromPlayerEntityHeadModel( cent, tagName, tagOrient ))
		return qtrue;
	if ( CG_GetTagOrientationFromPlayerEntityLegsModel( cent, tagName, tagOrient ))
		return qtrue;
	if ( CG_GetTagOrientationFromPlayerEntityCameraModel( cent, tagName, tagOrient ))
		return qtrue;

	return qfalse;

/*
	int				i, clientNum;
	orientation_t	lerped;
	vec3_t			tempAxis[3];
	playerEntity_t	*pe;

	if ( cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}

	if ( !tagName[0] ) {
		return qfalse;
	}

	// The client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}
	
	// It is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !cgs.clientinfo[clientNum].infoValid ) {
		return qfalse;
	}

	// HACK: Use this copy, which is sure to be stored correctly, unlike
	//       reading it from cg_entities, which tends to clear out its
	//       fields every now and then. WTF?!
	pe = &playerInfoDuplicate[clientNum];
	
	// Prepare the destination orientation_t
	AxisClear( tagOrient->axis );

	// Step through the sections of the model to find which one holds the tag, and calculate the orientation_t
	if ( trap_R_LerpTag( &lerped, pe->torsoRef.hModel, pe->torso.oldFrame, pe->torso.frame, 1.0 - pe->torso.backlerp, tagName ) ) {
		VectorCopy( pe->torsoRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->torsoRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->torsoRef.axis, tagOrient->axis );

		return qtrue;
	}
	if ( trap_R_LerpTag( &lerped, pe->headRef.hModel, pe->head.oldFrame, pe->head.frame, 1.0 - pe->head.backlerp, tagName ) ) {
		VectorCopy( pe->headRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->headRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->headRef.axis, tagOrient->axis );

		return qtrue;
	}
	if ( trap_R_LerpTag( &lerped, pe->legsRef.hModel, pe->legs.oldFrame, pe->legs.frame, 1.0 - pe->legs.backlerp, tagName ) ) {
		VectorCopy( pe->legsRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->legsRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->legsRef.axis, tagOrient->axis );

		return qtrue;
	}

	// If we didn't find the tag in any of the model's parts, return false
	return qfalse;
*/

}


//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity( centity_t *cent ) {
	cent->errorTime = -99999;		// guarantee no error decay added
	cent->extrapolated = qfalse;	

	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim, qfalse );
	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim, qfalse );

	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.apos, cg.time, cent->lerpAngles );

	VectorCopy( cent->lerpOrigin, cent->rawOrigin );
	VectorCopy( cent->lerpAngles, cent->rawAngles );

	memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset( &cent->pe.torso, 0, sizeof( cent->pe.legs ) );
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;

	memset( &cent->pe.camera, 0, sizeof( cent->pe.camera ) );
	cent->pe.camera.yawAngle = cent->rawAngles[YAW];
	cent->pe.camera.yawing = qfalse;
	cent->pe.camera.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.camera.pitching = qfalse;

	if ( cg_debugPosition.integer ) {
		CG_Printf("%i ResetPlayerEntity yaw=%f\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
}

void CG_SpawnLightSpeedGhost( centity_t *cent ) {
	if (random() < 0.2){
		trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound1 );
	}else if (random() < 0.4){
		trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound2 );
	}else if (random() < 0.6){
		trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound3 );
	}else if ( random() < 0.8){
		trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound4 );
	} else {
		trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound5 );
	}
}

