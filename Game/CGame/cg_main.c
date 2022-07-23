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
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"
int forceModelModificationCount = -1;

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void CG_Shutdown( void );


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {

	switch ( command ) {
	case CG_INIT:
		CG_Init( arg0, arg1, arg2 );
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame( arg0, arg1, arg2 );
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
	default:
		CG_Error( "vmMain: unknown command %i", command );
		break;
	}
	return -1;
}


cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
weaponInfo_t		cg_weapons[MAX_WEAPONS];

vmCvar_t	cg_railTrailTime;
vmCvar_t	cg_centertime;
vmCvar_t	cg_runpitch;
vmCvar_t	cg_runroll;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_gibs;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_draw3dIcons;
vmCvar_t	cg_drawIcons;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_crosshairMargin;
vmCvar_t	cg_crosshairBars;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairHealth;
vmCvar_t	cg_scripted2D;
vmCvar_t	cg_scriptedCamera;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_showmiss;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_addMarks;
vmCvar_t	cg_brassTime;
vmCvar_t	cg_chatTime;
vmCvar_t	cg_viewsize;
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_tracerWidth;
vmCvar_t	cg_tracerLength;
vmCvar_t	cg_displayObituary;
vmCvar_t	cg_ignore;
vmCvar_t	cg_fov;
vmCvar_t	cg_zoomFov;
vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_thirdPersonHeight;
vmCvar_t	cg_thirdPersonSlide;
vmCvar_t	cg_lockedRange;
vmCvar_t	cg_lockedAngle;
vmCvar_t	cg_lockedHeight;
vmCvar_t	cg_lockedSlide;
vmCvar_t	cg_advancedFlight;
vmCvar_t	cg_thirdPersonCameraDamp;
vmCvar_t	cg_thirdPersonTargetDamp;
vmCvar_t	cg_thirdPersonMeleeCameraDamp;
vmCvar_t	cg_thirdPersonMeleeTargetDamp;
vmCvar_t	cg_synchronousClients;
vmCvar_t 	cg_teamChatTime;
vmCvar_t 	cg_teamChatHeight;
vmCvar_t 	cg_stats;
vmCvar_t 	cg_buildScript;
vmCvar_t 	cg_forceModel;
vmCvar_t	cg_paused;
vmCvar_t	cg_blood;
vmCvar_t	cg_deferPlayers;
vmCvar_t	cg_drawTeamOverlay;
vmCvar_t	cg_teamOverlayUserinfo;
vmCvar_t	cg_drawFriend;
vmCvar_t	cg_teamChatsOnly;
vmCvar_t	cg_noVoiceChats;
vmCvar_t	cg_noVoiceText;
vmCvar_t	cg_hudFiles;
vmCvar_t 	cg_scorePlum;
vmCvar_t 	cg_smoothClients;
vmCvar_t	pmove_fixed;
//vmCvar_t	cg_pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t	cg_pmove_msec;
vmCvar_t	cg_cameraMode;
vmCvar_t	cg_cameraOrbit;
vmCvar_t	cg_cameraOrbitDelay;
vmCvar_t	cg_timescaleFadeEnd;
vmCvar_t	cg_timescaleFadeSpeed;
vmCvar_t	cg_timescale;
vmCvar_t	cg_smallFont;
vmCvar_t	cg_bigFont;
vmCvar_t	cg_trueLightning;
vmCvar_t	cg_enableDust;
vmCvar_t	cg_enableBreath;
//ADDING FOR ZEQ2
vmCvar_t	cg_lockonDistance;
vmCvar_t	cg_tailDetail;
vmCvar_t	cg_verboseParse;
vmCvar_t	r_beamDetail;
vmCvar_t	cg_soundAttenuation;
vmCvar_t	cg_thirdPersonCamera;
vmCvar_t	cg_beamControl;
vmCvar_t	cg_music;
vmCvar_t	cg_playTransformTrackToEnd;
vmCvar_t	cg_particlesQuality;
vmCvar_t	cg_particlesType;
vmCvar_t	cg_particlesStop;
vmCvar_t	cg_particlesMaximum;
vmCvar_t	cg_drawBBox;
//END ADDING
typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = {
	{ &cg_ignore, "cg_ignore", "0", 0 },	// used for debugging
	{ &cg_displayObituary, "cg_displayObituary", "0", CVAR_ARCHIVE },
	{ &cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE },
	{ &cg_fov, "cg_fov", "90", CVAR_ARCHIVE },
	{ &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE  },
	{ &cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE  },
	{ &cg_scripted2D, "cg_scripted2D", "1", CVAR_ARCHIVE  },
	{ &cg_scriptedCamera, "cg_scriptedCamera", "1", CVAR_ARCHIVE  },
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE  },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE  },
	{ &cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE  },
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE  },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE  },
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE  },
	{ &cg_advancedFlight, "cg_advancedFlight", "0", CVAR_USERINFO |CVAR_ARCHIVE  },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &cg_crosshairSize, "cg_crosshairSize", "3", CVAR_ARCHIVE },
	{ &cg_crosshairMargin, "cg_crosshairMargin", "4", CVAR_ARCHIVE },
	{ &cg_crosshairBars, "cg_crosshairBars", "0", CVAR_ARCHIVE },
	{ &cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE },
	{ &cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE },
	{ &cg_chatTime, "cg_chatTime", "3500", CVAR_ARCHIVE },
	{ &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE },
	{ &cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE  },
	{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT },
	{ &cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	{ &cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE },
	{ &cg_bobup , "cg_bobup", "0.005", CVAR_ARCHIVE },
	{ &cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE },
	{ &cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT },
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
	{ &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100", 0 },
	{ &cg_nopredict, "cg_nopredict", "0", 0 },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
	{ &cg_showmiss, "cg_showmiss", "0", 0 },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT },
	{ &cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT },
	{ &cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT },
	{ &cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT },
	{ &cg_lockedRange, "cg_lockedRange", "30", CVAR_ARCHIVE },
	{ &cg_lockedAngle, "cg_lockedAngle", "320", 0 },
	{ &cg_lockedHeight, "cg_lockedHeight", "0", CVAR_ARCHIVE },
	{ &cg_lockedSlide, "cg_lockedSlide", "40", CVAR_ARCHIVE },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "80", CVAR_ARCHIVE },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", 0 },
	{ &cg_thirdPersonHeight, "cg_thirdPersonHeight", "0", CVAR_ARCHIVE },
	{ &cg_thirdPersonSlide, "cg_thirdPersonSlide", "-20", CVAR_ARCHIVE },
	{ &cg_thirdPersonCameraDamp, "cg_thirdPersonCameraDamp", "0.2", CVAR_ARCHIVE },
	{ &cg_thirdPersonTargetDamp, "cg_thirdPersonTargetDamp", "0.5", CVAR_ARCHIVE },
	{ &cg_thirdPersonMeleeCameraDamp, "cg_thirdPersonMeleeCameraDamp", "0.1", CVAR_ARCHIVE },
	{ &cg_thirdPersonMeleeTargetDamp, "cg_thirdPersonMeleeTargetDamp", "0.9", CVAR_ARCHIVE },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", CVAR_ARCHIVE },
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE  },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE  },
	{ &cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE  },
	{ &cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE },
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE },
	{ &cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO },
	{ &cg_stats, "cg_stats", "0", 0 },
	{ &cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE },
	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE },
	{ &cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE },
	{ &cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE },
	{ &cg_buildScript, "com_buildScript", "0", 0 },	// force loading of all possible data and error on failures
	{ &cg_paused, "cl_paused", "0", CVAR_ROM },
	{ &cg_blood, "com_blood", "1", CVAR_ARCHIVE },
	{ &cg_synchronousClients, "g_synchronousClients", "1", 0 },	// communicated by systeminfo
	{ &cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO},
	{ &cg_enableBreath, "g_enableBreath", "1", CVAR_SERVERINFO},
	{ &cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_ARCHIVE},
	{ &cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{ &cg_timescale, "timescale", "1", 0},
	{ &cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE},
	{ &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO},
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO},
	{ &cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{ &cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},
	{ &cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE},
	// ADDING FOR ZEQ2
	{ &cg_lockonDistance, "cg_lockonDistance", "150", CVAR_CHEAT },
	{ &cg_tailDetail,	"cg_tailDetail", "50", CVAR_ARCHIVE},
	{ &cg_verboseParse, "cg_verboseParse", "0", CVAR_ARCHIVE},
	{ &r_beamDetail,	"r_beamDetail", "2", CVAR_ARCHIVE},
	{ &cg_soundAttenuation, "cg_soundAttenuation", "0.0001", CVAR_CHEAT},
	{ &cg_thirdPersonCamera, "cg_thirdPersonCamera", "1", CVAR_ARCHIVE},
	{ &cg_beamControl, "cg_beamControl", "1", CVAR_ARCHIVE},
	{ &cg_music, "cg_music", "0.6", CVAR_ARCHIVE},
	{ &cg_playTransformTrackToEnd, "cg_playTransformTrackToEnd", "0", CVAR_ARCHIVE},
	{ &cg_particlesType, "cg_particlesType", "1", CVAR_ARCHIVE},
	{ &cg_particlesQuality, "cg_particlesQuality", "1", CVAR_ARCHIVE},
	{ &cg_particlesStop, "cg_particlesStop", "0", CVAR_ARCHIVE},
	{ &cg_particlesMaximum, "cg_particlesMaximum", "1024", CVAR_ARCHIVE},
	{ &cg_drawBBox, "cg_drawBBox", "0", CVAR_CHEAT }
	// END ADDING
//	{ &cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE }
};

