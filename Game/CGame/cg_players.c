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
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"

char *cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"ballFlip",
	"death",
	"drown",
	"falling",
	"landLight",
	"landMedium",
	"landHeavy",
	"footsteps",
	"gasp",
	"jump",
	"jumpHigh",
	"idleBanter",
	"idleWinningBanter",
	"idleLosingBanter",
	"painLight",
	"painMedium",
	"painHeavy",
	"struggleBanter",
	"taunt"
};

// HACK: We have to copy the entire playerEntity_t information
//       because otherwise Q3 finds that all previously set information
//       regarding frames and refEntities are nulled out on certain occassions.
//       WTF is up with this stupid bug anyway?! Same thing happened when adding
//       certain fields to centity_t...
static playerEntity_t playerInfoDuplicate[MAX_GENTITIES];
/*================
CG_CustomSound
================*/
sfxHandle_t	CG_CustomSound(int clientNum, const char *soundName){
	clientInfo_t* ci;
	int i;
	int nextIndex = (fabs(crandom())*8)+1;
	nextIndex = Com_Clamp(1,9,nextIndex);
	if(clientNum < 0 || clientNum >= MAX_CLIENTS){
		Com_Printf("^3CG_CustomSound(): invalid client number %d.\n",clientNum);
		return cgs.media.nullSound;
	}
	ci = &cgs.clientinfo[clientNum];
	if(!ci->infoValid){
		Com_Printf("^3CG_CustomSound(): invalid info from client number %d.)",clientNum);
		return cgs.media.nullSound;
	}
	for(i=0;i<MAX_CUSTOM_SOUNDS && cg_customSoundNames[i];i++){
		qboolean foundType = !strcmp(soundName,cg_customSoundNames[i]);
		if(foundType){return ci->sounds[ci->tierCurrent][(i*9)+nextIndex];}
	}
	Com_Printf("^3CG_CustomSound(): unknown sound type '%s' from '%d/%s'. Using default.\n",soundName,clientNum,ci->name);
	return cgs.media.nullSound;
}
/*=============================================================================
CLIENT INFO
===============================================================================
CG_ParseAnimationFile

Read a configuration file containing animation counts and rates
players//visor/animation.cfg, etc
======================*/
// static qboolean CG_ParseAnimationFile( const char *filename, clientInfo_t *ci ) {
// FIXME: Needs to lose static to use it in cg_tiers.c
qboolean CG_ParseAnimationFile(const char *filename, clientInfo_t *ci, qboolean isCamera){
	char			*token, *text_p, *prev,
					text[32000];
	int				i;
	unsigned int	len;
	float			fps;
	fileHandle_t	f;
	animation_t		*animations;

	if(isCamera) animations = ci->camAnimations;
	else animations = ci->animations;
	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(len <= 0) return qfalse;
	if(len >= sizeof(text)- 1){
		CG_Printf("File %s too long\n", filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);
	// parse the text
	text_p = text;
	ci->footsteps = FOOTSTEP_NORMAL;
	VectorClear( ci->headOffset );
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;
	ci->overrideHead = qfalse;
	// read optional parameters
	while(1){
		prev = text_p;	// so we can unget
		token = COM_Parse(&text_p);
		if(!*token) break;
		if(!Q_stricmp(token, "footsteps")){
			token = COM_Parse(&text_p);
			if(!*token) break;
				 if(!Q_stricmp(token, "default") || !Q_stricmp(token, "normal")) ci->footsteps = FOOTSTEP_NORMAL;
			else if(!Q_stricmp(token, "boot")) ci->footsteps = FOOTSTEP_BOOT;
			else if(!Q_stricmp(token, "flesh")) ci->footsteps = FOOTSTEP_FLESH;
			else if(!Q_stricmp(token, "mech")) ci->footsteps = FOOTSTEP_MECH;
			else if(!Q_stricmp(token, "energy")) ci->footsteps = FOOTSTEP_ENERGY;
			else CG_Printf("Bad footsteps parm in %s: %s\n", filename, token);
			continue;
		}
		else if(!Q_stricmp(token, "headoffset")){
			for(i=0;i<3;i++){
				token = COM_Parse(&text_p);
				if(!*token) break;
				ci->headOffset[i] = atof(token);
			}
			continue;
		}
		else if(!Q_stricmp(token, "fixedlegs")){
			ci->fixedlegs = qtrue;
			continue;
		}
		else if(!Q_stricmp(token, "fixedtorso")){
			ci->fixedtorso = qtrue;
			continue;
		}
		else if(!Q_stricmp(token, "overrideHead")){
			ci->overrideHead = qtrue;
			continue;
		}
		// if it is a number, start parsing animations
		if(token[0] >= '0' && token[0] <= '9'){
			text_p = prev;	// unget the token
			break;
		}
		Com_Printf("unknown token '%s' is %s\n", token, filename);
	}
	// read information for each frame
	for(i=0;i<MAX_ANIMATIONS;i++){
		token = COM_Parse(&text_p);
		if(!*token){
// ADDING FOR ZEQ2
	// until we include optional extra taunts, this block will remain turned off.
/*			if( i >= ANIM_GETFLAG && i <= ANIM_NEGATIVE ) {
				animations[i].firstFrame = animations[ANIM_GESTURE].firstFrame;
				animations[i].frameLerp = animations[ANIM_GESTURE].frameLerp;
				animations[i].initialLerp = animations[ANIM_GESTURE].initialLerp;
				animations[i].loopFrames = animations[ANIM_GESTURE].loopFrames;
				animations[i].numFrames = animations[ANIM_GESTURE].numFrames;
				animations[i].reversed = qfalse;
				animations[i].flipflop = qfalse;
				continue;
			}
*/			
// END ADDING
			break;
		}
		animations[i].firstFrame = atoi(token);
		token = COM_Parse(&text_p);
		if(!*token) break;
		animations[i].numFrames = atoi(token);
		animations[i].reversed = qfalse;
		animations[i].flipflop = qfalse;
		// if numFrames is negative the animation is reversed
		if(animations[i].numFrames < 0){
			animations[i].numFrames = -animations[i].numFrames;
			animations[i].reversed = qtrue;
		}
		token = COM_Parse(&text_p);
		if(!*token) break;
		animations[i].loopFrames = atoi(token);
		token = COM_Parse(&text_p);
		if(!*token) break;
		fps = atof(token);
		if(!fps) fps = 1;
		animations[i].frameLerp = 1000 / fps;
		animations[i].initialLerp = 1000 / fps;
		if(!isCamera){
			token = COM_Parse(&text_p);
			if(!*token) break;
			animations[i].continuous = atoi(token);
		}
		else{
			// Read the continuous flag for ki attack animations
			if(i >= ANIM_KI_ATTACK1_FIRE && i < MAX_ANIMATIONS && (i - ANIM_KI_ATTACK1_FIRE) % 2 == 0){
				token = COM_Parse(&text_p);
				if(!*token) break;
				animations[i].continuous = atoi(token);
			}
		}
	}
	if(i != MAX_ANIMATIONS){
		CG_Printf("Error parsing animation file: %s\n", filename);
		return qfalse;
	}
	memcpy(&animations[ANIM_BACKWALK], &animations[ANIM_WALK], sizeof(animation_t));
	animations[ANIM_BACKWALK].reversed = qtrue;
	memcpy(&animations[ANIM_BACKRUN], &animations[ANIM_RUN], sizeof(animation_t));
	animations[ANIM_BACKRUN].reversed = qtrue;
	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static qboolean CG_RegisterClientModelname(clientInfo_t *ci, const char *modelName, const char *skinName){
	if(!CG_RegisterClientModelnameWithTiers(ci, modelName, skinName)) return qfalse;
	return qtrue;
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo(clientInfo_t *ci){
	const char	*dir;
	char soundPath[MAX_QPATH];
	char singlePath[MAX_QPATH];
	char loopPath[MAX_QPATH];
	int i,tier,count,currentIndex,soundIndex,loopIndex,clientNum;
	char filename[MAX_QPATH];
	if(!CG_RegisterClientModelname(ci,ci->modelName,ci->skinName)){
		strcpy(ci->modelName,DEFAULT_MODEL);
		strcpy(ci->skinName,"default");
		if(cg_buildScript.integer){
			CG_Error("CG_RegisterClientModelname(%s, %s) failed", ci->modelName, ci->skinName);
		}
		if(!CG_RegisterClientModelname(ci, DEFAULT_MODEL, "default")){
			CG_Error("DEFAULT_MODEL (%s) failed to register",DEFAULT_MODEL);
		}
	}
	//sounds
	dir = ci->modelName;
	//Custom sounds reset? Would it need a method?
	for(tier=0;tier<9;tier++){
		for(i=0;i<MAX_CUSTOM_SOUNDS;i++){
			ci->sounds[tier][i] = cgs.media.nullSound;
		}
	}
	for(tier=0;tier<9;tier++){
		for(i=0;i<MAX_CUSTOM_SOUNDS;i++){
			if(!cg_customSoundNames[i]) continue;
			loopIndex = 0;
			currentIndex = 1;
			for(count=1;count<=9;count++){
				soundIndex = (i*9)+(count-1);
				if(tier == 0){
					Com_sprintf(singlePath, sizeof(singlePath), "players/%s/%s.opus", dir, cg_customSoundNames[i]);
					Com_sprintf(soundPath, sizeof(soundPath), "players/%s/%s%i.opus", dir, cg_customSoundNames[i],count);
					Com_sprintf(loopPath, sizeof(loopPath), "players/%s/%s%i.opus", dir, cg_customSoundNames[i], currentIndex);
				}
				else{
					Com_sprintf(singlePath, sizeof(singlePath), "players/%s/tier%i/%s.opus", dir, tier, cg_customSoundNames[i]);
					Com_sprintf(soundPath, sizeof(soundPath), "players/%s/tier%i/%s%i.opus", dir, tier, cg_customSoundNames[i], count);
					Com_sprintf(loopPath, sizeof(loopPath), "players/%s/tier%i/%s%i.opus", dir, tier, cg_customSoundNames[i], currentIndex);
				}
				ci->sounds[tier][soundIndex] = 0;
				if(trap_FS_FOpenFile(singlePath, 0, FS_READ) > 0){
					ci->sounds[tier][soundIndex] = trap_S_RegisterSound(singlePath, qfalse);
					continue;
				}
				else if(trap_FS_FOpenFile(soundPath, 0, FS_READ) > 0){
					ci->sounds[tier][soundIndex] = trap_S_RegisterSound(soundPath, qfalse);
					loopIndex = count;
				}
				else if(loopIndex && trap_FS_FOpenFile(loopPath, 0, FS_READ) > 0){
					ci->sounds[tier][soundIndex] = trap_S_RegisterSound(loopPath, qfalse);
					currentIndex += 1;
					if(currentIndex > loopIndex) currentIndex = 1;
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
	for(i=0;i<MAX_GENTITIES;i++){
		qboolean isInvisible = cg_entities[i].currentState.eType == ET_INVISIBLE;
		qboolean isPlayer = cg_entities[i].currentState.eType == ET_PLAYER;
		if(cg_entities[i].currentState.clientNum == clientNum && (isInvisible || isPlayer)){
			CG_ResetPlayerEntity(&cg_entities[i]);
		}
	}
	// REFPOINT: Load the additional tiers' clientInfo here
	CG_RegisterClientAura(clientNum,ci);
	Com_sprintf(filename, sizeof(filename), "players/%s/%s.grfx", ci->modelName, ci->skinName);
	CG_weapGfx_Parse(filename, clientNum);
}
/*======================
CG_NewClientInfo
======================*/
void CG_NewClientInfo(int clientNum){
	clientInfo_t* ci;
	clientInfo_t newInfo;
	const char* configstring;
	const char* v;
	const char* model;
	char* skin;
	ci = &cgs.clientinfo[clientNum];
	configstring = CG_ConfigString(clientNum + CS_PLAYERS);
	if(!configstring[0]){
		memset(ci,0,sizeof(*ci));
		return;
	}
	memset(&newInfo,0,sizeof(newInfo));
	v = Info_ValueForKey(configstring,"n");
	Q_strncpyz(newInfo.name,v,sizeof(newInfo.name));
	model = Info_ValueForKey(configstring,"model"); 
	v = model;
	skin = strchr(v,'/');
	if(!skin){skin = "default";}
	else{*skin++ = '\0';}
	Q_strncpyz(newInfo.skinName,skin,sizeof(newInfo.skinName));
	Q_strncpyz(newInfo.modelName,v,sizeof(newInfo.modelName));
	v = Info_ValueForKey(configstring,"hmodel");
	if(!v){v = model;}
	skin = strchr(v,'/');
	if(!skin){skin = "default";}
	else{*skin++ = '\0';}
	Q_strncpyz(newInfo.headModelName,v,sizeof(newInfo.headModelName));
	v = Info_ValueForKey(configstring,"lmodel");
	if(!v){v = model;}
	skin = strchr(v,'/');
	if(!skin){skin = "default";}
	else{*skin++ = '\0';}
	Q_strncpyz(newInfo.legsModelName,v,sizeof(newInfo.legsModelName));
	v = Info_ValueForKey(configstring,"cmodel");
	if(!v){v = model;}
	skin = strchr(v,'/');
	if(!skin){skin = "default";}
	else{*skin++ = '\0';}
	Q_strncpyz(newInfo.cameraModelName,v,sizeof(newInfo.cameraModelName));
	newInfo.infoValid = qtrue;
	*ci = newInfo;
	CG_LoadClientInfo(ci);
}

/*
=====================================
PLAYER ANIMATION
=====================================
CG_SetLerpFrameAnimation
may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, qboolean isCamera){
	animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;
	if(newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS)
		CG_Error("Bad animation number: %i", newAnimation);
	if(isCamera) anim = &ci->animations[newAnimation];
	else anim = &ci->camAnimations[newAnimation];
	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;
	if(cg_debugAnim.integer) CG_Printf("Anim: %i\n", newAnimation);
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunLerpFrame(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale, qboolean isCamera){
	animation_t	*anim;
	int			f, numFrames;

	// debugging tool to get no animations
	if(!cg_animSpeed.integer){
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}	
	// see if the animation sequence is switching
	if(!lf->animation) CG_SetLerpFrameAnimation(ci, lf, newAnimation, isCamera);
	if(newAnimation != lf->animationNumber)
		// If the only difference is the togglebit, and the animation is supposed to be
		// continuous.
		if(!((newAnimation & ~ANIM_TOGGLEBIT) == (lf->animationNumber & ~ANIM_TOGGLEBIT) && lf->animation->continuous))
			CG_SetLerpFrameAnimation(ci, lf, newAnimation, isCamera);
	// see if the animation sequence is switching
	//if(newAnimation != lf->animationNumber || !lf->animation)
	//	CG_SetLerpFrameAnimation(ci, lf, newAnimation);

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if(cg.time >= lf->frameTime){
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;
		// get the next frame based on the animation
		anim = lf->animation;
		// shouldn't happen
		if(!anim->frameLerp) return;
		// initial lerp
		if(cg.time < lf->animationTime)
			lf->frameTime = lf->animationTime;
		else lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		f = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		// adjust for haste, etc
		f *= speedScale;	
		numFrames = anim->numFrames;
		if(anim->flipflop) numFrames *= 2;
		if(f >= numFrames){
			f -= numFrames;
			if(anim->loopFrames){
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else{
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		if(anim->reversed)
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		else if(anim->flipflop && f>=anim->numFrames)
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f%anim->numFrames);
		else lf->frame = anim->firstFrame + f;
		if(cg.time > lf->frameTime){
			lf->frameTime = cg.time;
			if(cg_debugAnim.integer) CG_Printf("Clamp lf->frameTime\n");
		}
	}
	if(lf->frameTime > cg.time + 200) lf->frameTime = cg.time;
	if(lf->oldFrameTime > cg.time) lf->oldFrameTime = cg.time;
	// calculate current lerp value
	if(lf->frameTime == lf->oldFrameTime) lf->backlerp = 0;
	else lf->backlerp = 1.f - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
}

/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame(clientInfo_t *ci, lerpFrame_t *lf, int animationNumber, qboolean isCamera){
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation(ci, lf, animationNumber, isCamera);
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}

/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation(centity_t *cent, int *legsOld, int *legs, float *legsBackLerp, int *torsoOld, int *torso, float *torsoBackLerp, int *headOld, int *head, float *headBackLerp, int *cameraOld, int *camera, float *cameraBackLerp){
	clientInfo_t	*ci;
	qboolean		onBodyQue;
	float			speedScale;
	int				clientNum, tier, i;
	
	clientNum = cent->currentState.clientNum;
	if(cg_noPlayerAnims.integer){
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}
	//MDave: Check if we're dealing with the actual client entity (and not a dead body from the body queue)
	onBodyQue = (cent->currentState.number != cent->currentState.clientNum);
	tier = onBodyQue ? 0 : cent->currentState.tier;
	speedScale = 1;
	ci = &cgs.clientinfo[clientNum];
	// do the shuffle turn frames locally
//	if(cent->pe.modelLerpFrames[Legs].yawing &&(cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) == ANIM_IDLE)
//		CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Legs], ANIM_TURN, speedScale, qfalse);
//	else
	for(i=0;i<4;i++) CG_RunLerpFrame(ci,&cent->pe.modelLerpFrames[Legs],cent->currentState.legsAnim,speedScale, qfalse);
	*legsOld = cent->pe.modelLerpFrames[Legs].oldFrame;
	*legs = cent->pe.modelLerpFrames[Legs].frame;
	*legsBackLerp = cent->pe.modelLerpFrames[Legs].backlerp;
	CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Torso], cent->currentState.torsoAnim, speedScale, qfalse);
	*torsoOld = cent->pe.modelLerpFrames[Torso].oldFrame;
	*torso = cent->pe.modelLerpFrames[Torso].frame;
	*torsoBackLerp = cent->pe.modelLerpFrames[Torso].backlerp;
	CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Camera], cent->currentState.torsoAnim, speedScale, qtrue);
	*cameraOld = cent->pe.modelLerpFrames[Camera].oldFrame;
	*camera = cent->pe.modelLerpFrames[Camera].frame;
	*cameraBackLerp = cent->pe.modelLerpFrames[Camera].backlerp;
	// ADDING FOR ZEQ2
	{
		int	torsoAnimNum;
		// NOTE: Torso animations take precedence over leg animations when deciding which head animation to play.
		torsoAnimNum = cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT;
		if(cg.predictedPlayerState.bitFlags & usingBoost){CG_RunLerpFrame(ci,&cent->pe.modelLerpFrames[Head],ANIM_KI_CHARGE,speedScale,qfalse);}
		else if(cent->currentState.eFlags & EF_AURA && ci->overrideHead){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_KI_CHARGE, speedScale, qfalse);}
		else if(ci->auraConfig[tier]->auraAlways && ci->overrideHead){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_KI_CHARGE, speedScale, qfalse);}
		else if(ANIM_FLY_UP == torsoAnimNum && ci->overrideHead){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_KI_CHARGE, speedScale, qfalse);}
		else if(ANIM_FLY_DOWN == torsoAnimNum && ci->overrideHead){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_KI_CHARGE, speedScale, qfalse);}
		else if(ANIM_FLOOR_RECOVER == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_FLOOR_RECOVER, speedScale, qfalse);}
		else if(ANIM_WALK == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_WALK, speedScale, qfalse);}
		else if(ANIM_RUN == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_RUN, speedScale, qfalse);}
		else if(ANIM_BACKRUN == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BACKRUN, speedScale, qfalse);}
		else if(ANIM_JUMP_UP == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_JUMP_UP, speedScale, qfalse);}
		else if(ANIM_LAND_UP == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_LAND_UP, speedScale, qfalse);}
		else if(ANIM_JUMP_FORWARD == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_JUMP_FORWARD, speedScale, qfalse);}
		else if(ANIM_LAND_FORWARD == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_LAND_FORWARD, speedScale, qfalse);}
		else if(ANIM_JUMP_BACK == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_JUMP_BACK, speedScale, qfalse);}
		else if(ANIM_LAND_BACK == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_LAND_BACK, speedScale, qfalse);}
		else if(ANIM_SWIM_IDLE == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_SWIM_IDLE, speedScale, qfalse);}
		else if(ANIM_SWIM == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_SWIM, speedScale, qfalse);}
		else if(ANIM_DASH_RIGHT == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_DASH_RIGHT, speedScale, qfalse);}
		else if(ANIM_DASH_LEFT == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_DASH_LEFT, speedScale, qfalse);}
		else if(ANIM_DASH_FORWARD == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_DASH_FORWARD, speedScale, qfalse);}
		else if(ANIM_DASH_BACKWARD == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_DASH_BACKWARD, speedScale, qfalse);}
		else if(ANIM_KI_CHARGE == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_TRANS_UP, speedScale, qfalse);}
		else if(ANIM_PL_DOWN == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_PL_DOWN, speedScale, qfalse);}
		else if(ANIM_PL_UP == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_TRANS_UP, speedScale, qfalse);}
		else if(ANIM_TRANS_UP == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_TRANS_UP, speedScale, qfalse);}
		else if(ANIM_TRANS_BACK == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_TRANS_BACK, speedScale, qfalse);}
		else if(ANIM_FLY_IDLE == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_FLY_IDLE, speedScale, qfalse);}
		else if(ANIM_FLY_START == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_FLY_START, speedScale, qfalse);}
		else if(ANIM_FLY_FORWARD == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_FLY_FORWARD, speedScale, qfalse);}
		else if(ANIM_FLY_BACKWARD == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_FLY_BACKWARD, speedScale, qfalse);}
		else if(ANIM_FLY_UP == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_FLY_UP, speedScale, qfalse);}
		else if(ANIM_FLY_DOWN == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_FLY_DOWN, speedScale, qfalse);}
		else if(ANIM_STUNNED == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_STUNNED, speedScale, qfalse);}
		else if(ANIM_PUSH == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_TRANS_UP, speedScale, qfalse);}
		else if(ANIM_DEATH_GROUND == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_DEATH_GROUND, speedScale, qfalse);}
		else if(ANIM_DEATH_AIR == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_DEATH_AIR, speedScale, qfalse);}
		else if(ANIM_DEATH_AIR_LAND == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_DEATH_AIR_LAND, speedScale, qfalse);}
		else if(ANIM_STUN == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_STUN, speedScale, qfalse);}
		else if(ANIM_DEFLECT == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_DEFLECT, speedScale, qfalse);}
		else if(ANIM_BLOCK == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BLOCK, speedScale, qfalse);}
		else if(ANIM_SPEED_MELEE_ATTACK == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_SPEED_MELEE_ATTACK, speedScale, qfalse);}
		else if(ANIM_SPEED_MELEE_DODGE == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_SPEED_MELEE_DODGE, speedScale, qfalse);}
		else if(ANIM_SPEED_MELEE_BLOCK == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_SPEED_MELEE_BLOCK, speedScale, qfalse);}
		else if(ANIM_SPEED_MELEE_HIT == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_SPEED_MELEE_HIT, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_ATTACK1 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_ATTACK1, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_ATTACK2 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_ATTACK2, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_ATTACK3 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_ATTACK3, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_ATTACK4 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_ATTACK4, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_ATTACK5 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_ATTACK5, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_ATTACK6 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_ATTACK6, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_HIT1 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_HIT1, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_HIT2 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_HIT2, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_HIT3 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_HIT3, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_HIT4 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_HIT4, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_HIT5 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_HIT5, speedScale, qfalse);}
		else if(ANIM_BREAKER_MELEE_HIT6 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_BREAKER_MELEE_HIT6, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_1_CHARGE == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_1_CHARGE, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_2_CHARGE == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_2_CHARGE, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_3_CHARGE == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_3_CHARGE, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_4_CHARGE == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_4_CHARGE, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_5_CHARGE == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_5_CHARGE, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_6_CHARGE == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_6_CHARGE, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_1_HIT == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_1_HIT, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_2_HIT == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_2_HIT, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_3_HIT == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_3_HIT, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_4_HIT == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_4_HIT, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_5_HIT == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_5_HIT, speedScale, qfalse);}
		else if(ANIM_POWER_MELEE_6_HIT == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_POWER_MELEE_6_HIT, speedScale, qfalse);}
		else if(ANIM_STUNNED_MELEE == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_STUNNED_MELEE, speedScale, qfalse);}
		else if(ANIM_KNOCKBACK == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_KNOCKBACK, speedScale, qfalse);}
		else if(ANIM_KNOCKBACK_HIT_WALL == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_KNOCKBACK_HIT_WALL, speedScale, qfalse);}
		else if(ANIM_KNOCKBACK_RECOVER_1 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_KNOCKBACK_RECOVER_1, speedScale, qfalse);}
		else if(ANIM_KNOCKBACK_RECOVER_2 == torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_KNOCKBACK_RECOVER_2, speedScale, qfalse);}
		else if(ANIM_KI_ATTACK1_PREPARE <= torsoAnimNum && ANIM_KI_ATTACK6_ALT_FIRE >= torsoAnimNum){CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], torsoAnimNum - ANIM_KI_ATTACK1_PREPARE + ANIM_KI_ATTACK1_PREPARE, speedScale, qfalse);}
		else{
			if(cg.predictedPlayerState.lockedTarget > 0){
				CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_IDLE_LOCKED, speedScale, qfalse);
			}
			else{
				CG_RunLerpFrame(ci, &cent->pe.modelLerpFrames[Head], ANIM_IDLE, speedScale, qfalse);
			}
		}
	}
	*headOld = cent->pe.modelLerpFrames[Head].oldFrame;
	*head = cent->pe.modelLerpFrames[Head].frame;
	*headBackLerp = cent->pe.modelLerpFrames[Head].backlerp;
	// END ADDING
}

/*
===============================================
PLAYER ANGLES
===============================================
CG_SwingAngles
==================
*/
static void CG_SwingAngles(float destination,float swingTolerance,float clampTolerance,float speed,float* angle,qboolean* swinging){
	float swing;
	float move;
	float scale;
	if(!*swinging){
		// see if a swing should be started
		swing = AngleSubtract(*angle,destination);
		if(swing > swingTolerance || swing < -swingTolerance){*swinging = qtrue;}
	}
	if(!*swinging){return;}
	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract(destination,*angle);
	scale = fabs(swing);
	if(scale < swingTolerance * 0.5f){scale = 0.5f;}
	else if(scale < swingTolerance){scale = 1.0f;}
	else{scale = 2.0f;}
	// swing towards the destination angle
	if(swing >= 0){
		move = cg.frametime * scale * speed;
		if(move >= swing){
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}
	else if(swing < 0){
		move = cg.frametime * scale * -speed;
		if(move <= swing){
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}
	// clamp to no more than tolerance
	swing = AngleSubtract(destination, *angle);
	if(swing > clampTolerance){*angle = AngleMod(destination - (clampTolerance - 1));}
	else if(swing < -clampTolerance){*angle = AngleMod(destination + (clampTolerance - 1));}
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
static void CG_PlayerAngles(centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3], vec3_t camera[3]){
	vec3_t		legsAngles, torsoAngles, headAngles, cameraAngles;
	float		dest;
	static	int	movementOffsets[8] = {0, 22, 45, -22, 0, 22, -45, -22};
	int			dir, clientNum;
	clientInfo_t	*ci;
	
	clientNum = cent->currentState.clientNum;
	if(clientNum < 0 || clientNum > MAX_CLIENTS){
		CG_Error("Bad clientNum on player entity");
		return;
	}
	ci = &cgs.clientinfo[clientNum];
	VectorCopy(cent->lerpAngles, headAngles);
	headAngles[YAW] = AngleMod(headAngles[YAW]);
	VectorClear(legsAngles);
	VectorClear(torsoAngles);
	VectorClear(cameraAngles);
	// --------- yaw -------------
	// allow yaw to drift a bit
	if((cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) != ANIM_IDLE
		|| (cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != ANIM_IDLE
		|| (cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) != ANIM_IDLE_LOCKED
		|| (cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != ANIM_IDLE_LOCKED){
		// if not standing still, always point all in the same direction
		cent->pe.modelLerpFrames[Torso].yawing = qtrue;	// always center
		cent->pe.modelLerpFrames[Torso].pitching = qtrue;	// always center
		cent->pe.modelLerpFrames[Legs].yawing = qtrue;	// always center
	}
	// adjust legs for movement dir.
	// don't let dead bodies twitch or let certain animations adjust angles
	dir = cent->currentState.angles2[YAW];
	if(dir < 0 || dir > 7){
		CG_Error("Bad player movement angle");
		return;
	}
	if(cent->currentState.eFlags & EF_DEAD){dir = 0;}
	if((cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) >= ANIM_DASH_RIGHT){
		 if((cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) <= ANIM_DASH_BACKWARD){
			dir = 0;
		 }
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[dir];
	torsoAngles[YAW] = headAngles[YAW] + 0.25f * movementOffsets[dir];
	switch((cent->currentState.legsAnim & ~ANIM_TOGGLEBIT)){
		case ANIM_IDLE:
		case ANIM_FLY_IDLE:
		case ANIM_FLY_UP:
		case ANIM_FLY_DOWN:
		case ANIM_SWIM_IDLE:
			if(((&cg.predictedPlayerState)->weaponstate != WEAPON_READY || cent->currentState.weaponstate != WEAPON_READY)){
				CG_SwingAngles(torsoAngles[YAW], 25, 90, 1, &cent->pe.modelLerpFrames[Torso].yawAngle, &cent->pe.modelLerpFrames[Torso].yawing);
				CG_SwingAngles(legsAngles[YAW], 40, 90, 1, &cent->pe.modelLerpFrames[Legs].yawAngle, &cent->pe.modelLerpFrames[Legs].yawing);
			}
		break;
		default:
			CG_SwingAngles(torsoAngles[YAW], 25, 90, 1, &cent->pe.modelLerpFrames[Torso].yawAngle, &cent->pe.modelLerpFrames[Torso].yawing);
			CG_SwingAngles(legsAngles[YAW], 40, 90, 1, &cent->pe.modelLerpFrames[Legs].yawAngle, &cent->pe.modelLerpFrames[Legs].yawing);
		break;
	}
	headAngles[YAW] = cent->pe.modelLerpFrames[Torso].yawAngle;
	torsoAngles[YAW] = cent->pe.modelLerpFrames[Torso].yawAngle;
	legsAngles[YAW] = cent->pe.modelLerpFrames[Legs].yawAngle;
	dest = headAngles[PITCH] > 180 ? (-360 + headAngles[PITCH]) * 0.75f : headAngles[PITCH] * 0.75f;
	CG_SwingAngles(dest, 15, 30, 0.1f, &cent->pe.modelLerpFrames[Torso].pitchAngle, &cent->pe.modelLerpFrames[Torso].pitching);
	//torsoAngles[PITCH] = cent->pe.modelLerpFrames[Torso].pitchAngle;
	if(ci->fixedtorso || cent->currentState.weaponstate == WEAPON_CHARGING || cent->currentState.weaponstate == WEAPON_ALTCHARGING || 
				(cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) == ANIM_DEATH_GROUND || 
				(cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) == ANIM_DEATH_AIR || 
				(cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) == ANIM_DEATH_AIR_LAND ||
				(cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) == ANIM_KNOCKBACK_HIT_WALL ||
				(cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) == ANIM_FLOOR_RECOVER){
		headAngles[PITCH] = 0.0f;
		torsoAngles[PITCH] = 0.0f;
	}
	if(ci->fixedlegs){
		legsAngles[YAW] = torsoAngles[YAW];
		legsAngles[PITCH] = 0.0f;
		legsAngles[ROLL] = 0.0f;
	}
	// ADDING FOR ZEQ2
	// We're flying, so we change the entire body's directions altogether.
	if(cg_advancedFlight.value || !cg_thirdPersonCamera.value || (&cg.predictedPlayerState)->bitFlags & usingSoar || cent->currentState.playerBitFlags & usingSoar){
		VectorCopy(cent->lerpAngles, headAngles);
		VectorCopy(cent->lerpAngles, torsoAngles);
		VectorCopy(cent->lerpAngles, legsAngles);
	}
	else if((&cg.predictedPlayerState)->weaponstate != WEAPON_READY || cent->currentState.weaponstate != WEAPON_READY){
		VectorCopy(cent->lerpAngles, headAngles);
	}
	VectorCopy(cent->lerpAngles, cameraAngles);
	// END ADDING
	// pull the angles back out of the hierarchial chain
	AnglesSubtract(cameraAngles, torsoAngles, cameraAngles);
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
	AnglesToAxis(legsAngles, legs);
	AnglesToAxis(torsoAngles, torso);
	AnglesToAxis(headAngles, head);
	AnglesToAxis(cameraAngles, camera);
}

//==========================================================================

/*
===============
CG_InnerAuraSpikes
===============
*/
static void CG_InnerAuraSpikes(centity_t *cent, refEntity_t *head){
	clientInfo_t	*ci;
	vec3_t			up, origin;
	float			xyzspeed;
	int				r[6];
	
	ci = &cgs.clientinfo[cent->currentState.number];
	if(ci->bubbleTrailTime > cg.time) return;
	xyzspeed = sqrt(cent->currentState.pos.trDelta[0] * cent->currentState.pos.trDelta[0] +
					cent->currentState.pos.trDelta[1] * cent->currentState.pos.trDelta[1] + 
					cent->currentState.pos.trDelta[2] * cent->currentState.pos.trDelta[2]);
	if(cent->currentState.eFlags & EF_AURA && !xyzspeed){
			r[0] = random() * 32;
			r[1] = random() * 32;
			r[2] = random() * 32;
			r[3] = random() * 32;
			r[4] = random() * 32;
			r[5] = random() * 2;
			VectorSet(up, 0, 0, 64);
			VectorMA(head->origin, 0, head->axis[0], origin);
			VectorMA(origin, -40, head->axis[2], origin);
			origin[0] += r[0];
			origin[1] += r[1];
			origin[2] += r[2];
			origin[0] -= r[3];
			origin[1] -= r[4];
			origin[2] -= r[5];
			CG_AuraSpike(origin, up, 10, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cent);
			ci->bubbleTrailTime = cg.time + 50;
	}
}

/*
===============
CG_BubblesTrail
===============
*/
static void CG_BubblesTrail(centity_t *cent, refEntity_t *head){
	vec3_t up, origin;
	int contents;
	float xyzspeed;
	int r[6];
	if(cent->currentState.eFlags & EF_DEAD) return;
	// Measure the objects velocity
	xyzspeed = sqrt(cent->currentState.pos.trDelta[0] * cent->currentState.pos.trDelta[0] +
					cent->currentState.pos.trDelta[1] * cent->currentState.pos.trDelta[1] + 
					cent->currentState.pos.trDelta[2] * cent->currentState.pos.trDelta[2]);
	contents = trap_CM_PointContents(head->origin, 0);
	if((cent->currentState.eFlags & EF_AURA) || xyzspeed){
		if(contents & (CONTENTS_WATER)){
			r[0] = random() * 32;
			r[1] = random() * 32;
			r[2] = random() * 32;
			r[3] = random() * 32;
			r[4] = random() * 32;
			r[5] = random() * 48;
			VectorSet(up, 0, 0, 8);
			VectorMA(head->origin, 0, head->axis[0], origin);
			VectorMA(origin, -8, head->axis[2], origin);
			origin[0] += r[0];
			origin[1] += r[1];
			origin[2] += r[2];
			origin[0] -= r[3];
			origin[1] -= r[4];
			origin[2] -= r[5];
			if(cent->currentState.eFlags & EF_AURA){
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 750, cg.time, cg.time + 375, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleMediumShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 750, cg.time, cg.time + 375, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleMediumShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 750, cg.time, cg.time + 375, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleMediumShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleLargeShader);
				CG_WaterBubble(origin, up, 1, 1, 1, 1, 1.f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleLargeShader);
			} else{
				CG_WaterBubble(origin, up, 2, 1, 1, 1, 1.f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
				CG_WaterBubble(origin, up, 2, 1, 1, 1, 1.f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
				CG_WaterBubble(origin, up, 2, 1, 1, 1, 1.f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
				CG_WaterBubble(origin, up, 2, 1, 1, 1, 1.f, 500, cg.time, cg.time + 250, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
				CG_WaterBubble(origin, up, 2, 1, 1, 1, 1.f, 750, cg.time, cg.time + 375, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader);
				CG_WaterBubble(origin, up, 2, 1, 1, 1, 1.f, 750, cg.time, cg.time + 375, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader);
				CG_WaterBubble(origin, up, 2, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleMediumShader);
				CG_WaterBubble(origin, up, 2, 1, 1, 1, 1.f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleLargeShader);
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
static void CG_BreathPuffs(centity_t *cent, refEntity_t *head){
	clientInfo_t *ci = &cgs.clientinfo[cent->currentState.number];
	vec3_t up, origin;
	int contents = trap_CM_PointContents(head->origin, 0);
	if(cent->currentState.eFlags & EF_DEAD || ci->breathPuffTime > cg.time) return;
	if(contents & (CONTENTS_WATER)){
		VectorSet(up, 0, 0, 8);
		VectorMA(head->origin, 6, head->axis[0], origin);
		VectorMA(origin, -2, head->axis[2], origin);
		CG_WaterBubble(origin, up, 3, 1, 1, 1, .66f, 3000, cg.time, cg.time + 2500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
		CG_WaterBubble(origin, up, 3, 1, 1, 1, .66f, 3000, cg.time, cg.time + 2500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
		CG_WaterBubble(origin, up, 3, 1, 1, 1, .66f, 3000, cg.time, cg.time + 2500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
		CG_WaterBubble(origin, up, 3, 1, 1, 1, .66f, 3000, cg.time, cg.time + 2500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleTinyShader);
		CG_WaterBubble(origin, up, 3, 1, 1, 1, .66f, 2500, cg.time, cg.time + 2000, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader);
		CG_WaterBubble(origin, up, 3, 1, 1, 1, .66f, 2500, cg.time, cg.time + 2000, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleSmallShader);
		CG_WaterBubble(origin, up, 3, 1, 1, 1, .66f, 2000, cg.time, cg.time + 1500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleMediumShader);
		CG_WaterBubble(origin, up, 3, 1, 1, 1, .66f, 1000, cg.time, cg.time + 500, LEF_PUFF_DONT_SCALE, cgs.media.waterBubbleLargeShader);
		ci->breathPuffTime = cg.time + 2500;
	}
}

/*===============
CG_PlayerFloatSprite
Float a sprite over the player's head
===============*/
static void CG_PlayerFloatSprite(centity_t *cent, qhandle_t shader){
	int	rf;
	refEntity_t	ent;
	// only show in mirrors
	if(cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson)
		rf = RF_THIRD_PERSON;
	else rf = 0;
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites(centity_t *cent){
	if(cent->currentState.eFlags & EF_CONNECTION){
		CG_PlayerFloatSprite(cent, cgs.media.connectionShader);
		return;
	}
	if(cent->currentState.eFlags & EF_TALK){
		CG_PlayerFloatSprite(cent, cgs.media.chatBubble);
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
#define	SHADOW_DISTANCE	128
static qboolean CG_PlayerShadow(centity_t *cent, float *shadowPlane){
	vec3_t		mins = {-15,-15,0},
				maxs = {15,15,2},
				end;
	trace_t		trace;
	float		alpha;
	
	*shadowPlane = 0;
	if(!cg_shadows.integer) return qfalse;
	// send a trace down from the player to the ground
	VectorCopy(cent->lerpOrigin, end);
	end[2] -= SHADOW_DISTANCE;
	trap_CM_BoxTrace(&trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID);
	// no shadow if too high
	if(trace.fraction == 1.f || trace.startsolid || trace.allsolid) return qfalse;
	*shadowPlane = trace.endpos[2]+1;
	// no mark for stencil or projection shadows
	if(cg_shadows.integer != 1) return qtrue;
	// fade the shadow out with height
	alpha = 1.f - trace.fraction;
	// hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f ) 
	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark(cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, 
		cent->pe.modelLerpFrames[Legs].yawAngle, alpha,alpha,alpha, 1, qfalse, 24, qtrue);
	return qtrue;
}

/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
void CG_PlayerSplash(centity_t *cent, int scale){
	vec3_t			start,
					end,
					origin;
	trace_t			trace;
	polyVert_t		verts[4];
	entityState_t	*s1 = &cent->currentState;
	int				contents = trap_CM_PointContents(origin, 0),
					powerBoost;
	float			xyzspeed;
	// Measure the objects velocity
	xyzspeed = sqrt(cent->currentState.pos.trDelta[0] * cent->currentState.pos.trDelta[0] +
					cent->currentState.pos.trDelta[1] * cent->currentState.pos.trDelta[1] + 
					cent->currentState.pos.trDelta[2] * cent->currentState.pos.trDelta[2]);
	// Get the objects content value and position.
	BG_EvaluateTrajectory(s1, &s1->pos, cg.time, origin);
	VectorCopy(cent->lerpOrigin, end);
	VectorCopy(cent->lerpOrigin, start);
	if(xyzspeed && cent->currentState.eFlags & EF_AURA){
		powerBoost = 1;
		start[2] += 128;
		end[2] -= 128;
	}
	else if(!xyzspeed && cent->currentState.eFlags & EF_AURA){
		powerBoost = 6;
		start[2] += 512;
		end[2] -= 512;
	}
	else if(xyzspeed){
		powerBoost = 1;
		start[2] += 64;
		end[2] -= 48;
	}
	else{
		powerBoost = 1;
		start[2] += 32;
		end[2] -= 24;
	}
	// trace down to find the surface
	trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA));
	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = CG_PointContents(end, 0);
	if(!(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))) return;
	// if the head isn't out of liquid, don't make a mark
	contents = CG_PointContents(start, 0);
	if(contents & (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) return;
	if(trace.fraction == 1.f) return;
	if(xyzspeed || (cent->currentState.eFlags & EF_AURA)){
		if(powerBoost) CG_WaterSplash(trace.endpos,powerBoost+(xyzspeed/scale));
		if(cent->trailTime > cg.time) return;
		cent->trailTime += 250;
		if(cent->trailTime < cg.time) cent->trailTime = cg.time;
		if(!xyzspeed && cent->currentState.eFlags & EF_AURA)
			 CG_WaterRipple(trace.endpos,scale/20,qtrue);
		else CG_WaterRipple(trace.endpos,powerBoost+(xyzspeed/scale),qfalse);
	}
	// create a mark polygon
	VectorCopy(trace.endpos, verts[0].xyz);
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = verts[0].st[1] = 0;
	verts[0].modulate[0] = verts[0].modulate[1] = verts[0].modulate[2] = verts[0].modulate[3] = 255;
	VectorCopy(trace.endpos, verts[1].xyz);
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = verts[1].modulate[1] = verts[1].modulate[2] = verts[1].modulate[3] = 255;
	VectorCopy(trace.endpos, verts[2].xyz);
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = verts[2].st[1] = 1;
	verts[2].modulate[0] = verts[2].modulate[1] = verts[2].modulate[2] = verts[2].modulate[3] = 255;
	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = verts[3].modulate[1] = verts[3].modulate[2] = verts[3].modulate[3] = 255;
	trap_R_AddPolyToScene(cgs.media.wakeMarkShader, 4, verts);
}

/*
===============
CG_PlayerDirtPush
===============
*/
void CG_PlayerDirtPush(centity_t *cent, int scale, qboolean once){
	vec3_t end;
	trace_t trace;
	if(!once){
		if(cent->dustTrailTime > cg.time) return;
		if((cent->dustTrailTime += 250) < cg.time) cent->dustTrailTime = cg.time;
	}
	VectorCopy(cent->lerpOrigin, end);
	if(once) end[2] -= 4096;
	else end[2] -= 512;
	CG_Trace(&trace, cent->currentState.pos.trBase, NULL, NULL, end, cent->currentState.number, MASK_PLAYERSOLID);
	if(trace.fraction == 1.f) return;
	VectorCopy(trace.endpos, end);
	end[2] += 8;
	CG_DirtPush(end,trace.plane.normal,scale);
}

/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts(vec3_t normal, int numVerts, polyVert_t *verts){
	int				i, j;
	float			incoming;
	vec3_t			ambientLight,
					lightDir,
					directedLight;
	trap_R_LightForPoint(verts[0].xyz, ambientLight, directedLight, lightDir);
	for(i=0;i<numVerts;i++){
		incoming = DotProduct(normal, lightDir);
		if(incoming <= 0){
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		j = ambientLight[0] + incoming * directedLight[0];
		if(j > 255) j = 255;
		verts[i].modulate[0] = j;
		j = ambientLight[1] + incoming * directedLight[1];
		if(j > 255) j = 255;
		verts[i].modulate[1] = j;
		j = ambientLight[2] + incoming * directedLight[2];
		if(j > 255) j = 255;
		verts[i].modulate[2] = j;
		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

clientInfo_t* CG_GetClientInfo(centity_t* clientEntity){
	clientInfo_t* clientInfo;
	int clientNumber = clientEntity->currentState.clientNum;
	if(clientNumber < 0 || clientNumber >= MAX_CLIENTS){
		CG_Printf("^3CG_GetClientInfo(): Bad clientNum on player entity.\n");
		return NULL;
	}
	clientInfo = &cgs.clientinfo[clientNumber];
	if(!clientInfo->infoValid){
		CG_Printf("^3CG_GetClientInfo(): Invalid client info.\n");
		return NULL;
	}
	return clientInfo;
}

/*
===============
CG_Player
===============
*/
void CG_Player(centity_t *cent){
	clientInfo_t	*ci;
	playerState_t	*ps;
	refEntity_t		modelEntities[4];
	qboolean		shadow;
	float			shadowPlane;
	float			meshScale;
	float			xyzspeed;
	qboolean		onBodyQue;
	int				i=0;
	int				tier;
	int				health;
	int				damageState;
	int 			damageTextureState;
	int 			damageModelState;
	int				scale;
	int				renderfx=0;
	vec3_t			mins = {-15, -15, -24};
	vec3_t			maxs = {15, 15, 32};
	ps = &cg.snap->ps;
	ci = CG_GetClientInfo(cent);
	if(!ci){return;}
	if(cg.resetValues){
		ci->damageModelState = 0;
		ci->damageTextureState = 0;
		cg.resetValues = qfalse;
	}
	onBodyQue = cent->currentState.number != cent->currentState.clientNum;
	if(onBodyQue){tier = 0;}
	else{
		tier = cent->currentState.tier;
		if(ci->tierCurrent != tier){
			ci->tierCurrent = tier;
			CG_AddEarthquake(cent->lerpOrigin, 400, 1, 0, 1, 400);
		}
	}
	if(ci->tierCurrent > ci->tierMax) ci->tierMax = ci->tierCurrent;
	// Measure the players velocity
	xyzspeed = sqrt(cent->currentState.pos.trDelta[0] * cent->currentState.pos.trDelta[0] +
					cent->currentState.pos.trDelta[1] * cent->currentState.pos.trDelta[1] + 
					cent->currentState.pos.trDelta[2] * cent->currentState.pos.trDelta[2]);
	scale = 11;
	if(cent->currentState.number == ps->clientNum){
		if(ps->powerLevel[plCurrent] <= 1000){scale = 5;}
		if(ps->powerLevel[plCurrent] <= 5462){scale = 6;}
		if(ps->powerLevel[plCurrent] <= 10924){scale = 7;}
		if(ps->powerLevel[plCurrent] <= 16386){scale = 8;}
		if(ps->powerLevel[plCurrent] <= 21848){scale = 9;}
		if(ps->powerLevel[plCurrent] <= 27310){scale = 10;}
	}
	CG_PlayerSplash(cent,10*scale);
	if(cent->currentState.playerBitFlags & isBlinking){return;}
	if(cent->currentState.playerBitFlags & usingZanzoken){return;}
	if(cent->currentState.number == ps->clientNum &&
		ps->bitFlags & usingZanzoken &&
		!(ps->bitFlags & isUnconcious) &&
		!(ps->bitFlags & isDead) &&
		!(ps->bitFlags & isCrashed)){
		return;
	}
	health = ((float)cent->currentState.attackPowerCurrent / (float)cent->currentState.attackPowerTotal) * 100;
	damageState = health/10-1;
	if(damageState < 0) damageState = 0;
	damageModelState = damageState;
	damageTextureState = damageState;
	if(ci->damageModelState && damageModelState > (ci->damageModelState-1) && !ci->tierConfig[tier].damageModelsRevertHealed)
		damageModelState = ci->damageModelState - 1;
	if(ci->damageTextureState && damageTextureState > (ci->damageTextureState-1) && !ci->tierConfig[tier].damageTexturesRevertHealed)
		damageTextureState = ci->damageTextureState - 1;
	ci->damageTextureState = damageTextureState + 1;
	ci->damageModelState = damageModelState + 1;
	CG_PlayerSprites(cent);
	memset(&modelEntities,0,sizeof(modelEntities));
	CG_PlayerAngles(cent, modelEntities[Legs].axis, modelEntities[Torso].axis, modelEntities[Head].axis, modelEntities[Camera].axis);
	CG_PlayerAnimation(cent, &modelEntities[Legs].oldframe, &modelEntities[Legs].frame, &modelEntities[Legs].backlerp,
						&modelEntities[Torso].oldframe, &modelEntities[Torso].frame, &modelEntities[Torso].backlerp,
						&modelEntities[Head].oldframe, &modelEntities[Head].frame, &modelEntities[Head].backlerp,
						&modelEntities[Camera].oldframe, &modelEntities[Camera].frame, &modelEntities[Camera].backlerp);
	shadow = CG_PlayerShadow(cent,&shadowPlane);
	if(cg_shadows.integer == 3 && shadow)
		renderfx |= RF_SHADOW_PLANE;
	// use the same origin for all
	renderfx |= RF_LIGHTING_ORIGIN;	
	modelEntities[Legs].hModel = ci->modelDamageState[tier][2][damageModelState];
	modelEntities[Legs].customSkin = ci->skinDamageState[tier][2][damageTextureState];
	VectorCopy(cent->lerpOrigin,modelEntities[Legs].origin);
	VectorCopy(cent->lerpOrigin,modelEntities[Legs].lightingOrigin);
	modelEntities[Legs].shadowPlane = shadowPlane;
	modelEntities[Legs].renderfx = renderfx;
	// don't positionally lerp at all
	VectorCopy(modelEntities[Legs].origin, modelEntities[Legs].oldorigin);
	if(!modelEntities[Legs].hModel) return;
	meshScale = ci->tierConfig[tier].meshScale;
	modelEntities[Torso].hModel = ci->modelDamageState[tier][1][damageModelState];
	modelEntities[Torso].customSkin = ci->skinDamageState[tier][1][damageTextureState];
	if(!modelEntities[Torso].hModel) return;
	VectorCopy(cent->lerpOrigin,modelEntities[Torso].lightingOrigin);
	modelEntities[Legs].origin[2] += ci->tierConfig[tier].meshOffset;
	for(;i<3;i++){
		VectorScale(modelEntities[Torso].axis[i],meshScale,modelEntities[Torso].axis[i]);
		VectorScale(modelEntities[Legs].axis[i],meshScale,modelEntities[Legs].axis[i]);		
	}
	CG_PositionRotatedEntityOnTag( &modelEntities[Torso], &modelEntities[Legs], modelEntities[Legs].hModel, "tag_torso");
	for(i=0;i<3;i++) VectorScale(modelEntities[Torso].axis[i],1/meshScale,modelEntities[Torso].axis[i]);
	modelEntities[Torso].shadowPlane = shadowPlane;
	modelEntities[Torso].renderfx = renderfx;
	modelEntities[Head].hModel = ci->modelDamageState[tier][0][damageModelState];
	modelEntities[Head].customSkin = ci->skinDamageState[tier][0][damageTextureState];
	if(!modelEntities[Head].hModel) return;
	VectorCopy(cent->lerpOrigin, modelEntities[Head].lightingOrigin);
	CG_PositionRotatedEntityOnTag(&modelEntities[Head], &modelEntities[Torso], modelEntities[Torso].hModel, "tag_head");
	modelEntities[Head].shadowPlane = shadowPlane;
	modelEntities[Head].renderfx = renderfx;
	VectorCopy(cent->lerpOrigin,modelEntities[Camera].origin);
	VectorCopy(cent->lerpOrigin,modelEntities[Camera].lightingOrigin);
	modelEntities[Camera].hModel = ci->cameraModel[tier];
	if(!modelEntities[Camera].hModel) return;
	modelEntities[Camera].shadowPlane = shadowPlane;
	modelEntities[Camera].renderfx = renderfx;
	// don't positionally lerp at all
	VectorCopy(modelEntities[Camera].origin, modelEntities[Camera].oldorigin);	
	CG_PositionRotatedEntityOnTag(&modelEntities[Camera], &modelEntities[Torso], modelEntities[Torso].hModel, "tag_torso");
	trap_R_AddRefEntityToScene(&modelEntities[Legs]);
	trap_R_AddRefEntityToScene(&modelEntities[Torso]);
	trap_R_AddRefEntityToScene(&modelEntities[Head]);
	CG_BreathPuffs(cent,&modelEntities[Head]);
	CG_BubblesTrail(cent,&modelEntities[Head]);
	CG_InnerAuraSpikes(cent, &modelEntities[Head]);
	memcpy(&(cent->pe.modelEntities[Camera]), &modelEntities[Camera], sizeof(refEntity_t));
	memcpy(&(cent->pe.modelEntities[Head]), &modelEntities[Head], sizeof(refEntity_t));
	memcpy(&(cent->pe.modelEntities[Torso]), &modelEntities[Torso], sizeof(refEntity_t));
	memcpy(&(cent->pe.modelEntities[Legs]), &modelEntities[Legs], sizeof(refEntity_t));
	memcpy(&playerInfoDuplicate[cent->currentState.number], &cent->pe, sizeof(playerEntity_t));
	if(onBodyQue) return;
	CG_Camera(cent);
	CG_AddPlayerWeapon(&modelEntities[Torso],NULL,cent);
	if((cent->currentState.eFlags & EF_AURA) || ci->auraConfig[tier]->auraAlways){
		CG_AuraStart(cent);
		if(!xyzspeed){ CG_PlayerDirtPush(cent,scale,qfalse);}
		if(ps->powerLevel[plCurrent] == ps->powerLevel[plMaximum] && ps->bitFlags & usingAlter)
			CG_AddEarthquake(cent->lerpOrigin, 400, 1, 1, 1, 25);
	}
	else{ CG_AuraEnd(cent);}
	CG_AddAuraToScene(cent);
	if(cg_drawBBox.value){
		trap_R_ModelBounds(modelEntities[Head].hModel, mins, maxs, modelEntities[Head].frame);
		CG_DrawBoundingBox(modelEntities[Head].origin, mins, maxs);
		trap_R_ModelBounds(modelEntities[Torso].hModel, mins, maxs, modelEntities[Torso].frame);
		CG_DrawBoundingBox(modelEntities[Torso].origin, mins, maxs);
		trap_R_ModelBounds(modelEntities[Legs].hModel, mins, maxs,modelEntities[Legs].frame);
		CG_DrawBoundingBox(modelEntities[Legs].origin, mins, maxs);
	}
	if(ps->timers[tmKnockback]){
		if(ps->timers[tmKnockback] > 0){
			trap_S_StartSound(cent->lerpOrigin,ENTITYNUM_NONE,CHAN_BODY,cgs.media.knockbackSound);
			trap_S_AddLoopingSound(cent->currentState.number,cent->lerpOrigin,vec3_origin,cgs.media.knockbackLoopSound);
		}
		return;
	}
	if(ci->auraConfig[tier]->showLightning){
		CG_LightningEffect(cent->lerpOrigin, ci, tier);
		if(ps->bitFlags & usingMelee)
			CG_BigLightningEffect(cent->lerpOrigin);
	}
}

/*
===============
CG_TryLerpPlayerTag

If the entity in question is of the type ET_PLAYER, this gets the orientation of a tag on the player's model
and stores it. If the entity is of a different type, the tag is not found, or the model is on
the bodyQue and belongs to a disconnected client with invalid clientInfo, false is returned. In other cases, true is returned.
===============
*/
qboolean CG_TryLerpPlayerTag(centity_t *clientEntity,char *tagName,orientation_t *out){
	playerEntity_t* playerEntity;
	// The client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	int clientNumber = clientEntity->currentState.clientNum;
	playerEntity = NULL;
	if(!tagName[0]){
		//CG_Printf("^3CG_TryLerpPlayerTag(): tagName is empty\n");
		return qfalse;
	}
	if(clientEntity->currentState.eType != ET_PLAYER){
		//CG_Printf("^3CG_TryLerpPlayerTag(): entity is not of player type\n");
		return qfalse;
	}
	if(clientNumber < 0 || clientNumber >= MAX_CLIENTS){
		//CG_Printf("^3CG_TryLerpPlayerTag(): Bad clientNumber.\n");
		return qfalse;
	}
	// It is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if(!cgs.clientinfo[clientNumber].infoValid){
		//CG_Printf("^3CG_TryLerpPlayerTag(): invalid clientInfo.\n");
		return qfalse;
	}
	// HACK: Use this copy, which is sure to be stored correctly, unlike
	//       reading it from cg_entities, which tends to clear out its
	//       fields every now and then. WTF?!
	playerEntity = &playerInfoDuplicate[clientNumber];
	for(int modelPart=0;modelPart<4;modelPart++){
		orientation_t	lerped;
		vec3_t			tempAxis[3];
		AxisClear(out->axis);
		if(trap_R_LerpTag(&lerped,playerEntity->modelEntities[modelPart].hModel,playerEntity->modelLerpFrames[modelPart].oldFrame,
							playerEntity->modelLerpFrames[modelPart].frame,1.0f - playerEntity->modelLerpFrames[modelPart].backlerp,tagName)){
			VectorCopy(playerEntity->modelEntities[modelPart].origin,out->origin);
			for(int axisIndex=0;axisIndex<3;axisIndex++){
				VectorMA(out->origin,lerped.origin[axisIndex],playerEntity->modelEntities[modelPart].axis[axisIndex],out->origin);
			}
			MatrixMultiply(out->axis,lerped.axis,tempAxis);
			MatrixMultiply(tempAxis,playerEntity->modelEntities[modelPart].axis,out->axis);
			return qtrue;
		}
	}
	return qfalse;
}

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity(centity_t *cent){
	cent->errorTime = -99999;		// guarantee no error decay added
	cent->extrapolated = qfalse;	
	CG_ClearLerpFrame(&cgs.clientinfo[cent->currentState.clientNum], &cent->pe.modelLerpFrames[Legs], cent->currentState.legsAnim, qfalse);
	CG_ClearLerpFrame(&cgs.clientinfo[cent->currentState.clientNum], &cent->pe.modelLerpFrames[Torso], cent->currentState.torsoAnim, qfalse);
	BG_EvaluateTrajectory(&cent->currentState, &cent->currentState.pos, cg.time, cent->lerpOrigin);
	BG_EvaluateTrajectory(&cent->currentState, &cent->currentState.apos, cg.time, cent->lerpAngles);
	VectorCopy(cent->lerpOrigin, cent->rawOrigin);
	VectorCopy(cent->lerpAngles, cent->rawAngles);
	memset(&cent->pe.modelLerpFrames[Legs], 0, sizeof(cent->pe.modelLerpFrames[Legs]));
	cent->pe.modelLerpFrames[Legs].yawAngle = cent->rawAngles[YAW];
	cent->pe.modelLerpFrames[Legs].yawing = qfalse;
	cent->pe.modelLerpFrames[Legs].pitchAngle = 0;
	cent->pe.modelLerpFrames[Legs].pitching = qfalse;
	memset(&cent->pe.modelLerpFrames[Torso], 0, sizeof(cent->pe.modelLerpFrames[Legs]));
	cent->pe.modelLerpFrames[Torso].yawAngle = cent->rawAngles[YAW];
	cent->pe.modelLerpFrames[Torso].yawing = qfalse;
	cent->pe.modelLerpFrames[Torso].pitchAngle = cent->rawAngles[PITCH];
	cent->pe.modelLerpFrames[Torso].pitching = qfalse;
	memset(&cent->pe.modelLerpFrames[Camera], 0, sizeof(cent->pe.modelLerpFrames[Camera]));
	cent->pe.modelLerpFrames[Camera].yawAngle = cent->rawAngles[YAW];
	cent->pe.modelLerpFrames[Camera].yawing = qfalse;
	cent->pe.modelLerpFrames[Camera].pitchAngle = cent->rawAngles[PITCH];
	cent->pe.modelLerpFrames[Camera].pitching = qfalse;
	if(cg_debugPosition.integer) CG_Printf("%i ResetPlayerEntity yaw=%f\n", cent->currentState.number, cent->pe.modelLerpFrames[Torso].yawAngle);
}

void CG_SpawnLightSpeedGhost(centity_t *cent){
	if(random() < .2f){trap_S_StartSound(cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound1);}
	if(random() < .4f){trap_S_StartSound(cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound2);}
	if(random() < .6f){trap_S_StartSound(cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound3);}
	if(random() < .8f){trap_S_StartSound(cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound4);}
	if(random() >= .8f){trap_S_StartSound(cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound5);}
}
