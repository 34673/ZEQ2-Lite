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
// cg_marks.c -- wall marks
#include "cg_local.h"

/*
===================================================================

MARK POLYS

===================================================================
*/

markPoly_t	cg_activeMarkPolys,			// double linked list
			*cg_freeMarkPolys,			// single linked list
			cg_markPolys[MAX_MARK_POLYS];
static int	markTotal;

/*
===================
CG_InitMarkPolys

This is called at startup and for tournement restarts
===================
*/
void CG_InitMarkPolys(void){
	int	i;

	memset(cg_markPolys, 0, sizeof(cg_markPolys));
	cg_activeMarkPolys.nextMark = cg_activeMarkPolys.prevMark = &cg_activeMarkPolys;
	cg_freeMarkPolys = cg_markPolys;
	for(i=0;i<MAX_MARK_POLYS-1;i++)
		cg_markPolys[i].nextMark = &cg_markPolys[i+1];
}

/*
==================
CG_FreeMarkPoly
==================
*/
void CG_FreeMarkPoly(markPoly_t *le){
	if(!le->prevMark) CG_Error("CG_FreeLocalEntity: inactive");
	// remove from the doubly linked active list
	le->prevMark->nextMark = le->nextMark;
	le->nextMark->prevMark = le->prevMark;
	// the free list is only singly linked
	le->nextMark = cg_freeMarkPolys;
	cg_freeMarkPolys = le;
}

/*
===================
CG_AllocMark

Will allways succeed, even if it requires freeing an old active mark
===================
*/
markPoly_t *CG_AllocMark(void){
	markPoly_t	*le;
	int			time;

	if(!cg_freeMarkPolys){
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		time = cg_activeMarkPolys.prevMark->time;
		while(cg_activeMarkPolys.prevMark && time == cg_activeMarkPolys.prevMark->time)
			CG_FreeMarkPoly(cg_activeMarkPolys.prevMark);
	}
	le = cg_freeMarkPolys;
	cg_freeMarkPolys = cg_freeMarkPolys->nextMark;
	memset(le, 0, sizeof(*le));
	// link into the active list
	le->nextMark = cg_activeMarkPolys.nextMark;
	le->prevMark = &cg_activeMarkPolys;
	cg_activeMarkPolys.nextMark->prevMark = cg_activeMarkPolys.nextMark = le;
	return le;
}

/*
=================
CG_ImpactMark

origin should be a point within a unit of the plane
dir should be the plane normal

temporary marks will not be stored or randomly oriented, but immediately
passed to the renderer.
=================
*/
#define	MAX_MARK_FRAGMENTS	128
#define	MAX_MARK_POINTS		384
void CG_ImpactMark(qhandle_t markShader, const vec3_t origin, const vec3_t dir, float orientation, float red, float green, float blue, float alpha, qboolean alphaFade, float radius, qboolean temporary){
	markFragment_t	markFragments[MAX_MARK_FRAGMENTS], *mf;
	vec3_t			axis[3], originalPoints[4], markPoints[MAX_MARK_POINTS], projection;
	byte			colors[4];
	float			texCoordScale;
	int				i, j, numFragments;

	if(!cg_addMarks.integer) return;
	if(radius <= 0) CG_Error("CG_ImpactMark: radius <= 0");
	//if(markTotal >= MAX_MARK_POLYS) return;
	// create the texture axis
	VectorNormalize2(dir, axis[0]);
	PerpendicularVector(axis[1], axis[0]);
	RotatePointAroundVector(axis[2], axis[0], axis[1], orientation);
	CrossProduct(axis[0], axis[2], axis[1]);
	texCoordScale = .5f *1.f / radius;
	// create the full polygon
	for(i=0;i<3;i++){
		originalPoints[0][i] = origin[i] -radius *axis[1][i] -radius *axis[2][i];
		originalPoints[1][i] = origin[i] +radius *axis[1][i] -radius *axis[2][i];
		originalPoints[2][i] = origin[i] +radius *axis[1][i] +radius *axis[2][i];
		originalPoints[3][i] = origin[i] -radius *axis[1][i] +radius *axis[2][i];
	}
	// get the fragments
	VectorScale(dir, -20, projection);
	numFragments = trap_CM_MarkFragments(4, (void*)originalPoints, projection, MAX_MARK_POINTS, markPoints[0], MAX_MARK_FRAGMENTS, markFragments);
	colors[0] = red * 255;
	colors[1] = green * 255;
	colors[2] = blue * 255;
	colors[3] = alpha * 255;
	for(i = 0, mf = markFragments; i < numFragments ; i++, mf++){
		polyVert_t	*v, verts[MAX_VERTS_ON_POLY];
		markPoly_t	*mark;

		// we have an upper limit on the complexity of polygons
		// that we store persistantly
		if(mf->numPoints > MAX_VERTS_ON_POLY)
			mf->numPoints = MAX_VERTS_ON_POLY;
		for(j = 0, v = verts ; j < mf->numPoints ; j++, v++){
			vec3_t delta;

			VectorCopy(markPoints[mf->firstPoint+j], v->xyz);
			VectorSubtract(v->xyz, origin, delta);
			v->st[0] = .5f +DotProduct(delta, axis[1]) *texCoordScale;
			v->st[1] = .5f +DotProduct(delta, axis[2]) *texCoordScale;
			*(int*)v->modulate = *(int*)colors;
		}
		// if it is a temporary (shadow) mark, add it immediately and forget about it
		if(temporary){
			trap_R_AddPolyToScene(markShader, mf->numPoints, verts);
			continue;
		}
		// otherwise save it persistantly
		mark = CG_AllocMark();
		mark->time = cg.time;
		mark->alphaFade = alphaFade;
		mark->markShader = markShader;
		mark->poly.numVerts = mf->numPoints;
		mark->color[0] = red;
		mark->color[1] = green;
		mark->color[2] = blue;
		mark->color[3] = alpha;
		memcpy(mark->verts, verts, mf->numPoints *sizeof(verts[0]));
		markTotal++;
	}
}

