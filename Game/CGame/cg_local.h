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
#include "../../Shared/q_shared.h" 
#include "../../Engine/renderer/tr_types.h"
#include "../Game/bg_public.h"
// ADDING FOR ZEQ2
#include "../Game/bg_userweapons.h"
#include "../Game/g_tiers.h"
#include "cg_userweapons.h"
#include "cg_auras.h"
#include "cg_tiers.h"
#include "cg_music.h"
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
#define	WEAPON_SELECT_TIME	1400
#define	ITEM_SCALEUP_TIME	1000
#define	ZOOM_TIME			150
#define	ITEM_BLOB_TIME		200
#define	MUZZLE_FLASH_TIME	80		//20
#define	SINK_TIME			1000	// time for fragments to sink into ground before going away
#define	ATTACKER_ANIM_TIME	10000
#define	REWARD_TIME			3000
#define	PULSE_SCALE			1.5		// amount to scale up the icons when activating
#define	MAX_STEP_CHANGE		32
#define	MAX_VERTS_ON_POLY	10
#define	MAX_MARK_POLYS		256
#define STAT_MINUS			10		// num frame for '-' stats digit
#define	ICON_SIZE			48
#define	CHAR_WIDTH			8		// 32
#define	CHAR_HEIGHT			12		// 48
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

typedef enum{
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,
	FOOTSTEP_TOTAL
} footstep_t;

typedef enum{
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
typedef struct{
	int			oldFrame,
				oldFrameTime,		// time when ->oldFrame was exactly on
				frame,
				frameTime;			// time when ->frame will be exactly on
	float		backlerp,
				yawAngle,
				pitchAngle;
	qboolean	yawing,
				pitching;
	int			animationNumber,	// may include ANIM_TOGGLEBIT
				animationTime;		// time when the first frame of the animation will be exact
	animation_t	*animation;
} lerpFrame_t;

typedef struct{
	lerpFrame_t		legs, torso, head, camera, flag;
	// needed to obtain tag positions after player entity has been processed.
	// For linking beam attacks, particle systems, etc.
	refEntity_t		legsRef, torsoRef, headRef, cameraRef;
} playerEntity_t;

//=================================================
// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s{
	entityState_t	currentState,		// from cg.frame
					nextState;			// from cg.nextFrame, if available
	qboolean		interpolate,		// true if next is valid to interpolate to
					currentValid;		// true if cg.frame holds this entity
	int				muzzleFlashTime,	// move to playerEntity?
					previousEvent,
					teleportFlag,
					trailTime,			// so missile trails can handle dropped initial packets
					dustTrailTime,
					miscTime,	
					snapShotTime;		// last time this entity was found in a snapshot
	playerEntity_t	pe;
	int				errorTime;			// decay the error from this time
	vec3_t			errorOrigin,
					errorAngles;
	qboolean		extrapolated;		// false if origin / angles is an interpolation
	vec3_t			rawOrigin,
					rawAngles,
					beamEnd,
	// exact interpolated position of entity on this frame
					lerpOrigin,
					lerpAngles;
	// ADDING FOR ZEQ2
	float			lerpPrim,
					lerpSec;
	int				lastTrailTime,		// last cg.time the trail for this cent was rendered
					lastPVSTime,		// last cg.time the entity was in the PVS
					lastAuraPSysTime;	// last cg.time especially for the coupled aura debris system
	// END ADDING
} centity_t;

//======================================================================
// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities
typedef struct markPoly_s{
	struct markPoly_s	*prevMark, *nextMark;
	int					time;
	qhandle_t			markShader;
	qboolean			alphaFade;	// fade alpha instead of rgb
	float				color[4];
	poly_t				poly;
	polyVert_t			verts[MAX_VERTS_ON_POLY];
} markPoly_t;

typedef enum{
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

typedef enum{
	LEF_PUFF_DONT_SCALE  = 0x0001,	// do not scale size over time
	LEF_TUMBLE			 = 0x0002,	// tumble over time, used for ejecting shells
	LEF_SOUND1			 = 0x0004,	// sound 1 for kamikaze
	LEF_SOUND2			 = 0x0008	// sound 2 for kamikaze
} leFlag_t;

typedef enum{
	LEMT_NONE,
	LEMT_BURN,
	LEMT_BLOOD
} leMarkType_t;			// fragment local entities can leave marks on walls

typedef enum{
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_BRASS
} leBounceSoundType_t;	// fragment local entities can make sounds on impacts

typedef struct localEntity_s{
	struct localEntity_s	*prev, *next;
	leType_t				leType;
	int						leFlags,
							startTime,
							endTime,
							fadeInTime;
	trajectory_t			pos,
							angles;
	float					lifeRate,			// 1.f / (endTime -startTime)
							bounceFactor,		// 0 = no bounce, 1.f = perfect
							color[4],
							radius,
							light;
	vec3_t					lightColor;
	leMarkType_t			leMarkType;			// mark to leave on fragment impact
	leBounceSoundType_t		leBounceSoundType;
	refEntity_t				refEntity;		
} localEntity_t;

//======================================================================
// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define	MAX_CUSTOM_SOUNDS	512

typedef struct{
	qboolean		infoValid;
	char			name[MAX_QPATH];
	team_t			team;
	int				location,		// location index for team mode
					powerLevel,		// you only get this info about your teammates
					armor,
					curWeapon,
					wins, losses,	// in tourney mode
					teamTask;		// task in teamplay (offence/defence)
	qboolean		teamLeader;		// true when this is a team leader
	int				powerups,		// so can display quad/flag status
					breathPuffTime,
					bubbleTrailTime;
	char			modelName[MAX_QPATH],
					skinName[MAX_QPATH],
					cameraModelName[MAX_QPATH],
					headModelName[MAX_QPATH],
					headSkinName[MAX_QPATH],
					legsModelName[MAX_QPATH],
					legsSkinName[MAX_QPATH],
					redTeam[MAX_TEAMNAME],
					blueTeam[MAX_TEAMNAME];
	qhandle_t		skinDamageState[8][3][10],
					modelDamageState[8][3][10];
	qboolean		deferred,
					fixedlegs,		// true if legs yaw is always the same as torso yaw
					fixedtorso,		// true if torso never changes yaw
					overrideHead;					
	vec3_t			headOffset;		// move head in icon views
	footstep_t		footsteps;
	qhandle_t		legsModel[8],
					legsSkin[8],
					torsoModel[8],
					torsoSkin[8],
					headModel[8],
					headSkin[8],
					cameraModel[8],
					modelIcon;
	animation_t		animations[MAX_TOTALANIMATIONS],
					camAnimations[MAX_TOTALANIMATIONS];
	sfxHandle_t		sounds[9][MAX_CUSTOM_SOUNDS];
	//ADDING FOR ZEQ2
	int				damageModelState,
					damageTextureState;
	qboolean		transformStart;
	int				transformLength,
					lockStartTimer,
					tierCurrent,
					tierMax,
					cameraBackup[4];
	tierConfig_cg	tierConfig[8];
	auraConfig_t	*auraConfig[8];
} clientInfo_t;

// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s{
	qboolean		registered;
	qhandle_t		handsModel,			// the hands don't actually draw, they just position the weapon
					weaponModel,
					barrelModel,
					flashModel;
	vec3_t			weaponMidpoint,		// so it will rotate centered instead of by tag
					flashDlightColor;
	float			flashDlight;
	sfxHandle_t		flashSound[4],		// fast firing weapons randomly choose
					voiceSound[4];

	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void			(*missileTrailFunc)(centity_t*, const struct weaponInfo_s *wi);
	float			missileDlight;
	vec3_t			missileDlightColor;
	int				missileRenderfx;
	void			(*ejectBrassFunc)(centity_t*);
	float			trailRadius;
	sfxHandle_t		readySound,
					firingSound;
} weaponInfo_t;

