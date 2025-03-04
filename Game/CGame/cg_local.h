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
#include "../../Shared/q_shared.h" 
#include "../../Engine/renderer/tr_types.h"
#include "../Game/bg_public.h"
// ADDING FOR ZEQ2
#include "../Game/bg_userweapons.h"
#include "../Game/g_tiers.h"
#include "cg_userweapons.h"
#include "cg_auras.h"
#include "cg_tiers.h"
// END ADDING
#include "cg_public.h"


// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define	POWERUP_BLINKS		5

#define	POWERUP_BLINK_TIME	1000
#define	FADE_TIME			200
#define	PULSE_TIME			200
#define	DAMAGE_DEFLECT_TIME	100
#define	DAMAGE_RETURN_TIME	400
#define DAMAGE_TIME			500
#define	LAND_DEFLECT_TIME	150
#define	LAND_RETURN_TIME	300
#define	STEP_TIME			200
#define	DUCK_TIME			100
#define	PAIN_TWITCH_TIME	200
#define	WEAPON_SELECT_TIME	1400
#define	ITEM_SCALEUP_TIME	1000
#define	ZOOM_TIME			150
#define	ITEM_BLOB_TIME		200
#define	MUZZLE_FLASH_TIME	80 //20
#define	SINK_TIME			1000		// time for fragments to sink into ground before going away
#define	ATTACKER_ANIM_TIME	10000
#define	REWARD_TIME			3000

#define	PULSE_SCALE			1.5			// amount to scale up the icons when activating

#define	MAX_STEP_CHANGE		32

#define	MAX_VERTS_ON_POLY	10
#define	MAX_MARK_POLYS		256

#define STAT_MINUS			10	// num frame for '-' stats digit

#define	ICON_SIZE			48
#define	CHAR_WIDTH			8	// 32
#define	CHAR_HEIGHT			12	// 48
#define	TEXT_ICON_SPACE		4

#define	TEAMCHAT_WIDTH		80
#define TEAMCHAT_HEIGHT		8

// very large characters
#define	GIANT_WIDTH			32
#define	GIANT_HEIGHT		48

#define	NUM_CROSSHAIRS		10

#define TEAM_OVERLAY_MAXNAME_WIDTH	12
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	16

#define	DEFAULT_MODEL			"goku"
#define	DEFAULT_TEAM_MODEL		"goku"
#define	DEFAULT_TEAM_HEAD		"goku"

#define DEFAULT_REDTEAM_NAME		"Evil"
#define DEFAULT_BLUETEAM_NAME		"Good"

typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,

	FOOTSTEP_TOTAL
} footstep_t;

typedef enum {
	IMPACTSOUND_DEFAULT,
	IMPACTSOUND_METAL,
	IMPACTSOUND_FLESH
} impactSound_t;

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;	// may include ANIM_TOGGLEBIT
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;


typedef struct {
	lerpFrame_t		legs, torso, head, camera, flag;
	int				painTime;
	int				painDirection;	// flip from 0 to 1
	int				lightningFiring;

	int				railFireTime;


	// needed to obtain tag positions after player entity has been processed.
	// For linking beam attacks, particle systems, etc.
	refEntity_t		legsRef, torsoRef, headRef, cameraRef;
} playerEntity_t;

//=================================================



// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	entityState_t	currentState;	// from cg.frame
	entityState_t	nextState;		// from cg.nextFrame, if available
	qboolean		interpolate;	// true if next is valid to interpolate to
	qboolean		currentValid;	// true if cg.frame holds this entity

	int				muzzleFlashTime;	// move to playerEntity?
	int				previousEvent;
	int				teleportFlag;

	int				trailTime;		// so missile trails can handle dropped initial packets
	int				dustTrailTime;
	int				miscTime;

	int				snapShotTime;	// last time this entity was found in a snapshot

	playerEntity_t	pe;

	int				errorTime;		// decay the error from this time
	vec3_t			errorOrigin;
	vec3_t			errorAngles;
	
	qboolean		extrapolated;	// false if origin / angles is an interpolation
	vec3_t			rawOrigin;
	vec3_t			rawAngles;

	vec3_t			beamEnd;

	// exact interpolated position of entity on this frame
	vec3_t			lerpOrigin;
	vec3_t			lerpAngles;
	
	
	// ADDING FOR ZEQ2
	float			lerpPrim;
	float			lerpSec;

	int				lastTrailTime;		// last cg.time the trail for this cent was rendered
	int				lastPVSTime;		// last cg.time the entity was in the PVS
	int				lastAuraPSysTime;	// last cg.time especially for the coupled aura debris system
	// END ADDING
} centity_t;


//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s	*prevMark, *nextMark;
	int			time;
	qhandle_t	markShader;
	qboolean	alphaFade;		// fade alpha instead of rgb
	float		color[4];
	poly_t		poly;
	polyVert_t	verts[MAX_VERTS_ON_POLY];
} markPoly_t;


typedef enum {
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_NO,
	LE_FADE_RGB,
	LE_FADE_ALPHA,
	LE_SCALE_FADE,
	LE_SCALE_FADE_RGB,
	LE_SCOREPLUM,
	LE_ZEQEXPLOSION,
	LE_ZEQSMOKE,
	LE_ZEQSPLASH,
	LE_STRAIGHTBEAM_FADE
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE  = 0x0001,			// do not scale size over time
	LEF_TUMBLE			 = 0x0002,			// tumble over time, used for ejecting shells
	LEF_SOUND1			 = 0x0004,			// sound 1 for kamikaze
	LEF_SOUND2			 = 0x0008			// sound 2 for kamikaze
} leFlag_t;

