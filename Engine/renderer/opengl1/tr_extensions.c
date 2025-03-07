/*
===========================================================================
Copyright (C) 2011 James Canete (use.less01@gmail.com)

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
// tr_extensions.c - extensions needed by the renderer not in sdl_glimp.c

#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

#include "tr_local.h"
enum { EXTENSION_IGNORED, EXTENSION_USED, EXTENSION_NOT_FOUND };
/*
===============
GLimp_InitExtensions
===============
*/
void GLimp_InitExtensions( qboolean fixedFunction )
{
	char* extension;
	const char* result[] = { "...ignoring %s\n", "...using %s\n", "...%s not found\n" };
	int resultIndex = EXTENSION_NOT_FOUND;
	if ( !r_allowExtensions->integer )
	{
		ri.Printf( PRINT_ALL, "* IGNORING OPENGL EXTENSIONS *\n" );
		return;
	}

#define GLE(ret, name, ...) qgl##name = (name##proc *) SDL_GL_GetProcAddress("gl" #name);

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	glConfig.textureCompression = TC_NONE;

	// GL_EXT_texture_compression_s3tc
	extension = "GL_EXT_texture_compression_s3tc";
	resultIndex = EXTENSION_NOT_FOUND;
	if ( SDL_GL_ExtensionSupported( "GL_ARB_texture_compression" ) && SDL_GL_ExtensionSupported( extension ) )
	{
		qboolean useCompression = !!r_ext_compressed_textures->value;
		if ( useCompression )
		{
			glConfig.textureCompression = TC_S3TC_ARB;
		}
		resultIndex = useCompression;
	}
	ri.Printf( PRINT_ALL, result[resultIndex], extension );

	// GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
	if (glConfig.textureCompression == TC_NONE)
	{
		extension = "GL_S3_s3tc";
		resultIndex = EXTENSION_NOT_FOUND;
		if ( SDL_GL_ExtensionSupported( extension ) )
		{
			qboolean useCompression = !!r_ext_compressed_textures->value;
			if ( useCompression )
			{
				glConfig.textureCompression = TC_S3TC;
			}
			resultIndex = useCompression;
		}
		ri.Printf( PRINT_ALL, result[resultIndex], extension );
	}

	// OpenGL 1 fixed function pipeline
	if ( fixedFunction )
	{
		// GL_EXT_texture_env_add
		extension = "GL_EXT_texture_env_add";
		glConfig.textureEnvAddAvailable = qfalse;
		resultIndex = EXTENSION_NOT_FOUND;
		if ( SDL_GL_ExtensionSupported( extension ) )
		{
			glConfig.textureEnvAddAvailable = !!r_ext_texture_env_add->integer;
			resultIndex = glConfig.textureEnvAddAvailable;
		}
		ri.Printf( PRINT_ALL, result[resultIndex], extension );

		// GL_ARB_multitexture
		extension = "GL_ARB_multitexture";
		if ( SDL_GL_ExtensionSupported( extension ) )
		{
			if ( r_ext_multitexture->value )
			{
				QGL_ARB_multitexture

				if ( qglActiveTextureARB )
				{
					GLint glint = 0;
					qglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glint );
					glConfig.numTextureUnits = (int) glint;
					if ( glConfig.numTextureUnits > 1 )
					{
						ri.Printf( PRINT_ALL, result[EXTENSION_USED], extension );
					}
					else
					{
						qglMultiTexCoord2fARB = NULL;
						qglActiveTextureARB = NULL;
						qglClientActiveTextureARB = NULL;
						ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
					}
				}
			}
			else
			{
				ri.Printf( PRINT_ALL, result[EXTENSION_IGNORED], extension );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, result[EXTENSION_NOT_FOUND], extension );
		}

		// GL_EXT_compiled_vertex_array
		extension = "GL_EXT_compiled_vertex_array";
		resultIndex = EXTENSION_NOT_FOUND;
		if ( SDL_GL_ExtensionSupported( extension ) )
		{
			qboolean useCompiledVertexArrays = !!r_ext_compiled_vertex_array->value;
			resultIndex = useCompiledVertexArrays;
			if ( useCompiledVertexArrays )
			{
				QGL_EXT_compiled_vertex_array
				if (!qglLockArraysEXT || !qglUnlockArraysEXT)
				{
					ri.Error (ERR_FATAL, "bad getprocaddress");
				}
			}
		}
		ri.Printf( PRINT_ALL, result[resultIndex], extension );
	}

	extension = "GL_EXT_texture_filter_anisotropic";
	textureFilterAnisotropic = qfalse;
	if ( SDL_GL_ExtensionSupported( extension ) )
	{
		if ( r_ext_texture_filter_anisotropic->integer ) {
			qglGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint *)&maxAnisotropy );
			if ( maxAnisotropy <= 0 ) {
				ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not properly supported!\n" );
				maxAnisotropy = 0;
			}
			else
			{
				ri.Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic (max: %i)\n", maxAnisotropy );
				textureFilterAnisotropic = qtrue;
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, result[EXTENSION_IGNORED], extension );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, result[EXTENSION_NOT_FOUND], extension );
	}

	extension = "GL_SGIS_texture_edge_clamp";
	haveClampToEdge = QGL_VERSION_ATLEAST( 1, 2 ) || QGLES_VERSION_ATLEAST( 1, 0 ) || SDL_GL_ExtensionSupported( extension );
	resultIndex = haveClampToEdge ? EXTENSION_USED : EXTENSION_NOT_FOUND;
	ri.Printf( PRINT_ALL, result[resultIndex], extension );
#undef GLE
}
