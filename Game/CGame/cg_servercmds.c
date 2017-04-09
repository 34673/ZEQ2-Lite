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
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame
#include "cg_local.h"
#include "../UI/menudef/menudef.h"
static void CG_ParseTeamInfo(void){
	int		i=0;
	int		client;
	numSortedTeamPlayers = atoi(CG_Argv(1));
	if(numSortedTeamPlayers < 0 || numSortedTeamPlayers > TEAM_MAXOVERLAY){
		CG_Error("CG_ParseTeamInfo: numSortedTeamPlayers out of range (%d)", numSortedTeamPlayers);
		return;
	}
	for(;i<numSortedTeamPlayers;i++){
		client = atoi(CG_Argv(i*6+2));
		if(client < 0 || client >= MAX_CLIENTS){
		  CG_Error("CG_ParseTeamInfo: bad client number: %d", client);
		  return;
		}
		sortedTeamPlayers[i] = client;
		cgs.clientinfo[client].location = atoi(CG_Argv(i*6+3));
		cgs.clientinfo[client].armor = atoi(CG_Argv(i*6+5));
		cgs.clientinfo[client].curWeapon = atoi(CG_Argv(i*6+6));
		cgs.clientinfo[client].powerups = atoi(CG_Argv(i*6+7));
	}
}
///This is called explicitly when the gamestate is first received,
//and whenever the server updates any serverinfo flagged cvars
void CG_ParseServerinfo(void){
	const char	*info;
	char		*mapname;
	info = CG_ConfigString(CS_SERVERINFO);
	cgs.gametype = atoi(Info_ValueForKey(info, "g_gametype"));
	trap_Cvar_Set("g_gametype", va("%i", cgs.gametype));
	cgs.dmflags = atoi( Info_ValueForKey( info, "dmflags" ) );
	cgs.teamflags = atoi( Info_ValueForKey( info, "teamflags" ) );
	cgs.fraglimit = atoi( Info_ValueForKey( info, "fraglimit" ) );
	cgs.capturelimit = atoi( Info_ValueForKey( info, "capturelimit" ) );
	cgs.timelimit = atoi( Info_ValueForKey( info, "timelimit" ) );
	cgs.maxclients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cgs.mapname, sizeof( cgs.mapname ), "maps/%s.bsp", mapname );
	Q_strncpyz( cgs.redTeam, Info_ValueForKey( info, "g_redTeam" ), sizeof(cgs.redTeam) );
	trap_Cvar_Set("g_redTeam", cgs.redTeam);
	Q_strncpyz( cgs.blueTeam, Info_ValueForKey( info, "g_blueTeam" ), sizeof(cgs.blueTeam) );
	trap_Cvar_Set("g_blueTeam", cgs.blueTeam);
#if MAPLENSFLARES
	cgs.editMode = atoi(Info_ValueForKey(info, "g_editmode"));
	if(cgs.maxclients > 1){cgs.editMode = EM_none;}
	if(atoi(Info_ValueForKey(CG_ConfigString(CS_SYSTEMINFO), "sv_cheats")) != 1){cgs.editMode = EM_none;}