typedef enum {
	LEMT_NONE,
	LEMT_BURN,
	LEMT_BLOOD
} leMarkType_t;			// fragment local entities can leave marks on walls

typedef enum {
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_BRASS
} leBounceSoundType_t;	// fragment local entities can make sounds on impacts

typedef struct localEntity_s {
	struct localEntity_s	*prev, *next;
	leType_t		leType;
	int				leFlags;

	int				startTime;
	int				endTime;
	int				fadeInTime;

	float			lifeRate;			// 1.0 / (endTime - startTime)

	trajectory_t	pos;
	trajectory_t	angles;

	float			bounceFactor;		// 0.0 = no bounce, 1.0 = perfect

	float			color[4];

	float			radius;

	float			light;
	vec3_t			lightColor;

	leMarkType_t		leMarkType;		// mark to leave on fragment impact
	leBounceSoundType_t	leBounceSoundType;

	refEntity_t		refEntity;		
} localEntity_t;

//======================================================================


typedef struct {
	int				client;
	int				score;
	int				ping;
	int				time;
	int				scoreFlags;
	int				powerUps;
	int				accuracy;
	int				impressiveCount;
	int				excellentCount;
	int				guantletCount;
	int				defendCount;
	int				assistCount;
	int				captures;
	qboolean	perfect;
	int				team;
} score_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define	MAX_CUSTOM_SOUNDS	256

typedef struct {
	qboolean		infoValid;
	char			name[MAX_QPATH];
	team_t			team;
	vec3_t			color1;
	vec3_t			color2;
	
	byte c1RGBA[4];
	byte c2RGBA[4];

	int				score;			// updated by score servercmds
	int				location;		// location index for team mode
	int				powerLevel;			// you only get this info about your teammates
	int				armor;
	int				curWeapon;
	int				wins, losses;	// in tourney mode
	int				teamTask;		// task in teamplay (offence/defence)
	qboolean		teamLeader;		// true when this is a team leader
	int				powerups;		// so can display quad/flag status
	int				medkitUsageTime;
	int				invulnerabilityStartTime;
	int				invulnerabilityStopTime;
	int				breathPuffTime;
	int				bubbleTrailTime;
	char			modelName[MAX_QPATH];
	char			skinName[MAX_QPATH];
	char			cameraModelName[MAX_QPATH];
	char			headModelName[MAX_QPATH];
	char			headSkinName[MAX_QPATH];
	char			legsModelName[MAX_QPATH];
	char			legsSkinName[MAX_QPATH];
	char			redTeam[MAX_TEAMNAME];
	char			blueTeam[MAX_TEAMNAME];
	qboolean		deferred;
	qboolean		newAnims;		// true if using the new mission pack animations
	qboolean		fixedlegs;		// true if legs yaw is always the same as torso yaw
	qboolean		fixedtorso;		// true if torso never changes yaw
	vec3_t			headOffset;		// move head in icon views
	qboolean		overrideHead;
	footstep_t		footsteps;
	qhandle_t		modelIcon;
	animation_t		animations[MAX_TOTALANIMATIONS];
	animation_t		camAnimations[MAX_TOTALANIMATIONS];
	sfxHandle_t		sounds[MAX_TIERS][MAX_CUSTOM_SOUNDS];
	//ADDING FOR ZEQ2
	int				damageModelState;
	int				damageTextureState;
	qboolean		transformStart;
	int				transformLength;
	int				lockStartTimer;
	int				tierCurrent;
	int				tierMax;
	int				cameraBackup[4];
	tierConfig_cg	tierConfig[MAX_TIERS];
	auraConfig_t	*auraConfig[MAX_TIERS];
} clientInfo_t;


// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean		registered;
	qhandle_t		handsModel;			// the hands don't actually draw, they just position the weapon
	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;

	vec3_t			weaponMidpoint;		// so it will rotate centered instead of by tag

	float			flashDlight;
	vec3_t			flashDlightColor;
	sfxHandle_t		flashSound[4];		// fast firing weapons randomly choose
	sfxHandle_t		voiceSound[4];

	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void			(*missileTrailFunc)( centity_t *, const struct weaponInfo_s *wi );
	float			missileDlight;
	vec3_t			missileDlightColor;
	int				missileRenderfx;

	void			(*ejectBrassFunc)( centity_t * );

	float			trailRadius;
	float			wiTrailTime;

	sfxHandle_t		readySound;
	sfxHandle_t		firingSound;
} weaponInfo_t;


// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean		registered;
	qhandle_t		icon;
} itemInfo_t;


typedef struct {
	int				itemNum;
} powerupInfo_t;


#define MAX_SKULLTRAIL		10

typedef struct {
	vec3_t positions[MAX_SKULLTRAIL];
	int numpositions;
} skulltrail_t;


#define MAX_REWARDSTACK		10
#define MAX_SOUNDBUFFER		20
//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS	16
 
#if EARTHQUAKE_SYSTEM	// JUHOX: definitions
typedef struct {
	vec3_t origin;
	float radius;	// negative for global earthquakes
	float amplitude;
	int startTime;
	int endTime;
	int fadeInTime;
	int fadeOutTime;
} earthquake_t;
#define MAX_EARTHQUAKES 64
#endif

typedef struct {
	qhandle_t	shader;
	qboolean	active;
	int			x;
	int			y;
	int			width;
	int			height;
	int			endTime;
} overlay2D;

