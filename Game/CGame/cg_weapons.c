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
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"
void CG_DrawLine(vec3_t start, vec3_t end, float width, qhandle_t shader, float RGBModulate){
	vec3_t line, offset, viewLine;
	polyVert_t verts[4];
	float len;
	VectorSubtract(end, start, line);
	VectorSubtract(start, cg.refdef.vieworg, viewLine);
	CrossProduct(viewLine, line, offset);
	len = VectorNormalize(offset);
	if(!len){return;}
	VectorMA(end, -width, offset, verts[0].xyz);
	verts[0].st[0] = 1;
	verts[0].st[1] = 0;
	VectorMA(end, width, offset, verts[1].xyz);
	verts[1].st[0] = 0;
	verts[1].st[1] = 0;
	VectorMA(start, width, offset, verts[2].xyz);
	verts[2].st[0] = 0;
	verts[2].st[1] = 1;
	VectorMA(start, -width, offset, verts[3].xyz);
	verts[3].st[0] = 1;
	verts[3].st[1] = 1;
	for(int vertex;vertex<4;vertex++){
		for(int channel;channel<4;channel++){
			verts[vertex].modulate[channel] = 255 * RGBModulate;
		}
	}
	trap_R_AddPolyToScene(shader, 4, verts);
}
void CG_DrawLineRGBA(vec3_t start, vec3_t end, float width, qhandle_t shader, vec4_t RGBA){
	vec3_t line, offset, viewLine;
	polyVert_t verts[4];
	float len;
	VectorSubtract(end, start, line);
	VectorSubtract(start, cg.refdef.vieworg, viewLine);
	CrossProduct(viewLine, line, offset);
	len = VectorNormalize(offset);
	if(!len){return;}
	VectorMA(end, -width, offset, verts[0].xyz);
	verts[0].st[0] = 1;
	verts[0].st[1] = 0;
	VectorMA(end, width, offset, verts[1].xyz);
	verts[1].st[0] = 0;
	verts[1].st[1] = 0;
	VectorMA(start, width, offset, verts[2].xyz);
	verts[2].st[0] = 0;
	verts[2].st[1] = 1;
	VectorMA(start, -width, offset, verts[3].xyz);
	verts[3].st[0] = 1;
	verts[3].st[1] = 1;
	for(int vertex;vertex<4;vertex++){
		for(int channel;channel<4;channel++){verts[vertex].modulate[channel] = RGBA[channel];}
	}
	trap_R_AddPolyToScene(shader, 4, verts);
}
/*================
VIEW WEAPON
================*/
//JUHOX
void CG_Draw3DLine(const vec3_t start, const vec3_t end, qhandle_t shader) {
	refEntity_t line;
	//if(DistanceSquared(start, end) < 10*10) return;	
	memset(&line, 0, sizeof(line));
	line.reType = RT_LIGHTNING;
	line.customShader = shader;
	VectorCopy(start, line.origin);
	VectorCopy(end, line.oldorigin);
	trap_R_AddRefEntityToScene(&line);
}

