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
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

static float CG_InfoWideVirtualLeft( void ) {
	if ( cgs.screenXScale <= 0.0f ) {
		return 0.0f;
	}
	return -cgs.screenXBias / cgs.screenXScale;
}

static void CG_DrawPicFill( qhandle_t shader ) {
	float s0 = 0.0f, s1 = 1.0f;
	float t0 = 0.0f, t1 = 1.0f;
	float screenAspect = (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight;
	float imageAspect = 4.0f / 3.0f;

	if ( screenAspect > imageAspect ) {
		float visibleHeight = imageAspect / screenAspect;
		float crop = 0.5f * ( 1.0f - visibleHeight );
		t0 += crop;
		t1 -= crop;
	}
	else if ( screenAspect < imageAspect ) {
		float visibleWidth = screenAspect / imageAspect;
		float crop = 0.5f * ( 1.0f - visibleWidth );
		s0 += crop;
		s1 -= crop;
	}

	trap_R_DrawStretchPic( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, s0, t0, s1, t1, shader );
}


#define MAX_LOADING_PLAYER_ICONS	16

static int			loadingPlayerIconCount;
static char	 		loadingPlayerNames[MAX_LOADING_PLAYER_ICONS][32];
static qhandle_t	loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];

/*
===================
CG_DrawLoadingIcons
===================
*/
static void CG_DrawLoadingIcons( void ) {
	int		n;
	int		y;
	float	baseX;

	baseX = CG_InfoWideVirtualLeft() + 24.0f;
	cgs.media.chatBackgroundShader = trap_R_RegisterShaderNoMip("chatBox");
	for( n = 0; n < loadingPlayerIconCount; n++ ) {
		y = 80 + (40 * n);
		CG_DrawPic(qfalse,baseX - 30.0f,y - 22.0f,369,75,cgs.media.chatBackgroundShader );
		CG_DrawPic(qfalse,baseX,y,32,32,loadingPlayerIcons[n] );
		CG_DrawSmallStringCustom(baseX + 37.0f,y + 10.0f,11,11,loadingPlayerNames[n],1.0,6);
		//cgs.clientinfo[clientNum].name
	}
}


/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );
	trap_UpdateScreen();
}


/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient( int clientNum ) {
	const char		*info;
	char			*skin;
	char			name[32];
	char			personality[MAX_QPATH];
	char			model[MAX_QPATH];
	char			iconName[MAX_QPATH];

	info = CG_ConfigString( CS_PLAYERS + clientNum );

	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		Q_strncpyz( name, Info_ValueForKey( info, "n" ), sizeof(name) );
		Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
		skin = strrchr( model, '/' );
		if ( skin ) {
			*skin++ = '\0';
		} else {
			skin = "default";
		}

		Com_sprintf( iconName, MAX_QPATH, "players//%s/icon_%s.tga", model, skin );
	
		strcpy(loadingPlayerNames[loadingPlayerIconCount],name);
		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "players//characters/%s/icon_%s.tga", model, skin );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "players//%s/icon_%s.tga", DEFAULT_MODEL, "default" );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}
	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof(personality) );
	Q_CleanStr( personality );
	CG_LoadingString( personality );
}


/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation( void ) {
	const char	*s;
	const char	*info;
	qhandle_t	levelshot,text,dots;
	float		leftX;

	info = CG_ConfigString( CS_SERVERINFO );
	s = Info_ValueForKey(info,"mapname");
	levelshot = trap_R_RegisterShaderNoMip(va("maps/%s.jpg",s));
	dots = trap_R_RegisterShaderNoMip("dots");
	if(!levelshot){levelshot = trap_R_RegisterShaderNoMip("menuback");}
	trap_R_SetColor(NULL);
	leftX = CG_InfoWideVirtualLeft();
	CG_DrawPicFill(levelshot);
	CG_DrawLoadingIcons();
	if(cg.infoScreenText[0]){
		text = trap_R_RegisterShaderNoMip("loading");
		CG_DrawPic(qfalse,leftX,18,127,64,text);
		CG_DrawPic(qfalse,leftX + 64.0f,52,8,4,dots);
	}
	else{
		text = trap_R_RegisterShaderNoMip("ready");
		CG_DrawPic(qfalse,leftX,18,127,64,text);
	}
	// Remote Server only
}

