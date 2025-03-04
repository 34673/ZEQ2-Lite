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

void GLimp_InitExtraExtensions(void)
{
	char *extension;
	const char* result[3] = { "...ignoring %s\n", "...using %s\n", "...%s not found\n" };

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

			ri.Printf(PRINT_ALL, result[glRefConfig.occlusionQuery], extension);
		}
		else
		{
			ri.Printf(PRINT_ALL, result[2], extension);
		}

		// GL_NV_read_depth
		extension = "GL_NV_read_depth";
		if (SDL_GL_ExtensionSupported(extension))
		{
			glRefConfig.readDepth = qtrue;
			ri.Printf(PRINT_ALL, result[glRefConfig.readDepth], extension);
		}
		else
		{
			ri.Printf(PRINT_ALL, result[2], extension);
		}

		// GL_NV_read_stencil
		extension = "GL_NV_read_stencil";
		if (SDL_GL_ExtensionSupported(extension))
		{
			glRefConfig.readStencil = qtrue;
			ri.Printf(PRINT_ALL, result[glRefConfig.readStencil], extension);
		}
		else
		{
			ri.Printf(PRINT_ALL, result[2], extension);
		}

		// GL_EXT_shadow_samplers
		extension = "GL_EXT_shadow_samplers";
		if (qglesMajorVersion >= 3 || SDL_GL_ExtensionSupported(extension))
		{
			glRefConfig.shadowSamplers = qtrue;
			ri.Printf(PRINT_ALL, result[glRefConfig.shadowSamplers], extension);
		}
		else
		{
			ri.Printf(PRINT_ALL, result[2], extension);
		}

		// GL_OES_standard_derivatives
		extension = "GL_OES_standard_derivatives";
		if (qglesMajorVersion >= 3 || SDL_GL_ExtensionSupported(extension))
		{
			glRefConfig.standardDerivatives = qtrue;
			ri.Printf(PRINT_ALL, result[glRefConfig.standardDerivatives], extension);
		}
		else
		{
			ri.Printf(PRINT_ALL, result[2], extension);
		}

		// GL_OES_element_index_uint
		extension = "GL_OES_element_index_uint";
		if (qglesMajorVersion >= 3 || SDL_GL_ExtensionSupported(extension))
		{
			glRefConfig.vaoCacheGlIndexType = GL_UNSIGNED_INT;
			glRefConfig.vaoCacheGlIndexSize = sizeof(unsigned int);
			ri.Printf(PRINT_ALL, result[1], extension);
		}
		else
		{
			ri.Printf(PRINT_ALL, result[2], extension);
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
	if( SDL_GL_ExtensionSupported( extension ) )
	{
		glRefConfig.memInfo = MI_NVX;

		ri.Printf(PRINT_ALL, result[1], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ATI_meminfo
	extension = "GL_ATI_meminfo";
	if( SDL_GL_ExtensionSupported( extension ) )
	{
		if (glRefConfig.memInfo == MI_NONE)
		{
			glRefConfig.memInfo = MI_ATI;

			ri.Printf(PRINT_ALL, result[1], extension);
		}
		else
		{
			ri.Printf(PRINT_ALL, result[0], extension);
		}
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

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
	if (SDL_GL_ExtensionSupported(extension))
	{
		glRefConfig.directStateAccess = !!r_ext_direct_state_access->integer;

		// QGL_*_PROCS becomes several functions, do not remove {}
		if (glRefConfig.directStateAccess)
		{
			QGL_EXT_direct_state_access_PROCS;
		}

		ri.Printf(PRINT_ALL, result[glRefConfig.directStateAccess], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

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
