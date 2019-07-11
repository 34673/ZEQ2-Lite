// Copyright (C) 2003-2005 RiO
// cg_auras.h -- aura headers
//Eagle: MD3 max tag per file
//Might be later removed if we are using IQM bones as tags
#define MAX_AURATAGS	48
typedef struct auraTag_s{
	vec3_t normal;
	vec3_t pos_world;
	vec2_t pos_screen;
	float length;
	qboolean is_tail;
}auraTag_t;
typedef struct auraConfig_s{
	qboolean showAura;
	qboolean auraAlways;
	qhandle_t auraShader;
	vec3_t auraColor;
	float auraScale;
	float tailLength;
	qboolean showTrail;
	qhandle_t trailShader;
	vec3_t trailColor;
	float trailWidth;
	qboolean showLight;
	vec3_t lightColor;
	float lightMin;
	float lightMax;
	float lightGrowthRate;
	qboolean showLightning;
	qhandle_t lightningShader;
	vec3_t lightningColor;
	qboolean showDebris;
	qhandle_t chargeStartSound;
	qhandle_t chargeLoopSound;
	qhandle_t boostStartSound;
	qhandle_t boostLoopSound;
	char particleSystem[MAX_QPATH];
	int numTags[3]; // 0 = Legs, 1 = Torso, 2 = Head
}auraConfig_t;
typedef struct auraState_s{
	qboolean isActive;
	auraTag_t convexHull[MAX_AURATAGS+1]; // Need MAX_AURATAGS + 1 extra for the tail position
	int convexHullCount;
	float convexHullCircumference;
	vec3_t origin;
	vec3_t rootPos; // Root position; Where the aura 'opens up'
	vec3_t tailPos; // Tail position
	float modulate;
	float lightAmt;
	int lightDev;
	auraConfig_t configurations[8]; // 7 = max tier
}auraState_t;
