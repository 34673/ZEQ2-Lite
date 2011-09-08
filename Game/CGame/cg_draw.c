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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#ifdef MISSIONPACK
#include "../UI/ui_shared.h"

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
#else
int drawTeamOverlayModificationCount = -1;
#endif

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

#ifdef MISSIONPACK

int CG_Text_Width(const char *text, float scale, int limit) {
  int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
// FIXME: see ui_main.c, same problem
//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  out = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
				out += glyph->xSkip;
				s++;
				count++;
			}
    }
  }
  return out * useScale;
}

int CG_Text_Height(const char *text, float scale, int limit) {
  int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
// TTimo: FIXME
//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  max = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
	      if (max < glyph->height) {
		      max = glyph->height;
			  }
				s++;
				count++;
			}
    }
  }
  return max * useScale;
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h;
  w = width * scale;
  h = height * scale;
  CG_AdjustFrom640( &x, &y, &w, &h,qtrue);
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  if (text) {
// TTimo: FIXME
//		const unsigned char *s = text;
		const char *s = text;
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
      //int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
      //float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar(x + ofs, y - yadj + ofs, 
														glyph->imageWidth,
														glyph->imageHeight,
														useScale, 
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar(x, y - yadj, 
													glyph->imageWidth,
													glyph->imageHeight,
													useScale, 
													glyph->s,
													glyph->t,
													glyph->s2,
													glyph->t2,
													glyph->glyph);
				// CG_DrawPic(qfalse,x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
    }
	  trap_R_SetColor( NULL );
  }
}