#define MAX_REWARDSTACK		10
#define MAX_SOUNDBUFFER		20
// JUHOX: definitions used for map lens flares
#if MAPLENSFLARES
#define MAX_LENSFLARE_EFFECTS 200
#define MAX_MISSILE_LENSFLARE_EFFECTS 16
#define MAX_LENSFLARES_PER_EFFECT 32
typedef enum{
	LFM_reflexion,
	LFM_glare,
	LFM_star
} lensFlareMode_t;

typedef struct{
	qhandle_t shader;
	lensFlareMode_t mode;
	float	pos,					// position at light axis
			size,
			rgba[4],
			rotationOffset,
			rotationYawFactor,
			rotationPitchFactor,
			rotationRollFactor,
			fadeAngleFactor,		// for spotlights
			entityAngleFactor,		// for spotlights
			intensityThreshold;
} lensFlare_t;

typedef struct{
	char		name[64];
	float		range,
				rangeSqr,
				fadeAngle;	// for spotlights
	int			numLensFlares;
	lensFlare_t lensFlares[MAX_LENSFLARES_PER_EFFECT];
} lensFlareEffect_t;

#define MAX_LIGHTS_PER_MAP 1024
#define LIGHT_INTEGRATION_BUFFER_SIZE 8	// must be a power of 2
typedef struct{
	float light;
	vec3_t origin;
} lightSample_t;

typedef struct{
	vec3_t						origin;
	centity_t*					lock;
	float						radius, lightRadius;
	vec3_t						dir;					// for spotlights
	float						angle,					// for spotlights
								maxVisAngle;
	const lensFlareEffect_t*	lfeff;
	int							libPos,
								libNumEntries;
	lightSample_t lib[LIGHT_INTEGRATION_BUFFER_SIZE];	// lib = light integration buffer
} lensFlareEntity_t;

typedef enum{
	LFEEM_none,
	LFEEM_pos,
	LFEEM_target,
	LFEEM_radius
} lfeEditMode_t;

typedef enum{
	LFEDM_normal,
	LFEDM_marks,
	LFEDM_none
} lfeDrawMode_t;

typedef enum{
	LFEMM_coarse,
	LFEMM_fine
} lfeMoveMode_t;

typedef enum{
	LFECS_small,
	LFECS_lightRadius,
	LFECS_visRadius
} lfeCursorSize_t;

typedef enum{
	LFECM_main,
	LFECM_copyOptions
} lfeCommandMode_t;

#define LFECO_EFFECT		1
#define LFECO_VISRADIUS		2
#define LFECO_LIGHTRADIUS	4
#define LFECO_SPOT_DIR		8
#define LFECO_SPOT_ANGLE	16
typedef struct{
	lensFlareEntity_t*	selectedLFEnt;	// NULL = none
	lfeDrawMode_t		drawMode;
	lfeEditMode_t		editMode;
	lfeMoveMode_t		moveMode;
	lfeCursorSize_t		cursorSize;
	float				fmm_distance;	// fmm = fine move mode
	vec3_t				fmm_offset;
	qboolean			delAck;
	int					selectedEffect,
						markedLFEnt;	// -1 = none
	lensFlareEntity_t	originalLFEnt;	// backup for undo
	int					oldButtons,
						lastClick;
	qboolean			editTarget;
	vec3_t				targetPosition;
	lfeCommandMode_t	cmdMode;
	int					copyOptions;
	lensFlareEntity_t	copiedLFEnt;	// for copy / paste
	qboolean			moversStopped;
	centity_t*			selectedMover;
} lfEditor_t;
#endif

//======================================================================
// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after
#define MAX_PREDICTED_EVENTS	16
#if EARTHQUAKE_SYSTEM	// JUHOX: definitions
typedef struct{
	vec3_t	origin;
	float	radius,		// negative for global earthquakes
			amplitude;
	int		startTime,
			endTime,
			fadeInTime,
			fadeOutTime;
} earthquake_t;
#define MAX_EARTHQUAKES	64
#endif

typedef struct{
	qhandle_t	shader;
	qboolean	active;
	int			x, y, width, height,
				endTime;
} overlay2D;