static int  cvarTableSize = ARRAY_LEN( cvarTable );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	char		var[MAX_TOKEN_CHARS];

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	forceModelModificationCount = cg_forceModel.modificationCount;

	trap_Cvar_Register(NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "legsmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "team_model", DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "team_headmodel", DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE );
}

/*																																			
===================
CG_ForceModelChange
===================
*/
static void CG_ForceModelChange( void ) {
	int		i;

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0] ) {
			continue;
		}
		CG_NewClientInfo( i );
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}

	// check for modications here

	// If team overlay is on, ask for updates from the server.  If it's off,
	// let the server know so we don't receive it
	if ( drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount ) {
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if ( cg_drawTeamOverlay.integer > 0 ) {
			trap_Cvar_Set( "teamoverlay", "1" );
		} else {
			trap_Cvar_Set( "teamoverlay", "0" );
		}
	}
}

int CG_CrosshairPlayer( void ) {
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) ) {
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
	if ( !cg.attackerTime ) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	CG_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	CG_Printf ("%s", text);
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	name[MAX_QPATH];
	const char	*soundName;

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound( soundName, qfalse );
	}
	// ADDING FOR ZEQ2

	cgs.media.radarwarningSound = trap_S_RegisterSound( "interface/sense/warning.ogg", qfalse );
	cgs.media.lightspeedSound1 = trap_S_RegisterSound( "effects/zanzoken/zanzoken1.ogg", qfalse );
	cgs.media.lightspeedSound2 = trap_S_RegisterSound( "effects/zanzoken/zanzoken2.ogg", qfalse );
	cgs.media.lightspeedSound3 = trap_S_RegisterSound( "effects/zanzoken/zanzoken3.ogg", qfalse );
	cgs.media.lightspeedSound4 = trap_S_RegisterSound( "effects/zanzoken/zanzoken4.ogg", qfalse );
	cgs.media.lightspeedSound5 = trap_S_RegisterSound( "effects/zanzoken/zanzoken5.ogg", qfalse );
	cgs.media.bigLightningSound1 = trap_S_RegisterSound( "effects/melee/lightning1.ogg", qfalse );
	cgs.media.bigLightningSound2 = trap_S_RegisterSound( "effects/melee/lightning2.ogg", qfalse );
	cgs.media.bigLightningSound3 = trap_S_RegisterSound( "effects/melee/lightning3.ogg", qfalse );
	cgs.media.bigLightningSound4 = trap_S_RegisterSound( "effects/melee/lightning4.ogg", qfalse );
	cgs.media.bigLightningSound5 = trap_S_RegisterSound( "effects/melee/lightning5.ogg", qfalse );
	cgs.media.bigLightningSound6 = trap_S_RegisterSound( "effects/melee/lightning6.ogg", qfalse );
	cgs.media.bigLightningSound7 = trap_S_RegisterSound( "effects/melee/lightning7.ogg", qfalse );
	cgs.media.blockSound = trap_S_RegisterSound( "effects/melee/block.ogg", qfalse );
	cgs.media.knockbackSound = trap_S_RegisterSound( "effects/melee/knockback.ogg", qfalse );
	cgs.media.knockbackLoopSound = trap_S_RegisterSound( "effects/melee/knockbackLoop.ogg", qfalse );
	cgs.media.speedMeleeSound = trap_S_RegisterSound( "effects/melee/speedHit1.ogg", qfalse );
	cgs.media.speedMissSound = trap_S_RegisterSound( "effects/melee/speedMiss1.ogg", qfalse );
	cgs.media.speedBlockSound = trap_S_RegisterSound( "effects/melee/speedBlock1.ogg", qfalse );
	cgs.media.stunSound = trap_S_RegisterSound( "effects/melee/stun1.ogg", qfalse );
	cgs.media.powerStunSound1 = trap_S_RegisterSound( "effects/melee/powerStun1.ogg", qfalse );
	cgs.media.powerStunSound2 = trap_S_RegisterSound( "effects/melee/powerStun2.ogg", qfalse );
	cgs.media.powerMeleeSound = trap_S_RegisterSound( "effects/melee/powerHit1.ogg", qfalse );
	cgs.media.powerMissSound = trap_S_RegisterSound( "effects/melee/powerMiss1.ogg", qfalse );
	cgs.media.lockonStart = trap_S_RegisterSound( "effects/powerSense.ogg", qfalse );
	cgs.media.nullSound = trap_S_RegisterSound( "effects/null.ogg", qfalse );
	cgs.media.airBrake1 = trap_S_RegisterSound( "effects/airBrake1.ogg", qfalse );
	cgs.media.airBrake2 = trap_S_RegisterSound( "effects/airBrake2.ogg", qfalse );
	cgs.media.hover = trap_S_RegisterSound( "effects/hover.ogg", qfalse );
	cgs.media.hoverFast = trap_S_RegisterSound( "effects/hoverFast.ogg", qfalse );
	cgs.media.hoverLong = trap_S_RegisterSound( "effects/hoverLong.ogg", qfalse );
	cgs.media.waterSplashSmall1 = trap_S_RegisterSound( "effects/water/SplashSmall.ogg", qfalse );
	cgs.media.waterSplashSmall2 = trap_S_RegisterSound( "effects/water/SplashSmall2.ogg", qfalse );
	cgs.media.waterSplashSmall3 = trap_S_RegisterSound( "effects/water/SplashSmall3.ogg", qfalse );
	cgs.media.waterSplashMedium1 = trap_S_RegisterSound( "effects/water/SplashMedium.ogg", qfalse );
	cgs.media.waterSplashMedium2 = trap_S_RegisterSound( "effects/water/SplashMedium2.ogg", qfalse );
	cgs.media.waterSplashMedium3 = trap_S_RegisterSound( "effects/water/SplashMedium3.ogg", qfalse );
	cgs.media.waterSplashMedium4 = trap_S_RegisterSound( "effects/water/SplashMedium4.ogg", qfalse );
	cgs.media.waterSplashLarge1 = trap_S_RegisterSound( "effects/water/SplashLarge.ogg", qfalse );
	cgs.media.waterSplashExtraLarge1 = trap_S_RegisterSound( "effects/water/SplashExtraLarge.ogg", qfalse );
	cgs.media.waterSplashExtraLarge2 = trap_S_RegisterSound( "effects/water/SplashExtraLarge2.ogg", qfalse );
	// END ADDING

}


