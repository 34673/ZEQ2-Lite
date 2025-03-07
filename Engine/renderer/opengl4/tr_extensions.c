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
#include "tr_dsa.h"
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

void GLimp_InitExtraExtensions(void)
{
	char *extension;
	const char* result[] = { "...ignoring %s\n", "...using %s\n", "...%s not found\n" };
	int resultIndex = EXTENSION_NOT_FOUND;

	// Check if we need Intel graphics specific fixes.
	glRefConfig.intelGraphics = qfalse;
	if (strstr((char *)qglGetString(GL_RENDERER), "Intel"))
		glRefConfig.intelGraphics = qtrue;

	if (qglesMajorVersion)
	{
		glRefConfig.vaoCacheGlIndexType = GL_UNSIGNED_SHORT;
		glRefConfig.vaoCacheGlIndexSize = sizeof(unsigned short);
	}
	else
	{
		glRefConfig.vaoCacheGlIndexType = GL_UNSIGNED_INT;
		glRefConfig.vaoCacheGlIndexSize = sizeof(unsigned int);
	}

	// set DSA fallbacks
#define GLE(ret, name, ...) qgl##name = GLDSA_##name;
	QGL_EXT_direct_state_access_PROCS;
#undef GLE

	// GL function loader, based on https://gist.github.com/rygorous/16796a0c876cf8a5f542caddb55bce8a
#define GLE(ret, name, ...) qgl##name = (name##proc *) SDL_GL_GetProcAddress("gl" #name);

	//
	// OpenGL ES extensions
	//
	if (qglesMajorVersion)
	{
		if (!r_allowExtensions->integer)
			goto done;

		extension = "GL_EXT_occlusion_query_boolean";
		resultIndex = EXTENSION_NOT_FOUND;
		if (qglesMajorVersion >= 3 || SDL_GL_ExtensionSupported(extension))
		{
			glRefConfig.occlusionQuery = qtrue;
			glRefConfig.occlusionQueryTarget = GL_ANY_SAMPLES_PASSED;

			if (qglesMajorVersion >= 3) {
				QGL_ARB_occlusion_query_PROCS;
			} else {
				// GL_EXT_occlusion_query_boolean uses EXT suffix
#undef GLE
#define GLE(ret, name, ...) qgl##name = (name##proc *) SDL_GL_GetProcAddress("gl" #name "EXT");

				QGL_ARB_occlusion_query_PROCS;

#undef GLE
#define GLE(ret, name, ...) qgl##name = (name##proc *) SDL_GL_GetProcAddress("gl" #name);
			}

			resultIndex = glRefConfig.occlusionQuery;
		}
		ri.Printf(PRINT_ALL, result[resultIndex], extension);

		// GL_NV_read_depth
		extension = "GL_NV_read_depth";
		glRefConfig.readDepth = SDL_GL_ExtensionSupported(extension);
		resultIndex = glRefConfig.readDepth ? EXTENSION_USED : EXTENSION_NOT_FOUND;
		ri.Printf(PRINT_ALL, result[resultIndex], extension);

		// GL_NV_read_stencil
		extension = "GL_NV_read_stencil";
		glRefConfig.readStencil = SDL_GL_ExtensionSupported(extension);
		resultIndex = glRefConfig.readStencil ? EXTENSION_USED : EXTENSION_NOT_FOUND;
		ri.Printf(PRINT_ALL, result[resultIndex], extension);

		// GL_EXT_shadow_samplers
		extension = "GL_EXT_shadow_samplers";
		glRefConfig.shadowSamplers = qglesMajorVersion >= 3 || SDL_GL_ExtensionSupported(extension);
		resultIndex = glRefConfig.shadowSamplers ? EXTENSION_USED : EXTENSION_NOT_FOUND;
		ri.Printf(PRINT_ALL, result[resultIndex], extension);

		// GL_OES_standard_derivatives
		extension = "GL_OES_standard_derivatives";
		glRefConfig.standardDerivatives = qglesMajorVersion >= 3 || SDL_GL_ExtensionSupported(extension);
		resultIndex = glRefConfig.standardDerivatives ? EXTENSION_USED : EXTENSION_NOT_FOUND;
		ri.Printf(PRINT_ALL, result[resultIndex], extension);

		// GL_OES_element_index_uint
		extension = "GL_OES_element_index_uint";
		{
			qboolean uintIndexSupported = qglesMajorVersion >= 3 || SDL_GL_ExtensionSupported(extension);
			if (uintIndexSupported)
			{
				glRefConfig.vaoCacheGlIndexType = GL_UNSIGNED_INT;
				glRefConfig.vaoCacheGlIndexSize = sizeof(unsigned int);
			}
			resultIndex = uintIndexSupported ? EXTENSION_USED : EXTENSION_NOT_FOUND;
			ri.Printf(PRINT_ALL, result[resultIndex], extension);
		}

		goto done;
	}

	// OpenGL 1.5 - GL_ARB_occlusion_query
	glRefConfig.occlusionQuery = qtrue;
	glRefConfig.occlusionQueryTarget = GL_SAMPLES_PASSED;
	QGL_ARB_occlusion_query_PROCS;

	// OpenGL 3.0 - GL_ARB_framebuffer_object
	qglGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &glRefConfig.maxRenderbufferSize);
	qglGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &glRefConfig.maxColorAttachments);
	QGL_ARB_framebuffer_object_PROCS;

	// OpenGL 3.0 - GL_ARB_vertex_array_object
	QGL_ARB_vertex_array_object_PROCS;

	// OpenGL 3.0 - GL_ARB_texture_float
	extension = "GL_ARB_texture_float";
	glRefConfig.textureFloat = !!r_ext_texture_float->integer;

	// OpenGL 3.2 - GL_ARB_depth_clamp
	// OpenGL 3.2 - GL_ARB_seamless_cube_map
	glRefConfig.seamlessCubeMap = !!r_arb_seamless_cube_map->integer;

	glRefConfig.memInfo = MI_NONE;

	// GL_NVX_gpu_memory_info
	extension = "GL_NVX_gpu_memory_info";
	glRefConfig.memInfo = SDL_GL_ExtensionSupported( extension ) ? MI_NVX : MI_NONE;
	resultIndex = glRefConfig.memInfo ? EXTENSION_USED : EXTENSION_NOT_FOUND;
	ri.Printf(PRINT_ALL, result[resultIndex], extension);

	// GL_ATI_meminfo
	extension = "GL_ATI_meminfo";
	resultIndex = EXTENSION_NOT_FOUND;
	if( SDL_GL_ExtensionSupported( extension ) )
	{
		glRefConfig.memInfo = glRefConfig.memInfo == MI_NONE ? MI_ATI : glRefConfig.memInfo;
		resultIndex = glRefConfig.memInfo == MI_ATI;
	}
	ri.Printf(PRINT_ALL, result[resultIndex], extension);

	glRefConfig.textureCompression = TCR_NONE;

	// OpenGL 3.0 - GL_ARB_texture_compression_rgtc
	if (r_ext_compressed_textures->integer >= 1)
			glRefConfig.textureCompression |= TCR_RGTC;

	glRefConfig.swizzleNormalmap = r_ext_compressed_textures->integer && !(glRefConfig.textureCompression & TCR_RGTC);

	// OpenGL 4.2 - GL_ARB_texture_compression_bptc
	if (r_ext_compressed_textures->integer >= 2)
		glRefConfig.textureCompression |= TCR_BPTC;

	// GL_EXT_direct_state_access
	extension = "GL_EXT_direct_state_access";
	glRefConfig.directStateAccess = qfalse;
	resultIndex = EXTENSION_NOT_FOUND;
	if (SDL_GL_ExtensionSupported(extension))
	{
		glRefConfig.directStateAccess = !!r_ext_direct_state_access->integer;

		// QGL_*_PROCS becomes several functions, do not remove {}
		if (glRefConfig.directStateAccess)
		{
			QGL_EXT_direct_state_access_PROCS;
		}

		resultIndex = glRefConfig.directStateAccess;
	}
	ri.Printf(PRINT_ALL, result[resultIndex], extension);

done:

	// Determine GLSL version
	if (1)
	{
		char version[256], *version_p;

		Q_strncpyz(version, (char *)qglGetString(GL_SHADING_LANGUAGE_VERSION), sizeof(version));

		// Skip leading text such as "OpenGL ES GLSL ES "
		version_p = version;
		while ( *version_p && !isdigit( *version_p ) )
		{
			version_p++;
		}

		sscanf(version_p, "%d.%d", &glRefConfig.glslMajorVersion, &glRefConfig.glslMinorVersion);

		ri.Printf(PRINT_ALL, "...using GLSL version %s\n", version);
	}

#undef GLE
}