typedef struct{
	int				clientFrame,			// incremented each frame
					clientNum;
	qboolean		resetValues,
					demoPlayback,
					levelShot;				// taking a level menu screenshot
	qboolean		loading,				// don't defer players at initial startup
					intermissionStarted;	// don't play voice rewards, because game will end shortly
	// there are only one or two snapshot_t that are relevent at a time
	int				latestSnapshotNum,		// the number of snapshots the client system has received
					latestSnapshotTime;		// the time from latestSnapshotNum, so we don't need to read the snapshot yet
	snapshot_t		*snap,					// cg.snap->serverTime <= cg.time
					*nextSnap,				// cg.nextSnap->serverTime > cg.time, or NULL
					activeSnapshots[2];
	float			frameInterpolation;		// (float)(cg.time -cg.frame->serverTime) / (cg.nextFrame->serverTime -cg.frame->serverTime)
	qboolean		thisFrameTeleport,
					nextFrameTeleport,
					lockonStart;
	int				frametime,				// cg.time -cg.oldTime
					time,					// this is the time value that the client is rendering at.
					oldTime,				// time at last frame, used for missile trails and prediction checking
					physicsTime;			// either cg.snap->time or cg.nextSnap->time
	qboolean		mapRestart,				// set on a map restart to set back the weapon
					renderingThirdPerson,	// old unused Quake III Arena cvar. We might want to use it for the new tag based first person support.
	// prediction state
					hyperspace;				// true if prediction has hit a trigger_teleport
	playerState_t	predictedPlayerState;
	centity_t		predictedPlayerEntity;
	qboolean		validPPS;				// clear until the first call to CG_PredictPlayerState
	int				predictedErrorTime;
	vec3_t			predictedError;
	int				eventSequence,
					predictableEvents[MAX_PREDICTED_EVENTS];
	float			stepChange;				// for stair up smoothing
	int				stepTime;
	float			landChange;				// for landing hard
	int				landTime;
	// input state sent to server
	int				weaponSelect,
					weaponDesired;
	byte			weaponChanged,
					weaponSelectionMode; 	// 0 means no change; 1 means previous weapon; 2 means next weapon; 3 means weapon directly selected
	int				tierSelect;
	byte			tierSelectionMode;		//0 means no change; 1 means previous tier; 2 means next tier; 3 means tier directly selected
	int				tierCurrent;
	// auto rotating items
	vec3_t			autoAngles,
					autoAxis[3],
					autoAnglesFast,
					autoAxisFast[3];
	byte			drawWeaponBar;
	// view rendering
	refdef_t		refdef;
	vec3_t			refdefViewAngles;		// will be converted to refdef.viewaxis
#if MAPLENSFLARES							// JUHOX: variables for map lens flares
	vec3_t			lastViewOrigin;
	float			viewMovement;
	int				numFramesWithoutViewMovement;	
	lfEditor_t		lfEditor;				// JUHOX: lens flare editor variables
#endif
#if EARTHQUAKE_SYSTEM						// JUHOX: earthquake variables
	int				earthquakeStartedTime,
					earthquakeEndTime;
	float 			earthquakeAmplitude;
	int 			earthquakeFadeInTime,
					earthquakeFadeOutTime,
					earthquakeSoundCounter,
					lastEarhquakeSoundStartedTime;
	earthquake_t	earthquakes[MAX_EARTHQUAKES];
	float			additionalTremble;
#endif
	// zoom key
	qboolean		zoomed;
	int				zoomTime;
	float			zoomSensitivity;
	// information screen text during loading
	char			infoScreenText[MAX_STRING_CHARS];
	// centerprinting
	int				centerPrintTime,
					centerPrintCharWidth,
					centerPrintY,
					centerPrintLines;
	char			centerPrint[1024];
	// Dynamic 2D effects
	overlay2D		scripted2D[16];
	// crosshair client ID
	int				crosshairClientNum,
					crosshairClientTime,
	// screen flashing
					screenFlashTime,
					screenFlastTimeTotal,
					screenFlashFadeTime;
	float			screenFlashFadeAmount;
	// powerup active flashing
	int				powerupActive,
					powerupTime,
	// attacking player
					attackerTime,
					voiceTime,
	// sound buffer mainly for announcer sounds
					soundBufferIn,
					soundBufferOut,
					soundTime;
	qhandle_t		soundBuffer[MAX_SOUNDBUFFER];
	// for voice chat buffer
	int				voiceChatTime,
					voiceChatBufferIn,
					voiceChatBufferOut,
	// warmup countdown
					warmup,
					warmupCount,
	//==========================
					itemPickup,
					itemPickupTime,
					itemPickupBlendTime,	// the pulse around the crosshair is timed seperately
					weaponSelectTime,
					weaponAnimation,
					weaponAnimationTime,
	// ADDING FOR ZEQ2
					PLBar_foldPct;			// Need to know how far we've folded in or out already
	// END ADDING
	// blend blobs
	float			damageTime,
					damageX, damageY, damageValue,
	// status bar head
					headYaw,
					headEndTime,
					headEndPitch,
					headEndYaw,
					headStartTime;
	float			headStartPitch,
					headStartYaw,
	// view movement
					v_dmg_time,
					v_dmg_pitch,
					v_dmg_roll,
	// temp working variables for player view
					bobfracsin;
	int				bobcycle;
	float			xyspeed;
	int				nextOrbitTime;
	//qboolean		cameraMode;				// if rendering from a loaded camera
	qboolean		guide_view;
	vec3_t			guide_target;			// where to look for guided missiles
	// END ADDING
} cg_t;

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t and weaponInfo_t
typedef struct{
	qhandle_t		charsetShader,
					charsetProp,
					charsetPropGlow,
					charsetPropB,
					whiteShader,
					clearShader,
					deferShader,
					hudShader,
					markerAscendShader,
					markerDescendShader,
					breakLimitShader,
					RadarBlipShader,
					RadarBlipTeamShader,
					RadarBurstShader,
					RadarWarningShader,
					RadarMidpointShader,
					speedLineShader,
					speedLineSpinShader,
					friendShader,
					chatBackgroundShader,
					chatBubble,
					connectionShader,
					selectShader,
					viewBloodShader,
					tracerShader,
					crosshairShader[NUM_CROSSHAIRS],
					backTileShader,
					globalCelLighting,
					waterSplashSkin,
					waterSplashModel,
					waterRippleSkin,
					waterRippleModel,
					waterRippleSingleSkin,
					waterRippleSingleModel,
					dirtPushSkin,
					dirtPushModel,
					dirtPushShader,
					waterBubbleLargeShader,
					waterBubbleMediumShader,
					waterBubbleSmallShader,
					waterBubbleTinyShader,
					bloodTrailShader,
					bfgLFGlare,	// JUHOX
					bfgLFDisc,	// JUHOX
					bfgLFRing,	// JUHOX
					bfgLFLine,	// JUHOX
					bfgLFStar,	// JUHOX
					numberShaders[11],
					shadowMarkShader,
					wakeMarkShader,
					teleportEffectModel,
					teleportEffectShader,
					meleeSpeedEffectShader,
					meleePowerEffectShader,
					powerStruggleRaysEffectShader,
					powerStruggleShockwaveEffectShader,
					boltEffectShader,
					auraLightningSparks1,
					auraLightningSparks2,
					invulnerabilityPowerupModel,
					cursor,
					selectCursor,
					sizeCursor;
	sfxHandle_t		nullSound,
					lockonStart,
					selectSound,
					useNothingSound,
					wearOffSound,
					footsteps[FOOTSTEP_TOTAL][4],
					teleInSound,
					teleOutSound,
					respawnSound,
					talkSound,
					landSound,
					fallSound,
					watrInSound,
					watrOutSound,
					watrUnSound,
				// ADDING FOR ZEQ2
					blinkingSound,
					radarwarningSound,
					lightspeedSound1,
					lightspeedSound2,
					lightspeedSound3,
					lightspeedSound4,
					lightspeedSound5,
					bigLightningSound1,
					bigLightningSound2,
					bigLightningSound3,
					bigLightningSound4,
					bigLightningSound5,
					bigLightningSound6,
					bigLightningSound7,
					blockSound,
					knockbackSound,
					knockbackLoopSound,
					stunSound,
					speedMeleeSound,
					speedMissSound,
					speedBlockSound,
					powerMeleeSound,
					powerStunSound1,
					powerStunSound2,
					powerMissSound,
					airBrake1,
					airBrake2,
					hover,
					hoverFast,
					hoverLong,
					waterSplashSmall1,
					waterSplashSmall2,
					waterSplashSmall3,
					waterSplashMedium1,
					waterSplashMedium2,
					waterSplashMedium3,
					waterSplashMedium4,
					waterSplashLarge1,
					waterSplashExtraLarge1,
					waterSplashExtraLarge2;
				// END ADDING
} cgMedia_t;