typedef struct {
	int			clientFrame;		// incremented each frame

	int			clientNum;
	qboolean	resetValues;
	qboolean	demoPlayback;
	qboolean	levelShot;			// taking a level menu screenshot
	int			deferredPlayerLoading;
	qboolean	loading;			// don't defer players at initial startup
	qboolean	intermissionStarted;	// don't play voice rewards, because game will end shortly
	// there are only one or two snapshot_t that are relevent at a time
	int			latestSnapshotNum;	// the number of snapshots the client system has received
	int			latestSnapshotTime;	// the time from latestSnapshotNum, so we don't need to read the snapshot yet
	snapshot_t	*snap;				// cg.snap->serverTime <= cg.time
	snapshot_t	*nextSnap;			// cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t	activeSnapshots[2];
	float		frameInterpolation;	// (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)
	qboolean	thisFrameTeleport;
	qboolean	nextFrameTeleport;
	qboolean	lockonStart;
	int			frametime;		// cg.time - cg.oldTime
	int			time;			// this is the time value that the client
								// is rendering at.
	int			oldTime;		// time at last frame, used for missile trails and prediction checking
	int			physicsTime;	// either cg.snap->time or cg.nextSnap->time
	int			timelimitWarnings;	// 5 min, 1 min, overtime
	int			fraglimitWarnings;
	qboolean	mapRestart;			// set on a map restart to set back the weapon
	qboolean	renderingThirdPerson;		// during deaths, chasecams, etc
	// prediction state
	qboolean	hyperspace;				// true if prediction has hit a trigger_teleport
	playerState_t	predictedPlayerState;
	centity_t		predictedPlayerEntity;
	qboolean	validPPS;				// clear until the first call to CG_PredictPlayerState
	int			predictedErrorTime;
	vec3_t		predictedError;
	int			eventSequence;
	int			predictableEvents[MAX_PREDICTED_EVENTS];
	float		stepChange;				// for stair up smoothing
	int			stepTime;
	float		duckChange;				// for duck viewheight smoothing
	int			duckTime;
	float		landChange;				// for landing hard
	int			landTime;

	// input state sent to server
	int			weaponSelect;
	int			weaponDesired;
	byte		weaponChanged;
	byte		weaponSelectionMode; // 0 means no change; 1 means previous weapon; 2 means next weapon; 3 means weapon directly selected
	int			tierSelect;
	byte		tierSelectionMode; //0 means no change; 1 means previous tier; 2 means next tier; 3 means tier directly selected
	int			tierCurrent;

	// auto rotating items
	vec3_t		autoAngles;
	vec3_t		autoAxis[3];
	vec3_t		autoAnglesFast;
	vec3_t		autoAxisFast[3];
	byte		drawWeaponBar;

	// view rendering
	refdef_t	refdef;
	vec3_t		refdefViewAngles;		// will be converted to refdef.viewaxis

#if EARTHQUAKE_SYSTEM	// JUHOX: earthquake variables
	int earthquakeStartedTime;
	int earthquakeEndTime;
	float earthquakeAmplitude;
	int earthquakeFadeInTime;
	int earthquakeFadeOutTime;
	int earthquakeSoundCounter;
	int lastEarhquakeSoundStartedTime;
	earthquake_t earthquakes[MAX_EARTHQUAKES];
	float additionalTremble;
#endif
	// zoom key
	qboolean	zoomed;
	int			zoomTime;
	float		zoomSensitivity;
	// information screen text during loading
	char		infoScreenText[MAX_STRING_CHARS];
	// scoreboard
	int			scoresRequestTime;
	int			numScores;
	int			selectedScore;
	int			teamScores[2];
	score_t		scores[MAX_CLIENTS];
	qboolean	showScores;
	qboolean	scoreBoardShowing;
	int			scoreFadeTime;
	char		killerName[MAX_NAME_LENGTH];
	char		spectatorList[MAX_STRING_CHARS];		// list of names
	int			spectatorLen;							// length of list
	float		spectatorWidth;							// width in device units
	int			spectatorTime;							// next time to offset
	int			spectatorPaintX;						// current paint x
	int			spectatorPaintX2;						// current paint x
	int			spectatorOffset;						// current offset from start
	int			spectatorPaintLen; 						// current offset from start

	// centerprinting
	int			centerPrintTime;
	int			centerPrintCharWidth;
	int			centerPrintY;
	char		centerPrint[1024];
	int			centerPrintLines;

	// Dynamic 2D effects
	overlay2D	scripted2D[16];

	// crosshair client ID
	int			crosshairClientNum;
	int			crosshairClientTime;

	// screen flashing
	int			screenFlashTime;
	int			screenFlastTimeTotal;
	int			screenFlashFadeTime;
	float		screenFlashFadeAmount;

	// powerup active flashing
	int			powerupActive;
	int			powerupTime;

	// attacking player
	int			attackerTime;
	int			voiceTime;

	// sound buffer mainly for announcer sounds
	int			soundBufferIn;
	int			soundBufferOut;
	int			soundTime;
	qhandle_t	soundBuffer[MAX_SOUNDBUFFER];

	// for voice chat buffer
	int			voiceChatTime;
	int			voiceChatBufferIn;
	int			voiceChatBufferOut;

	// warmup countdown
	int			warmup;
	int			warmupCount;

	//==========================

	int			itemPickup;
	int			itemPickupTime;
	int			itemPickupBlendTime;	// the pulse around the crosshair is timed seperately

	int			weaponSelectTime;
	int			weaponAnimation;
	int			weaponAnimationTime;

	// ADDING FOR ZEQ2
	int			PLBar_foldPct;	// Need to know how far we've folded in or out already
	// END ADDING

	// blend blobs
	float		damageTime;
	float		damageX, damageY, damageValue;

	// status bar head
	float		headYaw;
	float		headEndPitch;
	float		headEndYaw;
	int			headEndTime;
	float		headStartPitch;
	float		headStartYaw;
	int			headStartTime;

	// view movement
	float		v_dmg_time;
	float		v_dmg_pitch;
	float		v_dmg_roll;

	// temp working variables for player view
	float		bobfracsin;
	int			bobcycle;
	float		xyspeed;
	int			nextOrbitTime;

	//qboolean cameraMode;		// if rendering from a loaded camera

	qboolean	guide_view;
	vec3_t		guide_target;	// where to look for guided missiles

	// END ADDING

} cg_t;


// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
#define MAX_MEDIA_SOUNDS 10
typedef struct {
	qhandle_t	charsetShader;
	qhandle_t	charsetProp;
	qhandle_t	charsetPropGlow;
	qhandle_t	charsetPropB;
	qhandle_t	whiteShader;
	qhandle_t	clearShader;
	qhandle_t	deferShader;
	qhandle_t	hudShader;
	qhandle_t	markerAscendShader;
	qhandle_t	markerDescendShader;
	qhandle_t	breakLimitShader;

	qhandle_t	RadarBlipShader;
	qhandle_t	RadarBlipTeamShader;
	qhandle_t	RadarBurstShader;
	qhandle_t	RadarWarningShader;
	qhandle_t	RadarMidpointShader;
	qhandle_t	speedLineShader;
	qhandle_t	speedLineSpinShader;
	qhandle_t	friendShader;
	qhandle_t	chatBackgroundShader;
	qhandle_t	chatBubble;
	qhandle_t	connectionShader;
	qhandle_t	selectShader;
	qhandle_t	viewBloodShader;
	qhandle_t	tracerShader;
	qhandle_t	crosshairShader[NUM_CROSSHAIRS];
	qhandle_t	backTileShader;
	qhandle_t	globalCelLighting;
	qhandle_t	waterSplashSkin;
	qhandle_t	waterSplashModel;
	qhandle_t	waterRippleSkin;
	qhandle_t	waterRippleModel;
	qhandle_t	waterRippleSingleSkin;
	qhandle_t	waterRippleSingleModel;
	qhandle_t	dirtPushSkin;
	qhandle_t	dirtPushModel;
	qhandle_t	dirtPushShader;
	qhandle_t	waterBubbleLargeShader;
	qhandle_t	waterBubbleMediumShader;
	qhandle_t	waterBubbleSmallShader;
	qhandle_t	waterBubbleTinyShader;
	qhandle_t	bloodTrailShader;
	qhandle_t	bfgLFGlare;	// JUHOX
	qhandle_t	bfgLFDisc;	// JUHOX
	qhandle_t	bfgLFRing;	// JUHOX
	qhandle_t	bfgLFLine;	// JUHOX
	qhandle_t	bfgLFStar;	// JUHOX
	qhandle_t	numberShaders[11];
	qhandle_t	shadowMarkShader;
	qhandle_t	wakeMarkShader;
	qhandle_t	teleportEffectModel;
	qhandle_t	meleeSpeedEffectShader;
	qhandle_t	meleePowerEffectShader;
	qhandle_t	powerStruggleRaysEffectShader;
	qhandle_t	powerStruggleShockwaveEffectShader;
	qhandle_t	boltEffectShader;
	qhandle_t	auraLightningSparks1;
	qhandle_t	auraLightningSparks2;
	qhandle_t	invulnerabilityPowerupModel;
	sfxHandle_t	nullSound;
	sfxHandle_t	lockonStart;
	sfxHandle_t	selectSound;
	sfxHandle_t	useNothingSound;
	sfxHandle_t	wearOffSound;
	sfxHandle_t	footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t	teleInSound;
	sfxHandle_t	teleOutSound;
	sfxHandle_t	respawnSound;
	sfxHandle_t talkSound;
	sfxHandle_t landSound;
	sfxHandle_t fallSound;
	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;
	qhandle_t cursor;
	qhandle_t selectCursor;
	qhandle_t sizeCursor;
	// ADDING FOR ZEQ2
	sfxHandle_t	blinkingSound;
	sfxHandle_t	radarwarningSound;
	sfxHandle_t	lightspeedSound1;
	sfxHandle_t	lightspeedSound2;
	sfxHandle_t	lightspeedSound3;
	sfxHandle_t	lightspeedSound4;
	sfxHandle_t	lightspeedSound5;
	sfxHandle_t bigLightning[MAX_MEDIA_SOUNDS];
	sfxHandle_t	blockSound;
	sfxHandle_t	knockbackSound;
	sfxHandle_t	knockbackLoopSound;
	sfxHandle_t	stunSound;
	sfxHandle_t	speedMeleeSound;
	sfxHandle_t	speedMissSound;
	sfxHandle_t	speedBlockSound;
	sfxHandle_t	powerMeleeSound;
	sfxHandle_t	powerStunSound1;
	sfxHandle_t	powerStunSound2;
	sfxHandle_t	powerMissSound;
	sfxHandle_t	airBrake1;
	sfxHandle_t	airBrake2;
	sfxHandle_t hover;
	sfxHandle_t hoverFast;
	sfxHandle_t hoverLong;
	sfxHandle_t smallSplash[MAX_MEDIA_SOUNDS];
	sfxHandle_t mediumSplash[MAX_MEDIA_SOUNDS];
	sfxHandle_t largeSplash[MAX_MEDIA_SOUNDS];
	sfxHandle_t extraLargeSplash[MAX_MEDIA_SOUNDS];
	// END ADDING
} cgMedia_t;


// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t		gameState;			// gamestate from server
	glconfig_t		glconfig;			// rendering configuration
	float			screenXScale;		// derived from glconfig
	float			screenYScale;
	float			screenXBias;

	int				serverCommandSequence;	// reliable command stream counter
	int				processedSnapshotNum;// the number of snapshots cgame has requested

	qboolean		localServer;		// detected on startup by checking sv_running
	qboolean		clientPaused;

	// parsed from serverinfo
	gametype_t		gametype;
	int				dmflags;
	int				teamflags;
	int				fraglimit;
	int				capturelimit;
	int				timelimit;
	int				maxclients;
	char			mapname[MAX_QPATH];
	char			redTeam[MAX_QPATH];
	char			blueTeam[MAX_QPATH];

	int				voteTime;
	int				voteYes;
	int				voteNo;
	qboolean		voteModified;			// beep whenever changed
	char			voteString[MAX_STRING_TOKENS];

	int				teamVoteTime[2];
	int				teamVoteYes[2];
	int				teamVoteNo[2];
	qboolean		teamVoteModified[2];	// beep whenever changed
	char			teamVoteString[2][MAX_STRING_TOKENS];

	int				levelStartTime;

	int				scores1, scores2;		// from configstrings
	int				redflag, blueflag;		// flag status from configstrings
	int				flagStatus;

	qboolean  newHud;

	//
	// locally derived information from gamestate
	//
	qhandle_t		gameModels[MAX_MODELS];
	sfxHandle_t		gameSounds[MAX_SOUNDS];

	int				numInlineModels;
	qhandle_t		inlineDrawModel[MAX_MODELS];
	vec3_t			inlineModelMidpoints[MAX_MODELS];

	clientInfo_t	clientinfo[MAX_CLIENTS];

	// teamchat width is *3 because of embedded color codes
	char			teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH*3+1];
	int				teamChatMsgTimes[TEAMCHAT_HEIGHT];
	int				teamChatPos;
	int				teamLastChatPos;

	int cursorX;
	int cursorY;
	qboolean eventHandling;
	qboolean mouseCaptured;
	qboolean sizingHud;
	void *capturedItem;
	qhandle_t activeCursor;

	// orders
	int currentOrder;
	qboolean orderPending;
	int orderTime;
	int currentVoiceClient;
	int acceptOrderTime;
	int acceptTask;
	int acceptLeader;
	char acceptVoice[MAX_NAME_LENGTH];
	char	messages[3][256];
	int		messageTimer[3];
	int		messageClient[3];
	int		chatTimer;
	// media
	cgMedia_t	media;
} cgs_t;

//==============================================================================

extern	cgs_t			cgs;
extern	cg_t			cg;
extern	centity_t		cg_entities[MAX_GENTITIES];
extern	weaponInfo_t	cg_weapons[MAX_WEAPONS];
extern	itemInfo_t		cg_items[MAX_ITEMS];
extern	markPoly_t		cg_markPolys[MAX_MARK_POLYS];

#define EXTERN_CG_CVAR
	#include "cg_cvar.h"
#undef EXTERN_CG_CVAR

extern	radar_t			cg_playerOrigins[MAX_CLIENTS];

//
// cg_main.c
//
const char *CG_ConfigString( int index );
const char *CG_Argv( int arg );

void QDECL CG_Printf( const char *msg, ... ) __attribute__ ((format (printf, 1, 2)));
void QDECL CG_Error( const char *msg, ... ) __attribute__ ((noreturn, format (printf, 1, 2)));

void CG_UpdateCvars( void );

int CG_CrosshairPlayer( void );
int CG_LastAttacker( void );
void CG_LoadMenus(const char *menuFile);
void CG_KeyEvent(int key, qboolean down);
void CG_MouseEvent(int x, int y);
void CG_EventHandling(int type);
void CG_RankRunFrame( void );
void CG_SetScoreSelection(void *menu);
score_t *CG_GetSelectedScore( void );
void CG_BuildSpectatorString( void );
//
// cg_music.c
//
void CG_Music_Start(void);
void CG_Music_Update(void);
void CG_Music_Play(char* track,int duration);
void CG_Music_NextTrack(void);
void CG_Music_FadeNext(void);
int CG_Music_GetMilliseconds(char* time);
//
// cg_view.c
//
void CG_ZoomDown_f( void );
void CG_ZoomUp_f( void );
void CG_AddBufferedSound( sfxHandle_t sfx);
void CG_Camera( centity_t *cent );
qboolean CG_WorldCoordToScreenCoordFloat( vec3_t worldCoord, float *x, float *y );
qboolean CG_WorldCoordToScreenCoordVec( vec3_t world, vec2_t screen );

void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );

#if EARTHQUAKE_SYSTEM	// JUHOX: prototypes
void CG_AddEarthquake(
	const vec3_t origin, float radius,
	float duration, float fadeIn, float fadeOut,	// in seconds
	float amplitude
);
void CG_AdjustEarthquakes(const vec3_t delta);
#endif
//
// cg_drawtools.c
//
void CG_AdjustFrom640( float *x, float *y, float *w, float *h,qboolean stretch);
void CG_FillRect( float x, float y, float width, float height, const float *color );
void CG_DrawPic(qboolean stretch, float x, float y, float width, float height, qhandle_t hShader );
void CG_DrawString(float x,float y,const char* string,float charWidth,float charHeight,const float* modulate);
void CG_DrawLineRGBA (vec3_t start, vec3_t end, float width, qhandle_t shader, vec4_t RGBA);

void CG_DrawStringExt(int spacing, int x, int y, const char *string, const float *setColor, 
		qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars);
