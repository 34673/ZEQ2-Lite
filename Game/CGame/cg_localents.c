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
// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.
#include "cg_local.h"
#define	MAX_LOCAL_ENTITIES	8192
localEntity_t	cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t	cg_activeLocalEntities;		// double linked list
localEntity_t*	cg_freeLocalEntities;		// single linked list
/*
===================
CG_InitLocalEntities

This is called at startup and for tournement restarts
===================
*/
void CG_InitLocalEntities(void){
	memset(cg_localEntities, 0, sizeof(cg_localEntities));
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	for(int i=0;i<MAX_LOCAL_ENTITIES-1;i++){
		cg_localEntities[i].next = &cg_localEntities[i+1];
	}
}

/*
==================
CG_FreeLocalEntity
==================
*/
void CG_FreeLocalEntity(localEntity_t *le){
	if(!le->prev){CG_Error("CG_FreeLocalEntity: inactive");}
	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;
	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
===================
CG_AllocLocalEntity

Will allways succeed, even if it requires freeing an old active entity
===================
*/
localEntity_t* CG_AllocLocalEntity(void){
	localEntity_t* le;

	// no free entities, so free the one at the end of the chain
	// remove the oldest active entity
	if(!cg_freeLocalEntities)
		CG_FreeLocalEntity(cg_activeLocalEntities.prev);
	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;
	memset(le, 0, sizeof(*le));
	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	return le;
}

/*
=====================================================================

TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/
void CG_AddFadeNo(localEntity_t *le){
	refEntity_t *re;
	re = &le->refEntity;
	re->shaderRGBA[0] = le->color[0] * 255;
	re->shaderRGBA[1] = le->color[1] * 255;
	re->shaderRGBA[2] = le->color[2] * 255;
	trap_R_AddRefEntityToScene(re);
	// add the dlight
	if(le->light){
		float light;

		light = (float)(cg.time - le->startTime) / (le->endTime - le->startTime);
		light = light < 0.5f ? 1.0f : 1.0f - (light -0.5f) * 2;
		light = le->light * light;
		trap_R_AddLightToScene(re->origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2]);
	}
}
void CG_AddFadeRGB(localEntity_t *le){
	refEntity_t* re;
	float		c;

	re = &le->refEntity;
	c = (le->endTime - cg.time) * le->lifeRate;
	c *= 255;
	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;
	trap_R_AddRefEntityToScene(re);
}
void CG_AddFadeAlpha(localEntity_t *le){
	refEntity_t* re;
	float		c;

	re = &le->refEntity;
	c = (le->endTime - cg.time) * le->lifeRate;
	c *= 255;
	re->shaderRGBA[3] = le->color[3] * c;
	trap_R_AddRefEntityToScene(re);
}
static void CG_AddMoveScaleFade(localEntity_t *le){
	refEntity_t* re;
	float		c;
	float		len;
	vec3_t		delta;
	re = &le->refEntity;
	// fade / grow time
	if(le->fadeInTime > le->startTime && cg.time < le->fadeInTime){
		c = 1.f -(float)(le->fadeInTime -cg.time) / (le->fadeInTime -le->startTime);
	}
	else{
		c = (le->endTime - cg.time) * le->lifeRate;
	}
	re->shaderRGBA[3] = 255 * c * le->color[3];
	if(!(le->leFlags & LEF_PUFF_DONT_SCALE)){
		re->radius = le->radius * (1.0f - c) + 8;
	}
	BG_EvaluateTrajectory(NULL,&le->pos,cg.time,re->origin);
	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract(re->origin, cg.refdef.vieworg, delta);
	len = VectorLength(delta);
	if(len < le->radius){
		CG_FreeLocalEntity(le);
		return;
	}
	trap_R_AddRefEntityToScene(re);
}
/*
===================
CG_AddScaleFade

For rocket smokes that hang in place, fade out, and are
removed if the view passes through them.
There are often many of these, so it needs to be simple.
===================
*/
static void CG_AddScaleFade(localEntity_t *le){
	refEntity_t	*re;
	float		c, len;
	vec3_t		delta;

	re = &le->refEntity;
	// fade / grow time
	c = (le->endTime - cg.time) * le->lifeRate;
	re->shaderRGBA[3] = 255 * c * le->color[3];
	re->radius = le->radius * (1.0f - c) + 8;
	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract(re->origin,cg.refdef.vieworg,delta);
	len = VectorLength(delta);
	if(len < le->radius){
		CG_FreeLocalEntity(le);
		return;
	}
	trap_R_AddRefEntityToScene(re);
}

static void CG_AddScaleFadeRGB(localEntity_t* le){
	refEntity_t	*re;
	float		c;
	float		len;
	vec3_t		delta;

	re = &le->refEntity;
	// fade / grow time
	c = (le->endTime - cg.time) * le->lifeRate;
	re->shaderRGBA[0] = 255 * c * le->color[0];
	re->shaderRGBA[1] = 255 * c * le->color[1];
	re->shaderRGBA[2] = 255 * c * le->color[2];
	re->radius = le->radius * (1.0f - c) + 8;
	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract(re->origin,cg.refdef.vieworg,delta);
	len = VectorLength(delta);
	if(len < le->radius){
		CG_FreeLocalEntity(le);
		return;
	}
	trap_R_AddRefEntityToScene(re);
}

/*
=================
CG_AddFallScaleFade

This is just an optimized CG_AddMoveScaleFade
For blood mists that drift down, fade out, and are
removed if the view passes through them.
There are often 100+ of these, so it needs to be simple.
=================
*/
static void CG_AddFallScaleFade(localEntity_t* le){
	refEntity_t* re;
	float		c;
	float		len;
	vec3_t		delta;

	re = &le->refEntity;
	// fade time
	c = (le->endTime - cg.time) * le->lifeRate;
	re->shaderRGBA[3] = 255 * c *le->color[3];
	re->origin[2] = le->pos.trBase[2] - (1.0f - c) * le->pos.trDelta[2];
	re->radius = le->radius * (1.0f - c) + 16;
	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract(re->origin,cg.refdef.vieworg,delta);
	len = VectorLength(delta);
	if(len < le->radius){
		CG_FreeLocalEntity(le);
		return;
	}
	trap_R_AddRefEntityToScene(re);
}
static void CG_AddExplosion(localEntity_t* ex){
	refEntity_t* ent;
	ent = &ex->refEntity;
	// add the entity
	trap_R_AddRefEntityToScene(ent);
	// add the dlight
	if(ex->light){
		float light = (float)(cg.time - ex->startTime) / (ex->endTime - ex->startTime);
		light = light < 0.5f ? 1.0f : 1.0f - (light - 0.5f) * 2;
		light = ex->light * light;
		trap_R_AddLightToScene(ent->origin,light,ex->lightColor[0],ex->lightColor[1],ex->lightColor[2]);
	}
}
static void CG_AddStraightBeamFade(localEntity_t *le){
	refEntity_t	*ent;		// reference entity stored in the local entity
	float		RGBfade;	// stores the amount of fade to apply to the RGBA values
	float		scale_l;	// stores the scale factor for the beam's length
	float		scale_w;	// stores the scale factor for the beam's width
	vec3_t		start;		// temporary storage for the beam's start point
	vec3_t		direction;	// vector used in calculating the shortening of the beam
	
	// set up for quick reference
	ent = &le->refEntity;
	// Save the start vector so it can be recovered after having been rendered.
	VectorCopy(ent->origin, start);
	// calculate RGBfade and scale
	RGBfade = 1 - (float)(cg.time - le->startTime) / (le->endTime - le->startTime);
	scale_l = RGBfade;
	scale_w = 1 - (float)(cg.time - le->startTime) / (le->endTime - le->startTime);
	if(scale_w < 0){scale_w = 0;}
	// Set the scaled beam
	VectorSubtract(ent->origin, ent->oldorigin, direction);
	VectorScale(direction, scale_l, direction);
	VectorAdd(ent->oldorigin, direction, ent->origin);
	CG_DrawLine(ent->origin,ent->oldorigin,le->radius * scale_w,ent->customShader,RGBfade);
	// Restore the start vector
	VectorCopy(start, ent->origin);
}
static void CG_AddZEQExplosion(localEntity_t *le){
	refEntity_t* ent;
	float c;
	float phase;
	float RGBfade;
	vec3_t tmpAxes[3];

	ent = &le->refEntity;
	RGBfade = (float)(cg.time - le->startTime) / (le->endTime - le->startTime);
	RGBfade = RGBfade < 0.5f ? 1.0f : 1.0f - (RGBfade - 0.5f) * 2;
	ent->shaderRGBA[0] = 255 * RGBfade;
	ent->shaderRGBA[1] = 255 * RGBfade;
	ent->shaderRGBA[2] = 255 * RGBfade;
	ent->shaderRGBA[3] = 255 * RGBfade;
	// grow time
	phase = (float)(cg.time - le->startTime) / (le->endTime - le->startTime) * M_PI / 2.0f;
	c = sin(phase) + 1.0f;
	// preserve the full scale
	VectorCopy(ent->axis[0],tmpAxes[0]);
	VectorCopy(ent->axis[1],tmpAxes[1]);
	VectorCopy(ent->axis[2],tmpAxes[2]);
	// set the current scale
	VectorScale(ent->axis[0], 1-c, ent->axis[0]);
	VectorScale(ent->axis[1], 1-c, ent->axis[1]);
	VectorScale(ent->axis[2], 1-c, ent->axis[2]);
	if(cg_drawBBox.value){
		vec3_t	mins, maxs;
		
		trap_R_ModelBounds(ent->hModel, mins, maxs, ent->frame);
		VectorScale(mins, ent->radius, mins);
		VectorScale(maxs, ent->radius, maxs);
		VectorScale(mins, 1-c, mins);
		VectorScale(maxs, 1-c, maxs);
		CG_DrawBoundingBox(ent->origin, mins, maxs);
	}
	// add the entity
	trap_R_AddRefEntityToScene(ent);
	// set the full scale again
	VectorCopy(tmpAxes[0], ent->axis[0]);
	VectorCopy(tmpAxes[1], ent->axis[1]);
	VectorCopy(tmpAxes[2], ent->axis[2]);
	// add a Dlight
	if(le->light){
		float light, lightRad;

		light = (float)(cg.time - le->startTime) / (le->endTime - le->startTime);
		light = light < 0.5f ? light * 2 : 1.0f - (light - 0.5f) * 2;
		lightRad = le->light * light;
		trap_R_AddLightToScene(ent->origin, lightRad, light * le->lightColor[0], light * le->lightColor[1], light * le->lightColor[2]);
	}
}
static void CG_AddZEQSplash( localEntity_t *le ) {
	refEntity_t	*ent;
	float		c, RGBfade;
	vec3_t		tmpAxes[3];
	
	ent = &le->refEntity;
	RGBfade = (float)(cg.time - le->startTime) / (le->endTime - le->startTime);
	RGBfade = RGBfade < 0.5f ? 1.0f : 1.0f - (RGBfade - 0.5f) * 2;
	ent->shaderRGBA[3] = 255 * RGBfade;
	// grow time
	c = (le->endTime - cg.time) * le->lifeRate / 1.1f;
	// preserve the full scale
	VectorCopy(ent->axis[0], tmpAxes[0]);
	VectorCopy(ent->axis[1], tmpAxes[1]);
	VectorCopy(ent->axis[2], tmpAxes[2]);
	// set the current scale
	VectorScale(ent->axis[0], 1-c, ent->axis[0]);
	VectorScale(ent->axis[1], 1-c, ent->axis[1]);
	VectorScale(ent->axis[2], 1-c, ent->axis[2]);
	// add the entity
	trap_R_AddRefEntityToScene(ent);
	// set the full scale again
	VectorCopy(tmpAxes[0], ent->axis[0]);
	VectorCopy(tmpAxes[1], ent->axis[1]);
	VectorCopy(tmpAxes[2], ent->axis[2]);
}
static void CG_AddSpriteExplosion( localEntity_t *le ) {
	refEntity_t	re;
	float		c;

	re = le->refEntity;
	c = (le->endTime - cg.time) / (float)(le->endTime - le->startTime);
	// can happen during connection problems
	if(c > 1){c = 1.0f;}
	re.shaderRGBA[0] = 255;
	re.shaderRGBA[1] = 255;
	re.shaderRGBA[2] = 255;
	re.shaderRGBA[3] = 255 * c * 0.33f;
	re.reType = RT_SPRITE;
	re.radius = 42 * (1.0f - c) + 30;
	trap_R_AddRefEntityToScene(&re);
	// add the dlight
	if(le->light){
		float light;

		light = (float)(cg.time - le->startTime) / (le->endTime - le->startTime);
		light = light < 0.5f ? 1.0f : 1.0f - (light - 0.5f) * 2;
		light = le->light * light;
		trap_R_AddLightToScene(re.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2]);
	}
}
void CG_AddLocalEntities( void ) {
	localEntity_t *le, *next;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;
	for(;le!=&cg_activeLocalEntities;le=next){
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;
		if(cg.time >= le->endTime){
			CG_FreeLocalEntity(le);
			continue;
		}
		switch(le->leType){
		case LE_MARK:
			break;
		case LE_SPRITE_EXPLOSION:
			CG_AddSpriteExplosion(le);
			break;
		case LE_EXPLOSION:
			CG_AddExplosion(le);
			break;
		case LE_ZEQEXPLOSION:
			if(cg.time >= le->startTime)
				CG_AddZEQExplosion(le);
			break;
		case LE_ZEQSMOKE:
			if(cg.time >= le->startTime)
				CG_AddMoveScaleFade(le);
			break;
		case LE_ZEQSPLASH:
			if(cg.time >= le->startTime)
				CG_AddZEQSplash(le);
			break;
		case LE_STRAIGHTBEAM_FADE:
			CG_AddStraightBeamFade(le);
			break;
		case LE_MOVE_SCALE_FADE:		// water bubbles
			CG_AddMoveScaleFade(le);
			break;
		case LE_FADE_RGB:				// teleporters, railtrails
			CG_AddFadeRGB(le);
			break;
		case LE_FADE_ALPHA:				// teleporters, railtrails
			CG_AddFadeAlpha(le);
			break;
		case LE_FALL_SCALE_FADE:		// gib blood trails
			CG_AddFallScaleFade(le);
			break;
		case LE_SCALE_FADE:				// rocket trails
			CG_AddScaleFade(le);
			break;
		case LE_SCALE_FADE_RGB:			// rocket trails
			CG_AddScaleFadeRGB(le);
			break;
		case LE_FADE_NO:				// teleporters, railtrails
			CG_AddFadeNo(le);
			break;
		default:
			CG_Error("Invalid leType '%i'.", le->leType);
			return;
		}
	}
}