/*
Used for both the primary and the alternate fire of weapons.
Used case depends on input.
*/
static void CG_AddPlayerWeaponCharge(refEntity_t *parent, cg_userWeapon_t *weaponGraphics, refEntity_t *charge, float chargeLvl){
	float chargeScale;
	float chargeDlightScale;
	// Obtain the scale the charge must have.
	if(weaponGraphics->chargeGrowth){
		// above the end, we use the highest form
		if(weaponGraphics->chargeEndPct <= chargeLvl){
			chargeScale = weaponGraphics->chargeEndsize;
			chargeDlightScale = weaponGraphics->chargeDlightEndRadius;
		}
		else{
			// inbetween start and end, we work out the value
			float PctRange = weaponGraphics->chargeEndPct - weaponGraphics->chargeStartPct;
			float PctVal = chargeLvl - weaponGraphics->chargeStartPct;
			float SizeRange = weaponGraphics->chargeEndsize - weaponGraphics->chargeStartsize;
			float SizeVal = (PctVal / PctRange) * SizeRange;
			chargeScale = SizeVal + weaponGraphics->chargeStartsize;
			SizeRange = weaponGraphics->chargeDlightEndRadius - weaponGraphics->chargeDlightStartRadius;
			SizeVal = (PctVal / PctRange) * SizeRange;
			chargeDlightScale = SizeVal + weaponGraphics->chargeDlightStartRadius;
		}
	}
	else{
		chargeScale = weaponGraphics->chargeStartsize;
		chargeDlightScale = weaponGraphics->chargeDlightStartRadius;
	}
	// Add the charge model or sprite
	VectorCopy(parent->lightingOrigin, charge->lightingOrigin);
	charge->shadowPlane = parent->shadowPlane;
	charge->renderfx = parent->renderfx;
	if(!(weaponGraphics->chargeModel && weaponGraphics->chargeSkin)){
		charge->reType = RT_SPRITE;
		charge->radius = 4 * chargeScale;
		charge->rotation = 0;
		charge->customShader = weaponGraphics->chargeShader;
	}
	else{
		charge->reType = RT_MODEL;
		charge->hModel = weaponGraphics->chargeModel;
		charge->customSkin = weaponGraphics->chargeSkin;
		charge->nonNormalizedAxes = qtrue;
		VectorScale(charge->axis[0], chargeScale, charge->axis[0]);
		VectorScale(charge->axis[1], chargeScale, charge->axis[1]);
		VectorScale(charge->axis[2], chargeScale, charge->axis[2]);
		if(cg_drawBBox.value){
			vec3_t	mins,maxs;
			trap_R_ModelBounds(charge->hModel, mins, maxs, charge->frame);
			VectorScale(mins, chargeScale, mins);
			VectorScale(maxs, chargeScale, maxs);
			CG_DrawBoundingBox(charge->origin, mins, maxs);
		}
	}
	trap_R_AddRefEntityToScene(charge);
	// add dynamic light
	if(chargeDlightScale){trap_R_AddLightToScene(charge->origin, 100*chargeDlightScale, weaponGraphics->chargeDlightColor[0], weaponGraphics->chargeDlightColor[1], weaponGraphics->chargeDlightColor[2]);}
}
/*
Used for both the primary and the alternate fire of weapons.
Used case depends on input.
*/
static void CG_AddPlayerWeaponChargeVoices(centity_t *player, cg_userWeapon_t *weaponGraphics, float curChargeLvl, float prevChargeLvl){
	chargeVoice_t *voice;
	for(int index=MAX_CHARGE_VOICES-1;index>=0;index--){
		voice = &weaponGraphics->chargeVoice[index];
		// no sound; no complex boolean eval
		if(voice->voice){
			if((voice->startPct > prevChargeLvl) && (voice->startPct < curChargeLvl) && (prevChargeLvl < curChargeLvl)){
				trap_S_StartSound(NULL , player->currentState.number, CHAN_VOICE, voice->voice);
				break;
			}
		}
	}
}