// The client game static (cgs) structure holds everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct{
	gameState_t					gameState;						// gamestate from server
	glconfig_t					glconfig;						// rendering configuration
	float						screenXScale,					// derived from glconfig
								screenYScale,
								screenXBias;
	int							serverCommandSequence,			// reliable command stream counter
								processedSnapshotNum;			// the number of snapshots cgame has requested
	qboolean					localServer,					// detected on startup by checking sv_running
								clientPaused;
	// parsed from serverinfo
	gametype_t					gametype;
	int							dmflags,
								teamflags,
								fraglimit,
								capturelimit,
								timelimit,
								maxclients;
	char						mapname[MAX_QPATH],
								redTeam[MAX_QPATH],
								blueTeam[MAX_QPATH],
// JUHOX: serverinfo cvars for map lens flares
#if MAPLENSFLARES
								sunFlareEffect[128];
	editMode_t					editMode;
	float						sunFlareYaw,
								sunFlarePitch,
								sunFlareDistance;
#endif
	qboolean					voteModified;					// beep whenever changed
	char						voteString[MAX_STRING_TOKENS];
	int							voteTime,
								voteYes,
								voteNo,
								teamVoteTime[2],
								teamVoteYes[2],
								teamVoteNo[2];
	qboolean					teamVoteModified[2];			// beep whenever changed
	char						teamVoteString[2][MAX_STRING_TOKENS];
	int							levelStartTime;
	// locally derived information from gamestate
	qhandle_t					gameModels[MAX_MODELS];
	sfxHandle_t					gameSounds[MAX_SOUNDS];
	int							numInlineModels;
	qhandle_t					inlineDrawModel[MAX_MODELS];
	vec3_t						inlineModelMidpoints[MAX_MODELS];
	clientInfo_t				clientinfo[MAX_CLIENTS];
	// teamchat width is *3 because of embedded color codes
	char						teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH*3+1];
	int							teamChatMsgTimes[TEAMCHAT_HEIGHT],
								teamChatPos,
								teamLastChatPos;
	qboolean 					eventHandling,
	// orders
								orderPending;
	int							currentOrder,
								orderTime,
								currentVoiceClient,
								acceptOrderTime,
								acceptTask,
								acceptLeader;
	char						acceptVoice[MAX_NAME_LENGTH];
// JUHOX: variables for map lens flares
#if MAPLENSFLARES
	int							numLensFlareEffects, numLensFlareEntities;
	lensFlareEffect_t			lensFlareEffects[MAX_LENSFLARE_EFFECTS];
	lensFlareEntity_t			lensFlareEntities[MAX_LIGHTS_PER_MAP],
								sunFlare;
#endif
// JUHOX: variables for missile lens flares
	int 						numMissileLensFlareEffects;
	lensFlareEffect_t			missileLensFlareEffects[MAX_MISSILE_LENSFLARE_EFFECTS];
	const lensFlareEffect_t* 	lensFlareEffectBeamHead;
	const lensFlareEffect_t* 	lensFlareEffectSolarFlare;
	const lensFlareEffect_t* 	lensFlareEffectExplosion1;
	const lensFlareEffect_t* 	lensFlareEffectExplosion2;
	const lensFlareEffect_t* 	lensFlareEffectExplosion3;
	const lensFlareEffect_t* 	lensFlareEffectExplosion4;
	const lensFlareEffect_t*	lensFlareEffectEnergyGlowDarkBackground;
	char						messages[3][256];
	int							messageClient[3],
								chatTimer;
	// media
	cgMedia_t	media;
	musicSystem music;
} cgs_t;