/*
===============
CG_AddMarks
===============
*/
#define	MARK_TOTAL_TIME		10000
#define	MARK_FADE_TIME		1000
void CG_AddMarks(void){}

// cg_particles.c  
#define BLOODRED	2
#define EMISIVEFADE	3
#define GREY75		4

typedef struct particle_s{
	struct particle_s	*next;
	float				time,
						endtime;
	vec3_t				org,
						vel,
						accel;
	int					color;
	float				colorvel,
						alpha,
						alphavel;
	int					type;
	qhandle_t			pshader;
	float				height,
						width,
						endheight,
						endwidth,
						start,
						end,
						startfade;
	qboolean			rotate;
	int					snum;
	qboolean			link;
	// Ridah
	int					shaderAnim,
						roll,
						accumroll;
} cparticle_t;

typedef enum{
	P_NONE,
	P_WEATHER,
	P_FLAT,
	P_SMOKE,
	P_ROTATE,
	P_WEATHER_TURBULENT,
	P_ANIM,	// Ridah
	P_BAT,
	P_BLEED,
	P_FLAT_SCALEUP,
	P_FLAT_SCALEUP_FADE,
	P_WEATHER_FLURRY,
	P_SMOKE_IMPACT,
	P_BUBBLE,
	P_BUBBLE_TURBULENT,
	P_SPRITE
} particle_type_t;

#define				MAX_SHADER_ANIMS		32
#define				MAX_SHADER_ANIM_FRAMES	64
static char			*shaderAnimNames[MAX_SHADER_ANIMS] = { "explode1", NULL};
static qhandle_t	shaderAnims[MAX_SHADER_ANIMS][MAX_SHADER_ANIM_FRAMES];
static int			shaderAnimCounts[MAX_SHADER_ANIMS] = { 23};
static float		shaderAnimSTRatio[MAX_SHADER_ANIMS] = { 1.f};
static int			numShaderAnims;
#define				PARTICLE_GRAVITY		40
#define				MAX_PARTICLES			1024
cparticle_t			*active_particles,
					*free_particles,
					particles[MAX_PARTICLES];
int					cl_numparticles = MAX_PARTICLES;
qboolean			initparticles = qfalse;
vec3_t				pvforward, pvright, pvup,
					rforward, rright, rup;
float				oldtime;

/*
===============
CL_ClearParticles
===============
*/
void CG_ClearParticles(void){
	int	i;

	memset(particles, 0, sizeof(particles));
	free_particles = &particles[0];
	active_particles = NULL;
	for(i=0 ;i<cl_numparticles;i++){
		particles[i].next = &particles[i+1];
		particles[i].type = 0;
	}
	particles[cl_numparticles-1].next = NULL;
	oldtime = cg.time;
	// Ridah, init the shaderAnims
	for(i=0;shaderAnimNames[i];i++){
		int j;

		for(j=0; j<shaderAnimCounts[i]; j++)
			shaderAnims[i][j] = trap_R_RegisterShader(va("%s%i", shaderAnimNames[i], j+1));
	}
	numShaderAnims = i;
	initparticles = qtrue;
}

