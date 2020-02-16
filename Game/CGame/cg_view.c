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
// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"
#define MASK_CAMERACLIP (MASK_SOLID|CONTENTS_PLAYERCLIP)
#define CAMERA_SIZE	4
//Sets the coordinates of the rendered window
static void CG_CalcVrect(void){
	int	size;
	// the intermission should allways be full screen
	if(cg.snap->ps.pm_type == PM_INTERMISSION){size = 100;}
	else{
		// bound normal viewsize
		if(cg_viewsize.integer < 30){
			trap_Cvar_Set("cg_viewsize","30");
			size = 30;
		}
		else if(cg_viewsize.integer > 100){
			trap_Cvar_Set("cg_viewsize","100");
			size = 100;
		}
		else{size = cg_viewsize.integer;}
	}
	cg.refdef.width = cgs.glconfig.vidWidth*size/100;
	//strip less significant bit
	cg.refdef.width &= -2;
	cg.refdef.height = cgs.glconfig.vidHeight*size/100;
	cg.refdef.height &= -2;
	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)>>1;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)>>1;
}
//Gives screen projection of point in worldspace.
//Returns false if out of view.
qboolean CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y){
	float xcenter, ycenter;
	vec3_t local, transformed;
	vec3_t vforward;
	vec3_t vright;
	vec3_t vup;
	float xzi;
	float yzi;
	//gives screen coords in virtual 640x480, to be adjusted when drawn
	xcenter = 640.0f / 2.0f;
	ycenter = 480.0f / 2.0f;
	AngleVectors(cg.refdefViewAngles, vforward, vright, vup);
	VectorSubtract(worldCoord, cg.refdef.vieworg, local);
	transformed[0] = DotProduct(local,vright);
	transformed[1] = DotProduct(local,vup);
	transformed[2] = DotProduct(local,vforward);
	// Make sure Z is not negative.
	if(transformed[2] < 0.01f){return qfalse;}
	xzi = xcenter / transformed[2] * ( 96.0f / cg.refdef.fov_x);
	yzi = ycenter / transformed[2] * (102.0f / cg.refdef.fov_y);
	*x = xcenter + xzi * transformed[0];
	*y = ycenter - yzi * transformed[1];
	return qtrue;
}
qboolean CG_WorldCoordToScreenCoordVec(vec3_t world, vec2_t screen){
	return CG_WorldCoordToScreenCoordFloat(world, &screen[0], &screen[1]);
}
static void AddEarthquakeTremble(earthquake_t* quake);
/*===============
CG_Camera
===============*/
#define	NECK_LENGTH		8
#define CAMERA_BOUNDS	25
vec3_t cameraCurLoc;
vec3_t cameraCurAng;
int cameraLastFrame;
void CG_Camera(centity_t *cent){
	static vec3_t	cameramins = {-CAMERA_SIZE,-CAMERA_SIZE,-CAMERA_SIZE};
	static vec3_t	cameramaxs = {CAMERA_SIZE,CAMERA_SIZE,CAMERA_SIZE};
	vec3_t			view, right, forward, up, focusAngles, tagForwardAngles, locdiff, diff;
	int 			i=0,clientNum,cameraRange,cameraAngle,cameraSlide,cameraHeight;
	float			lerpFactor = 0.f, forwardScale, sideScale;
	orientation_t	tagOrient,tagOrient2;
	trace_t			trace;
	clientInfo_t	*ci;
	playerState_t	*ps;
	ps = &cg.snap->ps;
	clientNum = cent->currentState.clientNum;
	if(clientNum != ps->clientNum){return;}
	ci = &cgs.clientinfo[clientNum];
	cameraAngle = cg_thirdPersonAngle.value;
	cameraSlide = cg_thirdPersonSlide.value + ci->tierConfig[ci->tierCurrent].cameraOffset[0];
	cameraHeight = cg_thirdPersonHeight.value + ci->tierConfig[ci->tierCurrent].cameraOffset[1];
	cameraRange = cg_thirdPersonRange.value + ci->tierConfig[ci->tierCurrent].cameraOffset[2];
	if(cg_thirdPersonCamera.value <= 0){
		if(CG_TryLerpPlayerTag(cent,"tag_eyes",&tagOrient)){
			VectorCopy(tagOrient.origin, cg.refdef.vieworg);
		}
		if(CG_TryLerpPlayerTag(cent,"tag_head",&tagOrient)){
			VectorCopy(tagOrient.origin, cg.refdef.vieworg);
			cg.refdef.vieworg[2] -= NECK_LENGTH;
			AngleVectors(cg.refdefViewAngles, forward, NULL, up);
			VectorMA(cg.refdef.vieworg, 3, forward, cg.refdef.vieworg);
			VectorMA(cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg);
		}
		else{cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;}
	}
	else if(cg_thirdPersonCamera.value >= 1){
		if(CG_TryLerpPlayerTag(cent,"tag_cam",&tagOrient)){
			if(!cent->pe.modelLerpFrames[3].animation->continuous){
				VectorCopy(cent->lerpOrigin,tagOrient.origin);
				tagOrient.origin[2] += cg.predictedPlayerState.viewheight;
			}
			else if(!((cent->currentState.weaponstate == WEAPON_GUIDING) || (cent->currentState.weaponstate == WEAPON_ALTGUIDING) || (cent->currentState.playerBitFlags & usingSoar))){
				if(CG_TryLerpPlayerTag(cent,"tag_camTar",&tagOrient2)){
					VectorSubtract(tagOrient2.origin, tagOrient.origin, forward);
					VectorNormalize(forward);
					vectoangles(forward, tagForwardAngles);
					cg.refdefViewAngles[YAW] = tagForwardAngles[YAW];
					cg.refdefViewAngles[PITCH] = tagForwardAngles[PITCH];
				}
			}
			if(!cg_beamControl.value && ((cent->currentState.weaponstate == WEAPON_GUIDING) || (cent->currentState.weaponstate == WEAPON_ALTGUIDING)) && cg.guide_view){
					float oldRoll;
					VectorSubtract(cg.guide_target, tagOrient.origin, forward);
					VectorNormalize(forward);
					oldRoll = cg.refdefViewAngles[ROLL];
					vectoangles(forward, cg.refdefViewAngles);
					cg.refdefViewAngles[ROLL] = oldRoll;
					VectorCopy(tagOrient.origin,cg.guide_target);
					cg.guide_view = qfalse;
			}
			cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;
			VectorCopy(cg.refdefViewAngles, focusAngles);
			AngleVectors(focusAngles, forward, NULL, NULL);
			VectorCopy(tagOrient.origin, view);	
			AngleVectors(cg.refdefViewAngles, forward, right, up);
			if(!cent->pe.modelLerpFrames[3].animation->continuous){
				VectorMA(view,cameraSlide,right,view);
				view[2] += cameraHeight;
				forwardScale = cos(cameraAngle / 180 * M_PI);	
				sideScale = sin(cameraAngle / 180 * M_PI);
				VectorMA(view, -cameraRange * forwardScale, forward, view);
				VectorMA(view, -cameraRange * sideScale, right, view);
				cg.refdefViewAngles[YAW] -= cameraAngle;
			}
			CG_Trace(&trace, cg.refdef.vieworg, cameramins, cameramaxs, view, clientNum, MASK_CAMERACLIP);
			if(trace.fraction != 1.f){VectorCopy(trace.endpos, cg.refdef.vieworg);}
			else{VectorCopy(view, cg.refdef.vieworg);}
			if((cent->currentState.playerBitFlags & usingSoar) || (cg_thirdPersonCamera.value >= 5)){
				if(cameraLastFrame == 0 || cameraLastFrame > cg.time){
					VectorCopy(cg.refdef.vieworg, cameraCurLoc);
					cameraLastFrame = cg.time;
				}
				else{
					if(cg_thirdPersonCameraDamp.value != 0.f){lerpFactor = cg_thirdPersonCameraDamp.value;}
					if(lerpFactor>=1.f || cg.thisFrameTeleport || cent->currentState.playerBitFlags & usingZanzoken){
							VectorCopy(cg.refdef.vieworg, cameraCurLoc);
					}
					else if(lerpFactor>=0.f){
						VectorSubtract(cg.refdef.vieworg, cameraCurLoc, locdiff);
						lerpFactor = 1.f-lerpFactor;
						VectorMA(cg.refdef.vieworg, -lerpFactor, locdiff, cameraCurLoc);
					}
					CG_Trace(&trace, cg.refdef.vieworg, cameramins, cameramaxs, cameraCurLoc, cg.snap->ps.clientNum, MASK_CAMERACLIP);
					if(trace.fraction < 1.f){VectorCopy(trace.endpos, cameraCurLoc);}
				}
				if(cg_thirdPersonCamera.value >= 6){
					VectorMA(view, 500, forward, cameraCurAng);
					VectorSubtract(cameraCurAng, cameraCurLoc, diff);
					{
						float dist = VectorNormalize(diff);
						if(!dist || (diff[0] == 0 || diff[1] == 0)){VectorCopy(forward, diff);}
					}
					vectoangles(diff, cg.refdefViewAngles);
				}
				VectorCopy(cameraCurLoc, cg.refdef.vieworg);
				cameraLastFrame = cg.time;
			}
			else{VectorCopy(cg.refdef.vieworg, cameraCurLoc);}
			AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		}
	}
	for(;i<MAX_EARTHQUAKES;i++){
		earthquake_t* quake;
		quake = &cg.earthquakes[i];
		if(!quake->startTime){continue;}
		AddEarthquakeTremble(quake);
	}
	AddEarthquakeTremble(NULL);
}
void CG_ZoomDown_f(void){ 
	if(cg.zoomed){return;}
	cg.zoomed = qtrue;
	cg.zoomTime = cg.time;
}
void CG_ZoomUp_f(void){ 
	if(!cg.zoomed){return;}
	cg.zoomed = qfalse;
	cg.zoomTime = cg.time;
}
//Fixed fov at intermissions, otherwise account for fov variable and zooms.
#define	WAVE_AMPLITUDE 1
#define	WAVE_FREQUENCY .4f
static int CG_CalcFov(void){
	float	x;
	float	phase;
	float	v;
	int		contents;
	float	fov_x, fov_y;
	float	zoomFov;
	float	f;
	int		inwater;
	// if in intermission, use a fixed value
	if(cg.predictedPlayerState.pm_type == PM_INTERMISSION){fov_x = 90;}
	else{
		// user selectable
		// dmflag to prevent wide fov for all clients
		if(cgs.dmflags & DF_FIXED_FOV ){fov_x = 90;}
		else{
			fov_x = cg_fov.value;
			if(fov_x < 1){fov_x = 1;}
			else if(fov_x > 160){fov_x = 160;}
		}
		// account for zooms
		zoomFov = cg_zoomFov.value;
		if(zoomFov < 1){zoomFov = 1;}
		else if(zoomFov > 160){zoomFov = 160;}
		if(cg.zoomed ){
			f = (cg.time - cg.zoomTime) / (float)ZOOM_TIME;
			if(f > 1.f){fov_x = zoomFov;}
			else{fov_x = fov_x + f * (zoomFov - fov_x);}
		}
		else{
			f = (cg.time - cg.zoomTime ) / (float)ZOOM_TIME;
			if(f > 1.f){fov_x = fov_x;
			}
			else{fov_x = zoomFov + f * (fov_x - zoomFov);}
		}
	}
	x = cg.refdef.width / tan(fov_x / 360 * M_PI);
	fov_y = atan2(cg.refdef.height, x);
	fov_y = fov_y * 360 / M_PI;
	// warp if underwater
	contents = CG_PointContents(cg.refdef.vieworg, -1);
	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
		phase = cg.time / 1000.f * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin(phase);
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else{inwater = qfalse;}
	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;
	if(!cg.zoomed){cg.zoomSensitivity = 1;}
	else{cg.zoomSensitivity = cg.refdef.fov_y / 75.f;}
	return inwater;
}
static void CG_DamageBlendBlob(void){
	int			t;
	int			maxTime;
	refEntity_t	ent;
	// ragePro systems can't fade blends, so don't obscure the screen
	if(!cg.damageValue || cgs.glconfig.hardwareType == GLHW_RAGEPRO){return;}
	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if(t <= 0 || t >= maxTime){return;}
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;
	VectorMA(cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin);
	VectorMA(ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin);
	VectorMA(ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin);
	ent.radius = cg.damageValue * 3;
	ent.customShader = cgs.media.viewBloodShader;
	ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 200 * (1.f - ((float)t / maxTime));
	trap_R_AddRefEntityToScene(&ent);
}
//JUHOX
// fade in seconds
#if EARTHQUAKE_SYSTEM
void CG_AddEarthquake(const vec3_t origin, float radius, float duration, float fadeIn, float fadeOut, float amplitude){
	int i=0;
	// So camera dosn't make your head explode from too much shaking ...
	if(amplitude > 1500){amplitude = 1500;}
	if(duration <= 0){
		float a = amplitude / 100;
		if(radius > 0){
			float distance = Distance(cg.refdef.vieworg, origin);
			if(distance >= radius){return;}
			a *= 1 - (distance / radius);
		}
		cg.additionalTremble += a;
		return;
	}
	for(;i<MAX_EARTHQUAKES;i++){
		earthquake_t* quake;
		quake = &cg.earthquakes[i];
		if(quake->startTime){continue;}
		quake->startTime = cg.time;
		quake->endTime = (int)floor(cg.time + 1000 * duration + .5f);
		quake->fadeInTime = (int)floor(1000 * fadeIn + .5f);
		quake->fadeOutTime = (int)floor(1000 * fadeOut + .5f);
		quake->amplitude = amplitude;
		VectorCopy(origin, quake->origin);
		quake->radius = radius;
		break;
	}
}
#endif
//JUHOX
#if EARTHQUAKE_SYSTEM
void CG_AdjustEarthquakes(const vec3_t delta){
	int i=0;
	for(;i<MAX_EARTHQUAKES;i++){
		earthquake_t* quake;
		quake = &cg.earthquakes[i];
		if(!quake->startTime){continue;}
		if(quake->radius <= 0){continue;}
		VectorAdd(quake->origin, delta, quake->origin);
	}
}
#endif
//JUHOX
#if EARTHQUAKE_SYSTEM
static void AddEarthquakeTremble(earthquake_t* quake){
	int			time;
	float		a;
	const float offsetAmplitude = .2f;
	const float angleAmplitude = .2f;
	if(quake){
		if(cg.time >= quake->endTime){
			memset(quake, 0, sizeof(*quake));
			return;
		}
		if(quake->radius > 0){
			float distance;
			distance = Distance(cg.refdef.vieworg, quake->origin);
			if(distance >= quake->radius){return;}
			a = 1 - (distance / quake->radius);
		}
		else{a = 1;}
		time = cg.time - quake->startTime;
		a *= quake->amplitude / 100;
		if(time < quake->fadeInTime){a *= (float)time / (float)(quake->fadeInTime);}
		else if(cg.time > quake->endTime - quake->fadeOutTime){
			a *= (float)(quake->endTime - cg.time) / (float)(quake->fadeOutTime);
		}
	}
	else{a = cg.additionalTremble;}
	cg.refdef.vieworg[0] += offsetAmplitude * a * crandom();
	cg.refdef.vieworg[1] += offsetAmplitude * a * crandom();
	cg.refdef.vieworg[2] += offsetAmplitude * a * crandom();
	cg.refdefViewAngles[YAW] += angleAmplitude * a * crandom();
	cg.refdefViewAngles[PITCH] += angleAmplitude * a * crandom();
	cg.refdefViewAngles[ROLL] += angleAmplitude * a * crandom();
}
#endif
//Sets cg.refdef view values
static int CG_CalcViewValues(void){
	playerState_t *ps;
	memset(&cg.refdef, 0, sizeof(cg.refdef));
	CG_CalcVrect();
	ps = &cg.predictedPlayerState;
	if(ps->pm_type == PM_INTERMISSION){
		VectorCopy(ps->origin, cg.refdef.vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);		
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		return CG_CalcFov();
	}
	cg.bobcycle = (ps->bobCycle & 128)>>7;
	cg.bobfracsin = fabs(sin((ps->bobCycle & 127) / 127.f * M_PI));
	cg.xyspeed = sqrt(ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1]);
	VectorCopy(ps->origin, cg.refdef.vieworg);
	VectorCopy(ps->viewangles, cg.refdefViewAngles);
	if(cg_cameraOrbit.integer && cg.time > cg.nextOrbitTime){
			cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
	}
	if(cg_errorDecay.value > 0){
		int		t;
		float	f;
		t = cg.time - cg.predictedErrorTime;
		f = (cg_errorDecay.value - t) / cg_errorDecay.value;
		if(f > 0 && f < 1){VectorMA(cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg);}
		else{cg.predictedErrorTime = 0;}
	}