void CG_DrawBigString( int x, int y, const char *s, float alpha );
void CG_DrawBigStringColor( int x, int y, const char *s, vec4_t color );
void CG_DrawSmallString( int x, int y, const char *s, float alpha );
void CG_DrawSmallStringHalfHeight( int x, int y, const char *s, float alpha );
void CG_DrawSmallStringCustom( int x, int y, int w, int h, const char *s, float alpha,int spacing );
void CG_DrawSmallStringColor( int x, int y, const char *s, vec4_t color );
void CG_DrawMediumStringColor( int x, int y, const char *s, vec4_t color );

int CG_DrawStrlen( const char *str );

float	*CG_FadeColor( int startMsec, int totalMsec, int fadeTime );
float *CG_TeamColor( int team );
void CG_TileClear( void );
void CG_ColorForHealth( vec4_t hcolor );
void CG_GetColorForHealth( int powerLevel, int armor, vec4_t hcolor );

void UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color );
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color );
void CG_DrawSides(float x, float y, float w, float h, float size);
void CG_DrawTopBottom(float x, float y, float w, float h, float size);


//
// cg_draw.c, cg_newDraw.c
//
extern	int sortedTeamPlayers[TEAM_MAXOVERLAY];
extern	int	numSortedTeamPlayers;
extern	int drawTeamOverlayModificationCount;
extern  char systemChat[256];
extern  char teamChat1[256];
extern  char teamChat2[256];

void CG_DrawChat(char *text );
void CG_DrawScreenEffects(void);
void CG_DrawScoreboard(void);
void CG_CenterPrint( const char *str, int y, int charWidth );
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles );
void CG_DrawActive( stereoFrame_t stereoView );
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D );
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team );
void CG_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle);
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style);
int CG_Text_Width(const char *text, float scale, int limit);
int CG_Text_Height(const char *text, float scale, int limit);
void CG_SelectPrevPlayer( void );
void CG_SelectNextPlayer( void );
float CG_GetValue(int ownerDraw);
qboolean CG_OwnerDrawVisible(int flags);
void CG_RunMenuScript(char **args);
void CG_ShowResponseHead( void );
void CG_SetPrintString(int type, const char *p);
void CG_InitTeamChat( void );
void CG_GetTeamColor(vec4_t *color);
const char *CG_GetGameStatusText( void );
const char *CG_GetKillerText( void );
void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles);
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader);
void CG_CheckOrderPending( void );
const char *CG_GameTypeString( void );
qboolean CG_YourTeamHasFlag( void );
qboolean CG_OtherTeamHasFlag( void );
qhandle_t CG_StatusHandle(int task);
//
// cg_animations.c
//
void CG_SetLerpFrameAnimation(clientInfo_t* ci,lerpFrame_t* lf,int newAnimation,qboolean isCamera);
void CG_RunLerpFrame(clientInfo_t* ci,lerpFrame_t* lf,int newAnimation,float speedScale,qboolean isCamera);
void CG_ClearLerpFrame(clientInfo_t* ci,lerpFrame_t* lf,int animationNumber,qboolean isCamera);
void CG_PlayerAnimation(centity_t* cent,
								int* legsOld,int* legs,float* legsBackLerp,
								int* torsoOld,int* torso,float* torsoBackLerp,
								int* headOld,int* head,float* headBackLerp,
								int* cameraOld,int* camera,float* cameraBackLerp);
//
// cg_animationConfig.c
//
qboolean CG_ParseAnimationFile(const char* filename,clientInfo_t* ci,qboolean isCamera);
//
// cg_player.c
//
void CG_Player( centity_t *cent);
void CG_ResetPlayerEntity( centity_t *cent );
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team, qboolean auraAlways );
void CG_NewClientInfo( int clientNum );
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName );
qboolean CG_GetTagOrientationFromPlayerEntity( centity_t *cent, char *tagName, orientation_t *tagOrient );
qboolean CG_GetTagOrientationFromPlayerEntityCameraModel( centity_t *cent, char *tagName, orientation_t *tagOrient );
qboolean CG_GetTagOrientationFromPlayerEntityHeadModel( centity_t *cent, char *tagName, orientation_t *tagOrient );
qboolean CG_GetTagOrientationFromPlayerEntityTorsoModel( centity_t *cent, char *tagName, orientation_t *tagOrient );
qboolean CG_GetTagOrientationFromPlayerEntityLegsModel( centity_t *cent, char *tagName, orientation_t *tagOrient );
void CG_SpawnLightSpeedGhost( centity_t *cent );

//
// cg_predict.c
//
void CG_BuildSolidList( void );
int	CG_PointContents( const vec3_t point, int passEntityNum );
void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
					 int skipNumber, int mask );
#if 1	// JUHOX: prototype for CG_SmoothTrace()
void CG_SmoothTrace(
	trace_t *result,
	const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	int skipNumber, int mask
);
#endif
void CG_PredictPlayerState( void );
void CG_LoadDeferredPlayers( void );


//
// cg_events.c
//
void CG_CheckEvents( centity_t *cent );
const char	*CG_PlaceString( int rank );
void CG_EntityEvent( centity_t *cent, vec3_t position );
void CG_PainEvent( centity_t *cent, int powerLevel );


//
// cg_ents.c
//
void CG_DrawBoundingBox( vec3_t origin, vec3_t mins, vec3_t maxs );
void CG_SetEntitySoundPosition( centity_t *cent );
void CG_AddPacketEntities( void );
void CG_Beam( centity_t *cent );
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out );
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName );
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName );
void CG_GetTagPosition( refEntity_t *parent, char *tagName, vec3_t outpos);
void CG_GetTagOrientation( refEntity_t *parent, char *tagName, vec3_t dir);
//
// cg_tiers.c
//
void CG_NextTier_f( void );
void CG_PrevTier_f( void );
void CG_Tier_f( void );

