// Copyright (C) 2003-2004 RiO
// cg_auras.c -- generates and displays auras
#include "cg_local.h"
static auraState_t	auraStates[MAX_CLIENTS];
// =================================================
//
//    S E T T I N G   U P   C O N V E X   H U L L
//
// =================================================
/*
========================
CG_Aura_BuildTailPoint
========================
  Builds position of the aura's tail point, dependant on velocity vector of player entity
*/
static void CG_Aura_BuildTailPoint(centity_t *player,auraState_t *state,auraConfig_t *config){
	vec3_t tailDir;
	// Find the direction the tail should point. Low speeds will have it pointing up.
	if(VectorNormalize2(player->currentState.pos.trDelta,tailDir) < 180){
		VectorSet(tailDir,0,0,1);
	}
	else{
		VectorInverse(tailDir);
	}
	// Using the established direction, set up the tail's potential hull point.
	VectorMA(state->origin,config->auraScale * config->tailLength,tailDir,state->tailPos);
}
/*
=======================
CG_Aura_MarkRootPoint
=======================
  Marks a point as the aura's root point. The root point is the point that is placed
  furthest away from the tail. It is where the aura 'opens up'.
*/
void CG_Aura_MarkRootPoint(auraState_t *state){
	vec3_t prjPoint;
	vec3_t tailDir;
	float maxDist = 0;
	float newDist;
	// NOTE:
	// The root point is the point that is placed furthest down the aura.
	// This means the distance between the tail point and the root point's projection
	// onto the tail axis is maximal.
	for(int point=0;point<state->convexHullCount;point++){
		// Project the point onto the tail axis
		ProjectPointOnLine(state->convexHull[point].pos_world,state->origin,state->tailPos,prjPoint);
		// Measure the distance and copy over the projection of the point
		// as the new root if it's further away.
		newDist = Distance(prjPoint,state->tailPos);
		if(newDist > maxDist){
			maxDist = newDist;
			VectorCopy(prjPoint,state->rootPos);
		}
	}
	// HACK:
	// Nudge the root away a bit to account for the expansion
	// of the aura through the hull normals.
	VectorSubtract(state->tailPos,state->origin,tailDir);
	VectorNormalize(tailDir);
	VectorMA(state->rootPos,-4.8f,tailDir,state->rootPos);
}
/*
=======================
CG_Aura_GetHullPoints
=======================
  Reads and prepares the positions of the tags for a convex hull aura.
*/
#define MAX_AURATAGNAME 12
static void CG_Aura_GetHullPoints(centity_t *clientEntity,auraState_t *state,auraConfig_t *config){
	char tagName[MAX_AURATAGNAME];
	int hullIndex = 0;
	for(int bodyPart=0;bodyPart<3;bodyPart++){
		for(int tagIndex=0;tagIndex<config->numTags[bodyPart];tagIndex++){
			orientation_t tagOrient;
			Com_sprintf(tagName,sizeof(tagName),"tag_aura%d",tagIndex);
			if(!CG_TryLerpPlayerTag(clientEntity,tagName,&tagOrient)){continue;}
			VectorCopy(tagOrient.origin,state->convexHull[hullIndex].pos_world);
			if(CG_WorldCoordToScreenCoordVec(state->convexHull[hullIndex].pos_world,state->convexHull[hullIndex].pos_screen)){
				state->convexHull[hullIndex].is_tail = qfalse;
				hullIndex++;
			}
		}
	}
	// Find the aura's tail point
	CG_Aura_BuildTailPoint(clientEntity,state,config);
	// Add the tail tip to the hull points, if it's visible
	VectorCopy(state->tailPos,state->convexHull[hullIndex].pos_world);
	if(CG_WorldCoordToScreenCoordVec(state->convexHull[hullIndex].pos_world,state->convexHull[hullIndex].pos_screen)){
		state->convexHull[hullIndex].is_tail = qtrue;
		hullIndex++;
	}
	// Get the total number of possible vertices to account for in the hull
	state->convexHullCount = hullIndex;
	CG_Aura_MarkRootPoint(state);
}
/*
====================
CG_Aura_QSortAngle
====================
  Quicksort by ascending angles
*/
static void CG_Aura_QSortAngle(auraTag_t *points,float* angles,int lowbound,int highbound){
	int			low = lowbound;
	int			high = highbound;
	float		mid = angles[(low+high)/2];
	float		tempAngle;
	auraTag_t	tempPoint;
	do{
		while(angles[low] < mid){low++;}
		while(angles[high] > mid){high--;}
		if(low <= high){
			// swap points
			tempPoint = points[low];
			points[low] = points[high];
			points[high] = tempPoint;
			// swap angles
			tempAngle = angles[low];
			angles[low] = angles[high];
			angles[high] = tempAngle;
			low++;
			high--;
		}
	}
	while(!(low > high));
	if(high > lowbound){CG_Aura_QSortAngle(points,angles,lowbound,high);}
	if(low < highbound){CG_Aura_QSortAngle(points,angles,low,highbound);}
}