/*
Used for both the primary and the alternate fire of weapons.
Used case depends on input.
*/
static void CG_AddPlayerWeaponFlash(refEntity_t *parent, cg_userWeapon_t *weaponGraphics, refEntity_t *flash, int flashPowerLevelTotal, int flashPowerLevelCurrent){
	float flashScale;
	float radiusScale;
	if(flashPowerLevelCurrent >= (flashPowerLevelTotal*2)){flashPowerLevelCurrent = flashPowerLevelTotal*2;}
	radiusScale = (float)flashPowerLevelCurrent / (float)flashPowerLevelTotal;
	flashScale = weaponGraphics->flashSize * radiusScale;
	Com_Clamp(0.0f,1.0f,radiusScale);
	// Add the charge model or sprite
	VectorCopy(parent->lightingOrigin, flash->lightingOrigin);
	flash->shadowPlane = parent->shadowPlane;
	flash->renderfx = parent->renderfx;
	if(!(weaponGraphics->flashModel && weaponGraphics->flashSkin)){
		flash->reType = RT_SPRITE;
		flash->radius = 4 * flashScale;
		flash->rotation = 0;
		flash->customShader = weaponGraphics->flashShader;
	}
	else{
		flash->reType = RT_MODEL;
		flash->hModel = weaponGraphics->flashModel;
		flash->customSkin = weaponGraphics->flashSkin;
		flash->nonNormalizedAxes = qtrue;
		VectorScale(flash->axis[0], flashScale, flash->axis[0]);
		VectorScale(flash->axis[1], flashScale, flash->axis[1]);
		VectorScale(flash->axis[2], flashScale, flash->axis[2]);
		if(cg_drawBBox.value){
			vec3_t	mins,maxs;
			trap_R_ModelBounds(flash->hModel, mins, maxs, flash->frame);
			VectorScale(mins, flashScale, mins);
			VectorScale(maxs, flashScale, maxs);
			CG_DrawBoundingBox(flash->origin, mins, maxs);
		}
	}
	trap_R_AddRefEntityToScene(flash);
	// add dynamic light
	if(weaponGraphics->flashDlightRadius){trap_R_AddLightToScene(flash->origin, 100*weaponGraphics->flashDlightRadius, weaponGraphics->flashDlightColor[0],weaponGraphics->flashDlightColor[1], weaponGraphics->flashDlightColor[2]);}
}
void CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent, int team){
	cg_userWeapon_t		*weaponGraphics;
	orientation_t		orient;
	refEntity_t			refEnt;
	entityState_t		*ent;
	float				lerp;
	float				backLerp;
	int					newLerp;
	weaponstate_t		weaponState;
	// Set some shorthands
	ent = &cent->currentState;
	weaponState = ent->weaponstate;
	// Any of the charging states
	if(weaponState == WEAPON_CHARGING || weaponState == WEAPON_ALTCHARGING){
		// Set properties depending on primary or alternate fire
		if(weaponState == WEAPON_CHARGING){
			weaponGraphics = CG_FindUserWeaponGraphics(ent->clientNum, ent->weapon);
			newLerp = ent->charge1.chBase;
			backLerp = cent->lerpPrim;
			cent->lerpPrim = newLerp < backLerp ? newLerp : (cent->lerpPrim + newLerp) / 2.0f;
			lerp = cent->lerpPrim;
			cent->lerpSec = 0;
		}
		else{
			weaponGraphics = CG_FindUserWeaponGraphics(ent->clientNum, ent->weapon + ALTWEAPON_OFFSET);
			newLerp = ent->charge2.chBase;
			backLerp = cent->lerpSec;
			cent->lerpSec = newLerp < backLerp ? newLerp : (cent->lerpSec + newLerp) / 2.0f;
			lerp = cent->lerpSec;
			cent->lerpPrim = 0;
		}
		// Only bother with anything else if the charge in question is larger than the minimum used for display
		if(lerp > weaponGraphics->chargeStartPct){
			// Locate a tag wherever on the model's parts. Don't process further if the tag is not found
			if(CG_TryLerpPlayerTag(cent,weaponGraphics->chargeTag[0],&orient)){
				memset(&refEnt, 0, sizeof(refEnt));
				if(VectorLength(weaponGraphics->chargeSpin) != 0.f){
					vec3_t	tempAngles, lerpAxis[3], tempAxis[3];
					VectorCopy(orient.origin, refEnt.origin);
					VectorSet(tempAngles, cg.time / 4.f, cg.time / 4.f, cg.time / 4.f);
					VectorPieceWiseMultiply(tempAngles, weaponGraphics->chargeSpin, tempAngles);
					AnglesToAxis(tempAngles, lerpAxis);
					AxisClear(refEnt.axis);
					MatrixMultiply(refEnt.axis, lerpAxis, tempAxis);
					MatrixMultiply(tempAxis, orient.axis, refEnt.axis);
				}
				else{
					VectorCopy(orient.origin, refEnt.origin);
					AxisCopy(orient.axis, refEnt.axis);
				}
				CG_AddPlayerWeaponCharge(parent, weaponGraphics, &refEnt, lerp);
			}
		}
		if(weaponGraphics->chargeLoopSound){trap_S_AddLoopingSound(ent->number, cent->lerpOrigin, vec3_origin, weaponGraphics->chargeLoopSound);}
		CG_AddPlayerWeaponChargeVoices(cent, weaponGraphics, lerp, backLerp);
		// Set up any charging particle systems
		if(weaponGraphics->chargeParticleSystem[0]){
			// If the entity wasn't previously in the PVS, if the weapon nr switched, or if the weaponstate switched
			// we need to start a new system
			if(!CG_FrameHist_WasInPVS(ent->number) || CG_FrameHist_IsWeaponNr(ent->number) != CG_FrameHist_WasWeaponNr(ent->number) || CG_FrameHist_IsWeaponState(ent->number) != CG_FrameHist_WasWeaponState(ent->number)){
				PSys_SpawnCachedSystem(weaponGraphics->chargeParticleSystem, cent->lerpOrigin, NULL, cent, weaponGraphics->chargeTag[0], qfalse, qtrue);
			}
		}
	}
	// Any of the firing or guiding states
	else if(weaponState == WEAPON_GUIDING || weaponState == WEAPON_ALTGUIDING || weaponState == WEAPON_FIRING || weaponState == WEAPON_ALTFIRING){ 
		// Set properties depending on primary or alternate fire
		if(weaponState == WEAPON_GUIDING || weaponState == WEAPON_FIRING){weaponGraphics = CG_FindUserWeaponGraphics(ent->clientNum, ent->weapon);}
		else{weaponGraphics = CG_FindUserWeaponGraphics(ent->clientNum, ent->weapon + ALTWEAPON_OFFSET);}
		// Locate a tag wherever on the model's parts. Don't process further if the tag is not found
		if(CG_TryLerpPlayerTag(cent,weaponGraphics->chargeTag[0],&orient)){
			memset(&refEnt, 0, sizeof(refEnt));
			VectorCopy(orient.origin, refEnt.origin);
			AxisCopy(orient.axis, refEnt.axis);
			CG_AddPlayerWeaponFlash(parent, weaponGraphics, &refEnt, ent->attackPowerTotal, ent->attackPowerCurrent);
		}
		if(weaponGraphics->firingSound){trap_S_AddLoopingSound(ent->number, cent->lerpOrigin, vec3_origin, weaponGraphics->firingSound);}
		// Set up any firing particle systems
		if(weaponGraphics->firingParticleSystem[0]){
			// If the entity wasn't previously in the PVS, if the weapon nr switched, or if the weaponstate switched, we need to start a new system
			if(!CG_FrameHist_WasInPVS(ent->number) || CG_FrameHist_IsWeaponNr(ent->number) != CG_FrameHist_WasWeaponNr(ent->number) || CG_FrameHist_IsWeaponState(ent->number) != CG_FrameHist_WasWeaponState(ent->number)){
				PSys_SpawnCachedSystem(weaponGraphics->firingParticleSystem, cent->lerpOrigin, NULL, cent, weaponGraphics->chargeTag[0], qfalse, qtrue);
			}
		}
	}
}