//JUHOX: add earthquakes
#if EARTHQUAKE_SYSTEM	
	{
		int i=0;

		for(;i<MAX_EARTHQUAKES;i++){
			earthquake_t* quake;
			quake = &cg.earthquakes[i];
			if(!quake->startTime){continue;}
			AddEarthquakeTremble(quake);
		}
		// additional tremble
		AddEarthquakeTremble(NULL);	
	}
#endif
	// position eye reletive to origin
	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
	if(cg.hyperspace){cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;}
	// field of view
	return CG_CalcFov();
}
static void CG_PowerupTimerSounds(void){
	int		i;
	int		t;

	// powerup timers going away
	for (i = 0 ; i < MAX_POWERUPS ; i++ ){
		t = cg.snap->ps.powerups[i];
		if(t <= cg.time ){
			continue;
		}
		if(t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ){
			continue;
		}
		if((t - cg.time ) / POWERUP_BLINK_TIME != (t - cg.oldTime ) / POWERUP_BLINK_TIME ){
			trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound);
		}
	}
}
void CG_AddBufferedSound(sfxHandle_t sfx){
	if(!sfx){return;}
	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn+1) % MAX_SOUNDBUFFER;
	if(cg.soundBufferIn == cg.soundBufferOut){cg.soundBufferOut++;}
}
static void CG_PlayBufferedSounds(void){
	if(cg.soundTime < cg.time){
		if(cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]){
			trap_S_StartLocalSound(cg.soundBuffer[cg.soundBufferOut], CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut+1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
	}
}

//Generates and draws a game scene and status information at the given time.
void CG_DrawActiveFrame(int serverTime, stereoFrame_t stereoView, qboolean demoPlayback){
	int		inwater;
	float	attenuation;
	char	var[MAX_TOKEN_CHARS];
	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;
	// update cvars
	CG_UpdateCvars();
	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if(cg.infoScreenText[0]){
		CG_DrawInformation();
		return;
	}
	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds(qfalse);
	// clear all the render lists
	trap_R_ClearScene();
	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();
	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if(!cg.snap || (cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE)){
		CG_DrawInformation();
		return;
	}
	if(cg.tierSelect > -1){
		switch(cg.tierSelectionMode){
			case 1:
				cg.tierCurrent--;
				break;
			case 2:
				cg.tierCurrent++;
				break;
			default:
				cg.tierCurrent = cg.tierSelect;
				break;
		}
	}
	// let the client system know what our weapon and zoom settings are
	trap_SetUserCmdValue(cg.weaponDesired > 0 ? cg.weaponDesired : cg.weaponSelect, cg.zoomSensitivity, cg.tierCurrent, cg.weaponSelectionMode, cg.tierSelectionMode);
	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;
	// update cg.predictedPlayerState
	CG_PredictPlayerState();
	// decide on third person view
	cg.renderingThirdPerson = cg_thirdPerson.integer || (cg.snap->ps.powerLevel[plFatigue] <= 0) || (cg.snap->ps.bitFlags & isCrashed) ||
							 (cg.snap->ps.weaponstate == WEAPON_GUIDING) || (cg.snap->ps.weaponstate == WEAPON_ALTGUIDING);
	// END ADDING
	// build cg.refdef
	inwater = CG_CalcViewValues();
//JUHOX
#if EARTHQUAKE_SYSTEM
	cg.additionalTremble = 0;
#endif
	// first person blend blobs, done after AnglesToAxis
	if(!cg.renderingThirdPerson){CG_DamageBlendBlob();}
	// build the render lists
	if(!cg.hyperspace ){
		CG_FrameHist_NextFrame();
		// adter calcViewValues, so predicted player state is correct
		CG_AddPacketEntities();
		CG_AddBeamTables();
		CG_AddTrailsToScene();
		CG_AddMarks();
		CG_AddLocalEntities();
		CG_AddParticleSystems();
	}
	// add buffered sounds
	CG_PlayBufferedSounds();
	// finish up the rest of the refdef
	cg.refdef.time = cg.time;
	memcpy(cg.refdef.areamask, cg.snap->areamask, sizeof(cg.refdef.areamask));
	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();
	// 0.0001f; // Quake 3 default was 0.0008f;
	attenuation = cg_soundAttenuation.value;
	// update audio positions
	trap_S_Respatialize(cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater, attenuation);
	if(stereoView != STEREO_RIGHT){
		cg.frametime = cg.time - cg.oldTime;
		if(cg.frametime < 0){cg.frametime = 0;}
		cg.oldTime = cg.time;
	}
	if(cg_timescale.value != cg_timescaleFadeEnd.value){
		if(cg_timescale.value < cg_timescaleFadeEnd.value){
			cg_timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if(cg_timescale.value > cg_timescaleFadeEnd.value){cg_timescale.value = cg_timescaleFadeEnd.value;}
		}
		else{
			cg_timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if(cg_timescale.value < cg_timescaleFadeEnd.value){cg_timescale.value = cg_timescaleFadeEnd.value;}
		}
		if(cg_timescaleFadeSpeed.value){trap_Cvar_Set("timescale", va("%f", cg_timescale.value));}
	}
	// actually issue the rendering calls
	CG_DrawActive();
	CG_CheckMusic();
	trap_Cvar_VariableStringBuffer("cl_paused", var, sizeof(var));
	cgs.clientPaused = atoi(var);
	if(cg_stats.integer){CG_Printf("cg.clientFrame:%i\n", cg.clientFrame);}
}