/*
===========================
CG_Aura_ArrangeConvexHull
===========================
  Rearranges *points to contain its convex hull in the first *nr_points points.
*/
static qboolean CG_Aura_ArrangeConvexHull(auraTag_t *points,int *nr_points){
	// +1 for tail
	float angles[MAX_AURATAGS+1];
	int amount = *nr_points;
	int	index;
	int pivotIndex = 0;
	auraTag_t pivot;
	auraTag_t behind;
	auraTag_t infront;
	auraTag_t vecPoint;
	auraTag_t buffer[MAX_AURATAGS+1];
	qboolean rightTurn;
	// Already a convex hull
	if(amount == 3){return qtrue;}
	if(amount < 3){return qfalse;}
	// Find pivot point, which is known to be on the hull.
	// Point with lowest y - if there are multiple, point with highest x.
	for(index=1;index<amount;index++){
		if(points[index].pos_screen[1] < points[pivotIndex].pos_screen[1]){
			pivotIndex = index;
		}
		else if(points[index].pos_screen[1] == points[pivotIndex].pos_screen[1]){
			if(points[index].pos_screen[0] > points[pivotIndex].pos_screen[0]){
				pivotIndex = index;
			}
		}
	}
	// Put pivot into seperate variable and remove from array.
	pivot = points[pivotIndex];
	points[pivotIndex] = points[amount-1];
	amount--;
	// Calculate angle to pivot for each point in the array.
  for(index=0;index<amount;index++){
		// point vector
		vecPoint.pos_screen[0] = pivot.pos_screen[0] - points[index].pos_screen[0];
		vecPoint.pos_screen[1] = pivot.pos_screen[1] - points[index].pos_screen[1];
		// reduce to a unit-vector - length 1
		angles[index] = vecPoint.pos_screen[0] / Q_hypot(vecPoint.pos_screen[0],vecPoint.pos_screen[1]);
	}
	// Sort the points by angle.
	CG_Aura_QSortAngle(points,angles,0,amount-1);
	// Step through array to remove points that are not p of the convex hull.
	index = 1;
	do{
		 // Assign points behind and in front of current point.
		if(!index){
			rightTurn = qtrue;
		}
		else{
			float direction;
			behind = points[index-1];
			infront = index == amount-1 ? pivot : points[index+1];
			// Work out if we are making a right or left turn using vector product.
			direction = (behind.pos_screen[0] - points[index].pos_screen[0]) * (infront.pos_screen[1] - points[index].pos_screen[1]) -
						(infront.pos_screen[0] - points[index].pos_screen[0]) * (behind.pos_screen[1] - points[index].pos_screen[1]);
			rightTurn = direction < 0.0f ? qtrue : qfalse;
		}
		//point is currently considered part of the hull
		if(rightTurn){index++;}
		// point is not part of the hull
		else{
			// remove point from convex hull
			if(index == amount-1){
				amount--;
			}
			else{
				// move everything after the current value one step forward
				memcpy(buffer,&points[index+1],sizeof(auraTag_t) * (amount-index-1));
				memcpy(&points[index],buffer,sizeof(auraTag_t) * (amount-index-1));
				amount--;
			}
			// backtrack to previous point
			index--;
		}
	}
	while(index != amount-1);
	// add pivot back into points array
	points[amount] = pivot;
	amount++;
	*nr_points = amount;
	return qtrue;
}
/*
===========================
CG_Aura_SetHullAttributes
===========================
  Set segment lengths, normals, circumference, etc.
*/
static void CG_Aura_SetHullAttributes(auraState_t *state){
	int behind;
	int infront;
	int nr_points = state->convexHullCount;
	vec3_t line_behind;
	vec3_t line_infront;
	vec3_t viewLine;
	vec3_t temp_behind;
	vec3_t temp_infront;
	float circumference = 0.0f;
	auraTag_t *points = state->convexHull;
	for(int index=0;index<nr_points;index++){
		// Set successor and predeccessor
		if(!index){
			behind = nr_points-1;
			infront = index+1;
		}
		else if(index == nr_points-1){
			behind = index-1;
			infront = 0;
		}
		else{
			behind = index-1;
			infront = index+1;
		}
		VectorSubtract(points[index].pos_world,cg.refdef.vieworg,viewLine);
		// Calculate the normal
		VectorSubtract(points[index].pos_world,points[behind].pos_world,line_behind);
		VectorSubtract(points[infront].pos_world,points[index].pos_world,line_infront);
		VectorNormalize(line_behind);
		points[index].length = VectorNormalize(line_infront);
		circumference += points[index].length;
		CrossProduct(line_behind,viewLine,temp_behind);
		CrossProduct(line_infront,viewLine,temp_infront);
		if(!VectorNormalize(temp_behind) || !VectorNormalize(temp_infront)){
			viewLine[2] += 0.1f;
			CrossProduct(line_behind,viewLine,temp_behind);
			VectorNormalize(temp_behind);
			viewLine[2] -= 0.1f;
		}
		VectorAdd(temp_behind,temp_infront,points[index].normal);
		VectorNormalize(points[index].normal);
	}
	state->convexHullCircumference = circumference;
}
/*
=========================
CG_Aura_BuildConvexHull
=========================
  Calls all relevant functions to build up the aura's convex hull.
  Returns false if no hull can be made.
*/
static qboolean CG_Aura_BuildConvexHull(centity_t *player,auraState_t *state,auraConfig_t *config){
	// Retrieve hull points
	CG_Aura_GetHullPoints(player,state,config);
	// Arrange hull. Don't continue if there aren't enough points to form a hull.
	if(!CG_Aura_ArrangeConvexHull(state->convexHull,&state->convexHullCount)){return qfalse;}
	// Set hull's attributes
	CG_Aura_SetHullAttributes(state);
	// Hull building completed succesfully
	return qtrue;
}
// =================================
//
//    A U R A   R E N D E R I N G
//
// =================================
// Some constants needed for rendering.
// FIXME: Should become dynamic, but NR_AURASPIKES may pose a problem with allocation of poly buffer.
#define NR_AURASPIKES         50
#define AURA_FLATTEN_NORMAL		0.75f
#define AURA_ROOTCUTOFF_FRAQ	0.80f
#define AURA_ROOTCUTOFF_DIST	7.00f
/*
===================
CG_Aura_DrawSpike
===================
  Draws the polygons for one aura spike
*/
static void CG_Aura_DrawSpike(vec3_t start,vec3_t end,float width,qhandle_t shader,vec4_t RGBModulate){
	vec3_t line;
	vec3_t offset;
	vec3_t viewLine;
	polyVert_t verts[4];
	float len;
	VectorSubtract(end,start,line);
	VectorSubtract(start,cg.refdef.vieworg,viewLine);
	CrossProduct(viewLine,line,offset);
	len = VectorNormalize(offset);
	if(!len){return;}
	VectorMA(end,-width,offset,verts[0].xyz);
	verts[0].st[0] = 1;
	verts[0].st[1] = 0;
	VectorMA(end,width,offset,verts[1].xyz);
	verts[1].st[0] = 0;
	verts[1].st[1] = 0;
	VectorMA(start,width,offset,verts[2].xyz);
	verts[2].st[0] = 0;
	verts[2].st[1] = 1;
	VectorMA(start,-width,offset,verts[3].xyz);
	verts[3].st[0] = 1;
	verts[3].st[1] = 1;
	for(int i=0;i<4;i++){
		for(int j=0;j<4;j++){
			verts[i].modulate[j] = RGBModulate[j] * 255;
		}
	}
	trap_R_AddPolyToScene(shader,4,verts);
}
/*
==========================
CG_Aura_LerpSpikeSegment
==========================
  Lerps the position the aura spike should have along a segment of the convex hull
*/
static void CG_Aura_LerpSpikeSegment(auraState_t *state,int spikeNr,int *start,int *end,float *progress_pct){
	// Map i onto the circumference of the convex hull.
	float length_pos = state->convexHullCircumference * ((float)spikeNr / (float)(NR_AURASPIKES-1));
	float length_sofar;
	int i;
	int j;
	// Find the segment we are in right now.
	length_sofar = 0;
	for(i=0;length_sofar + state->convexHull[i].length < length_pos && i < NR_AURASPIKES;i++){
		length_sofar += state->convexHull[i].length;
	}
	j = i+1;
	if(j == state->convexHullCount){j = 0;}
	// Return found values.
	*start = i;
	*end = j;
	*progress_pct = (length_pos - length_sofar) / state->convexHull[i].length;
}
/*
==============
CG_LerpSpike
==============
  Lerps one spike in the aura
*/
static void CG_LerpSpike(auraState_t *state,auraConfig_t *config,int spikeNr,float alphaModulate){
	int start;
	int end;
	float	progress_pct;
	float lerpTime;
	float lerpModulate;
	float lerpBorder;
	float baseBorder;
	float lerpSize;
	vec3_t viewLine;
	vec3_t lerpPos;
	vec3_t lerpNormal;
	vec3_t lerpDir;
	vec3_t endPos;
	vec3_t basePos;
	vec4_t lerpColor;
	vec3_t edge;
	vec3_t tailDir;
	vec3_t tempVec;
	// Decide on which type of spike to use.
	if(!(spikeNr % 3)){
		lerpTime = (cg.time % 400) / 400.0f;
		lerpSize = 14.0f * (lerpTime+1.0f);
		lerpBorder = 2.5f * (lerpTime+1.75f);
		baseBorder = 2.5f * 1.75f;
	}
	else if(!(spikeNr % 2)){
		lerpTime = ((cg.time+200) % 400) / 400.0f;
		lerpSize = 12.0f * (lerpTime+1.0f);
		lerpBorder = 3.0f * (lerpTime+1.75f);
		baseBorder = 3.0f * 1.75f;
	}
	else{
		lerpTime = (cg.time % 500) / 500.0f;
		lerpSize = 10.0f * (lerpTime+1.0f);
		lerpBorder = 2.75f * (lerpTime+1.75f);
		baseBorder = 2.75f * 1.75f;
	}
	lerpModulate = -4.0f * lerpTime * lerpTime + 4.0f *lerpTime;
	// NOTE: Prepared for a cvar switch between additive and
	//       blended aura.
	if(qfalse){
		lerpColor[0] = config->auraColor[0] * lerpModulate * alphaModulate;
		lerpColor[1] = config->auraColor[1] * lerpModulate * alphaModulate;
		lerpColor[2] = config->auraColor[2] * lerpModulate * alphaModulate;
		lerpColor[3] = 1;
	}
	else{
		lerpColor[0] = config->auraColor[0];
		lerpColor[1] = config->auraColor[1];
		lerpColor[2] = config->auraColor[2];
		lerpColor[3] = lerpModulate * alphaModulate;
	}
	// Get our position in the hull
	CG_Aura_LerpSpikeSegment(state,spikeNr,&start,&end,&progress_pct);
	// Lerp the position using the stored normal to expand the aura a bit
	VectorSet(lerpNormal,0.0f,0.0f,0.0f);
	VectorMA(lerpNormal,1.0f - progress_pct,state->convexHull[start].normal,lerpNormal);
	VectorMA(lerpNormal,progress_pct,state->convexHull[end].normal,lerpNormal);
	VectorNormalize(lerpNormal);
	VectorSubtract(state->convexHull[end].pos_world,state->convexHull[start].pos_world,edge);
	VectorMA(state->convexHull[start].pos_world,progress_pct,edge,lerpPos);
	VectorMA(lerpPos,baseBorder,lerpNormal,basePos);
	VectorMA(lerpPos,lerpBorder,lerpNormal,lerpPos);
	// Create the direction
	VectorSubtract(lerpPos,state->rootPos,lerpDir);
	// Flatten the direction a bit so it doesn't point to the tip too drasticly.
	VectorSubtract(state->tailPos,state->origin,tailDir);
	VectorPllComponent(lerpDir,tailDir,edge);
	VectorScale(edge,AURA_FLATTEN_NORMAL,edge);
	VectorSubtract(lerpDir,edge,lerpDir);
	VectorNormalize(lerpDir);
	// Set the viewing direction
	VectorSubtract(lerpPos,cg.refdef.vieworg,viewLine);
	VectorNormalize(viewLine);
	// Don't display this spike if it would be travelling into / out of
	// the screen too much: It is part of the blank area surrounding the root.
	CrossProduct(viewLine,lerpDir,tempVec);
	if(VectorLength(tempVec) < AURA_ROOTCUTOFF_FRAQ){
		// Only disallow drawing if it's actually a segment originating
		// from close enough to the root.
		if(Distance(basePos,state->rootPos) < AURA_ROOTCUTOFF_DIST){return;}
	}
	VectorMA(lerpPos,lerpBorder,lerpDir,lerpPos);
	VectorMA(lerpPos,lerpSize,lerpDir,endPos);
	CG_Aura_DrawSpike(lerpPos,endPos,lerpSize / 1.25f,config->auraShader,lerpColor);
}
/*
==========================
CG_Aura_ConvexHullRender
==========================
*/
static void CG_Aura_ConvexHullRender(centity_t *player,auraState_t *state,auraConfig_t *config){
	if(!state->isActive && state->modulate == 0.0f){return;}
	if(!config->showAura){return;}
	// Build the hull. Don't continue if it can't be built.
	if(!CG_Aura_BuildConvexHull(player,state,config)){return;}
	// Clear the poly buffer
	// For each spike add it to the poly buffer
	// FIXME: Uses old style direct adding with trap call until buffer system is built
	for(int i=0;i<NR_AURASPIKES;i++){
		CG_LerpSpike(state,config,i,state->modulate);
	}
}
// ===================================
//
//    A U R A   M A N A G E M E N T
//
// ===================================
/*==================
CG_Aura_AddTrail
==================*/
static void CG_Aura_AddTrail(centity_t *player, auraState_t *state, auraConfig_t *config){
	if(!state->isActive || !config->showTrail){return;}
	if(!(player->currentState.playerBitFlags & usingBoost)) return;
	// If we didn't draw the tail last frame, this is a new instantiation
	// of the entity and we will have to reset the tail positions.
	// NOTE: Give 1.5 second leeway for 'snapping' the tail
	//       incase we (almost) immediately restart boosting.
	if(player->lastTrailTime < (cg.time - cg.frametime - 1500)){		
		CG_ResetTrail(player->currentState.clientNum,player->lerpOrigin,1000,config->trailWidth,config->trailShader,config->trailColor);
	}
	CG_UpdateTrailHead(player->currentState.clientNum,player->lerpOrigin);
	player->lastTrailTime = cg.time;
}
/*===================
CG_Aura_AddDebris
===================*/
static void CG_Aura_AddDebris(centity_t *player,auraState_t *state,auraConfig_t *config){
	if(!state->isActive || !config->showDebris) return;
	// Spawn the debris system if the player has just entered PVS
	if(!CG_FrameHist_HadAura(player->currentState.number)){
		PSys_SpawnCachedSystem("AuraDebris",player->lerpOrigin,NULL,player,NULL,qtrue,qfalse);
	}
	CG_FrameHist_SetAura(player->currentState.number);
}
/*===================
CG_Aura_AddParticleSystem
===================*/
static void CG_Aura_AddParticleSystem(centity_t *player,auraState_t *state,auraConfig_t *config){
	if(!state->isActive || !config->particleSystem[0]){return;}
	// If the entity wasn't previously in the PVS, we need to start a new system
	// Spawn the particle system if the player has just entered PVS
	if(!CG_FrameHist_HadAura(player->currentState.number)){
		PSys_SpawnCachedSystem(config->particleSystem,player->lerpOrigin,NULL,player,NULL,qtrue,qfalse);
	}
}

