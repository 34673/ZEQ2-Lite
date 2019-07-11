// cg_music.c
#include "cg_local.h"
void CG_CheckMusic(void){
	playerState_t	*ps = &cg.predictedPlayerState;
	clientInfo_t	*ci = &cgs.clientinfo[ps->clientNum];
	tierConfig_cg	*tier = &ci->tierConfig[ci->tierCurrent];
	if(!cgs.music.started) CG_ParsePlaylist();
	if(ps->bitFlags & isTransforming){
		if(cgs.music.currentType != 7){
			char var[8];
			cgs.music.currentType = 7;
			if(tier->transformMusic[0]) CG_PlayTransformTrack();
			else CG_NextTrack();
			trap_Cvar_VariableStringBuffer("cg_playTransformTrackToEnd", var, sizeof(var));
			if(atoi(var)) cgs.music.playToEnd = qtrue;
		}
	}
	else if(ps->bitFlags & isStruggling){
		if(cgs.music.currentType != 4){
			cgs.music.currentType = 4;
			CG_NextTrack();
		}
	}
	else if(!(ps->bitFlags & isUnsafe)){
		if(ps->bitFlags & isTargeted || ps->lockedTarget > 0){
			if(ps->powerLevel[plHealth] < (ps->powerLevel[plMaximum] * .4f)){
				if(cgs.music.currentType != 6){
					cgs.music.currentType = 6;
					CG_NextTrack();
				}
			}
			else if(cgs.music.currentType != 5){
				cgs.music.currentType = 5;
				CG_NextTrack();
			}
		}
		else{
			if(ps->powerLevel[plHealth] < (ps->powerLevel[plMaximum] * .4f)){
				if(cgs.music.currentType != 3){
					cgs.music.currentType = 3;
					CG_FadeNext();
				}
			}
			else if(ps->bitFlags & underWater){
				if(cgs.music.currentType != 2){
					cgs.music.currentType = 2;
					CG_NextTrack();
				}
			}
			else if(cgs.music.currentType != 1){
				cgs.music.currentType = 1;
				CG_FadeNext();
			}
		}
	}
	else if(cgs.music.currentType){
		cgs.music.currentType = 0;
		CG_NextTrack();		
	}
	if(cg.time > cgs.music.endTime){	
		int difference = (cg.time - cgs.music.endTime);
		float percent = 0.f;
		
		if(difference < cgs.music.fadeAmount){
			percent = 1.f - ((float)difference / (float)cgs.music.fadeAmount);
			trap_Cvar_Set("s_musicvolume",va("%f",percent * cg_music.value));
		}
		else{
			cgs.music.fading = qfalse;
			cgs.music.playToEnd = qfalse;
			CG_NextTrack();
		}
	}
}

int CG_GetMilliseconds(char *time){
	int compareIndex = 0;
	int index = 0;
	int amount = 0;
	char current[8] = {0};
	while(1){
		if(index > Q_PrintStrlen(time)){break;}
		current[compareIndex] = time[index];
		if(!strcmp(&current[index],":")){
			amount += atoi(current) * 60000;
			compareIndex = -1;
			memset(current, 0, 8);
		}
		index++;
		compareIndex++;
	}
	amount += atoi(current) * 1000;
	return amount;
}

void CG_ParsePlaylist(void){
	fileHandle_t	playlist;
	char			*token,*parse, first, last, fileContents[32000];
	int 			fileLength, trackIndex = 0, typeIndex = 0;
	
	cgs.music.currentIndex = -1;
	cgs.music.fadeAmount = 0;
	cgs.music.started = qtrue;
	cgs.music.random = qfalse;
	if(trap_FS_FOpenFile("music/playlist.cfg", 0, FS_READ)>0){
		fileLength = trap_FS_FOpenFile("music/playlist.cfg", &playlist, FS_READ);
		trap_FS_Read(fileContents, fileLength, playlist);
		fileContents[fileLength] = 0;
		trap_FS_FCloseFile(playlist);
		parse = fileContents;
		while(1){
			token = COM_Parse(&parse);
			if(!token[0]) break;
			first = token[0];
			last = token[Q_PrintStrlen(token)-1];
			last = *(va("%c", last));  // Psyco Fix?!
			if(!Q_stricmp(token, "type")){
				token = COM_Parse(&parse);
				if(!token[0]) break;
				if(!Q_stricmp(token, "random")) cgs.music.random = qtrue;
			}
			else if(!Q_stricmp(token, "fade")){
				token = COM_Parse(&parse);
				if(!token[0]) break;
				cgs.music.fadeAmount = CG_GetMilliseconds(token);
			}
			else if(last == '{'){
				cgs.music.lastTrack[typeIndex] = -1;
				++typeIndex;
			}
			else if(first == '}'){
				cgs.music.typeSize[typeIndex] = trackIndex;
				trackIndex = 0;
			}
			else{
				static char temp[16][32][256];
				
				strncpy(temp[typeIndex][trackIndex], token, 255);
				cgs.music.playlist[typeIndex][trackIndex] = temp[typeIndex][trackIndex];
				cgs.music.trackLength[typeIndex][trackIndex] = CG_GetMilliseconds(COM_Parse(&parse));
				++trackIndex;
			}
		}
	}
}

void CG_FadeNext(void){
	if(!cgs.music.playToEnd){
		cgs.music.fading = qtrue;
		cgs.music.endTime = cg.time;
	}
}

void CG_NextTrack(void){
	int duration, nextIndex, typeSize;
	char *path;

	if(cgs.music.fading || cgs.music.playToEnd) return;
	typeSize = cgs.music.typeSize[cgs.music.currentType];
	nextIndex = (cgs.music.currentIndex + 1 < typeSize) ? ++cgs.music.currentIndex : 0;
	if(cgs.music.random) nextIndex = fabs(crandom()) * typeSize;
	if(nextIndex == cgs.music.lastTrack[cgs.music.currentType]) nextIndex += 1;
	if(nextIndex < 0) nextIndex = typeSize-1;
	if(nextIndex >= typeSize) nextIndex = 0;
	path = va("music/%s", cgs.music.playlist[cgs.music.currentType][nextIndex]);
	duration = cgs.music.trackLength[cgs.music.currentType][nextIndex];
	if(duration > 300000) duration = 300000;
	cgs.music.endTime = cg.time + duration - cgs.music.fadeAmount;
	cgs.music.lastTrack[cgs.music.currentType] = nextIndex;
	trap_S_StartBackgroundTrack(path, path);
	trap_Cvar_Set("s_musicvolume", va("%f", cg_music.value));
}

void CG_PlayTransformTrack(void){
	playerState_t	*ps;
	clientInfo_t	*ci;
	tierConfig_cg	*tier;
	char			*path;
	int				duration;

	ps = &cg.predictedPlayerState;
	ci = &cgs.clientinfo[ps->clientNum];
	tier = &ci->tierConfig[ci->tierCurrent];
	path = va("music/%s", tier->transformMusic);
	duration = tier->transformMusicLength;
	if(duration > 300000) duration = 300000;
	cgs.music.endTime = cg.time + duration - cgs.music.fadeAmount;
	trap_S_StopBackgroundTrack();
	trap_S_StartBackgroundTrack(path, path);
	trap_Cvar_Set("s_musicvolume", va("%f", cg_music.value));
}
