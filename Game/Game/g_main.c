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
#include "g_local.h"
level_locals_t level;
typedef struct{
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
	int			modificationCount;  // for tracking changes
	qboolean	trackChange;	    // track this variable, and announce if changed
	qboolean	teamShader;			// track and if changed, update shader state
}cvarTable_t;
gentity_t g_entities[MAX_GENTITIES];
gclient_t g_clients[MAX_CLIENTS];
vmCvar_t g_gametype;
vmCvar_t g_dmflags;
vmCvar_t g_fraglimit;
vmCvar_t g_timelimit;
vmCvar_t g_capturelimit;
vmCvar_t g_friendlyFire;
vmCvar_t g_password;
vmCvar_t g_needpass;
vmCvar_t g_maxclients;
vmCvar_t g_maxGameClients;
vmCvar_t g_dedicated;
vmCvar_t g_cheats;
vmCvar_t g_knockback;
vmCvar_t g_quadfactor;
vmCvar_t g_forcerespawn;
vmCvar_t g_inactivity;
vmCvar_t g_debugMove;
vmCvar_t g_debugDamage;
vmCvar_t g_debugAlloc;
vmCvar_t g_weaponRespawn;
vmCvar_t g_weaponTeamRespawn;
vmCvar_t g_motd;
vmCvar_t g_synchronousClients;
vmCvar_t g_warmup;
vmCvar_t g_doWarmup;
vmCvar_t g_restarted;
vmCvar_t g_logfile;
vmCvar_t g_logfileSync;
vmCvar_t g_blood;
vmCvar_t g_podiumDist;
vmCvar_t g_podiumDrop;
vmCvar_t g_allowVote;
vmCvar_t g_banIPs;
vmCvar_t g_filterBan;
vmCvar_t g_smoothClients;
vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;
vmCvar_t g_rankings;
vmCvar_t g_listEntity;
// ADDING FOR ZEQ2
vmCvar_t g_verboseParse;
vmCvar_t g_powerlevel;
vmCvar_t g_powerlevelMaximum;
vmCvar_t g_breakLimitRate;
vmCvar_t g_allowTiers;
vmCvar_t g_allowScoreboard;
vmCvar_t g_allowSoar;
vmCvar_t g_allowBoost;
vmCvar_t g_allowFly;
vmCvar_t g_allowZanzoken;
vmCvar_t g_allowJump;
vmCvar_t g_allowBallFlip;
vmCvar_t g_allowOverheal;
vmCvar_t g_allowBreakLimit;
vmCvar_t g_allowMelee;
vmCvar_t g_allowLockon;
vmCvar_t g_allowBlock;
vmCvar_t g_allowAdvancedMelee;
vmCvar_t g_rolling;
vmCvar_t g_running;
vmCvar_t g_pointGravity;
vmCvar_t g_quickTransformCost;
vmCvar_t g_quickTransformCostPerTier ;
vmCvar_t g_quickZanzokenCost;
vmCvar_t g_quickZanzokenDistance;
static cvarTable_t gameCvarTable[] = {
	// don't override the cheat state set by the system
	{&g_cheats						,"sv_cheats"					,""				,0												,0,qfalse,qfalse},
	// noset vars
	{NULL							,"Directory"					,BASEDIR		,CVAR_SERVERINFO|CVAR_ROM						,0,qfalse,qfalse},
	{NULL							,"productDate"					,__DATE__		,CVAR_ROM										,0,qfalse,qfalse},
	{&g_restarted					,"g_restarted"					,"0"			,CVAR_ROM										,0,qfalse,qfalse},
	{NULL							,"sv_mapname"					,""				,CVAR_SERVERINFO|CVAR_ROM						,0,qfalse,qfalse},
	// latched vars
	{&g_gametype					,"g_gametype"					,"0"			,CVAR_SERVERINFO|CVAR_USERINFO|CVAR_LATCH		,0,qfalse,qfalse},
	{&g_maxclients					,"sv_maxclients"				,"8"			,CVAR_SERVERINFO|CVAR_LATCH|CVAR_ARCHIVE		,0,qfalse,qfalse},
	{&g_maxGameClients				,"g_maxGameClients"				,"0"			,CVAR_SERVERINFO|CVAR_LATCH|CVAR_ARCHIVE		,0,qfalse,qfalse},
	// change anytime vars			
	{&g_dmflags						,"dmflags"						,"0"			,CVAR_SERVERINFO|CVAR_ARCHIVE					,0,qtrue,qfalse},
	{&g_fraglimit					,"fraglimit"					,"20"			,CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART	,0,qtrue,qfalse},
	{&g_timelimit					,"timelimit"					,"0"			,CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART	,0,qtrue,qfalse},
	{&g_capturelimit				,"capturelimit"					,"8"			,CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART	,0,qtrue,qfalse},
	{&g_synchronousClients			,"g_synchronousClients"			,"0"			,CVAR_SYSTEMINFO								,0,qfalse,qfalse},
	{&g_friendlyFire				,"g_friendlyFire"				,"0"			,CVAR_ARCHIVE									,0,qtrue,qfalse},
	{&g_warmup						,"g_warmup"						,"20"			,CVAR_ARCHIVE									,0,qtrue,qfalse},
	{&g_doWarmup					,"g_doWarmup"					,"0"			,CVAR_ARCHIVE									,0,qtrue,qfalse},
	{&g_logfile						,"g_log"						,"games.log"	,CVAR_ARCHIVE									,0,qfalse,qfalse},
	{&g_logfileSync					,"g_logsync"					,"0"			,CVAR_ARCHIVE									,0,qfalse,qfalse},
	{&g_password					,"g_password"					,""				,CVAR_USERINFO									,0,qfalse,qfalse},
	{&g_banIPs						,"g_banIPs"						,""				,CVAR_ARCHIVE									,0,qfalse,qfalse},
	{&g_filterBan					,"g_filterBan"					,"1"			,CVAR_ARCHIVE									,0,qfalse,qfalse},
	{&g_needpass					,"g_needpass"					,"0"			,CVAR_SERVERINFO|CVAR_ROM						,0,qfalse,qfalse},
	{&g_dedicated					,"dedicated"					,"0"			,0												,0,qfalse,qfalse},
	{&g_knockback					,"g_knockback"					,"1000"			,0												,0,qtrue,qfalse},
	{&g_quadfactor					,"g_quadfactor"					,"3"			,0												,0,qtrue,qfalse},
	{&g_weaponRespawn				,"g_weaponrespawn"				,"5"			,0												,0,qtrue,qfalse},
	{&g_weaponTeamRespawn			,"g_weaponTeamRespawn"			,"30"			,0												,0,qtrue,qfalse},
	{&g_forcerespawn				,"g_forcerespawn"				,"20"			,0												,0,qtrue,qfalse},
	{&g_inactivity					,"g_inactivity"					,"0"			,0												,0,qtrue,qfalse},
	{&g_debugMove					,"g_debugMove"					,"0"			,0												,0,qfalse,qfalse},
	{&g_debugDamage					,"g_debugDamage"				,"0"			,0												,0,qfalse,qfalse},
	{&g_debugAlloc					,"g_debugAlloc"					,"0"			,0												,0,qfalse,qfalse},
	{&g_motd						,"g_motd"						,""				,0												,0,qfalse,qfalse},
	{&g_blood						,"com_blood"					,"1"			,0												,0,qfalse,qfalse},
	{&g_podiumDist					,"g_podiumDist"					,"80"			,0												,0,qfalse,qfalse},
	{&g_podiumDrop					,"g_podiumDrop"					,"70"			,0												,0,qfalse,qfalse},
	{&g_allowVote					,"g_allowVote"					,"1"			,CVAR_ARCHIVE									,0,qfalse,qfalse},
	{&g_listEntity					,"g_listEntity"					,"0"			,0												,0,qfalse,qfalse},
	{&g_smoothClients				,"g_smoothClients"				,"1"			,0												,0,qfalse,qfalse},
	{&pmove_fixed					,"pmove_fixed"					,"0"			,CVAR_SYSTEMINFO								,0,qfalse,qfalse},
	{&pmove_msec					,"pmove_msec"					,"8"			,CVAR_SYSTEMINFO								,0,qfalse,qfalse},
	{&g_rankings					,"g_rankings"					,"0"			,0												,0,qfalse,qfalse},
	// ADDING FOR ZEQ2
	{&g_verboseParse				,"g_verboseParse"				,"0"			,CVAR_ARCHIVE									,0,qfalse,qfalse},
	{&g_powerlevel					,"g_powerlevel"					,"1000"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_powerlevelMaximum			,"g_powerlevelMaximum"			,"32767"		,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_rolling						,"g_rolling"					,"1"			,CVAR_ARCHIVE									,0,qtrue,qfalse},
	{&g_running						,"g_running"					,"0"			,CVAR_ARCHIVE									,0,qtrue,qfalse},
	{&g_pointGravity				,"g_pointGravity"				,"0"			,CVAR_ARCHIVE									,0,qtrue,qfalse},
	{&g_allowTiers					,"g_allowTiers"					,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowScoreboard				,"g_allowScoreboard"			,"0"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowSoar					,"g_allowSoar"					,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowBoost					,"g_allowBoost"					,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowFly					,"g_allowFly"					,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowZanzoken				,"g_allowZanzoken"				,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowJump					,"g_allowJump"					,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowBallFlip				,"g_allowBallFlip"				,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowOverheal				,"g_allowOverheal"				,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowBreakLimit				,"g_allowBreakLimit"			,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowMelee					,"g_allowMelee"					,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowLockon					,"g_allowLockon"				,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowBlock					,"g_allowBlock"					,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_allowAdvancedMelee			,"g_allowAllowAdvancedMelee"	,"1"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_breakLimitRate				,"g_breakLimitRate"				,"1.0"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_quickTransformCost			,"g_quickTransformCost"			,"0.32"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_quickTransformCostPerTier	,"g_quickTransformCostPerTier"	,"0.08"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_quickZanzokenCost			,"g_quickZanzokenCost"			,"-1.0"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	{&g_quickZanzokenDistance		,"g_quickZanzokenDistance"		,"-1.0"			,CVAR_ARCHIVE|CVAR_SERVERINFO					,0,qtrue,qfalse},
	// END ADDING
};					
static int gameCvarTableSize = ARRAY_LEN(gameCvarTable);
void G_InitGame(int levelTime,int randomSeed,int restart);
void G_RunFrame(int levelTime);
void G_ShutdownGame(int restart);
/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain(int command,int arg0,int arg1,int arg2,int arg3,int arg4,int arg5,int arg6,int arg7,int arg8,int arg9,int arg10,int arg11){
	switch(command){
		case GAME_INIT:
			G_InitGame(arg0,arg1,arg2);
			return 0;
		case GAME_SHUTDOWN:
			G_ShutdownGame(arg0);
			return 0;
		case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect(arg0,arg1);
		case GAME_CLIENT_THINK:
			ClientThink(arg0);
			return 0;
		case GAME_CLIENT_USERINFO_CHANGED:
			ClientUserinfoChanged(arg0);
			return 0;
		case GAME_CLIENT_DISCONNECT:
			ClientDisconnect(arg0);
			return 0;
		case GAME_CLIENT_BEGIN:
			ClientBegin(arg0);
			return 0;
		case GAME_CLIENT_COMMAND:
			ClientCommand(arg0);
			return 0;
		case GAME_RUN_FRAME:
			G_RunFrame(arg0);
			return 0;
		case GAME_CONSOLE_COMMAND:
			return ConsoleCommand();
	}
	return -1;
}
void QDECL G_Printf(const char *fmt,...){
	va_list argptr;
	char text[1024];
	va_start(argptr,fmt);
	Q_vsnprintf(text,sizeof(text),fmt,argptr);
	va_end(argptr);
	trap_Print(text);
}
void QDECL G_Error(const char *fmt,...){
	va_list argptr;
	char text[1024];
	va_start(argptr,fmt);
	Q_vsnprintf(text,sizeof(text),fmt,argptr);
	va_end(argptr);
	trap_Error(text);
}
/*
================
G_FindTeams
Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams(void){
	gentity_t* e;
	gentity_t* e2;
	int i;
	int j;
	int c = 0;
	int c2 = 0;
	for(i=1,e=g_entities+i;i<level.num_entities;i++,e++){
		if(!e->inuse || !e->team){continue;}
		c++;
		c2++;
		for(j=i+1,e2=e+1;j<level.num_entities;j++,e2++){
			if(!e2->inuse || !e2->team){continue;}
			if(!strcmp(e->team,e2->team)){
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				// make sure that targets only point at the master
				if(e2->targetname){
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}
	G_Printf("%i teams with %i entities\n",c,c2);
}
void G_RegisterCvars(void){
	cvarTable_t* cv;
	int i;
	for(i=0,cv=gameCvarTable;i<gameCvarTableSize;i++,cv++){
		trap_Cvar_Register(cv->vmCvar,cv->cvarName,cv->defaultString,cv->cvarFlags);
		if(cv->vmCvar){cv->modificationCount = cv->vmCvar->modificationCount;}
	}
	if(g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE){
		G_Printf("g_gametype %i is out of range, defaulting to 0\n",g_gametype.integer);
		trap_Cvar_Set("g_gametype","0");
	}
	level.warmupModificationCount = g_warmup.modificationCount;
}
void G_UpdateCvars(void){
	cvarTable_t	*cv;
	int i;
	for(i=0,cv=gameCvarTable;i<gameCvarTableSize;i++,cv++){
		if(cv->vmCvar){
			trap_Cvar_Update(cv->vmCvar);
			if(cv->modificationCount != cv->vmCvar->modificationCount){
				cv->modificationCount = cv->vmCvar->modificationCount;
				if(cv->trackChange){
					//trap_SendServerCommand( -1, va("print \"Server: %s changed to %s\n\"",cv->cvarName, cv->vmCvar->string ) );
				}
			}
		}
	}
}
void G_InitGame(int levelTime,int randomSeed,int restart){
	int i;
	G_Printf("------- Game Initialization -------\n");
	G_Printf("Directory: %s\n",BASEDIR);
	G_Printf("Release: %s\n",__DATE__);
	srand(randomSeed);
	G_RegisterCvars();
	G_ProcessIPBans();
	G_InitMemory();
	// set some level globals
	memset(&level,0,sizeof(level));
	level.time = levelTime;
	level.startTime = levelTime;
	if(g_gametype.integer != GT_SINGLE_PLAYER && g_logfile.string[0]){
		int fileSystemMode = g_logfileSync.integer ? FS_APPEND_SYNC : FS_APPEND;
		trap_FS_FOpenFile(g_logfile.string,&level.logFile,fileSystemMode);
		if(!level.logFile){
			G_Printf("WARNING: Couldn't open logfile: %s\n",g_logfile.string);
		}
		else {
			char serverinfo[MAX_INFO_STRING];
			trap_GetServerinfo(serverinfo,sizeof(serverinfo));
			G_LogPrintf("------------------------------------------------------------\n");
			G_LogPrintf("InitGame: %s\n",serverinfo);
		}
	}
	else{
		G_Printf("Not logging to disk.\n");
	}
	G_InitWorldSession();
	// initialize all entities for this game
	memset(g_entities,0,MAX_GENTITIES * sizeof(g_entities[0]));
	level.gentities = g_entities;
	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset(g_clients,0,MAX_CLIENTS * sizeof(g_clients[0]));
	level.clients = g_clients;
	// set client fields on player ents
	for(i=0;i<level.maxclients;i++){
		g_entities[i].client = level.clients + i;
	}
	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;
	for(i=0;i<MAX_CLIENTS;i++){
		g_entities[i].classname = "clientslot";
	}
	// let the server system know where the entites are
	trap_LocateGameData(level.gentities,level.num_entities,sizeof(gentity_t),&level.clients[0].ps,sizeof(level.clients[0]));
	// reserve some spots for dead player bodies
	InitBodyQue();
	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();
	// general initialization
	G_FindTeams();
	// make sure we have flags for CTF, etc
	// ADDING FOR ZEQ2
	// Parse the weapon lists.
	// FIXME: As soon as these become variable, move them
	// to the client spawning functions!
	if(g_gametype.integer == GT_SINGLE_PLAYER || trap_Cvar_VariableIntegerValue("com_buildScript")){
		G_ModelIndex(SP_PODIUM_MODEL);
	}
}
/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame(int restart){
	G_Printf("==== ShutdownGame ====\n");
	if(level.logFile){
		G_LogPrintf("ShutdownGame:\n");
		G_LogPrintf("------------------------------------------------------------\n");
		trap_FS_FCloseFile(level.logFile);
		level.logFile = 0;
	}
	// write all the client session data so we can get it back
	G_WriteSessionData();
}
void QDECL Com_Error(int level,const char *error,...){
	va_list argptr;
	char text[1024];
	va_start(argptr,error);
	Q_vsnprintf(text,sizeof(text),error,argptr);
	va_end(argptr);
	G_Error("%s",text);
}
void QDECL Com_Printf(const char *msg,...){
	va_list argptr;
	char text[1024];
	va_start(argptr,msg);
	Q_vsnprintf(text,sizeof(text),msg,argptr);
	va_end(argptr);
	G_Printf("%s",text);
}
/*
========================================================================

MAP CHANGING

========================================================================
*/
/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar 

=============
*/
void ExitLevel(void){
	gclient_t *cl;
	char nextmap[MAX_STRING_CHARS];
	char d1[MAX_STRING_CHARS];
	trap_Cvar_VariableStringBuffer("nextmap",nextmap,sizeof(nextmap));
	trap_Cvar_VariableStringBuffer("d1",d1,sizeof(d1));
	if(!Q_stricmp(nextmap,"map_restart 0") && Q_stricmp(d1,"")){
		trap_Cvar_Set("nextmap","vstr d2");
		trap_SendConsoleCommand(EXEC_APPEND,"vstr d1\n");
	}
	else{
		trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");
	}
	level.changemap = NULL;
	level.intermissiontime = 0;
	for(int i=0;i<g_maxclients.integer;i++){
		cl = level.clients + i;
		if(cl->pers.connected != CON_CONNECTED){continue;}
		cl->ps.persistant[PERS_SCORE] = 0;
	}
	// we need to do this here before changing to CON_CONNECTING
	G_WriteSessionData();
	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for(int i=0;i<g_maxclients.integer;i++){
		if(level.clients[i].pers.connected == CON_CONNECTED){
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}
}
/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf(const char *fmt,...){
	va_list argptr;
	char string[1024];
	int min;
	int tens;
	int sec;
	sec = (level.time - level.startTime) / 1000;
	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;
	Com_sprintf(string,sizeof(string),"%3i:%i%i ",min,tens,sec);
	va_start(argptr,fmt);
	Q_vsnprintf(string + 7,sizeof(string) - 7,fmt,argptr);
	va_end(argptr);
	if(g_dedicated.integer){G_Printf("%s",string + 7);}
	if(level.logFile){trap_FS_Write(string,strlen(string),level.logFile);}
}
/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit(const char *string){
	int numSorted;
	gclient_t* cl;
	G_LogPrintf("Exit: %s\n",string);
	level.intermissionQueued = level.time;
	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring(CS_INTERMISSION,"1");
	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if(numSorted > 32){numSorted = 32;}
	for(int i=0;i<numSorted;i++){
		int ping;
		cl = &level.clients[level.sortedClients[i]];
		if(cl->sess.sessionTeam == TEAM_SPECTATOR || cl->pers.connected == CON_CONNECTING){continue;}
		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		G_LogPrintf("score: %i  ping: %i  client: %i %s\n",cl->ps.persistant[PERS_SCORE],ping,level.sortedClients[i],cl->pers.netname);
	}
}
/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit(void){ExitLevel();}
/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/
void CheckVote(void){
	if(level.voteExecuteTime && level.voteExecuteTime < level.time){
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand(EXEC_APPEND,va("%s\n",level.voteString));
	}
	if(!level.voteTime){return;}
	if(level.time - level.voteTime >= VOTE_TIME){
		trap_SendServerCommand(-1,"print \"Vote failed.\n\"");
	}
	else{
		if(level.voteYes > level.numVotingClients / 2){
			// execute the command, then remove the vote
			trap_SendServerCommand(-1,"print \"Vote passed.\n\"");
			level.voteExecuteTime = level.time + 3000;
		}
		else if(level.voteNo >= level.numVotingClients / 2){
			// same behavior as a timeout
			trap_SendServerCommand(-1,"print \"Vote failed.\n\"");
		}
		else{
			// still waiting for a majority
			return;
		}
	}
	level.voteTime = 0;
	trap_SetConfigstring(CS_VOTE_TIME,"");
}
void CheckCvars(void){
	static int lastMod = -1;
	if(g_password.modificationCount != lastMod){
		char value[MAX_STRING_CHARS];
		*g_password.string && Q_stricmp(g_password.string,"none") ? Q_strncpyz(value,"1",MAX_STRING_CHARS) : Q_strncpyz(value,"0",MAX_STRING_CHARS);
		trap_Cvar_Set("g_needpass",value);
		lastMod = g_password.modificationCount;
	}
}
/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink(gentity_t *ent){
	float thinkTime = ent->nextthink;
	if(thinkTime <= 0 || thinkTime > level.time){return;}
	ent->nextthink = 0;
	if(!ent->think){G_Error("NULL ent->think");}
	ent->think(ent);
}
/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame(int levelTime){
	gentity_t* ent;
	// if we are waiting for the level to restart, do nothing
	if(level.restarted){return;}
	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;
	G_UpdateCvars();
	//
	// go through all allocated objects
	//
	ent = &g_entities[0];
	for(int i=0;i<level.num_entities;i++,ent++){
		if(!ent->inuse){continue;}
		// clear events that are too old
		if(level.time - ent->eventTime > EVENT_VALID_MSEC){
			if(ent->s.event){
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if(ent->client){
					ent->client->ps.externalEvent = 0;
				}
			}
			if(ent->freeAfterEvent){
				G_FreeEntity(ent);
				continue;
			}
			else if(ent->unlinkAfterEvent){
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity(ent);
			}
		}
		if(ent->freeAfterEvent){continue;}
		if(!ent->r.linked && ent->neverFree){continue;}
		if(ent->s.eType == ET_MISSILE){
			G_RunUserMissile(ent);
			continue;
		}
		if(ent->s.eType == ET_EXPLOSION){
			G_RunUserExplosion(ent);
			continue;
		}
		if(ent->s.eType == ET_BEAMHEAD){G_RunUserMissile(ent);}
		if(ent->s.eType == ET_MOVER){
			//G_RunMover(ent);
			continue;
		}
		if(i < MAX_CLIENTS){
			G_RunClient(ent);
			continue;
		}
		G_RunThink(ent);
	}
	// perform final fixups on the players
	ent = &g_entities[0];
	for(int i=0;i<level.maxclients;i++,ent++){
		if(ent->inuse){ClientEndFrame(ent);}
	}
	G_RadarUpdateCS();
	// cancel vote if timed out
	CheckVote();
	// for tracking changes
	CheckCvars();
	if(g_listEntity.integer){
		for(int i=0;i<MAX_GENTITIES;i++){
			G_Printf("%4i: %s\n",i,g_entities[i].classname);
		}
		trap_Cvar_Set("g_listEntity","0");
	}
}