//
// cg_weapons.c
//
void CG_NextWeapon_f( void );
void CG_PrevWeapon_f( void );
void CG_Weapon_f( void );

void CG_RegisterWeapon( int weaponNum );
void CG_RegisterItemVisuals( int itemNum );

void CG_FireWeapon( centity_t *cent, qboolean altFire );
void CG_MissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType );
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum );
void CG_Bullet( vec3_t origin, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum );

void CG_RailTrail( clientInfo_t *ci, vec3_t start, vec3_t end );
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi );
void CG_AddViewWeapon (playerState_t *ps);
void CG_AddPlayerWeapon(refEntity_t* parent,centity_t* cent);
void CG_DrawWeaponSelect( void );

void CG_UserMissileHitWall( int weapon, int clientNum, int powerups, int number, vec3_t origin, vec3_t dir, qboolean inAir );
void CG_UserMissileHitPlayer( int weapon, int clientNum, int powerups, int number, vec3_t origin, vec3_t dir, int entityNum );
void CG_UserRailTrail( int weapon, int clientNum, vec3_t start, vec3_t end);

//
// cg_marks.c
//
void	CG_InitMarkPolys( void );
void	CG_AddMarks( void );
void	CG_ImpactMark( qhandle_t markShader, 
				    const vec3_t origin, const vec3_t dir, 
					float orientation, 
				    float r, float g, float b, float a, 
					qboolean alphaFade, 
					float radius, qboolean temporary );

//
// cg_localents.c
//
void	CG_InitLocalEntities( void );
localEntity_t	*CG_AllocLocalEntity( void );
void	CG_AddLocalEntities( void );

//
// cg_effects.c
//
void CG_Aura_DrawInnerSpike (vec3_t start, vec3_t end, float width, centity_t *player);
localEntity_t *CG_AuraSpike( const vec3_t p, const vec3_t vel, 
				   float radius,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   centity_t *player);
void CG_WaterBubble(const vec3_t p,float radius,float alpha,float duration,int fadeInTime,qhandle_t hShader);
void CG_PlayerSplash( centity_t *cent, int scale );
void CG_PlayerDirtPush( centity_t *cent, int scale, qboolean once );
void CG_BubbleTrail( vec3_t start, vec3_t end, float spacing );
void CG_WaterRipple( vec3_t org, int size, qboolean single );
void CG_DirtPush( vec3_t org, vec3_t dir, int size );
void CG_WaterSplash( vec3_t org, int size );
void CG_LightningEffect( vec3_t org, clientInfo_t *ci, int tier );
void CG_BigLightningEffect( vec3_t org );
void CG_SpeedMeleeEffect( vec3_t org, int tier );
void CG_PowerMeleeEffect( vec3_t org, int tier );
void CG_PowerStruggleEffect( vec3_t org, int size );

void CG_Bleed( vec3_t origin, int entityNum );

void CG_MakeUserExplosion( vec3_t origin, vec3_t dir, cg_userWeapon_t *weaponGraphics, int powerups, int number);
void CG_CreateStraightBeamFade(vec3_t start, vec3_t end, cg_userWeapon_t *weaponGraphics);

//
// cg_snapshot.c
//
void CG_ProcessSnapshots( void );

//
// cg_info.c
//
void CG_LoadingString( const char *s );
void CG_LoadingClient( int clientNum );
void CG_DrawInformation( void );

//
// cg_scoreboard.c
//
qboolean CG_DrawOldScoreboard( void );
void CG_DrawOldTourneyScoreboard( void );

//
// cg_consolecmds.c
//
qboolean CG_ConsoleCommand( void );
void CG_InitConsoleCommands( void );

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands( int latestSequence );
void CG_ParseServerinfo( void );
void CG_SetConfigValues( void );
void CG_LoadVoiceChats( void );
void CG_ShaderStateChanged(void);
void CG_VoiceChatLocal( int mode, qboolean voiceOnly, int clientNum, int color, const char *cmd );
void CG_PlayBufferedVoiceChats( void );

//
// cg_playerstate.c
//
void CG_Respawn( void );
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops );
void CG_CheckChangedPredictableEvents( playerState_t *ps );


//===============================================

//
// system traps
// These functions are how the cgame communicates with the main game system
//

// print message on the local console
void		trap_Print( const char *fmt );

// abort the game
void		trap_Error(const char *fmt) __attribute__((noreturn));

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int			trap_Milliseconds( void );

// console variable interaction
void		trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void		trap_Cvar_Update( vmCvar_t *vmCvar );
void		trap_Cvar_Set( const char *var_name, const char *value );
float		trap_Cvar_VariableValue( const char *var_name );
void		trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

// ServerCommand and ConsoleCommand parameter access
int			trap_Argc( void );
void		trap_Argv( int n, char *buffer, int bufferLength );
void		trap_Args( char *buffer, int bufferLength );

// filesystem access
// returns length of file
int			trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void		trap_FS_Read( void *buffer, int len, fileHandle_t f );
void		trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void		trap_FS_FCloseFile( fileHandle_t f );
int			trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void		trap_SendConsoleCommand( const char *text );

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void		trap_AddCommand( const char *cmdName );

// send a string to the server over the network
void		trap_SendClientCommand( const char *s );

// force a screen update, only used during gamestate load
void		trap_UpdateScreen( void );

// model collision
void		trap_CM_LoadMap( const char *mapname );
int			trap_CM_NumInlineModels( void );
clipHandle_t trap_CM_InlineModel( int index );		// 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs );
int			trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int			trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void		trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask );
void		trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask,
					  const vec3_t origin, const vec3_t angles );