/*
=====================
CG_AddParticleToScene
=====================
*/
void CG_AddParticleToScene(cparticle_t *p, vec3_t org, float alpha){
	polyVert_t	verts[4], TRIverts[3];
	vec3_t		point, color, rright2, rup2;
	float		width, height,
				time, time2,
				ratio, invratio;

	if(p->type == P_WEATHER || p->type == P_WEATHER_TURBULENT || p->type == P_WEATHER_FLURRY || p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT){
		// create a front facing polygon	
		if(p->type != P_WEATHER_FLURRY){
			if(p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT){
				if(org[2] > p->end){
					p->time = cg.time;	
					VectorCopy(org, p->org); // Ridah, fixes rare snow flakes that flicker on the ground
					p->org[2] = p->start +crandom () *4;
					if(p->type == P_BUBBLE_TURBULENT){
						p->vel[0] = crandom() *4;
						p->vel[1] = crandom() *4;
					}
				}
			}
			else{
				if(org[2] < p->end){	
					p->time = cg.time;	
					VectorCopy(org, p->org); // Ridah, fixes rare snow flakes that flicker on the ground
					while(p->org[2] < p->end)
						p->org[2] += p->start -p->end;
					if (p->type == P_WEATHER_TURBULENT){
						p->vel[0] = crandom() *16;
						p->vel[1] = crandom() *16;
					}
				}
			}
			// Rafael snow pvs check
			if(!p->link) return;
			p->alpha = 1;
		}
		// Ridah, had to do this or MAX_POLYS is being exceeded in village1.bsp
		if(Distance( cg.snap->ps.origin, org ) > 1024) return;
		// done.
		if(p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT){
			VectorMA(org, -p->height, pvup, point);	
			VectorMA(point, -p->width, pvright, point);	
			VectorCopy(point, verts[0].xyz);	
			verts[0].st[0] = verts[0].st[1] = 0;	
			verts[0].modulate[0] = verts[0].modulate[1] = verts[0].modulate[2] = 255;	
			verts[0].modulate[3] = 255 *p->alpha;
			VectorMA(org, -p->height, pvup, point);	
			VectorMA(point, p->width, pvright, point);	
			VectorCopy(point, verts[1].xyz);	
			verts[1].st[0] = 0;	
			verts[1].st[1] = 1;	
			verts[1].modulate[0] = verts[1].modulate[1] = verts[1].modulate[2] = 255;	
			verts[1].modulate[3] = 255 *p->alpha;	
			VectorMA(org, p->height, pvup, point);	
			VectorMA(point, p->width, pvright, point);	
			VectorCopy(point, verts[2].xyz);	
			verts[2].st[0] = verts[2].st[1] = 1;	
			verts[2].modulate[0] = verts[2].modulate[1] = verts[2].modulate[2] = 255;	
			verts[2].modulate[3] = 255 *p->alpha;	
			VectorMA(org, p->height, pvup, point);	
			VectorMA(point, -p->width, pvright, point);	
			VectorCopy(point, verts[3].xyz);	
			verts[3].st[0] = 1;	
			verts[3].st[1] = 0;	
			verts[3].modulate[0] = verts[3].modulate[1] = verts[3].modulate[2] = 255;	
			verts[3].modulate[3] = 255 *p->alpha;	
		}
		else{
			VectorMA(org, -p->height, pvup, point);	
			VectorMA(point, -p->width, pvright, point);	
			VectorCopy(point, TRIverts[0].xyz);
			TRIverts[0].st[0] = 1;
			TRIverts[0].st[1] = 0;
			TRIverts[0].modulate[0] = TRIverts[0].modulate[1] = TRIverts[0].modulate[2] = 255;
			TRIverts[0].modulate[3] = 255 *p->alpha;	
			VectorMA(org, p->height, pvup, point);	
			VectorMA(point, -p->width, pvright, point);	
			VectorCopy(point, TRIverts[1].xyz);	
			TRIverts[1].st[0] = TRIverts[1].st[1] = 0;
			TRIverts[1].modulate[0] = TRIverts[1].modulate[1] = TRIverts[1].modulate[2] = 255;
			TRIverts[1].modulate[3] = 255 *p->alpha;	
			VectorMA(org, p->height, pvup, point);	
			VectorMA(point, p->width, pvright, point);	
			VectorCopy(point, TRIverts[2].xyz);	
			TRIverts[2].st[0] = 0;
			TRIverts[2].st[1] = 1;
			TRIverts[2].modulate[0] = TRIverts[2].modulate[1] = TRIverts[2].modulate[2] = 255;
			TRIverts[2].modulate[3] = 255 *p->alpha;	
		}
	}
	else if(p->type == P_SPRITE){
		vec3_t	rr, ru, rotate_ang;
		
		VectorSet(color, 1.f, 1.f, .5f);
		time = cg.time -p->time;
		time2 = p->endtime -p->time;
		ratio = time / time2;
		width = p->width +(ratio *(p->endwidth -p->width));
		height = p->height +(ratio *(p->endheight -p->height));
		if(p->roll){
			vectoangles(cg.refdef.viewaxis[0], rotate_ang);
			rotate_ang[ROLL] += p->roll;
			AngleVectors(rotate_ang, NULL, rr, ru);
			VectorMA(org, -height, ru, point);	
			VectorMA(point, -width, rr, point);	
		}
		else{
			VectorMA(org, -height, pvup, point);	
			VectorMA(point, -width, pvright, point);	
		}
		VectorCopy(point, verts[0].xyz);	
		verts[0].st[0] = verts[0].st[1] = 0;	
		verts[0].modulate[0] = verts[0].modulate[1] = verts[0].modulate[2] = verts[0].modulate[3] = 255;
		if(p->roll) VectorMA(point, height *2, ru, point);
		else		VectorMA(point, 2*height *2, pvup, point);
		VectorCopy(point, verts[1].xyz);	
		verts[1].st[0] = 0;	
		verts[1].st[1] = 1;	
		verts[1].modulate[0] = verts[1].modulate[1] = verts[1].modulate[2] = verts[1].modulate[3] = 255;
		if(p->roll) VectorMA(point, width *2, rr, point);	
		else		VectorMA(point, width *2, pvright, point);	
		VectorCopy(point, verts[2].xyz);
		verts[2].st[0] = verts[2].st[1] = 1;	
		verts[2].modulate[0] = verts[2].modulate[1] = verts[2].modulate[2] = verts[2].modulate[3] = 255;	
		if(p->roll) VectorMA (point, -2*height, ru, point);	
		else		VectorMA (point, -2*height, pvup, point);	
		VectorCopy(point, verts[3].xyz);	
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = verts[3].modulate[1] = verts[3].modulate[2] = verts[3].modulate[3] = 255;	
	}
	else if(p->type == P_SMOKE || p->type == P_SMOKE_IMPACT){
		// create a front rotating facing polygon
		if(p->type == P_SMOKE_IMPACT && Distance(cg.snap->ps.origin, org) > 1024) return;
		if(p->color == BLOODRED)
			VectorSet (color, .22f, 0.f, 0.f);
		else if(p->color == GREY75){
			float	len, val, greyit;
			
			len = Distance(cg.snap->ps.origin, org);
			if(!len) len = 1;
			val = 4096 / len;
			greyit = .25f *val;
			if(greyit > .5f) greyit = .5f;
			VectorSet(color, greyit, greyit, greyit);
		}
		else VectorSet(color, 1.f, 1.f, 1.f);
		time = cg.time -p->time;
		time2 = p->endtime -p->time;
		ratio = time / time2;
		if(cg.time > p->startfade){
			invratio = 1 -((cg.time - p->startfade) / (p->endtime -p->startfade));
			if(p->color == EMISIVEFADE){
				float fval;
				
				fval = invratio *invratio;
				if(fval < 0) fval = 0;
				VectorSet(color, fval , fval , fval);
			}
			invratio *= p->alpha;
		}
		else 
			invratio = p->alpha;
		if(cgs.glconfig.hardwareType == GLHW_RAGEPRO) invratio = 1;
		if(invratio > 1) invratio = 1;
		width = p->width +(ratio *(p->endwidth -p->width));
		height = p->height +(ratio *(p->endheight -p->height));
		if (p->type != P_SMOKE_IMPACT){
			vec3_t temp;

			vectoangles(rforward, temp);
			p->accumroll += p->roll;
			temp[ROLL] += p->accumroll *.1f;
			AngleVectors(temp, NULL, rright2, rup2);
		}
		else{
			VectorCopy(rright, rright2);
			VectorCopy(rup, rup2);
		}
		if(p->rotate){
			VectorMA(org, -height, rup2, point);	
			VectorMA(point, -width, rright2, point);	
		}
		else{
			VectorMA(org, -p->height, pvup, point);	
			VectorMA(point, -p->width, pvright, point);	
		}
		VectorCopy(point, verts[0].xyz);	
		verts[0].st[0] = verts[0].st[1] = 0;	
		verts[0].modulate[0] = color[0] *255;
		verts[0].modulate[1] = color[1] *255;
		verts[0].modulate[2] = color[2] *255;
		verts[0].modulate[3] = invratio *255;
		if(p->rotate){
			VectorMA(org, -height, rup2, point);	
			VectorMA(point, width, rright2, point);	
		}
		else{
			VectorMA(org, -p->height, pvup, point);	
			VectorMA(point, p->width, pvright, point);	
		}
		VectorCopy(point, verts[1].xyz);	
		verts[1].st[0] = 0;	
		verts[1].st[1] = 1;	
		verts[1].modulate[0] = color[0] *255;
		verts[1].modulate[1] = color[1] *255;
		verts[1].modulate[2] = color[2] *255;
		verts[1].modulate[3] = invratio *255;
		if(p->rotate){
			VectorMA(org, height, rup2, point);	
			VectorMA(point, width, rright2, point);	
		}
		else{
			VectorMA(org, p->height, pvup, point);	
			VectorMA(point, p->width, pvright, point);	
		}
		VectorCopy(point, verts[2].xyz);	
		verts[2].st[0] = verts[2].st[1] = 1;	
		verts[2].modulate[0] = color[0] *255;
		verts[2].modulate[1] = color[1] *255;
		verts[2].modulate[2] = color[2] *255;
		verts[2].modulate[3] = invratio *255;
		if(p->rotate){
			VectorMA(org, height, rup2, point);	
			VectorMA(point, -width, rright2, point);	
		}
		else{
			VectorMA(org, p->height, pvup, point);	
			VectorMA(point, -p->width, pvright, point);	
		}
		VectorCopy(point, verts[3].xyz);	
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = color[0] *255;
		verts[3].modulate[1] = color[1] *255;
		verts[3].modulate[2] = color[2] *255;
		verts[3].modulate[3] = invratio *255;
	}
	else if(p->type == P_BLEED){
		vec3_t	rr, ru, rotate_ang;
		float	alpha;

		alpha = p->alpha;
		if(cgs.glconfig.hardwareType == GLHW_RAGEPRO)
			alpha = 1;
		if(p->roll){
			vectoangles(cg.refdef.viewaxis[0], rotate_ang);
			rotate_ang[ROLL] += p->roll;
			AngleVectors(rotate_ang, NULL, rr, ru);
		}
		else{
			VectorCopy(pvup, ru);
			VectorCopy(pvright, rr);
		}
		VectorMA(org, -p->height, ru, point);	
		VectorMA(point, -p->width, rr, point);	
		VectorCopy(point, verts[0].xyz);	
		verts[0].st[0] = verts[0].st[1] = 0;	
		verts[0].modulate[0] = 111;	
		verts[0].modulate[1] = 19;	
		verts[0].modulate[2] = 9;	
		verts[0].modulate[3] = alpha *255;
		VectorMA(org, -p->height, ru, point);	
		VectorMA(point, p->width, rr, point);	
		VectorCopy(point, verts[1].xyz);	
		verts[1].st[0] = 0;	
		verts[1].st[1] = 1;	
		verts[1].modulate[0] = 111;	
		verts[1].modulate[1] = 19;	
		verts[1].modulate[2] = 9;	
		verts[1].modulate[3] = alpha *255;	
		VectorMA(org, p->height, ru, point);	
		VectorMA(point, p->width, rr, point);	
		VectorCopy(point, verts[2].xyz);	
		verts[2].st[0] = verts[2].st[1] = 1;	
		verts[2].modulate[0] = 111;	
		verts[2].modulate[1] = 19;	
		verts[2].modulate[2] = 9;	
		verts[2].modulate[3] = alpha *255;
		VectorMA(org, p->height, ru, point);	
		VectorMA(point, -p->width, rr, point);	
		VectorCopy(point, verts[3].xyz);	
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = 111;	
		verts[3].modulate[1] = 19;	
		verts[3].modulate[2] = 9;	
		verts[3].modulate[3] = alpha *255;	
	}
	else if(p->type == P_FLAT_SCALEUP){
		float	width, height,
				sinR, cosR;

		if(p->color == BLOODRED)	VectorSet(color, 1.f, 1.f, 1.f);
		else						VectorSet(color, .5f, .5f, .5f);		
		time = cg.time -p->time;
		time2 = p->endtime -p->time;
		ratio = time / time2;
		width = p->width +(ratio *(p->endwidth -p->width));
		height = p->height +(ratio *(p->endheight -p->height));
		if(width > p->endwidth)		width = p->endwidth;
		if(height > p->endheight)	height = p->endheight;
		sinR = height *sin(DEG2RAD(p->roll)) *sqrt(2);
		cosR = width  *cos(DEG2RAD(p->roll)) *sqrt(2);
		VectorCopy(org, verts[0].xyz);	
		verts[0].xyz[0] -= sinR;
		verts[0].xyz[1] -= cosR;
		verts[0].st[0] = verts[0].st[1] = 0;	
		verts[0].modulate[0] = color[0] *255;
		verts[0].modulate[1] = color[1] *255;
		verts[0].modulate[2] = color[2] *255;
		verts[0].modulate[3] = 255;
		VectorCopy(org, verts[1].xyz);	
		verts[1].xyz[0] -= cosR;	
		verts[1].xyz[1] += sinR;	
		verts[1].st[0] = 0;	
		verts[1].st[1] = 1;	
		verts[1].modulate[0] = color[0] *255;
		verts[1].modulate[1] = color[1] *255;
		verts[1].modulate[2] = color[2] *255;
		verts[1].modulate[3] = 255;	
		VectorCopy (org, verts[2].xyz);	
		verts[2].xyz[0] += sinR;	
		verts[2].xyz[1] += cosR;	
		verts[2].st[0] = verts[2].st[1] = 1;
		verts[2].modulate[0] = color[0] *255;
		verts[2].modulate[1] = color[1] *255;
		verts[2].modulate[2] = color[2] *255;
		verts[2].modulate[3] = 255;	
		VectorCopy(org, verts[3].xyz);	
		verts[3].xyz[0] += cosR;	
		verts[3].xyz[1] -= sinR;	
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = color[0] *255;
		verts[3].modulate[1] = color[1] *255;
		verts[3].modulate[2] = color[2] *255;
		verts[3].modulate[3] = 255;		
	}
	else if(p->type == P_FLAT){
		VectorCopy(org, verts[0].xyz);	
		verts[0].xyz[0] -= p->height;	
		verts[0].xyz[1] -= p->width;	
		verts[0].st[0] = 0;	
		verts[0].st[1] = 0;	
		verts[0].modulate[0] = verts[0].modulate[1] = verts[0].modulate[2] = verts[0].modulate[3] = 255;	
		VectorCopy(org, verts[1].xyz);	
		verts[1].xyz[0] -= p->height;	
		verts[1].xyz[1] += p->width;	
		verts[1].st[0] = 0;	
		verts[1].st[1] = 1;	
		verts[1].modulate[0] = verts[1].modulate[1] = verts[1].modulate[2] = verts[1].modulate[3] = 255;	
		VectorCopy(org, verts[2].xyz);	
		verts[2].xyz[0] += p->height;	
		verts[2].xyz[1] += p->width;	
		verts[2].st[0] = verts[2].st[1] = 1;	
		verts[2].modulate[0] = verts[2].modulate[1] = verts[2].modulate[2] = verts[2].modulate[3] = 255;	
		VectorCopy(org, verts[3].xyz);	
		verts[3].xyz[0] += p->height;	
		verts[3].xyz[1] -= p->width;	
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = verts[3].modulate[1] = verts[3].modulate[2] = verts[3].modulate[3] = 255;	
	}
	// Ridah
	else if(p->type == P_ANIM){
		vec3_t	rr, ru, rotate_ang;
		int		i, j;

		time = cg.time -p->time;
		time2 = p->endtime -p->time;
		ratio = time / time2;
		if (ratio >= 1.f) ratio = .9999f;
		width = p->width +(ratio *(p->endwidth -p->width));
		height = p->height + (ratio *(p->endheight -p->height));
		// if we are "inside" this sprite, don't draw
		if(Distance(cg.snap->ps.origin, org) < width / 1.5f) return;
		i = p->shaderAnim;
		j = (int)floor(ratio *shaderAnimCounts[p->shaderAnim]);
		p->pshader = shaderAnims[i][j];
		if(p->roll){
			vectoangles(cg.refdef.viewaxis[0], rotate_ang);
			rotate_ang[ROLL] += p->roll;
			AngleVectors(rotate_ang, NULL, rr, ru);
			VectorMA(org, -height, ru, point);	
			VectorMA(point, -width, rr, point);	
		} else {
			VectorMA(org, -height, pvup, point);	
			VectorMA(point, -width, pvright, point);	
		}
		VectorCopy(point, verts[0].xyz);	
		verts[0].st[0] = verts[0].st[1] = 0;	
		verts[0].modulate[0] = verts[0].modulate[1] = verts[0].modulate[2] = verts[0].modulate[3] = 255;
		if(p->roll) VectorMA (point, 2*height, ru, point);
		else		VectorMA (point, 2*height, pvup, point);
		VectorCopy (point, verts[1].xyz);
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = verts[1].modulate[1] = verts[1].modulate[2] = verts[1].modulate[3] = 255;
		if(p->roll) VectorMA (point, 2*width, rr, point);
		else		VectorMA (point, 2*width, pvright, point);
		VectorCopy(point, verts[2].xyz);
		verts[2].st[0] = verts[2].st[1] = 1;	
		verts[2].modulate[0] = verts[2].modulate[1] = verts[2].modulate[2] = verts[2].modulate[3] = 255;	
		if(p->roll) VectorMA (point, -2*height, ru, point);
		else		VectorMA (point, -2*height, pvup, point);
		VectorCopy(point, verts[3].xyz);
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = verts[3].modulate[1] = verts[3].modulate[2] = verts[3].modulate[3] = 255;	
	}
	if(!p->pshader){
		CG_Printf("CG_AddParticleToScene type %d p->pshader == ZERO\n", p->type);
		return;
	}
	if(p->type == P_WEATHER || p->type == P_WEATHER_TURBULENT || p->type == P_WEATHER_FLURRY)
			trap_R_AddPolyToScene(p->pshader, 3, TRIverts);
	else	trap_R_AddPolyToScene(p->pshader, 4, verts);
}
// Ridah, made this static so it doesn't interfere with other files
static float roll = 0.f;