//==============================================================================
extern	cgs_t			cgs;
extern	cg_t			cg;
extern	centity_t		cg_entities[MAX_GENTITIES];
extern	weaponInfo_t	cg_weapons[MAX_WEAPONS];
extern	markPoly_t		cg_markPolys[MAX_MARK_POLYS];
extern	vmCvar_t		cg_centertime,
						cg_runpitch,
						cg_runroll,
						cg_bobup,
						cg_bobpitch,
						cg_bobroll,
						cg_swingSpeed,
						cg_shadows,
						cg_gibs,
						cg_drawTimer,
						cg_drawFPS,
						cg_drawSnapshot,
						cg_draw3dIcons,
						cg_drawIcons,
						cg_drawCrosshair,
						cg_drawCrosshairNames,
						cg_drawTeamOverlay,
						cg_teamOverlayUserinfo,
						cg_crosshairX,
						cg_crosshairY,
						cg_crosshairSize,
						cg_crosshairMargin,
						cg_crosshairHealth,
						cg_crosshairBars,
						cg_scripted2D,
						cg_scriptedCamera,
						cg_drawStatus,
						cg_draw2D,
						cg_animSpeed,
						cg_debugAnim,
						cg_debugPosition,
						cg_debugEvents,
						cg_railTrailTime,
						cg_errorDecay,
						cg_nopredict,
						cg_noPlayerAnims,
						cg_showmiss,
						cg_footsteps,
						cg_addMarks,
						cg_brassTime,
						cg_chatTime,
						cg_viewsize,
						cg_tracerChance,
						cg_tracerWidth,
						cg_tracerLength,
						cg_displayObituary,
						cg_ignore,
						cg_fov,
						cg_zoomFov,
						cg_advancedFlight,
						cg_thirdPersonCameraDamp,
						cg_thirdPersonTargetDamp,
						cg_thirdPersonMeleeCameraDamp,
						cg_thirdPersonMeleeTargetDamp,
						cg_thirdPersonRange,
						cg_thirdPersonAngle,
						cg_thirdPersonHeight,
						cg_thirdPersonSlide,
						cg_lockedRange,
						cg_lockedAngle,
						cg_lockedHeight,
						cg_lockedSlide,
						cg_thirdPerson,
						cg_synchronousClients,
						cg_teamChatTime,
						cg_teamChatHeight,
						cg_stats,
						cg_forceModel,
						cg_buildScript,
						cg_paused,
						cg_blood,
						cg_predictItems,
						cg_deferPlayers,
						cg_drawFriend,
						cg_teamChatsOnly,
						cg_noVoiceChats,
						cg_noVoiceText,
						cg_smoothClients,
						pmove_fixed,
						pmove_msec,
						//cg_pmove_fixed,
						cg_cameraOrbit,
						cg_cameraOrbitDelay,
						cg_timescaleFadeEnd,
						cg_timescaleFadeSpeed,
						cg_timescale,
						cg_cameraMode,
						cg_smallFont,
						cg_bigFont,
						cg_trueLightning,
						cg_redTeamName,
						cg_blueTeamName,
						cg_enableDust,
						cg_enableBreath,
					//ADDING FOR ZEQ2
						cg_lockonDistance,
						cg_tailDetail,
						cg_verboseParse,
						r_beamDetail,
						cg_soundAttenuation,
						cg_thirdPersonCamera,
						cg_beamControl,
						cg_music,
						cg_playTransformTrackToEnd,
						cg_particlesQuality,
						cg_particlesStop,
						cg_particlesMaximum,
						cg_drawBBox,
					// END ADDING
#if MAPLENSFLARES
						cg_lensFlare,		// JUHOX
						cg_mapFlare,		// JUHOX
						cg_sunFlare,		// JUHOX
						cg_missileFlare;	// JUHOX
#endif
extern	radar_t			cg_playerOrigins[MAX_CLIENTS];
//
// cg_main.c
//
const char		*CG_ConfigString(int index),
				*CG_Argv(int arg);
void QDECL		CG_Printf(const char *msg, ...) __attribute__ ((format(printf, 1, 2))),
				CG_Error(const char *msg, ...) __attribute__ ((noreturn, format (printf, 1, 2)));
int				CG_CrosshairPlayer(void),
				CG_LastAttacker(void);
void			CG_UpdateCvars(void),
				CG_LoadMenus(const char *menuFile),
				CG_KeyEvent(int key, qboolean down),
				CG_MouseEvent(int x, int y),
				CG_EventHandling(int type),
				CG_RankRunFrame(void),
				CG_BuildSpectatorString(void),
// JUHOX: prototypes
#if MAPLENSFLARES
				CG_LFEntOrigin(const lensFlareEntity_t* lfent, vec3_t origin),
				CG_SetLFEntOrigin(lensFlareEntity_t* lfent, const vec3_t origin),
				CG_SetLFEdMoveMode(lfeMoveMode_t mode),
				CG_SelectLFEnt(int lfentnum),
				CG_LoadLensFlares(void),
				CG_ComputeMaxVisAngle(lensFlareEntity_t* lfent),
				CG_LoadLensFlareEntities(void),
				CG_AddLFEditorCursor(void),
				CG_AddLensFlare(lensFlareEntity_t* lfent, int quality),
#endif
//
// cg_view.c
//
				CG_ZoomDown_f(void),
				CG_ZoomUp_f(void),
				CG_AddBufferedSound(sfxHandle_t sfx),
				CG_Camera(centity_t *cent),
				CG_DrawActiveFrame(int serverTime, stereoFrame_t stereoView, qboolean demoPlayback);
qboolean		CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y),
				CG_WorldCoordToScreenCoordVec(vec3_t world, vec2_t screen);
// JUHOX: prototypes
#if EARTHQUAKE_SYSTEM
void			CG_AddEarthquake(const vec3_t origin, float radius, float duration, float fadeIn, float fadeOut, float amplitude),	// fadeOut in seconds
				CG_AdjustEarthquakes(const vec3_t delta),