/*===================
CG_Aura_AddSounds
===================*/
static void CG_Aura_AddSounds(centity_t *player,auraState_t *state,auraConfig_t *config){
	if(!state->isActive){return;}
	// Add looping sounds depending on aura type (boost or charge)
	if(config->boostLoopSound && player->currentState.playerBitFlags & usingBoost){
		trap_S_AddLoopingSound(player->currentState.number,player->lerpOrigin,vec3_origin,config->boostLoopSound);
	}
	else if(config->chargeLoopSound){
		trap_S_AddLoopingSound(player->currentState.number,player->lerpOrigin,vec3_origin,config->chargeLoopSound);
	}
}
/*===================
CG_Aura_AddDLight
===================*/
static void CG_Aura_AddDLight(centity_t *player,auraState_t *state,auraConfig_t *config){
	vec3_t lightPos;
	// add dynamic light when necessary
	if(config->showLight && (state->isActive || state->lightAmt > config->lightMin)){
		// Since lerpOrigin is the lightingOrigin for the player, this will add a backsplash light for the aura.
		VectorAdd(player->lerpOrigin,cg.refdef.viewaxis[0],lightPos);
		trap_R_AddLightToScene(lightPos,state->lightAmt, // +(cos(cg.time / 50.0f) * state->lightDev),
								config->lightColor[0] * state->modulate,
								config->lightColor[1] * state->modulate,
								config->lightColor[2] * state->modulate);
	}
}