/*
===============
CG_AddParticles
===============
*/
void CG_AddParticles (void){
	cparticle_t		*p, *next, *active, *tail;
	vec3_t			org, rotate_ang;
	float			alpha, time, time2;
	int				color, type;

	if(!initparticles) CG_ClearParticles ();
	VectorCopy(cg.refdef.viewaxis[0], pvforward);
	VectorCopy(cg.refdef.viewaxis[1], pvright);
	VectorCopy(cg.refdef.viewaxis[2], pvup);
	vectoangles(cg.refdef.viewaxis[0], rotate_ang);
	roll += ((cg.time -oldtime) *.1f);
	rotate_ang[ROLL] += roll *.9f;
	AngleVectors(rotate_ang, rforward, rright, rup);
	oldtime = cg.time;
	active = tail = NULL;
	for(p=active_particles; p; p=next){
		next = p->next;
		time = (cg.time -p->time) *.001f;
		alpha = p->alpha +time *p->alphavel;
		if(alpha <= 0){
			// faded out
			p->next = free_particles;
			free_particles = p;
			p->type = p->color = p->alpha = 0;
			continue;
		}
		if((p->type == P_SMOKE || p->type == P_ANIM || p->type == P_BLEED || p->type == P_SMOKE_IMPACT || p->type == P_WEATHER_FLURRY || p->type == P_FLAT_SCALEUP_FADE) && cg.time > p->endtime){
				p->next = free_particles;
				free_particles = p;
				p->type = p->color = p->alpha = 0;
				continue;
		}
		if((p->type == P_BAT || p->type == P_SPRITE) && p->endtime < 0){
			// temporary sprite
			CG_AddParticleToScene(p, p->org, alpha);
			p->next = free_particles;
			free_particles = p;
			p->type = p->color = p->alpha = 0;
			continue;
		}
		p->next = NULL;
		if(!tail) active = tail = p;
		else{
			tail->next = tail = p;
		}
		if(alpha > 1.f) alpha = 1.f;
		color = p->color;
		time2 = time *time;
		org[0] = p->org[0] +p->vel[0] *time +p->accel[0] *time2;
		org[1] = p->org[1] +p->vel[1] *time +p->accel[1] *time2;
		org[2] = p->org[2] +p->vel[2] *time +p->accel[2] *time2;
		type = p->type;
		CG_AddParticleToScene(p, org, alpha);
	}
	active_particles = active;
}