#endif
//
// cg_drawtools.c
//
				CG_AdjustFrom640(float *x, float *y, float *w, float *h,qboolean stretch),
				CG_FillRect(float x, float y, float width, float height, const float *color),
				CG_DrawPic(qboolean stretch, float x, float y, float width, float height, qhandle_t hShader),
				CG_DrawString(float x, float y, const char *string, float charWidth, float charHeight, const float *modulate),
				CG_DrawStringExt(int spacing, int x, int y, const char *string, const float *setColor, qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars),
				CG_DrawBigString(int x, int y, const char *s, float alpha),
				CG_DrawBigStringColor(int x, int y, const char *s, vec4_t color),
				CG_DrawSmallString(int x, int y, const char *s, float alpha),
				CG_DrawSmallStringHalfHeight(int x, int y, const char *s, float alpha),
				CG_DrawSmallStringCustom(int x, int y, int w, int h, const char *s, float alpha,int spacing),
				CG_DrawSmallStringColor(int x, int y, const char *s, vec4_t color),
				CG_DrawMediumStringColor(int x, int y, const char *s, vec4_t color);
int				CG_DrawStrlen(const char *str);
float			*CG_FadeColor(int startMsec, int totalMsec, int fadeTime),
				*CG_TeamColor(int team);
void			CG_TileClear(void),
				CG_ColorForHealth(vec4_t hcolor),
				CG_GetColorForHealth(int powerLevel, int armor, vec4_t hcolor),
				UI_DrawProportionalString(int x, int y, const char* str, int style, vec4_t color),
				CG_DrawRect(float x, float y, float width, float height, float size, const float *color),
				CG_DrawSides(float x, float y, float w, float h, float size),
				CG_DrawTopBottom(float x, float y, float w, float h, float size);
//
// cg_draw.c
//
extern int		sortedTeamPlayers[TEAM_MAXOVERLAY],
				numSortedTeamPlayers;
void			CG_DrawChat(char *text),
				CG_CenterPrint(const char *str, int y, int charWidth),
				CG_DrawHead(float x, float y, float w, float h, int clientNum, vec3_t headAngles),
				CG_DrawActive(stereoFrame_t stereoView),
				CG_DrawFlagModel(float x, float y, float w, float h, int team, qboolean force2D),
				CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style);
int				CG_Text_Width(const char *text, float scale, int limit),
				CG_Text_Height(const char *text, float scale, int limit);
float			CG_GetValue(int ownerDraw);
void			CG_SelectPrevPlayer(void),
				CG_SelectNextPlayer(void),
				CG_RunMenuScript(char **args),
				CG_ShowResponseHead(void),
				CG_SetPrintString(int type, const char *p),
				CG_InitTeamChat(void),
				CG_GetTeamColor(vec4_t *color);
const char		*CG_GetGameStatusText(void),
				*CG_GetKillerText(void);
void			CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles),
				CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader),
				CG_CheckOrderPending(void);
const char		*CG_GameTypeString(void);
qboolean		CG_YourTeamHasFlag(void),
				CG_OtherTeamHasFlag(void);
qhandle_t		CG_StatusHandle(int task);
//
// cg_player.c
//
void			CG_Player(centity_t *cent),
				CG_ResetPlayerEntity(centity_t *cent),
				CG_AddRefEntityWithPowerups(refEntity_t *ent, entityState_t *state, int team, qboolean auraAlways),
				CG_NewClientInfo(int clientNum),
				CG_SpawnLightSpeedGhost(centity_t *cent);
sfxHandle_t		CG_CustomSound(int clientNum, const char *soundName);
qboolean		CG_ParseAnimationFile(const char *filename, clientInfo_t *ci, qboolean isCamera),
				CG_GetTagOrientationFromPlayerEntity(centity_t *cent, char *tagName, orientation_t *tagOrient),
				CG_GetTagOrientationFromPlayerEntityCameraModel(centity_t *cent, char *tagName, orientation_t *tagOrient),
				CG_GetTagOrientationFromPlayerEntityHeadModel(centity_t *cent, char *tagName, orientation_t *tagOrient),
				CG_GetTagOrientationFromPlayerEntityTorsoModel(centity_t *cent, char *tagName, orientation_t *tagOrient),
				CG_GetTagOrientationFromPlayerEntityLegsModel(centity_t *cent, char *tagName, orientation_t *tagOrient);
//
// cg_predict.c
//
int				CG_PointContents(const vec3_t point, int passEntityNum);
void			CG_BuildSolidList(void),
				CG_Trace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask),
// JUHOX: prototype for CG_SmoothTrace()
#if 1
				CG_SmoothTrace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask),
#endif
				CG_PredictPlayerState(void),
//
// cg_events.c
//
				CG_CheckEvents(centity_t *cent),
				CG_EntityEvent(centity_t *cent, vec3_t position),

//
// cg_ents.c
//
				CG_DrawBoundingBox(vec3_t origin, vec3_t mins, vec3_t maxs),
				CG_SetEntitySoundPosition(centity_t *cent),
				CG_AddPacketEntities(void),
				CG_Beam(centity_t *cent),
				CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out),
				CG_PositionEntityOnTag(refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel, char *tagName),
				CG_PositionRotatedEntityOnTag(refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel, char *tagName),
				CG_GetTagPosition(refEntity_t *parent, char *tagName, vec3_t outpos),
				CG_GetTagOrientation(refEntity_t *parent, char *tagName, vec3_t dir),
// JUHOX: prototypes
#if MAPLENSFLARES	
				CG_Mover(centity_t *cent),
#endif
//
// cg_tiers.c
//
				parseTier(char *path,tierConfig_cg *tier),
				CG_NextTier_f(void),
				CG_PrevTier_f(void),
				CG_Tier_f(void),
//
// cg_weapons.c
//
				CG_NextWeapon_f(void),
				CG_PrevWeapon_f(void),
				CG_Weapon_f(void),
				CG_RegisterWeapon(int weaponNum),
				CG_RegisterItemVisuals(int itemNum),
				CG_FireWeapon(centity_t *cent, qboolean altFire),
				CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType),
				CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir, int entityNum),
				CG_Bullet(vec3_t origin, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum),
				CG_Draw3DLine(const vec3_t start, const vec3_t end, qhandle_t shader),	// JUHOX
				CG_RailTrail(clientInfo_t *ci, vec3_t start, vec3_t end),
				CG_GrappleTrail(centity_t *ent, const weaponInfo_t *wi),
				CG_AddViewWeapon(playerState_t *ps),
				CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent, int team),
				CG_DrawWeaponSelect(void),