//===================================================================================

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
	static char		*sb_nums[11] = {
		"interface/fonts/numbers/0",
		"interface/fonts/numbers/1",
		"interface/fonts/numbers/2",
		"interface/fonts/numbers/3",
		"interface/fonts/numbers/4",
		"interface/fonts/numbers/5",
		"interface/fonts/numbers/6",
		"interface/fonts/numbers/7",
		"interface/fonts/numbers/8",
		"interface/fonts/numbers/9",
		"interface/fonts/numbers/-",
	};

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );
	CG_LoadingString( "game media" );

	for ( i=0 ; i<11 ; i++) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader( sb_nums[i] );
	}

	cgs.media.waterBubbleLargeShader = trap_R_RegisterShader( "waterBubbleLarge" );
	cgs.media.waterBubbleMediumShader = trap_R_RegisterShader( "waterBubbleMedium" );
	cgs.media.waterBubbleSmallShader = trap_R_RegisterShader( "waterBubbleSmall" );
	cgs.media.waterBubbleTinyShader = trap_R_RegisterShader( "waterBubbleTiny" );
	cgs.media.selectShader = trap_R_RegisterShader( "interface/hud/select.png" );
	cgs.media.chatBubble = trap_R_RegisterShader( "chatBubble");

	for ( i = 0 ; i < NUM_CROSSHAIRS ; i++ ) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("crosshair%c", 'a'+i) );
	}
	cgs.media.speedLineShader = trap_R_RegisterShaderNoMip("speedLines");
	cgs.media.speedLineSpinShader = trap_R_RegisterShaderNoMip("speedLinesSpin");
	cgs.media.globalCelLighting = trap_R_RegisterShader("GlobalCelLighting");
	cgs.media.waterSplashSkin = trap_R_RegisterSkin( "effects/water/waterSplash.skin" );
	cgs.media.waterSplashModel = trap_R_RegisterModel( "effects/water/waterSplash.md3" );
	cgs.media.waterRippleSkin = trap_R_RegisterSkin( "effects/water/waterRipple.skin" );
	cgs.media.waterRippleModel = trap_R_RegisterModel( "effects/water/waterRipple.md3" );
	cgs.media.waterRippleSingleSkin = trap_R_RegisterSkin( "effects/water/waterRippleSingle.skin" );
	cgs.media.waterRippleSingleModel = trap_R_RegisterModel( "effects/water/waterRippleSingle.md3" );
	cgs.media.meleeSpeedEffectShader = trap_R_RegisterShader( "skills/energyBlast" );
	cgs.media.meleePowerEffectShader = trap_R_RegisterShader( "shockwave" );
	cgs.media.teleportEffectShader = trap_R_RegisterShader( "teleportEffect" );
	cgs.media.boltEffectShader = trap_R_RegisterShader( "boltEffect" );
	cgs.media.auraLightningSparks1 = trap_R_RegisterShader( "AuraLightningSparks1" );
	cgs.media.auraLightningSparks2 = trap_R_RegisterShader( "AuraLightningSparks2" );
	cgs.media.powerStruggleRaysEffectShader = trap_R_RegisterShader( "PowerStruggleRays" );
	cgs.media.powerStruggleShockwaveEffectShader = trap_R_RegisterShader( "PowerStruggleShockwave" );
	cgs.media.bfgLFGlare = trap_R_RegisterShader("bfgLFGlare");
	cgs.media.bfgLFDisc = trap_R_RegisterShader("bfgLFDisc");
	cgs.media.bfgLFRing = trap_R_RegisterShader("bfgLFRing");
	cgs.media.bfgLFStar = trap_R_RegisterShader("bfgLFStar");
	cgs.media.bfgLFLine = trap_R_RegisterShader("bfgLFLine");

	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	cgs.media.wakeMarkShader = trap_R_RegisterShader( "wake" );
	cgs.media.dirtPushShader = trap_R_RegisterShader( "DirtPush" );
	cgs.media.dirtPushSkin = trap_R_RegisterSkin( "effects/shockwave/dirtPush.skin" );
	cgs.media.dirtPushModel = trap_R_RegisterModel( "effects/shockwave/dirtPush.md3" );

	cgs.media.hudShader = trap_R_RegisterShaderNoMip( "interface/hud/main.png" );
	cgs.media.chatBackgroundShader = trap_R_RegisterShaderNoMip("chatBox");
	cgs.media.markerAscendShader = trap_R_RegisterShaderNoMip( "interface/hud/markerAscend.png" );
	cgs.media.markerDescendShader = trap_R_RegisterShaderNoMip( "interface/hud/markerDescend.png" );
	cgs.media.breakLimitShader = trap_R_RegisterShaderNoMip( "breakLimit" );
	cgs.media.RadarBlipShader = trap_R_RegisterShaderNoMip( "interface/sense/blip.png" );
	cgs.media.RadarBlipTeamShader = trap_R_RegisterShaderNoMip( "interface/sense/blipteam.png" );
	cgs.media.RadarBurstShader = trap_R_RegisterShaderNoMip( "interface/sense/burst.png" );
	cgs.media.RadarWarningShader = trap_R_RegisterShaderNoMip( "interface/sense/warning.png" );
	cgs.media.RadarMidpointShader = trap_R_RegisterShaderNoMip( "interface/sense/midpoint.png" );
	// END ADDING

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs, 0 );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_MODELS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel( modelName );
	}
}