/*
=======================
WEAPON SELECTION
=======================
*/
static void CG_DrawWeaponSelectHorCenterBar(void){
	int		i = 1;
	int		bits;
	int		count = 0;
	int		x, y, w;
	// Need a little more leeway
	char	name[MAX_WEAPONNAME*2+4]; 
	float	*color;
	cg_userWeapon_t	*weaponInfo;
	cg_userWeapon_t	*alt_weaponInfo;
	color = CG_FadeColor(cg.weaponSelectTime, WEAPON_SELECT_TIME, 200);
	if(!color){
		cg.drawWeaponBar = 0;
		return;
	}
	trap_R_SetColor(color);
	// count the number of weapons owned
	bits = cg.snap->ps.stats[stSkills];
	for(;i<16;i++){
		if(bits & (1<<i)){count++;}
	}
	x = 164 - count * 15;
	y = 418;
	for (i=1;i<16;i++){
		qboolean usable = qfalse;
		if(!(bits & (1<<i))){continue;}
		weaponInfo = CG_FindUserWeaponGraphics(cg.snap->ps.clientNum,i);
		// draw weapon icon
		if(i == 1 && cg.snap->ps.powerups[PW_SKILLS] & USABLE_SKILL1){usable = qtrue;}
		if(i == 2 && cg.snap->ps.powerups[PW_SKILLS] & USABLE_SKILL2){usable = qtrue;}
		if(i == 3 && cg.snap->ps.powerups[PW_SKILLS] & USABLE_SKILL3){usable = qtrue;}
		if(i == 4 && cg.snap->ps.powerups[PW_SKILLS] & USABLE_SKILL4){usable = qtrue;}
		if(i == 5 && cg.snap->ps.powerups[PW_SKILLS] & USABLE_SKILL5){usable = qtrue;}
		if(i == 6 && cg.snap->ps.powerups[PW_SKILLS] & USABLE_SKILL6){usable = qtrue;}
		if(!usable){continue;}
		CG_DrawPic(qfalse, x, y, 24, 24, weaponInfo->weaponIcon);
		if(i == cg.weaponSelect){CG_DrawPic(qfalse, x-4, y-4, 32, 32, cgs.media.selectShader);}
		x += 30;
	}
	weaponInfo = CG_FindUserWeaponGraphics(cg.snap->ps.clientNum, cg.weaponSelect);
	alt_weaponInfo = CG_FindUserWeaponGraphics(cg.snap->ps.clientNum, ALTWEAPON_OFFSET + cg.weaponSelect);
	// draw the selected name
	if(weaponInfo->weaponName){
		if(alt_weaponInfo->weaponName){Com_sprintf(name, sizeof(name), "%s / %s", weaponInfo->weaponName, alt_weaponInfo->weaponName);}
		else{Com_sprintf(name, sizeof(name), "%s", weaponInfo->weaponName);}
		w = CG_DrawStrlen(name) * MEDIUMCHAR_WIDTH;
		x = (SCREEN_WIDTH-w)/2;
		//CG_DrawSmallStringColor(6, y - 30, name, color);
	}
	trap_R_SetColor(NULL);
}
void CG_DrawWeaponSelect(void){
	cg.itemPickupTime = 0;
	if(cg.drawWeaponBar == 1){
		CG_DrawWeaponSelectHorCenterBar();
	}
}
void CG_NextWeapon_f(void){
	if(!cg.snap || cg.snap->ps.bitFlags & usingMelee || (cg.snap->ps.pm_flags & PMF_FOLLOW)){return;}
	cg.weaponSelectionMode = 2;
	cg.weaponSelectTime = cg.time;
}
void CG_PrevWeapon_f(void){
	if (!cg.snap || cg.snap->ps.bitFlags & usingMelee || (cg.snap->ps.pm_flags & PMF_FOLLOW)){return;}
	cg.weaponSelectionMode = 1;
	cg.weaponSelectTime = cg.time;
}
void CG_Weapon_f(void){
	int num;
	if(!cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW)){return;}
	num = atoi(CG_Argv(1));
	if(num < 1 || num > 15){return;}
	cg.weaponSelectTime = cg.time;
	cg.weaponDesired = num;
	cg.weaponSelectionMode = 3;
}
/*
=====================
WEAPON EVENTS
=====================
*/
//Caused by an EV_FIRE_WEAPON event
void CG_FireWeapon(centity_t *cent, qboolean altFire){
	cg_userWeapon_t		*weaponGraphics;
	entityState_t		*ent;
	int					c = 0, weapNr;
	ent = &cent->currentState;
	if(ent->weapon == WP_NONE){return;}
	// Set the muzzle flash weapon Nr
	if(altFire){weapNr = ent->weapon + ALTWEAPON_OFFSET;}
	else{weapNr = ent->weapon;}
	weaponGraphics = CG_FindUserWeaponGraphics(ent->clientNum, weapNr);
	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model.
	cent->muzzleFlashTime = cg.time;
	if(weaponGraphics->flashParticleSystem[0]){PSys_SpawnCachedSystem(weaponGraphics->flashParticleSystem, cent->lerpOrigin, NULL, cent, weaponGraphics->chargeTag[0], qfalse, qfalse);}
	// play a sound
	for(;c<4;c++){
		if(weaponGraphics->flashSound[c] == 0){break;}
	}
	if(c>0){
		c = rand()%c;
		if(weaponGraphics->flashSound[c]){trap_S_StartSound(NULL , ent->number, CHAN_WEAPON, weaponGraphics->flashSound[c]);}
	}
	for(c=0;c<4;c++){
		if(weaponGraphics->voiceSound[c] == 0){break;}
	}
	if(c > 0){
		c = rand()%c;
		if (weaponGraphics->voiceSound[c]){trap_S_StartSound(NULL , ent->number, CHAN_WEAPON, weaponGraphics->voiceSound[c]);}
	}
}
//Caused by an EV_MISSILE_MISS event
void CG_UserMissileHitWall(int weapon, int clientNum, int powerups, int number, vec3_t origin, vec3_t dir, qboolean inAir){
	cg_userWeapon_t *weaponGraphics;
	vec3_t end;
	trace_t tr;
	//qhandle_t mark;
	int					c = 0;
	weaponGraphics = CG_FindUserWeaponGraphics(clientNum, weapon);
	//mark = cgs.media.burnMarkShader;
	// play an explosion sound
	for(;c<4;c++){
		if(!weaponGraphics->explosionSound[c]){break;}
	}
	if(c > 0){
		c = rand()%c;
		if(weaponGraphics->explosionSound[c]){trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, weaponGraphics->explosionSound[c]);}
	}
	// Create Explosion
	CG_MakeUserExplosion(origin, dir, weaponGraphics, powerups, number);
	if(!inAir){
		vec3_t tempAxis[3];
		VectorNormalize2(dir, tempAxis[0]);
		MakeNormalVectors(tempAxis[0], tempAxis[1], tempAxis[2]);
		VectorCopy(origin, end);
		end[2] -= 64;
		CG_Trace(&tr, origin, NULL, NULL, end, -1, MASK_PLAYERSOLID);
		if(!weaponGraphics->noRockDebris){
			if(cg_particlesQuality.value == 2) {
				if(weaponGraphics->explosionSize <= 10){PSys_SpawnCachedSystem("SmallExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse);}
				else if(weaponGraphics->explosionSize <= 25){PSys_SpawnCachedSystem("NormalExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse);}
				else if(weaponGraphics->explosionSize <= 50){PSys_SpawnCachedSystem("LargeExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse);}
				else{PSys_SpawnCachedSystem("ExtraLargeExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse);}
			}
			else if(cg_particlesQuality.value == 1){
				if(weaponGraphics->explosionSize <= 10){PSys_SpawnCachedSystem("SmallExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse);}
				else if(weaponGraphics->explosionSize <= 25){PSys_SpawnCachedSystem("NormalExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse);}
				else if(weaponGraphics->explosionSize <= 50){PSys_SpawnCachedSystem("LargeExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse);}
				else{PSys_SpawnCachedSystem("ExtraLargeExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse);}
			}
		}
		// Draw Impactmark
		if(weaponGraphics->markSize && weaponGraphics->markShader){CG_ImpactMark(weaponGraphics->markShader, origin, dir, random()*360, 1,1,1,1, qfalse,60, qfalse);}
	}
/* NOTE: Find another way of doing this in the new system...
	if ( weaponGraphics->missileTrailFunc == CG_TrailFunc_StraightBeam ||
		 weaponGraphics->missileTrailFunc == CG_TrailFunc_SpiralBeam ) {
		CG_CreateStraightBeamFade( cgs.clientinfo[ clientNum ].weaponTagPos0, origin, weaponGraphics);
	}
*/
}
void CG_UserMissileHitPlayer(int weapon, int clientNum, int powerups, int number, vec3_t origin, vec3_t dir, int entityNum){
	CG_UserMissileHitWall(weapon, clientNum, powerups, number, origin, dir, qtrue);
}