/*
======================
CG_AddParticles
======================
*/
void CG_ParticleSnowFlurry(qhandle_t pshader, centity_t *cent){
	cparticle_t	*p;
	qboolean	turb = qtrue;
	
	if(!pshader) CG_Printf("CG_ParticleSnowFlurry: no pshader!\n");
	if(!free_particles) return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->color = p->alphavel = 0;
	p->alpha = .90f;
	p->start = cent->currentState.origin2[0];
	p->end = cent->currentState.origin2[1];
	p->endtime =	cg.time +cent->currentState.time;
	p->startfade =	cg.time +cent->currentState.time2;
	p->pshader = pshader;
	if(rand()%100 > 90){
		p->height = p->width = 32;
		p->alpha = .10f;
	}
	else{
		p->height = p->width = 1;
	}
	p->vel[2] = -20;
	p->type = P_WEATHER_FLURRY;
	if(turb) p->vel[2] = -10;
	VectorCopy(cent->currentState.origin, p->org);
	p->org[0] = p->org[0];
	p->org[1] = p->org[1];
	p->org[2] = p->org[2];
	p->vel[0] = p->vel[1] = 0;
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	p->vel[0] += cent->currentState.angles[0] *32 +(crandom() *16);
	p->vel[1] += cent->currentState.angles[1] *32 +(crandom() *16);
	p->vel[2] += cent->currentState.angles[2];
	if(turb){
		p->accel[0] = crandom () *16;
		p->accel[1] = crandom () *16;
	}
}

