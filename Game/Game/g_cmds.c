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
#include "../UI/menudef/menudef.h"		// for the voice chats
qboolean CheatsOk(gentity_t* ent){
	if(!g_cheats.integer){
		trap_SendServerCommand(ent-g_entities,va("print \"Cheats are not enabled on this server.\n\""));
		return qfalse;
	}
	return qtrue;
}
char* ConcatArgs(int start){
	int i;
	int c = trap_Argc();
	int tlen;
	static char	line[MAX_STRING_CHARS];
	int len = 0;
	char arg[MAX_STRING_CHARS];
	for(i=start;i<c;i++){
		trap_Argv(i,arg,sizeof(arg));
		tlen = strlen(arg);
		if(len + tlen >= MAX_STRING_CHARS-1){break;}
		memcpy(line + len,arg,tlen);
		len += tlen;
		if(i != c-1){
			line[len] = ' ';
			len++;
		}
	}
	line[len] = 0;
	return line;
}
/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString(gentity_t* to,char* s){
	gclient_t* cl;
	int idnum;
	char cleanName[MAX_STRING_CHARS];
	// numeric values are just slot numbers
	if(s[0] >= '0' && s[0] <= '9'){
		idnum = atoi(s);
		if(idnum < 0 || idnum >= level.maxclients){
			trap_SendServerCommand(to-g_entities,va("print \"Bad client slot: %i\n\"",idnum));
			return -1;
		}
		cl = &level.clients[idnum];
		if(cl->pers.connected != CON_CONNECTED){
			trap_SendServerCommand(to-g_entities,va("print \"Client %i is not active\n\"",idnum));
			return -1;
		}
		return idnum;
	}
	// check for a name match
	for(idnum=0,cl=level.clients;idnum<level.maxclients;idnum++,cl++){
		if(cl->pers.connected != CON_CONNECTED){continue;}
		Q_strncpyz(cleanName,cl->pers.netname,sizeof(cleanName));
		Q_CleanStr(cleanName);
		if(!Q_stricmp(cleanName,s)){return idnum;}
	}
	trap_SendServerCommand(to-g_entities,va("print \"User %s is not on the server\n\"",s));
	return -1;
}
/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f(gentity_t* ent){
	if(!ent->client->pers.localClient){
		trap_SendServerCommand(ent-g_entities,"print \"The levelshot command must be executed by a local client\n\"");
		return;
	}
	if(!CheatsOk(ent)){return;}
	if(g_gametype.integer == GT_SINGLE_PLAYER){
		trap_SendServerCommand(ent-g_entities,"print \"Must not be in singleplayer mode for levelshot\n\"");
		return;
	}
	trap_SendServerCommand(ent-g_entities,"clientLevelShot");
}
void BroadcastTeamChange(gclient_t* client,int oldTeam){
	if(client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR){
		trap_SendServerCommand(-1,va("cp \"%s"S_COLOR_WHITE" joined the spectators.\n\"",client->pers.netname));
	}
	else if(client->sess.sessionTeam == TEAM_FREE){
		trap_SendServerCommand(-1,va("cp \"%s"S_COLOR_WHITE" joined the battle.\n\"",client->pers.netname));
	}
}
void SetTeam(gentity_t* ent,char* s){
	int team;
	int oldTeam;
	gclient_t* client = ent->client;
	int clientNum = client - level.clients;
	spectatorState_t specState = SPECTATOR_NOT;
	int specClient = 0;
	if(!Q_stricmp(s,"follow1")){
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	}
	else if(!Q_stricmp(s,"follow2")){
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	}
	else if(!Q_stricmp(s,"spectator") || !Q_stricmp(s,"s")){
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	}
	else{
		team = TEAM_FREE;
	}
	// override decision if limiting the players
	if(g_maxGameClients.integer > 0 && level.numNonSpectatorClients >= g_maxGameClients.integer){
		team = TEAM_SPECTATOR;
	}
	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if(team == oldTeam && team != TEAM_SPECTATOR){return;}
	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if(oldTeam != TEAM_SPECTATOR){
		// Kill him (makes sure he loses flags, etc)
		ent->client->ps.powerLevel[plCurrent] = ent->powerLevelTotal = 0;
	}
	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;
	BroadcastTeamChange(client, oldTeam);
	// get and distribute relevant parameters
	ClientUserinfoChanged(clientNum);
	ClientBegin(clientNum);
}
/*
=================
StopFollowing
If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing(gentity_t* ent){
	ent->client->ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;	
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->client->ps.clientNum = ent - g_entities;
}
void Cmd_Team_f(gentity_t* ent){
	int oldTeam;
	char s[MAX_TOKEN_CHARS];
	if(trap_Argc() != 2){
		oldTeam = ent->client->sess.sessionTeam;
		switch(oldTeam){
			case TEAM_FREE:
				trap_SendServerCommand(ent-g_entities,"print \"Free team\n\"");
				break;
			case TEAM_SPECTATOR:
				trap_SendServerCommand(ent-g_entities,"print \"Spectator team\n\"");
				break;
		}
		return;
	}
	if(ent->client->switchTeamTime > level.time){
		trap_SendServerCommand(ent-g_entities,"print \"May not switch teams more than once per 5 seconds.\n\"");
		return;
	}
	trap_Argv(1,s,sizeof(s));
	SetTeam(ent,s);
	ent->client->switchTeamTime = level.time + 5000;
}
void Cmd_Follow_f(gentity_t* ent){
	int i;
	char arg[MAX_TOKEN_CHARS];
	if(trap_Argc() != 2){
		if(ent->client->sess.spectatorState == SPECTATOR_FOLLOW){
			StopFollowing(ent);
		}
		return;
	}
	trap_Argv(1,arg,sizeof(arg));
	i = ClientNumberFromString(ent,arg);
	if(i == -1){return;}
	if(&level.clients[i] == ent->client){return;}
	if(level.clients[i].sess.sessionTeam == TEAM_SPECTATOR){return;}
	if(ent->client->sess.sessionTeam != TEAM_SPECTATOR){
		SetTeam(ent,"spectator");
	}
	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}
void Cmd_FollowCycle_f(gentity_t* ent,int dir){
	int clientnum;
	int original;
	if(ent->client->sess.spectatorState == SPECTATOR_NOT){
		SetTeam(ent,"spectator");
	}
	if(dir != 1 && dir != -1){
		G_Error("Cmd_FollowCycle_f: bad dir %i",dir);
	}
	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	do{
		clientnum += dir;
		if(clientnum >= level.maxclients){
			clientnum = 0;
		}
		if(clientnum < 0){
			clientnum = level.maxclients - 1;
		}
		if(level.clients[clientnum].pers.connected != CON_CONNECTED){
			continue;
		}
		if(level.clients[clientnum].sess.sessionTeam == TEAM_SPECTATOR){
			continue;
		}
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	}
	while(clientnum != original);
}
static void G_SayTo(gentity_t* other,int color,const char* name,const char* message){
	if(!other){return;}
	if(!other->inuse){return;}
	if(!other->client){return;}
	if(other->client->pers.connected != CON_CONNECTED){return;}
	trap_SendServerCommand(other - g_entities,va("%s \"%s%c%c%s\"","chat",name,Q_COLOR_ESCAPE,color,message));
}
#define EC "\x19"
void G_Say(gentity_t* ent,gentity_t* target,int mode,const char* chatText){
	int j;
	gentity_t* other;
	int color;
	char name[64];
	char text[MAX_SAY_TEXT];
	switch(mode){
		case SAY_ALL:
			G_LogPrintf("say: %s: %s\n",ent->client->pers.netname,chatText);
			Com_sprintf(name,sizeof(name),"%s%c%c"EC": ",ent->client->pers.netname,Q_COLOR_ESCAPE,COLOR_WHITE);
			color = COLOR_YELLOW;
			break;
		case SAY_TELL:
			Com_sprintf(name,sizeof(name),EC"[%s%c%c"EC"]"EC": ",ent->client->pers.netname,Q_COLOR_ESCAPE,COLOR_WHITE);
			color = COLOR_CYAN;
			break;
	}
	Q_strncpyz(text,chatText,sizeof(text));
	if(target){
		G_SayTo(target,color,name,text);
		return;
	}
	if(g_dedicated.integer){
		G_Printf("%s%s\n",name,text);
	}
	for(j=0;j<level.maxclients;j++){
		other = &g_entities[j];
		G_SayTo(other,color,name,text);
	}
}
static void Cmd_Say_f(gentity_t* ent,int mode,qboolean arg0){
	if(trap_Argc() < 2 && !arg0){return;}
	char* p = ConcatArgs(arg0 ? 0 : 1);
	G_Say(ent,NULL,mode,p);
}
static void Cmd_Tell_f(gentity_t* ent){
	int targetNum;
	gentity_t* target;
	char* p;
	char arg[MAX_TOKEN_CHARS];
	if(trap_Argc() < 2){return;}
	trap_Argv(1,arg,sizeof(arg));
	targetNum = atoi(arg);
	if(targetNum < 0 || targetNum >= level.maxclients){return;}
	target = &g_entities[targetNum];
	if(!target || !target->inuse || !target->client){return;}
	p = ConcatArgs(2);
	G_LogPrintf("tell: %s to %s: %s\n",ent->client->pers.netname,target->client->pers.netname,p);
	G_Say(ent,target,SAY_TELL,p);
	if(ent != target){G_Say(ent,ent,SAY_TELL,p);}
}
static void G_VoiceTo(gentity_t* ent,gentity_t* other,int mode,const char* id,qboolean voiceonly){
	int color;
	char *cmd;
	if(!other){return;}
	if(!other->inuse){return;}
	if(!other->client){return;}
	else if(mode == SAY_TELL){
		color = COLOR_MAGENTA;
		cmd = "vtell";
	}
	else{
		color = COLOR_GREEN;
		cmd = "vchat";
	}
	trap_SendServerCommand(other-g_entities,va("%s %d %d %d %s",cmd,voiceonly,ent->s.number,color,id));
}
void G_Voice(gentity_t* ent,gentity_t* target,int mode,const char* id,qboolean voiceonly){
	int j;
	gentity_t* other;
	if(target){
		G_VoiceTo(ent,target,mode,id,voiceonly);
		return;
	}
	if(g_dedicated.integer){
		G_Printf("voice: %s %s\n",ent->client->pers.netname,id);
	}
	for(j=0;j<level.maxclients;j++){
		other = &g_entities[j];
		G_VoiceTo(ent,other,mode,id,voiceonly);
	}
}
static void Cmd_Voice_f(gentity_t* ent,int mode,qboolean arg0,qboolean voiceonly){
	if(trap_Argc() < 2 && !arg0){return;}
	char* p = ConcatArgs(arg0 ? 0 : 1);
	G_Voice(ent,NULL,mode,p,voiceonly);
}
static void Cmd_VoiceTell_f(gentity_t* ent,qboolean voiceonly){
	int targetNum;
	gentity_t* target;
	char* id;
	char arg[MAX_TOKEN_CHARS];
	if(trap_Argc() < 2){return;}
	trap_Argv(1,arg,sizeof(arg));
	targetNum = atoi(arg);
	if(targetNum < 0 || targetNum >= level.maxclients){return;}
	target = &g_entities[targetNum];
	if(!target || !target->inuse || !target->client){return;}
	id = ConcatArgs(2);
	G_LogPrintf("vtell: %s to %s: %s\n",ent->client->pers.netname,target->client->pers.netname,id);
	G_Voice(ent,target,SAY_TELL,id,voiceonly);
	if(ent != target){
		G_Voice(ent,ent,SAY_TELL,id,voiceonly);
	}
}
void Cmd_Where_f(gentity_t* ent){
	trap_SendServerCommand(ent- g_entities,va("print \"%s\n\"",vtos(ent->r.currentOrigin)));
}
static const char *gameNames[] = {
	"Free For All",
	"Single Player",
};
void Cmd_CallVote_f(gentity_t* ent){
	char* c;
	int i;
	char arg1[MAX_STRING_TOKENS];
	char arg2[MAX_STRING_TOKENS];
	if(!g_allowVote.integer){
		trap_SendServerCommand(ent - g_entities,"print \"Voting not allowed here.\n\"");
		return;
	}
	if(level.voteTime){
		trap_SendServerCommand(ent - g_entities,"print \"A vote is already in progress.\n\"");
		return;
	}
	if(ent->client->pers.voteCount >= MAX_VOTE_COUNT){
		trap_SendServerCommand(ent - g_entities,"print \"You have called the maximum number of votes.\n\"");
		return;
	}
	if(ent->client->sess.sessionTeam == TEAM_SPECTATOR){
		trap_SendServerCommand(ent - g_entities,"print \"Not allowed to call a vote as spectator.\n\"");
		return;
	}
	// make sure it is a valid command to vote on
	trap_Argv(1,arg1,sizeof(arg1));
	trap_Argv(2,arg2,sizeof(arg2));
	// check for command separators in arg2
	for(c=arg2;*c;++c){
		if(*c == ';'){
			trap_SendServerCommand(ent - g_entities,"print \"Invalid vote string.\n\"");
			return;
		}
	}
	if(!Q_stricmp(arg1,"map_restart")){}
	else if(!Q_stricmp(arg1,"nextmap")){}
	else if(!Q_stricmp(arg1,"map")){}
	else if(!Q_stricmp(arg1,"g_gametype")){}
	else if(!Q_stricmp(arg1,"kick")){}
	else if(!Q_stricmp(arg1,"clientkick")){}
	else if(!Q_stricmp(arg1,"g_doWarmup")){}
	else if(!Q_stricmp(arg1,"timelimit")){}
	else if(!Q_stricmp(arg1,"fraglimit")){}
	else{
		trap_SendServerCommand(ent - g_entities,"print \"Invalid vote string.\n\"");
		trap_SendServerCommand(ent - g_entities,"print \"Vote commands are: map_restart, nextmap, map <mapname>, g_gametype <n>, kick <player>, clientkick <clientnum>, g_doWarmup, timelimit <time>, fraglimit <frags>.\n\"");
		return;
	}
	// if there is still a vote to be executed
	if(level.voteExecuteTime){
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand(EXEC_APPEND,va("%s\n",level.voteString));
	}
	// special case for g_gametype, check for bad values
	if(!Q_stricmp(arg1,"g_gametype")){
		i = atoi(arg2);
		if(i == GT_SINGLE_PLAYER || i < GT_FFA || i >= GT_MAX_GAME_TYPE){
			trap_SendServerCommand(ent - g_entities,"print \"Invalid gametype.\n\"");
			return;
		}
		Com_sprintf(level.voteString,sizeof(level.voteString),"%s %d",arg1,i);
		Com_sprintf(level.voteDisplayString,sizeof(level.voteDisplayString),"%s %s",arg1,gameNames[i]);
	}
	else if(!Q_stricmp(arg1,"map")){
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char s[MAX_STRING_CHARS];
		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if(*s){
			Com_sprintf(level.voteString,sizeof(level.voteString),"%s %s; set nextmap \"%s\"",arg1,arg2,s);
		}
		else{
			Com_sprintf(level.voteString,sizeof(level.voteString),"%s %s",arg1,arg2);
		}
		Com_sprintf(level.voteDisplayString,sizeof(level.voteDisplayString),"%s",level.voteString);
	}else if(!Q_stricmp(arg1,"nextmap")){
		char s[MAX_STRING_CHARS];
		trap_Cvar_VariableStringBuffer("nextmap",s,sizeof(s));
		if(!*s){
			trap_SendServerCommand(ent - g_entities,"print \"nextmap not set.\n\"");
			return;
		}
		Com_sprintf(level.voteString,sizeof(level.voteString),"vstr nextmap");
		Com_sprintf(level.voteDisplayString,sizeof(level.voteDisplayString),"%s",level.voteString);
	}else{
		Com_sprintf(level.voteString,sizeof(level.voteString),"%s \"%s\"",arg1,arg2);
		Com_sprintf(level.voteDisplayString,sizeof(level.voteDisplayString),"%s",level.voteString);
	}
	trap_SendServerCommand(-1,va("print \"%s called a vote.\n\"",ent->client->pers.netname));
	// start the voting, the caller autoamtically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;
	for(i=0;i<level.maxclients;i++){
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}
	ent->client->ps.eFlags |= EF_VOTED;
	trap_SetConfigstring(CS_VOTE_TIME,va("%i",level.voteTime));
	trap_SetConfigstring(CS_VOTE_STRING,level.voteDisplayString);	
	trap_SetConfigstring(CS_VOTE_YES,va("%i",level.voteYes));
	trap_SetConfigstring(CS_VOTE_NO,va("%i",level.voteNo));	
}
void Cmd_Vote_f(gentity_t* ent){
	char msg[64];
	if(!level.voteTime){
		trap_SendServerCommand(ent - g_entities,"print \"No vote in progress.\n\"");
		return;
	}
	if(ent->client->ps.eFlags & EF_VOTED){
		trap_SendServerCommand(ent - g_entities,"print \"Vote already cast.\n\"");
		return;
	}
	if(ent->client->sess.sessionTeam == TEAM_SPECTATOR){
		trap_SendServerCommand(ent - g_entities,"print \"Not allowed to vote as spectator.\n\"");
		return;
	}
	trap_SendServerCommand(ent - g_entities,"print \"Vote cast.\n\"");
	ent->client->ps.eFlags |= EF_VOTED;
	trap_Argv(1,msg,sizeof(msg));
	if(msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ){
		level.voteYes++;
		trap_SetConfigstring(CS_VOTE_YES,va("%i",level.voteYes));
	}
	else{
		level.voteNo++;
		trap_SetConfigstring(CS_VOTE_NO,va("%i",level.voteNo));	
	}
}
void Cmd_SetViewpos_f(gentity_t* ent){
	vec3_t origin;
	vec3_t angles;
	char buffer[MAX_TOKEN_CHARS];
	if(!g_cheats.integer){
		trap_SendServerCommand(ent - g_entities,va("print \"Cheats are not enabled on this server.\n\""));
		return;
	}
	if(trap_Argc() != 5){
		trap_SendServerCommand(ent - g_entities,va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}
	VectorClear(angles);
	for(int i=0;i<3;i++){
		trap_Argv(i+1,buffer,sizeof(buffer));
		origin[i] = atof(buffer);
	}
	trap_Argv(4,buffer,sizeof(buffer));
	angles[YAW] = atof(buffer);
	TeleportPlayer(ent,origin,angles);
}
void ClientCommand(int clientNum){
	gentity_t* ent = g_entities + clientNum;
	char cmd[MAX_TOKEN_CHARS];
	// not fully in game yet
	if(!ent->client){return;}
	trap_Argv(0,cmd,sizeof(cmd));
	if(!Q_stricmp(cmd,"say")){
		Cmd_Say_f(ent,SAY_ALL,qfalse);
		return;
	}
	if(!Q_stricmp(cmd,"tell")) {
		Cmd_Tell_f ( ent );
		return;
	}
	if(!Q_stricmp(cmd,"vsay")){
		Cmd_Voice_f(ent,SAY_ALL,qfalse,qfalse);
		return;
	}
	if(!Q_stricmp(cmd,"vtell")){
		Cmd_VoiceTell_f(ent,qfalse);
		return;
	}
	if(!Q_stricmp(cmd,"vosay")) {
		Cmd_Voice_f(ent,SAY_ALL,qfalse,qtrue);
		return;
	}
	if(!Q_stricmp(cmd,"votell")) {
		Cmd_VoiceTell_f(ent,qtrue);
		return;
	}
	// ignore all other commands when at intermission
	if(level.intermissiontime) {
		Cmd_Say_f(ent,qfalse,qtrue);
		return;
	}
	else if (!Q_stricmp(cmd,"levelshot")){Cmd_LevelShot_f(ent);}
	else if (!Q_stricmp(cmd,"follow")){Cmd_Follow_f(ent);}
	else if (!Q_stricmp(cmd,"follownext")){Cmd_FollowCycle_f(ent,1);}
	else if (!Q_stricmp(cmd,"followprev")){Cmd_FollowCycle_f(ent,-1);}
	else if (!Q_stricmp(cmd,"team")){Cmd_Team_f(ent);}
	else if (!Q_stricmp(cmd,"where")){Cmd_Where_f(ent);}
	else if (!Q_stricmp(cmd,"callvote")){Cmd_CallVote_f(ent);}
	else if (!Q_stricmp(cmd,"vote")){Cmd_Vote_f(ent);}
	else if (!Q_stricmp(cmd,"setviewpos")){Cmd_SetViewpos_f(ent);}
	else{trap_SendServerCommand(clientNum,va("print \"unknown cmd %s\n\"",cmd));}
}