// FIXME: Should these be in drawtools instead?
//        Should these be generalized for use in all manual poly drawing functions? (probably, yes...)
				CG_DrawLine(vec3_t start, vec3_t end, float width, qhandle_t shader, float RGBModulate),
				CG_DrawLineRGBA(vec3_t start, vec3_t end, float width, qhandle_t shader, vec4_t RGBA),
				CG_UserMissileHitWall(int weapon, int clientNum, int powerups, int number, vec3_t origin, vec3_t dir, qboolean inAir),
				CG_UserMissileHitPlayer(int weapon, int clientNum, int powerups, int number, vec3_t origin, vec3_t dir, int entityNum),
				CG_UserRailTrail(int weapon, int clientNum, vec3_t start, vec3_t end),
//
// cg_marks.c
//
				CG_InitMarkPolys(void),
				CG_AddMarks(void),
				CG_ImpactMark(qhandle_t markShader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a, qboolean alphaFade, float radius, qboolean temporary),
//
// cg_localents.c
//
				CG_InitLocalEntities(void),
				CG_AddLocalEntities(void);
localEntity_t	*CG_AllocLocalEntity(void),
//
// cg_effects.c
//
				*CG_SmokePuff(const vec3_t p, const vec3_t vel, float radius, float r, float g, float b, float a, float duration, int startTime, int fadeInTime, int leFlags, qhandle_t hShader),
				*CG_WaterBubble(const vec3_t p, const vec3_t vel, float radius, float r, float g, float b, float a, float duration, int startTime, int fadeInTime, int leFlags, qhandle_t hShader),
				*CG_AuraSpike(const vec3_t p, const vec3_t vel, float radius, float duration, int startTime, int fadeInTime, int leFlags, centity_t *player);
void			CG_PlayerSplash(centity_t *cent, int scale),
				CG_PlayerDirtPush(centity_t *cent, int scale, qboolean once),
				CG_BubbleTrail(vec3_t start, vec3_t end, float spacing),
				CG_SpawnEffect(vec3_t org),
				CG_WaterRipple(vec3_t org, int size, qboolean single),
				CG_DirtPush(vec3_t org, vec3_t dir, int size),
				CG_WaterSplash(vec3_t org, int size),
				CG_LightningEffect(vec3_t org, clientInfo_t *ci, int tier),
				CG_BigLightningEffect(vec3_t org),
				CG_SpeedMeleeEffect(vec3_t org, int tier),
				CG_PowerMeleeEffect(vec3_t org, int tier),
				CG_PowerStruggleEffect(vec3_t org, int size);
				// CG_Bleed(vec3_t origin, int entityNum),
localEntity_t	*CG_MakeExplosion(vec3_t origin, vec3_t dir, qhandle_t hModel, qhandle_t shader, int msec, qboolean isSprite);
void			CG_MakeUserExplosion(vec3_t origin, vec3_t dir, cg_userWeapon_t *weaponGraphics, int powerups, int number),
				CG_CreateStraightBeamFade(vec3_t start, vec3_t end, cg_userWeapon_t *weaponGraphics),
//
// cg_snapshot.c
//
				CG_ProcessSnapshots(void),
//
// cg_info.c
//
				CG_LoadingString(const char *s),
				CG_LoadingClient(int clientNum),
				CG_DrawInformation(void);
//
// cg_consolecmds.c
//
qboolean		CG_ConsoleCommand(void);
void			CG_InitConsoleCommands(void),
//
// cg_servercmds.c
//
				CG_ExecuteNewServerCommands(int latestSequence),
				CG_ParseServerinfo(void),
				CG_SetConfigValues(void),
				CG_ShaderStateChanged(void),
//
// cg_playerstate.c
//
				CG_Respawn(void),
				CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops),
				CG_CheckChangedPredictableEvents(playerState_t *ps),
//===============================================
// system traps
// These functions are how the cgame communicates with the main game system
//
// print message on the local console
				trap_Print(const char *fmt),
// abort the game
				trap_Error(const char *fmt) __attribute__((noreturn));
// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int				trap_Milliseconds(void);
// console variable interaction
void			trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags),
				trap_Cvar_Update(vmCvar_t *vmCvar),
				trap_Cvar_Set(const char *var_name, const char *value),
				trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);
float			trap_Cvar_VariableValue(const char *var_name);
// ServerCommand and ConsoleCommand parameter access
void			trap_Argv(int n, char *buffer, int bufferLength),
				trap_Args(char *buffer, int bufferLength);
int				trap_Argc(void),
// filesystem access
// returns length of file
				trap_FS_Seek(fileHandle_t f, long offset, int origin),	// fsOrigin_t
				trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize),
				trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void			trap_FS_FCloseFile(fileHandle_t f),
				trap_FS_Read(void *buffer, int len, fileHandle_t f),
				trap_FS_Write(const void *buffer, int len, fileHandle_t f),
// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
				trap_SendConsoleCommand(const char *text),
// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
				trap_AddCommand(const char *cmdName),
// send a string to the server over the network
				trap_SendClientCommand(const char *s),
// force a screen update, only used during gamestate load
				trap_UpdateScreen(void),
// model collision
				trap_CM_LoadMap(const char *mapname);
clipHandle_t	trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs),
				trap_CM_InlineModel(int index);		// 0 = world, 1+ = bmodels
int				trap_CM_NumInlineModels(void),
				trap_CM_PointContents(const vec3_t p, clipHandle_t model),
				trap_CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles),
// Returns the projection of a polygon onto the solid brushes in the world
				trap_CM_MarkFragments(int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer);
void			trap_CM_BoxTrace(trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask),
				trap_CM_TransformedBoxTrace(trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles),
// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
				trap_S_StartSound(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx),
				trap_S_StopLoopingSound(int entnum),
// a local sound is always played full volume
				trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum),
				trap_S_ClearLoopingSounds(qboolean killall),
				trap_S_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx),
				trap_S_AddRealLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx),
				trap_S_UpdateEntityPosition(int entityNum, const vec3_t origin),
// respatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
				trap_S_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater, float attenuation);