void CG_ParticleSnow(qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum){
	cparticle_t *p;

	if(!pshader) CG_Printf("CG_ParticleSnow: no pshader!\n");
	if(!free_particles) return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->color = p->alphavel = 0;
	p->alpha = .40f;
	p->start = origin[2];
	p->end = origin2[2];
	p->pshader = pshader;
	p->height = p->width = 1;
	p->vel[2] = -50;
	if(turb){
		p->type = P_WEATHER_TURBULENT;
		p->vel[2] = -50 *1.3f;
	}
	else p->type = P_WEATHER;
	VectorCopy(origin, p->org);
	p->org[0] = p->org[0] +(crandom() *range);
	p->org[1] = p->org[1] +(crandom() *range);
	p->org[2] = p->org[2] +(crandom() *(p->start -p->end)); 
	p->vel[0] = p->vel[1] = 0;
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	if (turb){
		p->vel[0] = crandom() *16;
		p->vel[1] = crandom() *16;
	}
	// Rafael snow pvs check
	p->snum = snum;
	p->link = qtrue;
}

void CG_ParticleBubble(qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum){
	cparticle_t		*p;
	float			randsize;

	if(!pshader) CG_Printf("CG_ParticleSnow: no pshader!\n");
	if (!free_particles) return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->color = p->alphavel = 0;
	p->alpha = .40f;
	p->start = origin[2];
	p->end = origin2[2];
	p->pshader = pshader;
	randsize = (crandom() *.5f) +1;
	p->height = randsize;randsize;
	p->vel[2] = 50 +(crandom() *10);
	if (turb){
		p->type = P_BUBBLE_TURBULENT;
		p->vel[2] = 50 *1.3f;
	}
	else p->type = P_BUBBLE;
	VectorCopy(origin, p->org);
	p->org[0] = p->org[0] +(crandom() *range);
	p->org[1] = p->org[1] +(crandom() *range);
	p->org[2] = p->org[2] +(crandom() *(p->start -p->end));
	p->vel[0] = p->vel[1] = 0;	
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	if (turb){
		p->vel[0] = crandom() *4;
		p->vel[1] = crandom() *4;
	}
	// Rafael snow pvs check
	p->snum = snum;
	p->link = qtrue;
}

