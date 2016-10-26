// Copyright (C) 2003-2005 RiO
// cg_auras.h -- aura headers
#define AURATAGS_LEGS	0
#define AURATAGS_TORSO	1
#define AURATAGS_HEAD	2
#define MAX_AURATAGS	48	// old md3 limit

typedef struct auraTag_s {
	vec3_t		pos_world;
	vec2_t		pos_screen;
	vec3_t		normal;
	float		length;
	qboolean	is_tail;
} auraTag_t;

typedef struct auraConfig_s {
	qboolean		showAura;
	qboolean		auraAlways;
	qhandle_t		auraShader;
	vec3_t			auraColor;
	float			auraScale;
	float			tailLength;
	qboolean		showTrail;
	qhandle_t		trailShader;
	vec3_t			trailColor;
	float			trailWidth;
	qboolean		showLight;
	vec3_t			lightColor;
	float			lightMin;
	float			lightMax;
	float			lightGrowthRate;
	qboolean		showLightning;
	qhandle_t		lightningShader;
	vec3_t			lightningColor;
	qboolean		showDebris;
	sfxHandle_t		chargeStartSound;
	sfxHandle_t		chargeLoopSound;
	sfxHandle_t		boostStartSound;
	sfxHandle_t		boostLoopSound;
	int				numTags[3]; // 0 = head, 1 = torso, 2 = legs
}auraConfig_t;

typedef struct auraState_s {
	qboolean		isActive;
	auraTag_t		convexHull[MAX_AURATAGS+1]; // Need MAX_AURATAGS + 1 extra for the tail position
	int				convexHullCount;
	float			convexHullCircumference;
	vec3_t			origin;
	vec3_t			rootPos; // Root position; Where the aura 'opens up'
	vec3_t			tailPos; // Tail position
	float			modulate;
	float			lightAmt;
	int				lightDev;
	auraConfig_t	configurations[8]; // 7 = max tier
} auraState_t;
