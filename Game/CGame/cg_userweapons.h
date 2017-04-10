#define MAX_CHARGES			 2
#define MAX_TAGNAME			20
#define MAX_WEAPONNAME		40
#define MAX_CHARGE_VOICES	 6
typedef struct{
	sfxHandle_t		voice;
	float			startPct;
}chargeVoice_t;
// NOTE: In cg_userWeapon_t the trailFunc and particleFuncs have an int
//       for an argument to make sure things compile. (centity_t* is unknown
//       at this point.) So, we pass the clientside entity number to the function,
//       then retrieve a pointer to work with in there.
typedef struct{
	//lerpFrame		chargeAnimation,flashAnimation,missileAnimation,explosionAnimation,shockwaveAnimation;
	qhandle_t		chargeModel;
	qhandle_t		chargeSkin;
	//for sprites only!
	qhandle_t		chargeShader;
	vec3_t			chargeSpin;
	qboolean		chargeGrowth;
	// percentage at which charge becomes visible and growth starts	
	int				chargeStartPct;
	int				chargeEndPct;
	float			chargeEndsize;
	float			chargeStartsize;
	float			chargeDlightStartRadius;
	float			chargeDlightEndRadius;
	vec3_t			chargeDlightColor;
	// the names of the player model tags on which to place an instance of the charge
	char			chargeTag[MAX_CHARGES][MAX_TAGNAME];
	chargeVoice_t	chargeVoice[MAX_CHARGE_VOICES];
	sfxHandle_t		chargeLoopSound;
	char			chargeParticleSystem[MAX_QPATH];
	qhandle_t		flashModel;
	qhandle_t		flashSkin;
	//for sprites only!
	qhandle_t		flashShader;
	// If the weapon was charged, multiply with chargeSize;
	float			flashSize;
	float			flashDlightRadius;
	vec3_t			flashDlightColor;
	// if more than one is specified, a random one is chosen. (Breaks repetitiveness for fastfiring weapons)
	sfxHandle_t		flashSound[4];
	sfxHandle_t		voiceSound[4];
	// Played only at the start of a firing session, instead of with each projectile. Resets when attack button comes up.
	sfxHandle_t		flashOnceSound;
	// When doing a sustained blast
	sfxHandle_t		firingSound;
	char			flashParticleSystem[MAX_QPATH];
	char			firingParticleSystem[MAX_QPATH];
	qhandle_t		missileModel;
	qhandle_t		missileSkin;
	qhandle_t		missileShader;
	qhandle_t		missileStruggleModel;
	qhandle_t		missileStruggleSkin;
	qhandle_t		missileStruggleShader;
	// If the weapon was charged, multiply with chargeSize;
	float			missileSize;
	vec3_t			missileSpin;
	float			missileDlightRadius;
	vec3_t			missileDlightColor;
	float			missileTrailRadius;
	qhandle_t		missileTrailShader;
	qhandle_t		missileTrailSpiralShader;
	float			missileTrailSpiralRadius;
	float			missileTrailSpiralOffset;
	char			missileParticleSystem[MAX_QPATH];
	sfxHandle_t		missileSound;
	qhandle_t		explosionModel;
	qhandle_t		explosionSkin;
	qhandle_t		explosionShader;
	int				explosionTime;
	// If the weapon was charged, multiply with chargeSize;
	float			explosionSize;
	float			explosionDlightRadius;
	vec3_t			explosionDlightColor;
	qhandle_t		shockwaveModel;
	qhandle_t		shockwaveSkin;
	char			explosionParticleSystem[MAX_QPATH];
	char			smokeParticleSystem[MAX_QPATH];
	qhandle_t		markShader;
	qhandle_t		markSize;
	qboolean		noRockDebris;
	// if more than one is specified, a random one is chosen. (Breaks repetitiveness for fastfiring weapons)
	sfxHandle_t		explosionSound[4];
	qhandle_t		weaponIcon;
	char			weaponName[MAX_WEAPONNAME];			
}cg_userWeapon_t;
cg_userWeapon_t *CG_FindUserWeaponGraphics(int clientNum, int index);
void CG_CopyUserWeaponGraphics(int from, int to);
// cg_userWeaponParseBuffer is used by cg_weapGfxParser.
// Instead of qhandle_t, we will store filenames as char[].
// This means we won't have to cache models and sounds that
// will be overrided in inheritance.
typedef struct{
	char			voice[MAX_QPATH];
	float			startPct;
}chargeVoiceParseBuffer_t;
typedef struct{
	char			chargeModel[MAX_QPATH];
	char			chargeSkin[MAX_QPATH];
	//for sprites only!
	char			chargeShader[MAX_QPATH];
	vec3_t			chargeSpin;

	qboolean		chargeGrowth;
	int				chargeEndPct;
	// percentage at which charge becomes visible and growth starts
	int				chargeStartPct;
	float			chargeEndsize;
	float			chargeStartsize;
	float			chargeDlightStartRadius;
	float			chargeDlightEndRadius;
	vec3_t			chargeDlightColor;
	// the names of the player model tags on which to place an instance of the charge
	char			chargeTag[MAX_CHARGES][MAX_TAGNAME];
	chargeVoiceParseBuffer_t	chargeVoice[MAX_CHARGE_VOICES];
	char			chargeLoopSound[MAX_QPATH];
	char			chargeParticleSystem[MAX_QPATH];
	char			flashModel[MAX_QPATH];
	char			flashSkin[MAX_QPATH];
	//for sprites only!
	char			flashShader[MAX_QPATH];
	// If the weapon was charged, multiply with chargeSize;
	float			flashSize;
	float			flashDlightRadius;
	vec3_t			flashDlightColor;
	// if more than one is specified, a random one is chosen. (Breaks repetitiveness for fastfiring weapons)
	char			flashSound[4][MAX_QPATH];
	char			voiceSound[4][MAX_QPATH];
	// Played only at the start of a firing session, instead of with each projectile. Resets when attack button comes up.
	char			flashOnceSound[MAX_QPATH];
	// When doing a sustained blast
	char			firingSound[MAX_QPATH];
	char			flashParticleSystem[MAX_QPATH];
	char			firingParticleSystem[MAX_QPATH];
	char			missileModel[MAX_QPATH];
	char			missileSkin[MAX_QPATH];
	char			missileShader[MAX_QPATH];
	char			missileStruggleModel[MAX_QPATH];
	char			missileStruggleSkin[MAX_QPATH];
	char			missileStruggleShader[MAX_QPATH];
	// If the weapon was charged, multiply with chargeSize;
	float			missileSize;
	vec3_t			missileSpin;
	float			missileDlightRadius;
	vec3_t			missileDlightColor;
	float			missileTrailRadius;
	char			missileTrailShader[MAX_QPATH];
	char			missileTrailSpiralShader[MAX_QPATH];
	float			missileTrailSpiralRadius;
	float			missileTrailSpiralOffset;
	char			missileParticleSystem[MAX_QPATH];
	char			missileSound[MAX_QPATH];
	char			explosionModel[MAX_QPATH];
	char			explosionSkin[MAX_QPATH];
	char			explosionShader[MAX_QPATH];
	int				explosionTime;
	// If the weapon was charged, multiply with chargeSize;
	float			explosionSize;
	float			explosionDlightRadius;
	vec3_t			explosionDlightColor;
	char			shockwaveModel[MAX_QPATH];
	char			shockwaveSkin[MAX_QPATH];
	char			explosionParticleSystem[MAX_QPATH];
	char			smokeParticleSystem[MAX_QPATH];
	char			markShader[MAX_QPATH];
	qhandle_t		markSize;
	qboolean		noRockDebris;
	// if more than one is specified, a random one is chosen. (Breaks repetitiveness for fastfiring weapons)
	char			explosionSound[4][MAX_QPATH];
	char			weaponIcon[MAX_QPATH];
	char			weaponName[MAX_WEAPONNAME];
}cg_userWeaponParseBuffer_t;