void CG_ParticleSmoke(qhandle_t pshader, centity_t *cent){
	// using cent->density = enttime
	//		 cent->frame = startfade
	cparticle_t	*p;

	if(!pshader) CG_Printf("CG_ParticleSmoke: no pshader!\n");
	if(!free_particles) return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->endtime =	cg.time +cent->currentState.time;
	p->startfade =	cg.time +cent->currentState.time2;
	p->color = p->alphavel = 0;
	p->alpha = 1.f;
	p->start =	cent->currentState.origin[2];
	p->end =	cent->currentState.origin2[2];
	p->pshader = pshader;
	p->rotate = qfalse;
	p->height = p->width = 8;
	p->endheight = p->endwidth = 32;
	p->type = P_SMOKE;
	VectorCopy(cent->currentState.origin, p->org);
	p->vel[0] = p->vel[1] = 0;
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	p->vel[2] = 5;
	// reverse gravity	
	if(cent->currentState.frame == 1) p->vel[2] *= -1;
	p->roll = 8 +(crandom() *4);
}

void CG_ParticleBulletDebris (vec3_t org, vec3_t vel, int duration){
	cparticle_t	*p;

	if(!free_particles) return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->endtime = cg.time +duration;
	p->startfade = cg.time +duration / 2;
	p->color = EMISIVEFADE;
	p->alpha = 1.f;
	p->alphavel = 0;
	p->height = p->width = p->endheight = p->endwidth = .5f;
	p->pshader = cgs.media.tracerShader;
	p->type = P_SMOKE;
	VectorCopy(org, p->org);
	p->vel[0] = vel[0];
	p->vel[1] = vel[1];
	p->vel[2] = vel[2];
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	p->accel[2] = -60;
	p->vel[2] += -20;
}

