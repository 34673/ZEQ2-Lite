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
// g_client.c -- client functions that don't happen every frame
#include "g_local.h"
#define	MAX_SPAWN_POINTS 128
static vec3_t	playerMins = {-15, -15, -24};
static vec3_t	playerMaxs = {15, 15, 32};
/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nohumans" will prevent non-humans from using this spot.
*/
void SP_info_player_deathmatch(gentity_t* ent){}
/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t* ent) {
	ent->classname = "info_player_deathmatch";
}
/*
=======================================================================
  SelectSpawnPoint
=======================================================================
*/
qboolean SpotWouldTelefrag(gentity_t* spot){
	int num;
	int touch[MAX_GENTITIES];
	gentity_t* hit;
	vec3_t mins;
	vec3_t maxs;
	VectorAdd(spot->s.origin,playerMins,mins);
	VectorAdd(spot->s.origin,playerMaxs,maxs);
	num = trap_EntitiesInBox(mins,maxs,touch,MAX_GENTITIES);
	for(int i=0;i<num;i++){
		hit = &g_entities[touch[i]];
		if(hit->client){
			return qtrue;
		}
	}
	return qfalse;
}
/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t* SelectRandomFurthestSpawnPoint(vec3_t avoidPoint,vec3_t origin,vec3_t angles){
	gentity_t* spot = NULL;
	vec3_t delta;
	float dist;
	float list_dist[MAX_SPAWN_POINTS];
	gentity_t* list_spot[MAX_SPAWN_POINTS];
	int numSpots = 0;
	int rnd;
	int i;
	while((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL){
		if(SpotWouldTelefrag(spot)){continue;}
		VectorSubtract(spot->s.origin,avoidPoint,delta);
		dist = VectorLength(delta);
		for(i=0;i<numSpots;i++){
			if(dist > list_dist[i]){
				if(numSpots >= MAX_SPAWN_POINTS){numSpots = MAX_SPAWN_POINTS - 1;}
				for(int j=numSpots;j>i;j--){
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				break;
			}
		}
		if(i >= numSpots && numSpots < MAX_SPAWN_POINTS){
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if(!numSpots){
		spot = G_Find(NULL,FOFS(classname),"info_player_deathmatch");
		if(!spot){G_Error("Couldn't find a spawn point");}
		VectorCopy(spot->s.origin,origin);
		origin[2] += 9;
		VectorCopy(spot->s.angles,angles);
		return spot;
	}
	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);
	VectorCopy(list_spot[rnd]->s.origin,origin);
	origin[2] += 9;
	VectorCopy(list_spot[rnd]->s.angles,angles);
	return list_spot[rnd];
}
gentity_t* SelectSpawnPoint(vec3_t avoidPoint,vec3_t origin,vec3_t angles){
	return SelectRandomFurthestSpawnPoint(avoidPoint,origin,angles);
}
/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
gentity_t* SelectInitialSpawnPoint(vec3_t origin,vec3_t angles){
	gentity_t* spot = NULL;
	while((spot = G_Find(spot,FOFS(classname),"info_player_deathmatch")) != NULL){
		if((spot->spawnflags & 0x01)){break;}
	}
	if(!spot || SpotWouldTelefrag(spot)){
		return SelectSpawnPoint(vec3_origin,origin,angles);
	}
	VectorCopy(spot->s.origin,origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles,angles);
	return spot;
}
void InitBodyQue(void){
	gentity_t* ent;
	level.bodyQueIndex = 0;
	for(int i=0;i<BODY_QUEUE_SIZE;i++){
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}
void SetClientViewAngle(gentity_t* ent,vec3_t angle){
	for(int i=0;i<3;i++){
		int cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy(angle,ent->s.angles);
	VectorCopy(ent->s.angles,ent->client->ps.viewangles);
}
void ClientRespawn(gentity_t* ent){
	gentity_t* tent;
	ClientSpawn(ent);
	ent->client->ps.powerLevel[plTierCurrent] = 0;
	ent->client->ps.powerLevel[plTierDesired] = 0;
	ent->client->ps.powerLevel[plTierChanged] = 2;
	ent->client->ps.lockonData[lkLastLockedPlayer] = -1;
	// add a teleportation effect
	tent = G_TempEntity(ent->client->ps.origin,EV_PLAYER_TELEPORT_IN);
	tent->s.clientNum = ent->s.clientNum;
}
/*
================
TeamCount
Returns number of players on a team
================
*/
team_t TeamCount(int ignoreClientNum,unsigned int team){
	int count = 0;
	for(int i=0;i<level.maxclients;i++){
		if(i == ignoreClientNum){continue;}
		if(level.clients[i].pers.connected == CON_DISCONNECTED){continue;}
		if(level.clients[i].sess.sessionTeam == team){count++;}
	}
	return count;
}
static void ClientCleanName(const char* in,char* out,int outSize){
	int outpos = 0;
	int colorlessLen = 0;
	int spaces = 0;
	// discard leading spaces
	for(;*in == ' ';in++);
	for(;*in && outpos < outSize-1;in++){
		out[outpos] = *in;
		if(*in == ' '){
			// don't allow too many consecutive spaces
			if(spaces > 2){continue;}
			spaces++;
		}
		else if(outpos > 0 && out[outpos-1] == Q_COLOR_ESCAPE){
			if(Q_IsColorString(&out[outpos-1])){
				colorlessLen--;
				if(ColorIndex(*in) == 0){
					// Disallow color black in names to prevent players
					// from getting advantage playing in front of black backgrounds
					outpos--;
					continue;
				}
			}
			else{
				spaces = 0;
				colorlessLen++;
			}
		}
		else{
			spaces = 0;
			colorlessLen++;
		}
		outpos++;
	}
	out[outpos] = '\0';
	// don't allow empty names
	if(*out == '\0' || colorlessLen == 0){
		Q_strncpyz(out,"UnnamedPlayer",outSize);
	}
}
/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged(int clientNum){
	gentity_t* ent = g_entities + clientNum;
	int shouldRespawn = 0;
	char* s;
	char model[MAX_QPATH];
	char headModel[MAX_QPATH];
	char legsModel[MAX_QPATH];
	char oldname[MAX_STRING_CHARS];
	gclient_t* client = ent->client;
	char userinfo[MAX_INFO_STRING];
	trap_GetUserinfo(clientNum,userinfo,sizeof(userinfo));
	if(!Info_Validate(userinfo)){strcpy(userinfo,"\\name\\badinfo");}
	s = Info_ValueForKey(userinfo,"ip");
	if(!strcmp(s,"localhost")){client->pers.localClient = qtrue;}
	s = Info_ValueForKey(userinfo,"cg_advancedFlight");
	client->ps.options &= ~advancedFlight;
	if(!Q_stricmp(s,"1")){client->ps.options |= advancedFlight;}
	s = Info_ValueForKey(userinfo,"cg_predictItems");
	// set name
	Q_strncpyz(oldname,client->pers.netname,sizeof(oldname));
	s = Info_ValueForKey(userinfo,"name");
	ClientCleanName(s,client->pers.netname,sizeof(client->pers.netname));
	if(client->pers.connected == CON_CONNECTED){
		if(strcmp(oldname,client->pers.netname)){
			trap_SendServerCommand(-1,va("print \"%s" S_COLOR_WHITE " renamed to %s\n\"",oldname,client->pers.netname));
		}
	}
	// set model
	Q_strncpyz(model,Info_ValueForKey(userinfo,"model"),sizeof(model));
	Q_strncpyz(headModel,Info_ValueForKey(userinfo,"headmodel"),sizeof(headModel));
	Q_strncpyz(legsModel,Info_ValueForKey(userinfo,"legsmodel"),sizeof(legsModel));
	if(strcmp(model,ent->modelName)){
		if(client->ps.powerLevel[plHealth] > 0){shouldRespawn = 1;}
		if(((float)client->ps.powerLevel[plHealth]/(float)client->ps.powerLevel[plMaximum])<0.75){shouldRespawn = 2;}
	}
	Q_strncpyz(ent->modelName,model,sizeof(ent->modelName));
	client->playerEntity = ent;
	client->ps.rolling = g_rolling.value;
	client->ps.running = g_running.value;
	if(g_pointGravity.value){client->ps.options |= pointGravity;}
	// ADDING FOR ZEQ2
	// REFPOINT: Loading the serverside weaponscripts here.
	{
		char	filename[MAX_QPATH];
		char	modelName[MAX_QPATH];
		char	skinName[MAX_QPATH];
		char	modelStr[MAX_QPATH];
		char	*skin;
		int		i;
		Q_strncpyz( modelStr, model, sizeof(modelStr));
		if((skin = strchr(modelStr, '/')) == NULL){
			skin = "default";
		}
		else{
			*skin++ = 0;
		}
		Q_strncpyz(skinName,skin,sizeof(skinName));
		Q_strncpyz(modelName,modelStr,sizeof(modelName));
		client->modelName = modelName;
		setupTiers(client);
		Com_sprintf(filename,sizeof(filename),"players/%s/%s.phys",modelName,skinName);
		if(trap_FS_FOpenFile(filename,0,FS_READ) <= 0){
			Com_sprintf(filename,sizeof(filename),"players/%s/%s.phys","goku","default");
		}
		G_weapPhys_Parse(filename,clientNum);
		// Set the weapon mask here, incase we changed models on the fly.
		// FIXME: Can be removed eventually, when we will disallow on the fly
		//        switching.
		client->ps.stats[stSkills] = *G_FindUserWeaponMask(clientNum);
		// force a new weapon up if the current one is unavailable now.
		// Search downward
		for(i=MAX_PLAYERWEAPONS;i>0;i--){
			// We found a valid weapon
			if(client->ps.stats[stSkills] & (1<<i)){break;}
		}
		client->ps.weapon = i;
	}
	// END ADDING
	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\lmodel\\%s",
		client->pers.netname, client->sess.sessionTeam, model, headModel, legsModel);
	client->ps.powerLevel[plTierCurrent] = 0;
	client->ps.powerLevel[plTierDesired] = 0;
	client->ps.powerLevel[plTierChanged] = 2;
	client->ps.lockonData[lkLastLockedPlayer] = -1;
	trap_SetConfigstring(CS_PLAYERS+clientNum,s);
	if(shouldRespawn==1){ClientRespawn(ent);}
	if(shouldRespawn==2){client->ps.powerLevel[plUseHealth] = 32767;}
	G_LogPrintf("ClientUserinfoChanged: %i %s\n",clientNum,s);
}
/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes.
============
*/
char* ClientConnect(int clientNum,qboolean firstTime){
	char* value;
	gclient_t* client;
	char userinfo[MAX_INFO_STRING];
	gentity_t* ent;
	ent = &g_entities[clientNum];
	trap_GetUserinfo(clientNum,userinfo,sizeof(userinfo));
 	// IP filtering
 	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=500
 	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
 	// check to see if they are on the banned IP list
	value = Info_ValueForKey(userinfo,"ip");
	if(G_FilterPacket(value)){return "You are banned from this server.";}
  	// we don't check password for local client
  	// NOTE: local client <-> "ip" "localhost"
  	// this means this client is not running in our current process
	if((strcmp(value, "localhost") != 0)){
		qboolean isNone = !Q_stricmp(g_password.string,"none");
		qboolean isCorrect = !strcmp(g_password.string, value) != 0;
		value = Info_ValueForKey(userinfo,"password");
		if(g_password.string[0] && !isNone && !isCorrect){
			return "Invalid password";
		}
	}
	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;
	memset(client,0,sizeof(*client));
	client->pers.connected = CON_CONNECTING;
	// read or initialize the session data
	if(firstTime || level.newSession){G_InitSessionData(client,userinfo);}
	G_ReadSessionData(client);
	// get and distribute relevent paramters
	G_LogPrintf("ClientConnect: %i\n",clientNum);
	ClientUserinfoChanged(clientNum);
	// don't do the "xxx connected" messages if they were caried over from previous level
	if(firstTime){
		trap_SendServerCommand(-1,va("print \"%s"S_COLOR_WHITE" connected\n\"",client->pers.netname));
	}
	return NULL;
}
/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin(int clientNum){
	gentity_t* ent = g_entities + clientNum;
	gclient_t* client = level.clients + clientNum;
	gentity_t* tent;
	int flags;
	if(ent->r.linked){trap_UnlinkEntity(ent);}
	G_InitGentity(ent);
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;
	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;
	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	memset(&client->ps,0,sizeof(client->ps));
	client->ps.eFlags = flags;
	// ADDING FOR ZEQ2
	// Set the starting cap
	ClientUserinfoChanged(clientNum);
	client->ps.powerLevel[plMaximum] = g_powerlevel.value;
	client->ps.powerLevel[plHealthPool] = client->ps.powerLevel[plMaximum] / 3;
	client->ps.powerLevel[plMaximumPool] = client->ps.powerLevel[plMaximum] / 3;
	client->ps.powerLevel[plLimit] = g_powerlevelMaximum.value;
	client->ps.bitFlags |= isUnsafe;
	// END ADDING
	// locate ent at a spawn point
	ClientSpawn(ent);
	if(client->sess.sessionTeam != TEAM_SPECTATOR){
		// send event
		tent = G_TempEntity(ent->client->ps.origin,EV_PLAYER_TELEPORT_IN);
		tent->s.clientNum = ent->s.clientNum;
		trap_SendServerCommand(-1,va("print \"%s"S_COLOR_WHITE" entered the game\n\"",client->pers.netname));
	}
	G_LogPrintf("ClientBegin: %i\n",clientNum);
}
/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn(gentity_t *ent) {
	int index = ent - g_entities;
	vec3_t spawn_origin;
	vec3_t spawn_angles;
	gclient_t* client = ent->client;
	int i;
	clientPersistant_t saved;
	clientSession_t savedSess;
	int persistant[MAX_PERSISTANT];
	int flags;
	int savedPing;
	int eventSequence;
	char model[MAX_QPATH];
	char userinfo[MAX_INFO_STRING];
	// find a spawn point
	// do it before setting powerLevel back up, so farthest
	// ranging doesn't count this client.
	// the first spawn should be at a good looking spot
	if(!client->pers.initialSpawn && client->pers.localClient){
		client->pers.initialSpawn = qtrue;
		SelectInitialSpawnPoint(spawn_origin,spawn_angles);
	}
	// don't spawn near existing origin if possible
	else{
		SelectSpawnPoint(client->ps.origin,spawn_origin,spawn_angles);
	}
	client->pers.teamState.state = TEAM_ACTIVE;
	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT|EF_VOTED);
	flags ^= EF_TELEPORT_BIT;
	// clear everything but the persistant data
	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	for(i=0;i<MAX_PERSISTANT;i++){
		persistant[i] = client->ps.persistant[i];
	}
	eventSequence = client->ps.eventSequence;
	//Eagle: this is present in quake 3 and is probably there to prevent undefined behavior
	//If uncommented, ZEQ2-Lite clients won't have access to many gameplay actions
	//probably because some fields are missing in the initialization steps below
	//NEEDS INVESTIGATION
	//Com_Memset(client,0,sizeof(*client));
	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	client->lastkilled_client = -1;
	for(i=0;i<MAX_PERSISTANT;i++){
		client->ps.persistant[i] = persistant[i];
	}
	client->ps.eventSequence = eventSequence;
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;
	client->airOutTime = level.time + 12000;
	trap_GetUserinfo(index,userinfo,sizeof(userinfo));
	Q_strncpyz(model,Info_ValueForKey(userinfo,"model"),sizeof(model));
	// clear entity values
	client->ps.eFlags = flags;
	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;
	VectorCopy(playerMins,ent->r.mins);
	VectorCopy(playerMaxs,ent->r.maxs);
	client->ps.clientNum = index;
	client->ps.stats[stSkills] = *G_FindUserWeaponMask(index);
	client->ps.stats[stChargePercentPrimary] = 0;
	client->ps.stats[stChargePercentSecondary] = 0;
	client->ps.rolling = g_rolling.value;
	client->ps.running = g_running.value;
	if(g_pointGravity.value){client->ps.options |= pointGravity;}
	client->ps.powerLevel[plTierCurrent] = 0;
	client->ps.powerLevel[plTierTotal] = 0;
	client->ps.powerLevel[plTierDesired] = 0;
	client->ps.powerLevel[plTierChanged] = 2;
	client->ps.lockonData[lkLastLockedPlayer] = -1;
	if(g_powerlevel.value > 32767){g_powerlevel.value = 32767;}
	if(g_powerlevelMaximum.value > 32767){g_powerlevelMaximum.value = 32767;}
	// make sure all bitFlags are OFF, and explicitly turn off the aura
	client->ps.bitFlags = 0;
	client->ps.eFlags &= ~EF_AURA;
	// END ADDING
	client->ps.powerLevel[plMaximum] = client->ps.powerLevel[plMaximum] > g_powerlevel.value ? client->ps.powerLevel[plMaximum] * 0.75 : g_powerlevel.value;
	client->ps.powerLevel[plHealthPool] = client->ps.powerLevel[plMaximum] / 4;
	client->ps.powerLevel[plMaximumPool] = client->ps.powerLevel[plMaximum] / 4;
	client->ps.powerLevel[plLimit] = g_powerlevelMaximum.value;
	client->ps.powerLevel[plCurrent] = client->ps.powerLevel[plMaximum];
	client->ps.powerLevel[plHealth] = client->ps.powerLevel[plMaximum];
	client->ps.powerLevel[plFatigue] = client->ps.powerLevel[plMaximum];
	G_SetOrigin(ent,spawn_origin);
	VectorCopy(spawn_origin,client->ps.origin);
	trap_GetUsercmd(client-level.clients,&ent->client->pers.cmd);
	SetClientViewAngle(ent,spawn_angles);
	if(ent->client->sess.sessionTeam != TEAM_SPECTATOR){
		G_KillBox(ent);
		trap_LinkEntity(ent);
		// force the best weapon up
		for(int i=MAX_PLAYERWEAPONS;i>0;i--){
			// We found a valid weapon
			if(client->ps.stats[stSkills] & (1<<i)){break;}
		}
		// Either we assign a valid weapon, or we assign 0, which means
		client->ps.weapon = i;
		client->ps.weaponstate = WEAPON_READY;
	}
	// don't allow full run speed for a bit
	client->ps.pm_time = 100;
	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;
	// set default animations
	client->ps.torsoAnim = ANIM_IDLE;
	client->ps.legsAnim = ANIM_IDLE;
	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink(ent-g_entities);
	// run the presend to set anything else
	ClientEndFrame(ent);
	// clear entity state values
	BG_PlayerStateToEntityState(&client->ps,&ent->s,qtrue);
}
/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect(int clientNum){
	gentity_t* ent = g_entities + clientNum;
	gentity_t* tent;
	if(!ent->client){return;}
	// stop any following clients
	for(int i=0;i<level.maxclients;i++){
		qboolean isSpectator = level.clients[i].sess.sessionTeam == TEAM_SPECTATOR;
		qboolean isFollowing = level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW;
		qboolean correctClient = level.clients[i].sess.spectatorClient == clientNum;
		if(isSpectator && isFollowing && correctClient){
			StopFollowing(&g_entities[i]);
		}
	}
	// send effect if they were completely connected
	qboolean isConnected = ent->client->pers.connected == CON_CONNECTED;
	qboolean isSpectator = ent->client->sess.sessionTeam == TEAM_SPECTATOR;
	if(isConnected && !isSpectator){
		tent = G_TempEntity(ent->client->ps.origin,EV_PLAYER_TELEPORT_OUT);
		tent->s.clientNum = ent->s.clientNum;
	}
	G_LogPrintf("ClientDisconnect: %i\n",clientNum);
	trap_UnlinkEntity(ent);
	ent->client->ps.powerLevel[plHealth] = 0;
	ent->s.modelindex = 0;
	ent->r.contents &= ~CONTENTS_BODY;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;
	trap_SetConfigstring(CS_PLAYERS + clientNum,"");
}