/*==================
CG_Aura_DimLight
==================*/
static void CG_Aura_DimLight(centity_t *player,auraState_t *state,auraConfig_t *config){
	if(state->isActive){
		if(state->lightAmt < config->lightMax){
			state->lightAmt += config->lightGrowthRate * (cg.frametime / 25.0f);
			state->lightAmt = Com_Clamp(config->lightMin,config->lightMax,state->lightAmt);
			state->lightDev = state->lightAmt / 10;
		}
	}
	else if(state->lightAmt > config->lightMin){
		state->lightAmt -= config->lightGrowthRate * (cg.frametime / 50.0f);
		state->lightAmt = Com_Clamp(config->lightMin,config->lightMax,state->lightAmt);
	}
	state->modulate = (float)(state->lightAmt - config->lightMin) / (float)(config->lightMax - config->lightMin);
	state->modulate = Com_Clamp(0.0f,1.0f,state->modulate);
}
/*========================
CG_AddAuraToScene
========================*/
void CG_AddAuraToScene(centity_t *player){
	// Get the aura system corresponding to the player
	int clientNum = player->currentState.clientNum;
	int tier = cgs.clientinfo[clientNum].tierCurrent;
	auraState_t* state;
	auraConfig_t* config;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS){
		CG_Error("Bad clientNum on player entity");
		return;
	}
	state = &auraStates[clientNum];
	config = &state->configurations[tier];
	VectorCopy(player->lerpOrigin,state->origin);
	CG_Aura_DimLight(player,state,config);
	CG_Aura_AddSounds(player,state,config);
	CG_Aura_AddTrail(player,state,config);
	CG_Aura_AddDebris(player,state,config);
	CG_Aura_AddDLight(player,state,config);
	CG_Aura_AddParticleSystem(player,state,config);
	CG_Aura_ConvexHullRender(player,state,config);
}
/*==============
CG_AuraStart
==============*/
void CG_AuraStart(centity_t *player){
	// Get the aura system corresponding to the player
	int clientNum = player->currentState.clientNum;
	int tier = cgs.clientinfo[clientNum].tierCurrent;
	auraState_t* state;
	auraConfig_t* config;
	vec3_t groundPoint;
	trace_t trace;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS){
		CG_Error( "Bad clientNum on player entity");
		return;
	}
	state = &auraStates[clientNum];
	config = &state->configurations[tier];
	if(state->isActive){return;}
	state->isActive = qtrue;
	state->lightAmt = config->lightMin;
	// create a small camerashake
	CG_AddEarthquake(player->lerpOrigin,1000,1,0,1,200);
	// We don't want smoke jets if this is a boost aura instead of a charge aura.
	if(!(player->currentState.playerBitFlags & usingBoost)){
		// Check if we're on, or near ground level
		VectorCopy(player->lerpOrigin,groundPoint);
		groundPoint[2] -= 48;
		CG_Trace(&trace,player->lerpOrigin,NULL,NULL,groundPoint,player->currentState.number,CONTENTS_SOLID);
		if(trace.allsolid || trace.startsolid){trace.fraction = 1;}
		if(trace.fraction < 1){
			vec3_t tempAxis[3];
			// Place the explosion just a bit off the surface
			VectorNormalize2(trace.plane.normal,groundPoint);
			VectorMA(trace.endpos,0,groundPoint,groundPoint);
			VectorNormalize2(trace.plane.normal,tempAxis[0]);
			MakeNormalVectors(tempAxis[0],tempAxis[1],tempAxis[2]);
			PSys_SpawnCachedSystem("AuraSmokeBurst",groundPoint,tempAxis,NULL,NULL,qfalse,qfalse);
		}
	}
}
/*============
CG_AuraEnd
============*/
void CG_AuraEnd(centity_t *player){
	int clientNum = player->currentState.clientNum;
	auraState_t* state;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS){
		CG_Error( "Bad clientNum on player entity");
		return;
	}
	state = &auraStates[clientNum];
	if(!state->isActive){return;}
	state->isActive = qfalse;
}
/*=======================
CG_RegisterClientAura
=======================*/
#define MAX_AURA_FILELEN 32000
void parseAura(char* path,auraConfig_t* aura){
	fileHandle_t auraCFG;
	int i;
	int fileLength;
	char* token;
	char* parse;
	char fileContents[32000];
	qboolean hasHeader = qfalse;
	if(trap_FS_FOpenFile(path,0,FS_READ)>0){
		fileLength = trap_FS_FOpenFile(path,&auraCFG,FS_READ);
		trap_FS_Read(fileContents,fileLength,auraCFG);
		fileContents[fileLength] = 0;
		trap_FS_FCloseFile(auraCFG);
		parse = fileContents;
		while(1){
			token = COM_Parse(&parse);
			if(!token[0]){break;}
			if(!hasHeader){
				if(!Q_stricmp(token,"[Aura]")){hasHeader = qtrue;}
			}
			else{
				//Eagle: Found another header, marking the end of the aura section.
				if(token[0] == '['){break;}
				else if(!Q_stricmp(token,"showAura")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->showAura = strlen(token) == 4 ? qtrue : qfalse;
				}
				else if(!Q_stricmp(token,"auraAlways")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->auraAlways = strlen(token) == 4 ? qtrue : qfalse;
				}
				else if(!Q_stricmp(token,"auraShader")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->auraShader = trap_R_RegisterShader(token);
				}
				else if (!Q_stricmp(token,"auraColor")){
					for(i=0;i<3;i++){
						token = COM_Parse(&parse);
						if(!token[0]){break;}
						aura->auraColor[i] = atof( token);
						if(aura->auraColor[i] < 0){aura->auraColor[i] = 0;}
						if(aura->auraColor[i] > 1){aura->auraColor[i] = 1;}
					}
				}
				else if(!Q_stricmp(token,"auraScale")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->auraScale = atof(token);
				}
				else if(!Q_stricmp(token,"auraLength")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->tailLength = atoi(token);
				}
				else if(!Q_stricmp(token,"auraNumTags")){
					for(i=0;i<3;i++){
						token = COM_Parse(&parse);
						if(!token[0]){break;}
						aura->numTags[i] = atoi(token);
					}
				}
				else if(!Q_stricmp(token,"showLight")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->showLight = strlen(token) == 4 ? qtrue : qfalse;
				}
				else if(!Q_stricmp(token,"lightColor")){
					for(i=0;i<3;i++){
						token = COM_Parse(&parse);
						if(!token[0]){break;}
						aura->lightColor[i] = atof( token);
						if(aura->lightColor[i] < 0){aura->lightColor[i] = 0;}
						if(aura->lightColor[i] > 1){aura->lightColor[i] = 1;}
					}
				}
				else if(!Q_stricmp(token,"lightMin")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->lightMin = atoi(token);
				}
				else if(!Q_stricmp(token,"lightMax")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->lightMax = atoi(token);
				}
				else if(!Q_stricmp(token,"lightGrowthRate")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->lightGrowthRate = atoi(token);
				}
				else if(!Q_stricmp(token,"showTrail")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->showTrail = strlen(token) == 4 ? qtrue : qfalse;
				}
				else if(!Q_stricmp(token,"trailShader")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->trailShader = trap_R_RegisterShader(token);
				}
				else if(!Q_stricmp(token,"trailColor")){
					for(i=0;i<3;i++){
						token = COM_Parse(&parse);
						if(!token[0]){break;}
						aura->trailColor[i] = atof(token);
						if(aura->trailColor[i] < 0){aura->trailColor[i] = 0;}
						if(aura->trailColor[i] > 1){aura->trailColor[i] = 1;}
					}
				}
				else if(!Q_stricmp(token,"trailWidth")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->trailWidth = atoi(token);
				}
				else if(!Q_stricmp(token,"showLightning")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->showLightning = strlen(token) == 4 ? qtrue : qfalse;
				}
				else if(!Q_stricmp(token,"lightningShader")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->lightningShader = trap_R_RegisterShaderNoMip(token);
				}
				else if(!Q_stricmp(token,"lightningColor")){
					for(i=0;i<3;i++){
						token = COM_Parse(&parse);
						if(!token[0]){break;}
						aura->lightningColor[i] = atof(token);
						if(aura->lightningColor[i] < 0){aura->lightningColor[i] = 0;}
						if(aura->lightningColor[i] > 1){aura->lightningColor[i] = 1;}
					}
				}
				else if(!Q_stricmp(token,"showDebris")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->showDebris = strlen(token) == 4 ? qtrue : qfalse;
				}
				else if(!Q_stricmp(token,"particleSystem")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					else{Q_strncpyz(aura->particleSystem,token,sizeof(aura->particleSystem));}
				}
				else if(!Q_stricmp(token,"chargeLoop")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->chargeLoopSound = trap_S_RegisterSound(token,qfalse);
				}
				else if(!Q_stricmp(token,"chargeStart")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->chargeStartSound = trap_S_RegisterSound(token,qfalse);
				}
				else if(!Q_stricmp(token,"boostLoop")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->boostLoopSound = trap_S_RegisterSound(token,qfalse);
				}
				else if(!Q_stricmp(token,"boostStart")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->boostStartSound = trap_S_RegisterSound(token,qfalse);
				}
				else{
					//Eagle: This warning shows up for parameters too. We could prevent this by
					//detecting newlines or storing buffer per line in an array of strings.
					CG_Printf("^3CG_Aura_ParseConfig(): unknown keyword '%s' in: '%s'.\n",token,path);
				}
			}
		}
		if(!hasHeader){
			CG_Printf("^3CG_Aura_ParseConfig(): missing '[Aura]' header in: '%s'\n",path);
		}
	}
}
void CG_RegisterClientAura(int clientNum,clientInfo_t *ci){
	char filename[MAX_QPATH*2];
	memset(&(auraStates[clientNum]),0,sizeof(auraState_t));
	for(int i=0;i<8;i++){ 
		ci->auraConfig[i] = &(auraStates[clientNum].configurations[i]);
		memset(ci->auraConfig[i],0,sizeof(auraConfig_t));
		Com_sprintf(filename,sizeof(filename),"players/tierDefault.cfg");
		parseAura(filename,ci->auraConfig[i]);
		Com_sprintf(filename,sizeof(filename),"players/%s/tier%i/tier.cfg",ci->modelName,i+1);
		parseAura(filename,ci->auraConfig[i]);
	}
}
/*
==================
CG_AuraSpikes

For inner aura
==================
*/
localEntity_t *CG_AuraSpike(const vec3_t p,const vec3_t vel,float radius,float duration,int startTime,int fadeInTime,int leFlags,centity_t *player){
	localEntity_t	*le;
	refEntity_t		*re;
	auraState_t		*state;
	auraConfig_t	*config;
	// Get the aura system corresponding to the player
	int clientNum = player->currentState.clientNum;
	int tier;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS){CG_Error("Bad clientNum on player entity");}
	state = &auraStates[clientNum];
	tier = cgs.clientinfo[clientNum].tierCurrent;
	config = &state->configurations[tier];
	le = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius = radius;
	re = &le->refEntity;
	re->radius = radius;
	re->shaderTime = startTime / 1000.0f;
	le->leType = LE_MOVE_SCALE_FADE;
	le->startTime = startTime;
	le->fadeInTime = fadeInTime;
	le->endTime = startTime + duration;
	le->lifeRate = fadeInTime > startTime ? 1.0f / (le->endTime - le->fadeInTime) : 1.0f / (le->endTime - le->startTime);
	le->color[0] = config->auraColor[0];
	le->color[1] = config->auraColor[1]; 
	le->color[2] = config->auraColor[2];
	le->color[3] = 1.0f;
	le->pos.trType = TR_LINEAR;
	le->pos.trTime = startTime;
	VectorCopy(vel,le->pos.trDelta);
	VectorCopy(p,le->pos.trBase);
	VectorCopy(p,re->origin);
	re->customShader = config->auraShader;
	// rage pro can't alpha fade, so use a different shader
	re->shaderRGBA[0] = le->color[0] * 255;
	re->shaderRGBA[1] = le->color[1] * 255;
	re->shaderRGBA[2] = le->color[2] * 255;
	re->shaderRGBA[3] = 255;
	re->reType = RT_SPRITE;
	re->radius = le->radius;
	return le;
}