// Returns the projection of a polygon onto the solid brushes in the world
int			trap_CM_MarkFragments( int numPoints, const vec3_t *points, 
			const vec3_t projection,
			int maxPoints, vec3_t pointBuffer,
			int maxFragments, markFragment_t *fragmentBuffer );

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void		trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
void		trap_S_StopLoopingSound(int entnum);

// a local sound is always played full volume
void		trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void		trap_S_ClearLoopingSounds( qboolean killall );
void		trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );

// respatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void		trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater, float attenuation );
sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed );		// returns buzz if not found
void		trap_S_StartBackgroundTrack( const char *intro, const char *loop );	// empty name stops music
void	trap_S_StopBackgroundTrack( void );


void		trap_R_LoadWorldMap( const char *mapname );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t	trap_R_RegisterModel( const char *name );			// returns rgb axis if not found
qhandle_t	trap_R_RegisterSkin( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShader( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShaderNoMip( const char *name );			// returns all white if not found

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void		trap_R_ClearScene( void );
void		trap_R_AddRefEntityToScene( const refEntity_t *re );

// polys are intended for simple wall marks, not really for doing
// significant construction
void		trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts );
void		trap_R_AddPolysToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int numPolys );
void		trap_R_AddFogToScene( float start, float end, float r, float g, float b, float opacity, float mode, float hint );
void		trap_R_AddLightToScene( const vec3_t org, float intensity,vec3_t color);
int			trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
void		trap_R_RenderScene( const refdef_t *fd );
void		trap_R_SetColor( const float *rgba );	// NULL = 1,1,1,1
void		trap_R_DrawStretchPic( float x, float y, float w, float h, 
			float s1, float t1, float s2, float t2, qhandle_t hShader );
void		trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs, int frame );
int			trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, 
					   float frac, const char *tagName );
void		trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void		trap_GetGlconfig( glconfig_t *glconfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void		trap_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void		trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean	trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean	trap_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int			trap_GetCurrentCmdNumber( void );	

qboolean	trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );

// used for the weapon select and zoom
void		trap_SetUserCmdValue( int stateValue, float sensitivityScale, int tierStateValue, byte weaponSelectionModeState, byte tierSelectionModeState );

// aids for VM testing
void		testPrintInt( char *string, int i );
void		testPrintFloat( char *string, float f );

size_t		trap_MemoryRemaining( void );
void		trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);
qboolean	trap_Key_IsDown( int keynum );
int			trap_Key_GetCatcher( void );
void		trap_Key_SetCatcher( int catcher );
int			trap_Key_GetKey( const char *binding );


typedef enum {
  SYSTEM_PRINT,
  CHAT_PRINT,
  TEAMCHAT_PRINT
} q3print_t;


int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status trap_CIN_StopCinematic(int handle);
e_status trap_CIN_RunCinematic (int handle);
void trap_CIN_DrawCinematic (int handle);
void trap_CIN_SetExtents (int handle, int x, int y, int w, int h);

void trap_SnapVector( float *v );

qboolean	trap_loadCamera(const char *name);
void		trap_startCamera(int time);
qboolean	trap_getCameraInfo(int time, vec3_t *origin, vec3_t *angles);

qboolean	trap_GetEntityToken( char *buffer, int bufferSize );

//ADDING FOR ZEQ2

//
// cg_auras.c
//
void CG_AuraStart( centity_t *player );
void CG_AuraEnd( centity_t *player );
void CG_RegisterClientAura(int clientNum,clientInfo_t *ci);
void CG_AddAuraToScene( centity_t *player );
void CG_CopyClientAura( int from, int to );

//
// cg_beamtables.c
//
void CG_InitBeamTables( void );
void CG_AddBeamTables( void );
void CG_BeamTableUpdate( centity_t *cent, float width, qhandle_t shader, char *tagName );


//
// cg_trails.c
//
void CG_InitTrails( void );
void CG_ResetTrail( int entityNum, vec3_t origin );
void CG_Trail( int entityNum, vec3_t origin, qboolean remove, int maxNumSegments, float width, qhandle_t hShader, vec3_t color );


//
// cg_radar.c
//
void CG_InitRadarBlips( void );
void CG_DrawRadar( void );
void CG_UpdateRadarBlips( char *cmd );


//
// cg_weapGfxParser.c
//
qboolean CG_weapGfx_Parse( char *filename, int clientNum );

//
// cg_tiersystem.c
//
qboolean CG_RegisterClientModelnameWithTiers( clientInfo_t *ci, const char *modelName, const char *skinName );

//
// cg_particlesystem.c
//
void CG_InitParticleSystems( void );
void CG_AddParticleSystems( void );
void PSys_SpawnCachedSystem( char* systemName, vec3_t origin, vec3_t *axis,
							 centity_t *cent, char* tagName,
							 qboolean auraLink, qboolean weaponLink );

//
// cg_frameHist.c
//
void CG_FrameHist_Init( void );
void CG_FrameHist_NextFrame( void );

void CG_FrameHist_SetPVS( int num );
qboolean CG_FrameHist_IsInPVS( int num );
qboolean CG_FrameHist_WasInPVS( int num );

void CG_FrameHist_SetAura( int num );
qboolean CG_FrameHist_HasAura( int num );
qboolean CG_FrameHist_HadAura( int num );

int CG_FrameHist_IsWeaponState( int num );
int CG_FrameHist_WasWeaponState( int num );

int CG_FrameHist_IsWeaponNr( int num );
int CG_FrameHist_WasWeaponNr( int num );


//
// cg_motionblur.c
//
void CG_MotionBlur( void );
//END ADDING