sfxHandle_t		trap_S_RegisterSound(const char *sample, qboolean compressed);		// returns buzz if not found
void			trap_S_StartBackgroundTrack(const char *intro, const char *loop),	// empty name stops music
				trap_S_StopBackgroundTrack(void),
				trap_R_LoadWorldMap(const char *mapname);
// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t		trap_R_RegisterModel(const char *name),			// returns rgb axis if not found
				trap_R_RegisterSkin(const char *name),			// returns all white if not found
				trap_R_RegisterShader(const char *name),		// returns all white if not found
				trap_R_RegisterShaderNoMip(const char *name);	// returns all white if not found
// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void			trap_R_ClearScene(void),
				trap_R_AddRefEntityToScene(const refEntity_t *re),
// polys are intended for simple wall marks, not really for doing
// significant construction
				trap_R_AddPolyToScene(qhandle_t hShader , int numVerts, const polyVert_t *verts),
				trap_R_AddPolysToScene(qhandle_t hShader , int numVerts, const polyVert_t *verts, int numPolys),
				trap_R_AddFogToScene(float start, float end, float r, float g, float b, float opacity, float mode, float hint),
				trap_R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b);
int				trap_R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);
void			trap_R_RenderScene(const refdef_t *fd),
				trap_R_SetColor(const float *rgba),	// NULL = 1,1,1,1
				trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader),
				trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs, int frame);
int				trap_R_LerpTag(orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName);
void			trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset),
// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
				trap_GetGlconfig(glconfig_t *glconfig),
// the gamestate should be grabbed at startup, and whenever a
// configstring changes
				trap_GetGameState(gameState_t *gamestate),
// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
				trap_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime);
// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean		trap_GetSnapshot(int snapshotNumber, snapshot_t *snapshot),
// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
				trap_GetServerCommand(int serverCommandNumber);
// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int				trap_GetCurrentCmdNumber(void);
qboolean		trap_GetUserCmd(int cmdNumber, usercmd_t *ucmd);
// used for the weapon select and zoom
void			trap_SetUserCmdValue(int stateValue, float sensitivityScale, int tierStateValue, byte weaponSelectionModeState, byte tierSelectionModeState),
// aids for VM testing
				testPrintInt(char *string, int i),
				testPrintFloat(char *string, float f),
				trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);
int				trap_MemoryRemaining(void),
				trap_Key_GetKey(const char *binding),
				trap_Key_GetCatcher(void);
qboolean		trap_Key_IsDown(int keynum);
void			trap_Key_SetCatcher(int catcher);
int				trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status		trap_CIN_StopCinematic(int handle),
				trap_CIN_RunCinematic (int handle);
void			trap_CIN_DrawCinematic(int handle),
				trap_CIN_SetExtents(int handle, int x, int y, int w, int h),
				trap_SnapVector(float *v),
				trap_startCamera(int time);
qboolean		trap_loadCamera(const char *name),
				trap_getCameraInfo(int time, vec3_t *origin, vec3_t *angles),
				trap_GetEntityToken(char *buffer, int bufferSize);
void			CG_ClearParticles(void),
				CG_AddParticles(void),
				CG_ParticleSnow(qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum),
				CG_ParticleSmoke(qhandle_t pshader, centity_t *cent),
				CG_AddParticleShrapnel(localEntity_t *le),
				CG_ParticleSnowFlurry(qhandle_t pshader, centity_t *cent),
				CG_ParticleBulletDebris(vec3_t	org, vec3_t vel, int duration),
				CG_ParticleSparks(vec3_t org, vec3_t vel, int duration, float x, float y, float speed),
				CG_ParticleDust(centity_t *cent, vec3_t origin, vec3_t dir),
				CG_ParticleMisc(qhandle_t pshader, vec3_t origin, int size, int duration, float alpha),
				CG_ParticleExplosion(char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd);
int				CG_NewParticleArea(int num);
extern qboolean	initparticles;
//ADDING FOR ZEQ2
//
// cg_auras.c
//
void			CG_AuraStart(centity_t *player),
				CG_AuraEnd(centity_t *player),
				CG_RegisterClientAura(int clientNum,clientInfo_t *ci),
				CG_AddAuraToScene(centity_t *player),
				CG_CopyClientAura(int from, int to),
//
// cg_beamtables.c
//
				CG_InitBeamTables(void),
				CG_AddBeamTables(void),
				CG_BeamTableUpdate(centity_t *cent, float width, qhandle_t shader, char *tagName),
//
// cg_trails.c
//
				CG_InitTrails(void),
				CG_ResetTrail(int entityNum, vec3_t origin, float baseSpeed, float width, qhandle_t shader, vec3_t color),
				CG_UpdateTrailHead(int entityNum, vec3_t origin),
				CG_AddTrailsToScene(void),
//
// cg_radar.c
//
				CG_InitRadarBlips(void),
				CG_DrawRadar(void),
				CG_UpdateRadarBlips(char *cmd);
//
// cg_weapGfxParser.c
//
qboolean		CG_weapGfx_Parse(char *filename, int clientNum),
//
// cg_tiers.c
//
				CG_RegisterClientModelnameWithTiers(clientInfo_t *ci, const char *modelName, const char *skinName);
//
// cg_particlesystem.c
//
void			CG_InitParticleSystems(void),
				CG_AddParticleSystems(void),
				PSys_SpawnCachedSystem(char* systemName, vec3_t origin, vec3_t *axis, centity_t *cent, char* tagName, qboolean auraLink, qboolean weaponLink),
//
// cg_frameHist.c
//
				CG_FrameHist_Init(void),
				CG_FrameHist_NextFrame(void),
				CG_FrameHist_SetPVS(int num);
qboolean		CG_FrameHist_IsInPVS(int num),
				CG_FrameHist_WasInPVS(int num);
void			CG_FrameHist_SetAura(int num);
qboolean		CG_FrameHist_HasAura(int num),
				CG_FrameHist_HadAura(int num);
int				CG_FrameHist_IsWeaponState(int num),
				CG_FrameHist_WasWeaponState(int num),
				CG_FrameHist_IsWeaponNr(int num),
				CG_FrameHist_WasWeaponNr(int num);
//
// cg_motionblur.c
//
void			CG_MotionBlur(void);
//END ADDING