#endif

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
#ifndef MISSIONPACK
static void CG_DrawField (int x, int y, int width, int value) {
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic(qfalse, x,y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}
#endif // !MISSIONPACK


/*
=================
CG_DrawHorGauge

=================
*/
void CG_DrawHorGauge( float x, float y, float w, float h, vec4_t color_bar, vec4_t color_empty, int value, int maxvalue, qboolean reversed) {
	float pct;
	float bar_w;

	pct = (float)value / (float)maxvalue;
	bar_w = w * pct;

	if (color_empty[3]) {
		trap_R_SetColor( color_empty );
		if (!reversed) {
			CG_DrawPic(qfalse, x + bar_w, y, w - bar_w, h, cgs.media.whiteShader );
		} else {
			CG_DrawPic(qfalse, x - bar_w, y, w + bar_w, h, cgs.media.whiteShader );
		}
	}
	trap_R_SetColor( color_bar );

	if (bar_w > w) {
		bar_w = w;
	}

	if (bar_w == 0) {
		trap_R_SetColor( NULL );
		return;
	}

	if (!reversed) {
		CG_DrawPic(qfalse, x, y, bar_w, h, cgs.media.whiteShader );
	} else {
		CG_DrawPic(qfalse, x + w - bar_w, y, bar_w, h, cgs.media.whiteShader );
	}

	trap_R_SetColor( NULL );
}
void CG_DrawDiffGauge(float x,float y,float width,float height,vec4_t color,vec4_t empty,int base,int value,int maxValue,int direction){
	float percent;
	int newWidth,newX,difference;
	difference = base - value;
	if(difference > 0 && direction <= 0){return;}
	if(difference < 0 && direction >= 0){return;}
	percent = ((float)value / (float)maxValue);
	newX = x + ((float)(width * percent));
	newWidth = ((float)difference / (float)maxValue) * width;
	CG_DrawHorGauge(newX,y,newWidth,height,color,empty,1,1,qfalse);
}
void CG_DrawRightGauge(float x,float y,float width,float height,vec4_t color,vec4_t empty,int value,int maxValue){
	float percent;
	int newWidth,newX,change;
	percent = ((float)value / (float)maxValue);
	change = width * percent;
	newX = x + change;
	newWidth = width - change;
	CG_DrawHorGauge(newX,y,newWidth,height,color,empty,1,1,qfalse);
}
void CG_DrawReverseGauge(float x,float y,float width,float height,vec4_t color,vec4_t empty,int value,int maxValue){
	float percent;
	int newWidth,newHeight,newX,newY;
	percent = (float)value / (float)maxValue;
	newWidth = width * percent;
	newX = (x + width) - newWidth;
	newHeight = height * percent;
	newY = (y + height) - newHeight;
	CG_DrawHorGauge(newX,newY,newWidth,newHeight,color,empty,1,1,qfalse);
}

/*
==================
CG_DrawVertGauge

==================
*/
void CG_DrawVertGauge( float x, float y, float w, float h, vec4_t color_bar, vec4_t color_empty, int value, int maxvalue, qboolean reversed) {
	float pct;
	float bar_h;

	pct = (float)value / (float)maxvalue;
	bar_h = h * pct;

	if (color_empty[3]) {
		trap_R_SetColor( color_empty );
		if (reversed) {
			CG_DrawPic(qfalse, x, y + bar_h, w, h - bar_h, cgs.media.whiteShader );
		} else {
			CG_DrawPic(qfalse, x, y, w, h - bar_h, cgs.media.whiteShader );
		}
	}
	trap_R_SetColor( color_bar );

	if (bar_h > h) {
		bar_h = h;
	}

	if (bar_h == 0) {
		trap_R_SetColor( NULL );
		return;
	}

	if (reversed) {
		CG_DrawPic(qfalse, x, y, w, bar_h, cgs.media.whiteShader );
	} else {
		CG_DrawPic(qfalse, x, y + h - bar_h, w, bar_h, cgs.media.whiteShader );
	}

	trap_R_SetColor( NULL );
}



/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;
	CG_AdjustFrom640( &x, &y, &w, &h,qtrue);
	memset( &refdef, 0, sizeof( refdef ) );
	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW | RF_DEPTHHACK | RF_LIGHTING_ORIGIN;
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear(refdef.viewaxis);
	refdef.fov_x = 30;
	refdef.fov_y = 30;
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;
	refdef.time = cg.time;
	trap_R_ClearScene();
	trap_R_AddRefEntityToScene(&ent);
	trap_R_RenderScene(&refdef);
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;
	tierConfig_cg	*tier;
	qhandle_t		icon;
	ci = &cgs.clientinfo[clientNum];
	tier = &ci->tierConfig[ci->tierCurrent];
	if(!cg_draw3dIcons.integer){
		icon = tier->icon2D[0];
		CG_DrawPic(qfalse, x, y, w, h,icon);
	}
	else{
		cm = ci->headModel[ci->tierCurrent];
		if(!cm){return;}
		trap_R_ModelBounds(cm,mins,maxs );
		len = 0.7 * ( maxs[2] - mins[2] );		
		origin[0] = len / (1.0 - tier->icon3DZoom);
		origin[1] = 0.5 * (mins[1] + maxs[1]);
		origin[2] = -0.5 * (mins[2] + maxs[2]);
		headAngles[0] = tier->icon3DRotation[0];
		headAngles[1] = tier->icon3DRotation[1];
		headAngles[2] = tier->icon3DRotation[2];
		CG_Draw3DModel(x+tier->icon3DOffset[0],y+tier->icon3DOffset[1],w+tier->icon3DSize[0],h+tier->icon3DSize[1],ci->headModel[ci->tierCurrent],ci->headSkin[ci->tierCurrent],origin,headAngles);
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {}

/*
================
CG_DrawStatusBarHead

================
*/
#ifndef MISSIONPACK

static void CG_DrawStatusBarHead( float x ) {
	vec3_t		angles;
	float		size, stretch;
	float		frac;

	VectorClear( angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.headEndPitch = 5 * cos( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

	CG_DrawHead( x, 480 - size, size, size, 
				cg.snap->ps.clientNum, angles );
}
#endif // MISSIONPACK

/*
================
CG_DrawStatusBarFlag

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBarFlag( float x, int team ) {
	CG_DrawFlagModel( x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse );
}
#endif // MISSIONPACK

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	trap_R_SetColor( NULL );
}


/*================
CG_DRAWCHAT
================*/
void strrep(char *str, char old, char new)  {
    char *pos;
    while (1)  {
        pos = strchr(str, old);
        if (pos == NULL)  {
            break;
        }
        *pos = new;
    }
}
void CG_CheckChat(void){
	int index,offset;
	vec3_t angles;
	int yStart = cg.predictedPlayerState.lockedTarget ? 330 : 0;
	int yOffset = cg.predictedPlayerState.lockedTarget ? -40 : 40;
	if(cg.time < cgs.chatTimer){
		for(index=0;index<3;++index){
			if(cgs.messageClient[index] >= 0){
				CG_DrawPic(qfalse,-15,-15+yStart+(yOffset*index),369,75,cgs.media.chatBackgroundShader);
				CG_DrawHead(7,7+yStart+(yOffset*index),32,32,cgs.messageClient[index],angles);
				CG_DrawSmallStringCustom(45,16+yStart+(yOffset*index),8,8,cgs.messages[index],1.0,4);
			}
		}
	}
	else{
		for(index=0;index<3;++index){
			cgs.messageClient[index] = -1;
			strcpy(cgs.messages[index],"");
		}
	}
}
void CG_DrawChat(char *text){
	int clientNum,index,safeIndex;
	char cleaned[256];
	char name[14];
	char *safe;
	char find = ':';
	char find2[] = "^7";
	char replace = ' ';
	safe = text;
	strrep(safe, find, replace);
	strcpy(cleaned, text);
	strrep(safe, *find2, replace);
	cgs.chatTimer = cg.time + cg_chatTime.integer;
	strcpy(name,COM_Parse(&safe));
	for(safeIndex=0; safeIndex<3; ++safeIndex){if(!strcmp(cgs.messages[safeIndex],"")){break;}}
	if(safeIndex>=2 && cgs.messageClient[2]>= 0){
		safeIndex = 2;
		cgs.messageClient[0] = cgs.messageClient[1];
		cgs.messageClient[1] = cgs.messageClient[2];
		strcpy(cgs.messages[0],cgs.messages[1]);
		strcpy(cgs.messages[1],cgs.messages[2]);
	}
	for(clientNum=0;clientNum<MAX_CLIENTS;++clientNum){
		if(!strcmp(name,cgs.clientinfo[clientNum].name)){
			cgs.messageClient[safeIndex] = clientNum;
			break;
		}
	}
	strcpy(cgs.messages[safeIndex],cleaned);
}
void CG_DrawScreenEffects(){
	clientInfo_t	*ci;
	tierConfig_cg	*tier;
	playerState_t	*ps;
	qhandle_t		effect;
	ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	tier = &ci->tierConfig[ci->tierCurrent];
	ps = &cg.snap->ps;
	effect = tier->screenEffect[ci->damageTextureState-1];
	if(ps->bitFlags & isBreakingLimit && tier->screenEffectPowering){
		effect = tier->screenEffectPowering;
	}
	else if(ps->bitFlags & isTransforming && tier->screenEffectTransforming){
		effect = tier->screenEffectTransforming;
	}
	CG_DrawPic(qfalse,0,0,640,480,effect);
}
/*================
CG_Scoreboard
================*/
void CG_DrawScoreboard(){
	int clientNum;
	vec3_t	angles;
	for(clientNum=0;clientNum<MAX_CLIENTS;++clientNum){
		if(cgs.clientinfo[clientNum].infoValid){
			CG_DrawHead(180,(36*clientNum)+180,50,50,clientNum,angles);
			CG_DrawSmallStringHalfHeight(240,(36*clientNum)+200,cgs.clientinfo[clientNum].name,1.0F);
			CG_DrawSmallStringHalfHeight(320,(36*clientNum)+200,va("%i",cg.scores[clientNum].score),1.0F);
			CG_DrawSmallStringHalfHeight(380,(36*clientNum)+200,va("%i",cg.scores[clientNum].ping),1.0F);
			CG_DrawSmallStringHalfHeight(420,(36*clientNum)+200,va("%i",cg.scores[clientNum].time),1.0F);
		}
	}
}
/*================
CG_HUD
================*/
void CG_DrawHUD(playerState_t *ps,int clientNum,int x,int y,qboolean flipped){
	const char	*powerLevelString;
	int 		powerLevelOffset;
	long	 	powerLevelDisplay;
	float		multiplier;
	vec4_t	powerColor = {0.0f,0.588f,1.0f,1.0f};
	vec4_t	dullColor = {0.188f,0.278f,0.345f,1.0f};
	vec4_t	limitColor = {1.0f,0.1f,0.0f,1.0f};
	vec4_t	beyondFatigueColor = {0.9f,0.5f,0.0f,1.0f};
	vec4_t	beyondHealthColor = {0.8f,0.2f,0.2f,1.0f};
	vec4_t	healthFatigueColor = {1.0f,0.4f,0.2f,1.0f};
	vec4_t	plFatigueHealthColor = {0.5f,0.16f,0.16f,1.0f};
	vec4_t	plFatigueColor = {0.4f,0.4f,0.5f,1.0f};
	vec4_t	clearColor = {0.0f,0.0f,0.0f,0.0f};
	vec3_t	angles;
	if(ps->bitFlags & usingSoar){return;}
	CG_DrawHorGauge(x+60,y+41,200,16,powerColor,dullColor,ps->powerLevel[plCurrent],ps->powerLevel[plMaximum],qfalse);	
	CG_DrawRightGauge(x+60,y+41,200,16,plFatigueColor,plFatigueColor,ps->powerLevel[plFatigue],ps->powerLevel[plMaximum]);
	CG_DrawRightGauge(x+60,y+41,200,16,limitColor,limitColor,ps->powerLevel[plHealth],ps->powerLevel[plMaximum]);
	CG_DrawDiffGauge(x+60,y+41,200,16,beyondFatigueColor,beyondFatigueColor,ps->powerLevel[plCurrent],ps->powerLevel[plFatigue],ps->powerLevel[plMaximum],1);
	CG_DrawDiffGauge(x+60,y+41,200,16,plFatigueHealthColor,plFatigueHealthColor,ps->powerLevel[plFatigue],ps->powerLevel[plHealth],ps->powerLevel[plMaximum],1);
	CG_DrawDiffGauge(x+60,y+41,200,16,beyondHealthColor,beyondHealthColor,ps->powerLevel[plCurrent],ps->powerLevel[plHealth],ps->powerLevel[plMaximum],1);
	if((ps->powerLevel[plCurrent] > ps->powerLevel[plFatigue]) && (ps->powerLevel[plFatigue] > ps->powerLevel[plHealth])){
		CG_DrawDiffGauge(x+60,y+41,200,16,healthFatigueColor,healthFatigueColor,ps->powerLevel[plCurrent],ps->powerLevel[plFatigue],ps->powerLevel[plMaximum],1);
	}
	CG_DrawPic(qfalse,x,y,288,72,cgs.media.hudShader);
	CG_DrawHead(x+6,y+22,50,50,clientNum,angles);
	if(ps->powerLevel[plCurrent] == ps->powerLevel[plMaximum] && ps->bitFlags & usingAlter){
		CG_DrawPic(qfalse,x+243,y+25,40,44,cgs.media.breakLimitShader);
	}
	if(ps->powerLevel[plCurrent] == 9001){
		powerLevelString = "Over ^3NINE-THOUSAND!!!";
	}
	multiplier = cgs.clientinfo[clientNum].tierConfig[cgs.clientinfo[clientNum].tierCurrent].hudMultiplier;
	if(multiplier <= 0){
		multiplier = 1.0;
	}
	powerLevelDisplay = (float)ps->powerLevel[plCurrent] * multiplier;
	powerLevelString = powerLevelDisplay >= 1000000 ? va("%.1f ^3mil",(float)powerLevelDisplay / 1000000.0) : va("%i",powerLevelDisplay);
	powerLevelOffset = (Q_PrintStrlen(powerLevelString)-2)*8;
	CG_DrawSmallStringHalfHeight(x+239-powerLevelOffset,y+44,powerLevelString,1.0F);
}
static void CG_DrawStatusBar( void ) {
	centity_t		*cent;
	playerState_t	*ps;
	float			tierLast,tierNext,tier;
	int				base;
	clientInfo_t	*ci;
	cg_userWeapon_t	*weaponGraphics;
	tierConfig_cg	*activeTier;

	ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;
	if((ci->lockStartTimer > cg.time) /*(&& cg.time > ci->lockStartTimer - 500)*/){
		CG_DrawPic(qfalse,0,0,640,480,cgs.media.speedLineSpinShader);
	}
	if(ps->bitFlags & usingBoost){
		CG_DrawPic(qfalse,0,0,640,480,cgs.media.speedLineShader);
	}
	if(cg_drawStatus.integer == 0){return;}
	cent = &cg_entities[cg.snap->ps.clientNum];
	tier = (float)ps->powerLevel[plTierCurrent];
	CG_CheckChat();	
	//CG_DrawScoreboard();
	CG_DrawScreenEffects();
	if(ps->lockedTarget > 0 && cgs.clientinfo[ps->lockedTarget-1].infoValid){
		playerState_t lockedTargetPS;
		lockedTargetPS.clientNum = ps->lockedTarget-1;
		lockedTargetPS.powerLevel[plCurrent] = ps->lockedPlayerData[lkPowerCurrent];
		lockedTargetPS.powerLevel[plHealth] = ps->lockedPlayerData[lkPowerHealth];
		lockedTargetPS.powerLevel[plMaximum] = ps->lockedPlayerData[lkPowerMaximum];
		lockedTargetPS.powerLevel[plFatigue] = lockedTargetPS.powerLevel[plMaximum];
		lockedTargetPS.powerLevel[plTierCurrent] = cgs.clientinfo[lockedTargetPS.clientNum].tierCurrent;
		CG_DrawHUD(ps,ps->clientNum,0,0,qfalse);
		CG_DrawHUD(&lockedTargetPS,lockedTargetPS.clientNum,320,0,qtrue);
	}
	else{

		CG_DrawHUD(ps,ps->clientNum,0,408,qfalse);
		if(tier){
			activeTier = &ci->tierConfig[ci->tierCurrent];
			tierLast = 32767;
			if(activeTier->sustainCurrent && activeTier->sustainCurrent < tierLast){tierLast = (float)activeTier->sustainCurrent;}
			if(activeTier->sustainFatigue && activeTier->sustainFatigue < tierLast){tierLast = (float)activeTier->sustainFatigue;}
			if(activeTier->sustainHealth && activeTier->sustainHealth < tierLast){tierLast = (float)activeTier->sustainHealth;}
			if(activeTier->sustainMaximum && activeTier->sustainMaximum < tierLast){tierLast = (float)activeTier->sustainMaximum;}
			if(tierLast < 32767){
				tierLast = tierLast / (float)ps->powerLevel[plMaximum];
				CG_DrawPic(qfalse,(187*tierLast)+60,428,13,38,cgs.media.markerDescendShader);
			}
		}
		if(tier < ps->powerLevel[plTierTotal]){
			activeTier = &ci->tierConfig[ci->tierCurrent+1];
			tierNext = 0;
			if(activeTier->requirementCurrent && activeTier->requirementCurrent > tierNext){tierNext = (float)activeTier->requirementCurrent;}
			if(activeTier->requirementFatigue && activeTier->requirementFatigue > tierNext){tierNext = (float)activeTier->requirementFatigue;}
			if(activeTier->requirementMaximum && activeTier->requirementMaximum > tierNext){tierNext = (float)activeTier->requirementMaximum;}
			if(activeTier->requirementHealth && activeTier->requirementHealth > tierNext){tierNext = (float)activeTier->requirementHealth;}
			if(tierNext){
				tierNext = tierNext / (float)ps->powerLevel[plMaximum];
				if(tierNext < 1.0){
					CG_DrawPic(qfalse,(187*tierNext)+60,428,13,38,cgs.media.markerAscendShader);
				}
			}
		}
	}
}/*==================
CG_DrawSnapshot
==================*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	16
static float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int			i, total;
	int			fps;
	static	int	previous, lastupdate;
	int			t, frameTime;
	const int	xOffset = 0;
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;
	if (t - lastupdate > 50){
		lastupdate = t;
		previousTimes[index % FPS_FRAMES] = frameTime;
		index++;
	}
	total = 0;
	for(i = 0 ; i < FPS_FRAMES ; i++){
		total += previousTimes[i];
	}
	if(!total){total = 1;}
	fps = 1000 * FPS_FRAMES / total;
	s = va( "%ifps", fps );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 635 - w + xOffset, y + 2, s, 1.0F);
	return y + BIGCHAR_HEIGHT + 4;
}

/*=================
CG_DrawTimer
=================*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float y;
	y = 0;
	if(cg_drawSnapshot.integer){
		y = CG_DrawSnapshot( y );
	}
	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
}

/*==============
CG_DrawDisconnect
Should we draw something differnet for long lag vs no packets?
==============*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic(qfalse, x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
}
/*==============
CG_CenterPrint
Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}
/*===================
CG_DrawCenterString
===================*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		x, y, w;
	float	*color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value, 200 );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;
		w = cg.centerPrintCharWidth * CG_DrawStrlen( linebuffer );
		x = ( SCREEN_WIDTH - w ) / 2;
		CG_DrawStringExt(-1, x, y, linebuffer, color, qfalse, qtrue,
			cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0 );
		y += cg.centerPrintCharWidth * 1.5;
		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}
/*=================
CG_DrawCrosshair
=================*/
static void CG_DrawCrosshair(void) {
	float			w, h;
	qhandle_t		hShader;
	float			f;
	float			x, y;
	int				ca;
	int				i;
	trace_t			trace;
	playerState_t	*ps;
	clientInfo_t	*ci;
	tierConfig_cg	*tier;
	vec3_t			muzzle, forward, up;
	vec3_t			start, end;
	vec4_t			lockOnEnemyColor	= {1.0f,0.0f,0.0f,1.0f};
	vec4_t			lockOnAllyColor		= {0.0f,1.0f,0.0f,1.0f};
	vec4_t			chargeColor			= {0.5f,0.5f,1.0f,1.0f};
	radar_t			cg_playerOrigins[MAX_CLIENTS];
	if(!cg_drawCrosshair.integer || cg.snap->ps.lockedTarget > 0){return;}
	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR){return;}
	ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	tier = &ci->tierConfig[ci->tierCurrent];
	ps = &cg.predictedPlayerState;
	if(ps->bitFlags & usingMelee){return;}
	AngleVectors( ps->viewangles, forward, NULL, up );
	VectorCopy( ps->origin, muzzle );
	VectorMA( muzzle, ps->viewheight, up, muzzle );
	VectorMA( muzzle, 14, forward, muzzle );
	VectorCopy( muzzle, start );
	VectorMA(start, 131072, forward, end);
	CG_Trace(&trace, start, NULL, NULL, end, cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY);	
	if(!CG_WorldCoordToScreenCoordFloat( trace.endpos, &x, &y)){return;}
	w = h = (cg_crosshairSize.value * 8 + 8);
	f = cg.time - cg.itemPickupBlendTime;
	if(f > 0 && f < ITEM_BLOB_TIME){
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}
	if(cg_crosshairHealth.integer){
		vec4_t		hcolor;
		CG_ColorForHealth(hcolor);
		trap_R_SetColor(hcolor);
	}
	else{
		trap_R_SetColor( NULL );
	}
	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ca % NUM_CROSSHAIRS];
	if(tier->crosshair){
		hShader = tier->crosshair;
		if(ps->bitFlags & isBreakingLimit && tier->crosshairPowering){
			hShader = tier->crosshairPowering;
		}
	}
	if ( cg.snap->ps.currentSkill[WPSTAT_BITFLAGS] & WPF_READY || cg.snap->ps.currentSkill[WPSTAT_ALT_BITFLAGS] & WPF_READY) {
		trap_R_SetColor( chargeColor );
	}
	else if (cg.crosshairClientNum > 0 && cg.crosshairClientNum <= MAX_CLIENTS || ps->lockedTarget > 0) {
		if( cgs.clientinfo[cg.crosshairClientNum].team == cg.snap->ps.persistant[PERS_TEAM] && cgs.clientinfo[cg.crosshairClientNum].team != TEAM_FREE  ) {
			trap_R_SetColor( lockOnAllyColor );
		}
		else{
			trap_R_SetColor( lockOnEnemyColor );
		}
	}
	else{
		trap_R_SetColor( NULL );
	}
	CG_DrawPic(qfalse, x - 0.5f * w, y - 0.5f * h, w, h, hShader );
	trap_R_SetColor( NULL );
}
/*=================
CG_ScanForCrosshairEntity
=================*/
static void CG_ScanForCrosshairEntity(void){
	trace_t			trace,trace2;
	vec3_t			start,end,muzzle,forward,up,minSize,maxSize;
	playerState_t	*ps;
	ps = &cg.predictedPlayerState;
	AngleVectors(ps->viewangles,forward,NULL,up);
	VectorCopy(ps->origin, muzzle );
	VectorMA(muzzle,ps->viewheight,up,muzzle);
	VectorMA(muzzle,14,forward,muzzle);
	VectorCopy(muzzle,start);
	VectorMA(start,131072,forward,end);
	minSize[0] = -(float)cg_lockonDistance.value;
	minSize[1] = -(float)cg_lockonDistance.value;
	minSize[2] = -(float)cg_lockonDistance.value;
	maxSize[0] = -minSize[0];
	maxSize[1] = -minSize[1];
	maxSize[2] = -minSize[2];
	CG_Trace(&trace,start,minSize,maxSize,end,cg.snap->ps.clientNum,CONTENTS_BODY);
	if (trace.entityNum>=MAX_CLIENTS){cg.crosshairClientNum= -1;return;}
	cg.crosshairClientNum=trace.entityNum;
	cg.crosshairClientTime=cg.time;
}
/*=====================
CG_DrawCrosshairNames
=====================*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	char		*name;
	float		w;
	if ( !cg_drawCrosshair.integer ) {
		return;
	}
	CG_ScanForCrosshairEntity();

	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	color = CG_FadeColor( cg.crosshairClientTime, 1000, 200 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}
	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
	w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w / 2, 170, name, color[3] * 0.5f );
	trap_R_SetColor( NULL );
}
/*=================
CG_DrawSpectator
=================*/
static void CG_DrawSpectator(void) {
	CG_DrawBigString(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
	if ( cgs.gametype == GT_TOURNAMENT ) {
		CG_DrawBigString(320 - 15 * 8, 460, "waiting to play", 1.0F);
	}
	else if ( cgs.gametype >= GT_TEAM ) {
		CG_DrawBigString(320 - 39 * 8, 460, "press ESC and use the JOIN menu to play", 1.0F);
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	char	*s;
	int		sec;

	if ( !cgs.voteTime ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo );
	CG_DrawSmallString( 0, 58, s, 1.0F );
}

/*=================
CG_DrawTeamVote
=================*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo->team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo->team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
							cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	CG_DrawSmallString( 0, 90, s, 1.0F );
}
/*=================
CG_DrawWarmup
=================*/
static void CG_DrawWarmup( void ) {
	int			w;
	int			sec;
	int			i;
	float scale;
	clientInfo_t	*ci1, *ci2;
	int			cw;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
		s = "Waiting for players";		
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString(320 - w / 2, 24, s, 1.0F);
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_TOURNAMENT) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s vs %s", ci1->name, ci2->name );
			w = CG_DrawStrlen( s );
			if ( w > 640 / GIANT_WIDTH ) {
				cw = 640 / w;
			} else {
				cw = GIANT_WIDTH;
			}
			CG_DrawStringExt(-1, 320 - w * cw/2, 20,s, colorWhite, 
					qfalse, qtrue, cw, (int)(cw * 1.5f), 0 );
		}
	} else {
		if ( cgs.gametype == GT_FFA ) {
			s = "Free For All";
		} else if ( cgs.gametype == GT_STRUGGLE ) {
			s = "Struggle";
		} else if ( cgs.gametype == GT_TEAM ) {
			s = "Team Deathmatch";
		} else if ( cgs.gametype == GT_CTF ) {
			s = "Capture the Flag";
		} else {
			s = "";
		}
		w = CG_DrawStrlen( s );
		if ( w > 640 / GIANT_WIDTH ) {
			cw = 640 / w;
		} else {
			cw = GIANT_WIDTH;
		}
		CG_DrawStringExt(-1, 320 - w * cw/2, 25,s, colorWhite, 
				qfalse, qtrue, cw, (int)(cw * 1.1f), 0 );
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
	s = va( "Starts in: %i", sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;
	}
	scale = 0.45f;
	switch ( cg.warmupCount ) {
	case 0:
		cw = 28;
		scale = 0.54f;
		break;
	case 1:
		cw = 24;
		scale = 0.51f;
		break;
	case 2:
		cw = 20;
		scale = 0.48f;
		break;
	default:
		cw = 16;
		scale = 0.45f;
		break;
	}
	w = CG_DrawStrlen( s );
	CG_DrawStringExt(-1, 320 - w * cw/2, 70, s, colorWhite, 
			qfalse, qtrue, cw, (int)(cw * 1.5), 0 );
}
/*=================
CG_Draw2D
=================*/
static void CG_Draw2D( void ) {
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}
	if ( cg_draw2D.integer == 0 ) {
		return;
	}
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();
		CG_DrawCrosshair();
		CG_DrawCrosshairNames();
		CG_DrawRadar();
	}
	else {
		if (!(cg.snap->ps.timers[tmTransform] > 1) && !(cg.snap->ps.powerups[PW_STATE] < 0)){
		

			playerState_t	*ps;
			clientInfo_t *ci;
			ci = &cgs.clientinfo[cg.snap->ps.clientNum];
			ps = &cg.snap->ps;
			CG_DrawStatusBar();
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawRadar();
			if(!(cg.snap->ps.bitFlags & usingMelee)){
				CG_DrawWeaponSelect();
			}
		}
	}
	CG_DrawVote();
	CG_DrawTeamVote();
	CG_DrawUpperRight();
}
void CG_DrawScreenFlash ( void ) {
	float		*color;
	vec4_t		white = {1.0f,1.0f,1.0f,0.5f};
	vec4_t		black = {0.0f,0.0f,0.0f,0.5f};
	color = CG_FadeColor( cg.screenFlashTime, cg.screenFlastTimeTotal, cg.screenFlashFadeTime );
	if ( !color ) {
		return;
	}
	if ( cg.snap->ps.timers[tmBlind] > 0 ){
		trap_R_SetColor( color );
		white[3] = color[3] * cg.screenFlashFadeAmount * 0.5f;
		CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, white);
		trap_R_SetColor( NULL );
	}
}

/*=====================
CG_DrawActive
Perform all drawing needed to completely fill the screen
=====================*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	vec4_t		water = {0.25f,0.5f,1.0f,0.1f};
	int			contents;
	if(!cg.snap){
		CG_DrawInformation();
		return;
	}
	CG_TileClear();
	CG_MotionBlur();
	trap_R_RenderScene(&cg.refdef);
	contents = CG_PointContents(cg.refdef.vieworg,-1);
	if(contents & CONTENTS_WATER){
		float phase = cg.time / 1000.0 * 0.2f * M_PI * 2;
		water[3] = 0.1f + (0.02f*sin( phase ));
		CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, water);
		//trap_R_AddFogToScene(0,5000,0,0,0,1,2,2);
	}
	else{
		//trap_R_AddFogToScene(10000,20000,0,0,0,0.2,2,0);
	}
	CG_DrawScreenFlash();
 	CG_Draw2D();
	CG_LoadDeferredPlayers();
}