/*																																			
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString(void) {
	int i;
	cg.spectatorList[0] = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
}


/*																																			
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients( void ) {
	int		i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		if (cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0]) {
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i );
	}
	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}



/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum ) {
	const char	*s;

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof(cg_entities) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader		= trap_R_RegisterShader( "interface/fonts/font0.png" );
	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );
	cgs.media.clearShader		= trap_R_RegisterShader( "clear" );
	cgs.media.charsetProp		= trap_R_RegisterShaderNoMip( "interface/fonts/font1.png" );
	cgs.media.charsetPropGlow	= trap_R_RegisterShaderNoMip( "interface/fonts/font1Glow.png" );
	cgs.media.charsetPropB		= trap_R_RegisterShaderNoMip( "interface/fonts/font2.png" );

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	cg.weaponSelect = 1;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;			 // old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strcmp( s, GAME_VERSION ) ) {
		CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo();

	// load the new map
	CG_LoadingString( "collision map" );

	trap_CM_LoadMap( cgs.mapname );
	cg.loading = qtrue;		// force players to load instead of defer

	CG_LoadingString( "sounds" );

	CG_RegisterSounds();

	CG_LoadingString( "graphics" );

	CG_RegisterGraphics();

	CG_LoadingString( "clients" );

	CG_RegisterClients();		// if low on memory, some clients will be deferred


	cg.loading = qfalse;	// future players will be deferred
	CG_InitLocalEntities();

	// ADDING FOR ZEQ2
	CG_FrameHist_Init();
	CG_InitTrails();
	CG_InitParticleSystems();
	CG_InitBeamTables();
	CG_InitRadarBlips();
	CG_Music_Start();
	// END ADDING

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_LoadingString( "" );
	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds( qtrue );
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}


/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
void CG_EventHandling(int type) {
}



void CG_KeyEvent(int key, qboolean down) {
}

void CG_MouseEvent(int x, int y) {
}
