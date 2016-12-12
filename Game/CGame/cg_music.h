//cg_music.h
#define MUSICTYPES 8

typedef struct{
	char		*playlist[MUSICTYPES][32];
	qboolean	random,
				started,
				fading,
				playToEnd;
	int			trackLength[MUSICTYPES][32],
				hasPlayed[MUSICTYPES][32],
				typeSize[MUSICTYPES],
				lastTrack[MUSICTYPES],
				currentType,
				currentIndex,
				fadeAmount,
				endTime,
				volume;
}musicSystem;

typedef enum{
	battle,
	idle,
	idleWater,
	idleDanger,
	struggle,
	standoff,
	standoffDanger,
	transform,
}trackTypes;

int		CG_GetMilliseconds(char*);
void	CG_CheckMusic(void),
		CG_ParsePlaylist(void),
		CG_FadeNext(void),
		CG_NextTrack(void),
		CG_PlayTransformTrack(void);