#endif
}
static void CG_ParseWarmup(void){
	const char	*info;
	int			warmup;
	info = CG_ConfigString(CS_WARMUP);
	warmup = atoi(info);
	cg.warmupCount = -1;
	cg.warmup = warmup;
}
//Called on load to set the initial values from configure strings
void CG_SetConfigValues(void){
	const char *s;
	cgs.levelStartTime = atoi(CG_ConfigString(CS_LEVEL_START_TIME));
	cg.warmup = atoi(CG_ConfigString(CS_WARMUP));
}
void CG_ShaderStateChanged(void){
	char originalShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	char timeOffset[16];
	const char *o;
	char *n,*t;
	o = CG_ConfigString(CS_SHADERSTATE);
	while(o && *o){
		n = strstr(o, "=");
		if(n && *n){
			strncpy(originalShader, o, n-o);
			originalShader[n-o] = 0;
			n++;
			t = strstr(n, ":");
			if(t && *t){
				strncpy(newShader, n, t-n);
				newShader[t-n] = 0;
			}
			else{break;}
			t++;
			o = strstr(t, "@");
			if(o){
				strncpy(timeOffset, t, o-t);
				timeOffset[o-t] = 0;
				o++;
				trap_R_RemapShader(originalShader, newShader, timeOffset);
			}
		}
		else{break;}
	}
}
static void CG_ConfigStringModified(void){
	const char	*str;
	int			num;
	num = atoi(CG_Argv(1));
	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState(&cgs.gameState);
	// look up the individual string that was modified
	str = CG_ConfigString(num);
	// do something with it if necessary
	if(num == CS_SERVERINFO){CG_ParseServerinfo();}
	else if(num == CS_WARMUP){CG_ParseWarmup();}
	else if(num == CS_LEVEL_START_TIME){cgs.levelStartTime = atoi(str);}
	else if(num == CS_VOTE_TIME){
		cgs.voteTime = atoi(str);
		cgs.voteModified = qtrue;
	}
	else if(num == CS_VOTE_YES){
		cgs.voteYes = atoi(str);
		cgs.voteModified = qtrue;
	}
	else if(num == CS_VOTE_NO){
		cgs.voteNo = atoi(str);
		cgs.voteModified = qtrue;
	}
	else if(num == CS_VOTE_STRING){
		Q_strncpyz(cgs.voteString, str, sizeof(cgs.voteString));
	}
	else if(num >= CS_TEAMVOTE_TIME && num <= CS_TEAMVOTE_TIME + 1){
		cgs.teamVoteTime[num-CS_TEAMVOTE_TIME] = atoi(str);
		cgs.teamVoteModified[num-CS_TEAMVOTE_TIME] = qtrue;
	}
	else if(num >= CS_TEAMVOTE_YES && num <= CS_TEAMVOTE_YES + 1){
		cgs.teamVoteYes[num-CS_TEAMVOTE_YES] = atoi(str);
		cgs.teamVoteModified[num-CS_TEAMVOTE_YES] = qtrue;
	}
	else if(num >= CS_TEAMVOTE_NO && num <= CS_TEAMVOTE_NO + 1){
		cgs.teamVoteNo[num-CS_TEAMVOTE_NO] = atoi(str);
		cgs.teamVoteModified[num-CS_TEAMVOTE_NO] = qtrue;
	}
	else if(num >= CS_TEAMVOTE_STRING && num <= CS_TEAMVOTE_STRING + 1){
		Q_strncpyz(cgs.teamVoteString[num-CS_TEAMVOTE_STRING], str, sizeof(cgs.teamVoteString));
	}
	else if(num == CS_INTERMISSION){cg.intermissionStarted = atoi(str);}
	else if(num >= CS_MODELS && num < CS_MODELS+MAX_MODELS){
		cgs.gameModels[ num-CS_MODELS ] = trap_R_RegisterModel(str);
	}
	else if(num >= CS_SOUNDS && num < CS_SOUNDS+MAX_SOUNDS){
		// player specific sounds don't register here
		if(str[0] != '*'){cgs.gameSounds[num-CS_SOUNDS] = trap_S_RegisterSound(str, qfalse);}
	}
	else if(num >= CS_PLAYERS && num < CS_PLAYERS+MAX_CLIENTS){CG_NewClientInfo(num - CS_PLAYERS);}
	else if(num == CS_SHADERSTATE){CG_ShaderStateChanged();}
}
static void CG_AddToTeamChat(const char *str){
	int		len=0;
	char	*p, *ls;
	int		lastcolor;
	int		chatHeight;
	if(cg_teamChatHeight.integer < TEAMCHAT_HEIGHT){
		chatHeight = cg_teamChatHeight.integer;
	}
	else{chatHeight = TEAMCHAT_HEIGHT;}
	if(chatHeight <= 0 || cg_teamChatTime.integer <= 0){
		// team chat disabled, dump into normal chat
		cgs.teamChatPos = cgs.teamLastChatPos = 0;
		return;
	}
	p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
	*p = 0;
	lastcolor = '7';
	ls = NULL;
	while(*str){
		if(len > TEAMCHAT_WIDTH - 1){
			if(ls){
				str -= (p - ls);
				str++;
				p -= (p - ls);
			}
			*p = 0;
			cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
			cgs.teamChatPos++;
			p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
			*p = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len = 0;
			ls = NULL;
		}
		if(Q_IsColorString(str)){
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if(*str == ' '){ls = p;}
		*p++ = *str++;
		len++;
	}
	*p = 0;
	cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
	cgs.teamChatPos++;
	if(cgs.teamChatPos - cgs.teamLastChatPos > chatHeight)
		cgs.teamLastChatPos = cgs.teamChatPos - chatHeight;
}
//The server has issued a map_restart, so the next snapshot
//is completely new and should not be interpolated to.
//A tournement restart will clear everything, but doesn't
//require a reload of all the media
static void CG_MapRestart(void){
	if(cg_showmiss.integer){CG_Printf("CG_MapRestart\n");}
	CG_InitLocalEntities();
	CG_InitMarkPolys();
	// ADDING FOR ZEQ2
	CG_FrameHist_Init();
	CG_InitTrails();
	CG_InitParticleSystems();
	CG_InitBeamTables();
	CG_InitRadarBlips();
	cg.intermissionStarted = qfalse;
	cg.levelShot = qfalse;
	cgs.voteTime = 0;
	cg.mapRestart = qtrue;
	trap_S_ClearLoopingSounds(qtrue);
	// we really should clear more parts of cg here and stop sounds
	trap_Cvar_Set("cg_thirdPerson", "1"); // ZEQ2-Lite Stays in third person
}
#define MAX_VOICEFILESIZE	16384
#define MAX_VOICEFILES		8
#define MAX_VOICECHATS		64
#define MAX_VOICESOUNDS		64
#define MAX_CHATSIZE		64
#define MAX_HEADMODELS		64
typedef struct voiceChat_s{
	char		id[64];
	int			numSounds;
	sfxHandle_t	sounds[MAX_VOICESOUNDS];
	char		chats[MAX_VOICESOUNDS][MAX_CHATSIZE];
}voiceChat_t;
typedef struct voiceChatList_s{
	char		name[64];
	int			numVoiceChats;
	voiceChat_t	voiceChats[MAX_VOICECHATS];
}voiceChatList_t;
typedef struct headModelVoiceChat_s{
	char	headmodel[64];
	int		voiceChatNum;
}headModelVoiceChat_t;
voiceChatList_t			voiceChatLists[MAX_VOICEFILES];
headModelVoiceChat_t	headModelVoiceChat[MAX_HEADMODELS];
int CG_ParseVoiceChats(const char *filename, voiceChatList_t *voiceChatList, int maxVoiceChats){
	fileHandle_t 	f;
	int				i=0;
	int				len = trap_FS_FOpenFile(filename, &f, FS_READ);
	char 			buf[MAX_VOICEFILESIZE];
	char 			**p, *ptr;
	char 			*token;
	voiceChat_t 	*voiceChats;
	qboolean 		compress = qtrue;
	sfxHandle_t 	sound;
	if(cg_buildScript.integer){compress = qfalse;}
	if(!f){
		trap_Print(va(S_COLOR_RED "voice chat file not found: %s\n", filename));
		return qfalse;
	}
	if(len >= MAX_VOICEFILESIZE){
		trap_Print(va(S_COLOR_RED "voice chat file too large: %s is %i, max allowed is %i\n", filename, len, MAX_VOICEFILESIZE));
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);
	ptr = buf;
	p = &ptr;
	Com_sprintf(voiceChatList->name, sizeof(voiceChatList->name), "%s", filename);
	voiceChats = voiceChatList->voiceChats;
	for(;i<maxVoiceChats;i++){voiceChats[i].id[0] = 0;}
	token = COM_ParseExt(p, qtrue);
	if(!token || token[0] == 0){return qtrue;}
	voiceChatList->numVoiceChats = 0;
	while(1){
		token = COM_ParseExt(p, qtrue);
		if(!token || token[0] == 0){return qtrue;}
		Com_sprintf(voiceChats[voiceChatList->numVoiceChats].id, sizeof(voiceChats[voiceChatList->numVoiceChats].id), "%s", token);
		token = COM_ParseExt(p, qtrue);
		if(Q_stricmp(token, "{")){
			trap_Print(va(S_COLOR_RED "expected { found %s in voice chat file: %s\n", token, filename));
			return qfalse;
		}
		voiceChats[voiceChatList->numVoiceChats].numSounds = 0;
		while(1){
			token = COM_ParseExt(p, qtrue);
			if(!token || token[0] == 0){return qtrue;}
			if(!Q_stricmp(token, "}")){break;}
			sound = trap_S_RegisterSound(token, compress);
			voiceChats[voiceChatList->numVoiceChats].sounds[voiceChats[voiceChatList->numVoiceChats].numSounds] = sound;
			token = COM_ParseExt(p, qtrue);
			if(!token || token[0] == 0){return qtrue;}
			Com_sprintf(voiceChats[voiceChatList->numVoiceChats].chats[voiceChats[voiceChatList->numVoiceChats].numSounds], MAX_CHATSIZE, "%s", token);
			if(sound){voiceChats[voiceChatList->numVoiceChats].numSounds++;}
			if(voiceChats[voiceChatList->numVoiceChats].numSounds >= MAX_VOICESOUNDS){break;}
		}
		voiceChatList->numVoiceChats++;
		if(voiceChatList->numVoiceChats >= maxVoiceChats){return qtrue;}
	}
	return qtrue;
}
static void CG_RemoveChatEscapeChar(char *text){
	int i=0;
	int l=0;
	for(;text[i];i++){
		if(text[i] == '\x19'){continue;}
		text[l++] = text[i];
	}
	text[l] = '\0';
}
//The string has been tokenized and can be retrieved with
//Cmd_Argc() / Cmd_Argv()
static void CG_ServerCommand(void){
	const char	*cmd;
	char		text[MAX_SAY_TEXT];
	cmd = CG_Argv(0);
	// server claimed the command
	if(!cmd[0]){return;}
	if(!strcmp(cmd, "cp")){
		CG_CenterPrint(CG_Argv(1), SCREEN_HEIGHT * .3f, BIGCHAR_WIDTH);
		return;
	}
	if(!strcmp(cmd, "cs")){
		CG_ConfigStringModified();
		return;
	}
	if(!strcmp(cmd, "print")){
		CG_Printf("%s", CG_Argv(1));
		return;
	}
	if(!strcmp(cmd, "chat")){
		if(!cg_teamChatsOnly.integer){
			trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
			Q_strncpyz(text, CG_Argv(1), MAX_SAY_TEXT);
			CG_RemoveChatEscapeChar(text);
			CG_Printf("%s\n", text);
			CG_DrawChat(text);
		}
		return;
	}
	if(!strcmp(cmd, "tchat")){
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
		Q_strncpyz(text, CG_Argv(1), MAX_SAY_TEXT);
		CG_RemoveChatEscapeChar(text);
		CG_AddToTeamChat(text);
		CG_Printf("%s\n", text);
		return;
	}
	if(!strcmp(cmd, "tinfo")){
		CG_ParseTeamInfo();
		return;
	}
	if(!strcmp(cmd, "map_restart")){
		CG_MapRestart();
		return;
	}
	if(!Q_stricmp(cmd, "remapShader")){
		if(trap_Argc() == 4){
			char shader1[MAX_QPATH];
			char shader2[MAX_QPATH];
			char shader3[MAX_QPATH];
			Q_strncpyz(shader1, CG_Argv(1), sizeof(shader1));
			Q_strncpyz(shader2, CG_Argv(2), sizeof(shader2));
			Q_strncpyz(shader3, CG_Argv(3), sizeof(shader3));
			trap_R_RemapShader(shader1, shader2, shader3);
		}
		return;
	}
	// clientLevelShot is sent before taking a special screenshot for
	// the menu system during development
	if(!strcmp(cmd, "clientLevelShot")){
		cg.levelShot = qtrue;
		return;
	}
	if(!strcmp(cmd, "radar")){
		int			count;
		int 		i=0;
		const char	*ptr;
		// expand the string into our settings
		// clear out old list
		memset(cg_playerOrigins, 0, sizeof(cg_playerOrigins));
		// get amount in current list
		count = atof(CG_Argv(1));
		for(;i<count;i++){
			// set the string pointer to the correct set of parameters
			ptr = CG_Argv(i+2);
			//read in the clientNum
			cg_playerOrigins[i].clientNum = atof(ptr);
			//move the ptr on until we come to a comma
			ptr = strchr(ptr, ',');
			//skip over the comma
			ptr++;
			cg_playerOrigins[i].pl = atof(ptr);
			ptr = strchr(ptr, ',');
			ptr++;
			cg_playerOrigins[i].plMax = atof(ptr);
			ptr = strchr(ptr, ',');
			ptr++;
			cg_playerOrigins[i].team = atof(ptr);
			ptr = strchr(ptr, ',');
			ptr++;
			cg_playerOrigins[i].properties = atof(ptr);
			ptr = strchr(ptr, ',');
			ptr++;
			//read in the first position number
			cg_playerOrigins[i].pos[0] = atof(ptr);
			ptr = strchr(ptr, ',');
			ptr++;
			//read in the next position number
			cg_playerOrigins[i].pos[1] = atof(ptr);
			ptr = strchr(ptr, ',');
			ptr++;
			//read in the final position number
			cg_playerOrigins[i].pos[2] = atof(ptr);
			// Do a probability check on whether we're going to display the dot
			// or not.
			//mark the entry as valid
			cg_playerOrigins[i].valid = qtrue;
		}
		return;
	}
	CG_Printf("Unknown client game command: %s\n", cmd);
}
//Execute all of the server commands that were received along with this this snapshot.
void CG_ExecuteNewServerCommands(int latestSequence){
	while(cgs.serverCommandSequence < latestSequence){
		if(trap_GetServerCommand(++cgs.serverCommandSequence)){CG_ServerCommand();}
	}
}