/*
======================
CG_ParticleExplosion
======================
*/
void CG_ParticleExplosion(char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd){
	cparticle_t		*p;
	int				anim;

	if(animStr < (char*)10)
		CG_Error("CG_ParticleExplosion: animStr is probably an index rather than a string");
	// find the animation string
	for(anim=0; shaderAnimNames[anim]; anim++)
		if(!Q_stricmp(animStr, shaderAnimNames[anim])) break;
	if(!shaderAnimNames[anim])
		CG_Error("CG_ParticleExplosion: unknown animation string: %s\n", animStr);
	if(!free_particles) return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->alpha = .5f;
	p->alphavel = 0;
	if(duration < 0){
		duration *= -1;
		p->roll = 0;
	}
	else p->roll = crandom() *179;
	p->shaderAnim = anim;
	p->width = sizeStart;
	p->height = sizeStart *shaderAnimSTRatio[anim];	// for sprites that are stretch in either direction
	p->endheight = sizeEnd;
	p->endwidth = sizeEnd *shaderAnimSTRatio[anim];
	p->endtime = cg.time +duration;
	p->type = P_ANIM;
	VectorCopy(origin, p->org);
	VectorCopy(vel, p->vel);
	VectorClear(p->accel);
}

int CG_NewParticleArea(int num){
	vec3_t	origin, origin2;
	float	range = 0;
	char	*str, *token;
	int		i, type, turb, snum, numparticles;
	
	str = (char*)CG_ConfigString(num);
	if(!str[0]) return (0);
	// returns type 128 64 or 32
	token = COM_Parse(&str);
	type = atoi(token);
		 if(type == 0)	range = 256;
	else if(type == 1)	range = 128;
	else if(type == 2)	range = 64;
	else if(type == 3)	range = 32;
	else if(type == 4)	range = 8;
	else if(type == 5)	range = 16;
	else if(type == 6)	range = 32;
	else if(type == 7)	range = 64;
	for(i=0; i<3; i++){
		token = COM_Parse(&str);
		origin[i] = atof(token);
	}
	for(i=0; i<3; i++){
		token = COM_Parse(&str);
		origin2[i] = atof(token);
	}
	token = COM_Parse(&str);
	numparticles = atoi(token);
	token = COM_Parse(&str);
	turb = atoi(token);
	token = COM_Parse(&str);
	snum = atoi(token);
	for(i=0; i<numparticles; i++){
		if(type >= 4)	CG_ParticleBubble(cgs.media.waterBubbleMediumShader, origin, origin2, turb, range, snum);
		else			CG_ParticleSnow(cgs.media.waterBubbleMediumShader, origin, origin2, turb, range, snum);
	}
	return (1);
}
