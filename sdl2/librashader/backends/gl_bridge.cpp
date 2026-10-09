/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "librashader/gl_bridge.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "librashader/frame_conversion.h"
#include "librashader/librashader_loader.h"

typedef void (*VAEG_GL_ACTIVE_TEXTURE)(GLenum texture);
typedef void (*VAEG_GL_ATTACH_SHADER)(GLuint program, GLuint shader);
typedef void (*VAEG_GL_BIND_FRAMEBUFFER)(GLenum target, GLuint framebuffer);
typedef void (*VAEG_GL_BIND_TEXTURE)(GLenum target, GLuint texture);
typedef void (*VAEG_GL_BIND_VERTEX_ARRAY)(GLuint array);
typedef void (*VAEG_GL_CLEAR)(GLbitfield mask);
typedef void (*VAEG_GL_CLEAR_COLOR)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (*VAEG_GL_COMPILE_SHADER)(GLuint shader);
typedef GLuint (*VAEG_GL_CREATE_PROGRAM)(void);
typedef GLuint (*VAEG_GL_CREATE_SHADER)(GLenum type);
typedef void (*VAEG_GL_DELETE_PROGRAM)(GLuint program);
typedef void (*VAEG_GL_DELETE_SHADER)(GLuint shader);
typedef void (*VAEG_GL_DELETE_TEXTURES)(GLsizei count, const GLuint *textures);
typedef void (*VAEG_GL_DELETE_VERTEX_ARRAYS)(GLsizei count, const GLuint *arrays);
typedef void (*VAEG_GL_DISABLE)(GLenum cap);
typedef void (*VAEG_GL_DRAW_ARRAYS)(GLenum mode, GLint first, GLsizei count);
typedef void (*VAEG_GL_ENABLE)(GLenum cap);
typedef void (*VAEG_GL_GEN_TEXTURES)(GLsizei count, GLuint *textures);
typedef void (*VAEG_GL_GEN_VERTEX_ARRAYS)(GLsizei count, GLuint *arrays);
typedef void (*VAEG_GL_GET_FLOATV)(GLenum pname, GLfloat *data);
typedef void (*VAEG_GL_GET_INTEGERV)(GLenum pname, GLint *data);
typedef void (*VAEG_GL_GET_PROGRAM_INFO_LOG)(GLuint program, GLsizei max_length,
                                              GLsizei *length, GLchar *info_log);
typedef void (*VAEG_GL_GET_PROGRAM_IV)(GLuint program, GLenum pname, GLint *params);
typedef void (*VAEG_GL_GET_SHADER_INFO_LOG)(GLuint shader, GLsizei max_length, GLsizei *length,
                                             GLchar *info_log);
typedef void (*VAEG_GL_GET_SHADER_IV)(GLuint shader, GLenum pname, GLint *params);
typedef GLint (*VAEG_GL_GET_UNIFORM_LOCATION)(GLuint program, const GLchar *name);
typedef GLboolean (*VAEG_GL_IS_ENABLED)(GLenum cap);
typedef void (*VAEG_GL_LINK_PROGRAM)(GLuint program);
typedef void (*VAEG_GL_PIXEL_STOREI)(GLenum pname, GLint param);
typedef void (*VAEG_GL_SHADER_SOURCE)(GLuint shader, GLsizei count, const GLchar *const *string,
                                       const GLint *length);
typedef void (*VAEG_GL_TEX_IMAGE_2D)(GLenum target, GLint level, GLint internal_format,
                                      GLsizei width, GLsizei height, GLint border, GLenum format,
                                      GLenum type, const void *pixels);
typedef void (*VAEG_GL_TEX_PARAMETERI)(GLenum target, GLenum pname, GLint param);
typedef void (*VAEG_GL_TEX_SUB_IMAGE_2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                          GLsizei width, GLsizei height, GLenum format, GLenum type,
                                          const void *pixels);
typedef void (*VAEG_GL_UNIFORM_1I)(GLint location, GLint value);
typedef void (*VAEG_GL_USE_PROGRAM)(GLuint program);
typedef void (*VAEG_GL_VIEWPORT)(GLint x, GLint y, GLsizei width, GLsizei height);

struct VAEG_GL_FUNCTIONS {
	VAEG_GL_ACTIVE_TEXTURE active_texture;
	VAEG_GL_ATTACH_SHADER attach_shader;
	VAEG_GL_BIND_FRAMEBUFFER bind_framebuffer;
	VAEG_GL_BIND_TEXTURE bind_texture;
	VAEG_GL_BIND_VERTEX_ARRAY bind_vertex_array;
	VAEG_GL_CLEAR clear;
	VAEG_GL_CLEAR_COLOR clear_color;
	VAEG_GL_COMPILE_SHADER compile_shader;
	VAEG_GL_CREATE_PROGRAM create_program;
	VAEG_GL_CREATE_SHADER create_shader;
	VAEG_GL_DELETE_PROGRAM delete_program;
	VAEG_GL_DELETE_SHADER delete_shader;
	VAEG_GL_DELETE_TEXTURES delete_textures;
	VAEG_GL_DELETE_VERTEX_ARRAYS delete_vertex_arrays;
	VAEG_GL_DISABLE disable;
	VAEG_GL_DRAW_ARRAYS draw_arrays;
	VAEG_GL_ENABLE enable;
	VAEG_GL_GEN_TEXTURES gen_textures;
	VAEG_GL_GEN_VERTEX_ARRAYS gen_vertex_arrays;
	VAEG_GL_GET_FLOATV get_floatv;
	VAEG_GL_GET_INTEGERV get_integerv;
	VAEG_GL_GET_PROGRAM_INFO_LOG get_program_info_log;
	VAEG_GL_GET_PROGRAM_IV get_program_iv;
	VAEG_GL_GET_SHADER_INFO_LOG get_shader_info_log;
	VAEG_GL_GET_SHADER_IV get_shader_iv;
	VAEG_GL_GET_UNIFORM_LOCATION get_uniform_location;
	VAEG_GL_IS_ENABLED is_enabled;
	VAEG_GL_LINK_PROGRAM link_program;
	VAEG_GL_PIXEL_STOREI pixel_storei;
	VAEG_GL_SHADER_SOURCE shader_source;
	VAEG_GL_TEX_IMAGE_2D tex_image_2d;
	VAEG_GL_TEX_PARAMETERI tex_parameteri;
	VAEG_GL_TEX_SUB_IMAGE_2D tex_sub_image_2d;
	VAEG_GL_UNIFORM_1I uniform_1i;
	VAEG_GL_USE_PROGRAM use_program;
	VAEG_GL_VIEWPORT viewport;
};

struct VAEG_GL_STATE {
	SDL_Window *window;
	SDL_GLContext context;
	VAEG_GL_FUNCTIONS gl;
	GLuint source_texture;
	GLuint output_texture;
	GLuint program;
	GLuint vertex_array;
	GLint sampler_location;
	uint8_t *upload_buffer;
	size_t upload_capacity;
	uint32_t source_width;
	uint32_t source_height;
	uint32_t upload_pitch;
	uint32_t output_width;
	uint32_t output_height;
	uint32_t drawable_width;
	uint32_t drawable_height;
	libra_instance_t librashader;
	libra_gl_filter_chain_t filter_chain;
	bool filter_enabled;
	bool filter_first_frame;
};

struct VAEG_GL_SAVED_STATE {
	GLint active_texture;
	GLint texture;
	GLint framebuffer;
	GLint program;
	GLint vertex_array;
	GLint unpack_alignment;
	GLint viewport[4];
	GLfloat clear_color[4];
	GLboolean blend;
	GLboolean cull_face;
	GLboolean depth_test;
	GLboolean scissor_test;
};

template <typename T>
static T vaeg_gl_load(const char *name) {
	return reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
}

static int vaeg_gl_load_functions(VAEG_GL_STATE *state) {
#define VAEG_GL_LOAD(member, name) state->gl.member = vaeg_gl_load<decltype(state->gl.member)>(name)
	VAEG_GL_LOAD(active_texture, "glActiveTexture");
	VAEG_GL_LOAD(attach_shader, "glAttachShader");
	VAEG_GL_LOAD(bind_framebuffer, "glBindFramebuffer");
	VAEG_GL_LOAD(bind_texture, "glBindTexture");
	VAEG_GL_LOAD(bind_vertex_array, "glBindVertexArray");
	VAEG_GL_LOAD(clear, "glClear");
	VAEG_GL_LOAD(clear_color, "glClearColor");
	VAEG_GL_LOAD(compile_shader, "glCompileShader");
	VAEG_GL_LOAD(create_program, "glCreateProgram");
	VAEG_GL_LOAD(create_shader, "glCreateShader");
	VAEG_GL_LOAD(delete_program, "glDeleteProgram");
	VAEG_GL_LOAD(delete_shader, "glDeleteShader");
	VAEG_GL_LOAD(delete_textures, "glDeleteTextures");
	VAEG_GL_LOAD(delete_vertex_arrays, "glDeleteVertexArrays");
	VAEG_GL_LOAD(disable, "glDisable");
	VAEG_GL_LOAD(draw_arrays, "glDrawArrays");
	VAEG_GL_LOAD(enable, "glEnable");
	VAEG_GL_LOAD(gen_textures, "glGenTextures");
	VAEG_GL_LOAD(gen_vertex_arrays, "glGenVertexArrays");
	VAEG_GL_LOAD(get_floatv, "glGetFloatv");
	VAEG_GL_LOAD(get_integerv, "glGetIntegerv");
	VAEG_GL_LOAD(get_program_info_log, "glGetProgramInfoLog");
	VAEG_GL_LOAD(get_program_iv, "glGetProgramiv");
	VAEG_GL_LOAD(get_shader_info_log, "glGetShaderInfoLog");
	VAEG_GL_LOAD(get_shader_iv, "glGetShaderiv");
	VAEG_GL_LOAD(get_uniform_location, "glGetUniformLocation");
	VAEG_GL_LOAD(is_enabled, "glIsEnabled");
	VAEG_GL_LOAD(link_program, "glLinkProgram");
	VAEG_GL_LOAD(pixel_storei, "glPixelStorei");
	VAEG_GL_LOAD(shader_source, "glShaderSource");
	VAEG_GL_LOAD(tex_image_2d, "glTexImage2D");
	VAEG_GL_LOAD(tex_parameteri, "glTexParameteri");
	VAEG_GL_LOAD(tex_sub_image_2d, "glTexSubImage2D");
	VAEG_GL_LOAD(uniform_1i, "glUniform1i");
	VAEG_GL_LOAD(use_program, "glUseProgram");
	VAEG_GL_LOAD(viewport, "glViewport");
#undef VAEG_GL_LOAD
	return (state->gl.active_texture != nullptr) && (state->gl.attach_shader != nullptr) &&
	       (state->gl.bind_framebuffer != nullptr) && (state->gl.bind_texture != nullptr) &&
	       (state->gl.bind_vertex_array != nullptr) && (state->gl.clear != nullptr) &&
	       (state->gl.clear_color != nullptr) && (state->gl.compile_shader != nullptr) &&
	       (state->gl.create_program != nullptr) && (state->gl.create_shader != nullptr) &&
	       (state->gl.delete_program != nullptr) && (state->gl.delete_shader != nullptr) &&
	       (state->gl.delete_textures != nullptr) && (state->gl.delete_vertex_arrays != nullptr) &&
	       (state->gl.disable != nullptr) && (state->gl.draw_arrays != nullptr) &&
	       (state->gl.enable != nullptr) && (state->gl.gen_textures != nullptr) &&
	       (state->gl.gen_vertex_arrays != nullptr) && (state->gl.get_floatv != nullptr) &&
	       (state->gl.get_integerv != nullptr) && (state->gl.get_program_info_log != nullptr) &&
	       (state->gl.get_program_iv != nullptr) && (state->gl.get_shader_info_log != nullptr) &&
	       (state->gl.get_shader_iv != nullptr) && (state->gl.get_uniform_location != nullptr) &&
	       (state->gl.is_enabled != nullptr) && (state->gl.link_program != nullptr) &&
	       (state->gl.pixel_storei != nullptr) && (state->gl.shader_source != nullptr) &&
	       (state->gl.tex_image_2d != nullptr) && (state->gl.tex_parameteri != nullptr) &&
	       (state->gl.tex_sub_image_2d != nullptr) && (state->gl.uniform_1i != nullptr) &&
	       (state->gl.use_program != nullptr) && (state->gl.viewport != nullptr);
}

static const void *vaeg_gl_librashader_loader(const char *name) {
	return SDL_GL_GetProcAddress(name);
}

static void vaeg_gl_report_librashader_error(VAEG_GL_STATE *state, libra_error_t error,
                                             const char *operation) {
	if (error == nullptr) {
		return;
	}
	fprintf(stderr, "librashader OpenGL %s failed\n", operation);
	if (state->librashader.error_print != nullptr) {
		(void)state->librashader.error_print(error);
	}
	if (state->librashader.error_free != nullptr) {
		(void)state->librashader.error_free(&error);
	}
}

static int vaeg_gl_create_filter_chain(VAEG_GL_STATE *state, const char *preset_path) {
	libra_shader_preset_t preset;
	filter_chain_gl_opt_t options{};
	libra_error_t error;

	if ((state == nullptr) || (preset_path == nullptr) || (preset_path[0] == '\0')) {
		return 0;
	}
	state->librashader = librashader_load_instance();
	if (!state->librashader.instance_loaded ||
	    (state->librashader.gl_filter_chain_create == nullptr)) {
		fprintf(stderr, "librashader OpenGL runtime unavailable\n");
		return 0;
	}
	preset = nullptr;
	error = state->librashader.preset_create(preset_path, &preset);
	if ((error != nullptr) || (preset == nullptr)) {
		vaeg_gl_report_librashader_error(state, error, "preset creation");
		return 0;
	}
	options.version = LIBRASHADER_CURRENT_VERSION;
	options.glsl_version = 330;
	options.use_dsa = false;
	options.force_no_mipmaps = true;
	options.disable_cache = true;
	error = state->librashader.gl_filter_chain_create(
		&preset, vaeg_gl_librashader_loader, &options, &state->filter_chain);
	if ((error != nullptr) || (state->filter_chain == nullptr)) {
		vaeg_gl_report_librashader_error(state, error, "filter-chain creation");
		return 0;
	}
	state->filter_enabled = true;
	state->filter_first_frame = true;
	return 1;
}

static const char vaeg_gl_shader_source[] =
	"#version 330 core\n"
	"out vec2 vaeg_texcoord;\n"
	"void main() {\n"
	"  const vec2 positions[4] = vec2[4](vec2(-1.0, -1.0), vec2(1.0, -1.0),\n"
	"    vec2(-1.0, 1.0), vec2(1.0, 1.0));\n"
	"  const vec2 texcoords[4] = vec2[4](vec2(0.0, 1.0), vec2(1.0, 1.0),\n"
	"    vec2(0.0, 0.0), vec2(1.0, 0.0));\n"
	"  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);\n"
	"  vaeg_texcoord = texcoords[gl_VertexID];\n"
	"}\n"
	"#ifdef VAEG_FRAGMENT_SHADER\n"
	"uniform sampler2D vaeg_source;\n"
	"out vec4 vaeg_color;\n"
	"void main() { vaeg_color = texture(vaeg_source, vaeg_texcoord); }\n"
	"#endif\n";

static GLuint vaeg_gl_compile_shader(VAEG_GL_STATE *state, GLenum type, const char *source) {
	GLuint shader = state->gl.create_shader(type);
	GLint status = GL_FALSE;
	GLint length = 0;
	GLchar log[1024];
	const GLchar *source_pointer = source;

	if (shader == 0) {
		return 0;
	}
	state->gl.shader_source(shader, 1, &source_pointer, nullptr);
	state->gl.compile_shader(shader);
	state->gl.get_shader_iv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_TRUE) {
		return shader;
	}
	state->gl.get_shader_info_log(shader, sizeof(log), &length, log);
	log[(length >= (GLint)sizeof(log)) ? sizeof(log) - 1 : length] = '\0';
	fprintf(stderr, "VAEG OpenGL shader compilation failed: %s\n", log);
	state->gl.delete_shader(shader);
	return 0;
}

static int vaeg_gl_create_program(VAEG_GL_STATE *state) {
	const char *fragment_source =
		"#version 330 core\n"
		"in vec2 vaeg_texcoord;\n"
		"uniform sampler2D vaeg_source;\n"
		"out vec4 vaeg_color;\n"
		"void main() { vaeg_color = texture(vaeg_source, vaeg_texcoord); }\n";
	GLuint vertex_shader = vaeg_gl_compile_shader(state, GL_VERTEX_SHADER, vaeg_gl_shader_source);
	GLuint fragment_shader = vaeg_gl_compile_shader(state, GL_FRAGMENT_SHADER, fragment_source);
	GLint status = GL_FALSE;
	GLint length = 0;
	GLchar log[1024];

	if ((vertex_shader == 0) || (fragment_shader == 0)) {
		if (vertex_shader != 0) {
			state->gl.delete_shader(vertex_shader);
		}
		if (fragment_shader != 0) {
			state->gl.delete_shader(fragment_shader);
		}
		return 0;
	}
	state->program = state->gl.create_program();
	if (state->program == 0) {
		state->gl.delete_shader(vertex_shader);
		state->gl.delete_shader(fragment_shader);
		return 0;
	}
	state->gl.attach_shader(state->program, vertex_shader);
	state->gl.attach_shader(state->program, fragment_shader);
	state->gl.link_program(state->program);
	state->gl.delete_shader(vertex_shader);
	state->gl.delete_shader(fragment_shader);
	state->gl.get_program_iv(state->program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		state->gl.get_program_info_log(state->program, sizeof(log), &length, log);
		log[(length >= (GLint)sizeof(log)) ? sizeof(log) - 1 : length] = '\0';
		fprintf(stderr, "VAEG OpenGL program link failed: %s\n", log);
		state->gl.delete_program(state->program);
		state->program = 0;
		return 0;
	}
	state->sampler_location = state->gl.get_uniform_location(state->program, "vaeg_source");
	return (state->sampler_location >= 0) ? 1 : 0;
}

static int vaeg_gl_ensure_source(VAEG_GL_STATE *state, uint32_t width, uint32_t height) {
	const size_t required_capacity = static_cast<size_t>(width) * height * 4U;

	if ((width == 0) || (height == 0) ||
	    (required_capacity / 4U != static_cast<size_t>(width) * height)) {
		return 0;
	}
	if ((state->source_texture != 0) && (state->source_width == width) &&
	    (state->source_height == height) && (state->upload_capacity >= required_capacity)) {
		return 1;
	}
	if (required_capacity > state->upload_capacity) {
		uint8_t *new_buffer = static_cast<uint8_t *>(realloc(state->upload_buffer, required_capacity));
		if (new_buffer == nullptr) {
			return 0;
		}
		state->upload_buffer = new_buffer;
		state->upload_capacity = required_capacity;
	}
	if (state->source_texture == 0) {
		state->gl.gen_textures(1, &state->source_texture);
	}
	if (state->source_texture == 0) {
		return 0;
	}
	state->gl.active_texture(GL_TEXTURE0);
	state->gl.bind_texture(GL_TEXTURE_2D, state->source_texture);
	state->gl.tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	state->gl.tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	state->gl.tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	state->gl.tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	state->gl.pixel_storei(GL_UNPACK_ALIGNMENT, 1);
	state->gl.tex_image_2d(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
	                       GL_UNSIGNED_BYTE, nullptr);
	state->source_width = width;
	state->source_height = height;
	state->upload_pitch = width * 4U;
	return 1;
}

static int vaeg_gl_ensure_output(VAEG_GL_STATE *state, uint32_t width, uint32_t height) {
	if ((width == 0) || (height == 0)) {
		return 0;
	}
	if ((state->output_texture != 0) && (state->output_width == width) &&
	    (state->output_height == height)) {
		return 1;
	}
	if (state->output_texture == 0) {
		state->gl.gen_textures(1, &state->output_texture);
	}
	if (state->output_texture == 0) {
		return 0;
	}
	state->gl.active_texture(GL_TEXTURE0);
	state->gl.bind_texture(GL_TEXTURE_2D, state->output_texture);
	state->gl.tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	state->gl.tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	state->gl.tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	state->gl.tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	state->gl.pixel_storei(GL_UNPACK_ALIGNMENT, 1);
	state->gl.tex_image_2d(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
	                       GL_UNSIGNED_BYTE, nullptr);
	state->output_width = width;
	state->output_height = height;
	return 1;
}

static void vaeg_gl_save_state(VAEG_GL_STATE *state, VAEG_GL_SAVED_STATE *saved) {
	state->gl.get_integerv(GL_ACTIVE_TEXTURE, &saved->active_texture);
	state->gl.active_texture(GL_TEXTURE0);
	state->gl.get_integerv(GL_TEXTURE_BINDING_2D, &saved->texture);
	state->gl.active_texture(static_cast<GLenum>(saved->active_texture));
	state->gl.get_integerv(GL_FRAMEBUFFER_BINDING, &saved->framebuffer);
	state->gl.get_integerv(GL_CURRENT_PROGRAM, &saved->program);
	state->gl.get_integerv(GL_VERTEX_ARRAY_BINDING, &saved->vertex_array);
	state->gl.get_integerv(GL_UNPACK_ALIGNMENT, &saved->unpack_alignment);
	state->gl.get_integerv(GL_VIEWPORT, saved->viewport);
	state->gl.get_floatv(GL_COLOR_CLEAR_VALUE, saved->clear_color);
	saved->blend = state->gl.is_enabled(GL_BLEND);
	saved->cull_face = state->gl.is_enabled(GL_CULL_FACE);
	saved->depth_test = state->gl.is_enabled(GL_DEPTH_TEST);
	saved->scissor_test = state->gl.is_enabled(GL_SCISSOR_TEST);
}

static void vaeg_gl_restore_state(VAEG_GL_STATE *state, const VAEG_GL_SAVED_STATE *saved) {
	state->gl.bind_framebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(saved->framebuffer));
	state->gl.viewport(saved->viewport[0], saved->viewport[1], saved->viewport[2],
	                  saved->viewport[3]);
	state->gl.clear_color(saved->clear_color[0], saved->clear_color[1], saved->clear_color[2],
	                     saved->clear_color[3]);
	state->gl.pixel_storei(GL_UNPACK_ALIGNMENT, saved->unpack_alignment);
	state->gl.use_program(static_cast<GLuint>(saved->program));
	state->gl.bind_vertex_array(static_cast<GLuint>(saved->vertex_array));
	state->gl.active_texture(GL_TEXTURE0);
	state->gl.bind_texture(GL_TEXTURE_2D, static_cast<GLuint>(saved->texture));
	state->gl.active_texture(static_cast<GLenum>(saved->active_texture));
	(saved->blend ? state->gl.enable : state->gl.disable)(GL_BLEND);
	(saved->cull_face ? state->gl.enable : state->gl.disable)(GL_CULL_FACE);
	(saved->depth_test ? state->gl.enable : state->gl.disable)(GL_DEPTH_TEST);
	(saved->scissor_test ? state->gl.enable : state->gl.disable)(GL_SCISSOR_TEST);
}

static void vaeg_gl_release_state(VAEG_GL_STATE *state) {
	if (state == nullptr) {
		return;
	}
	if (state->context != nullptr) {
		(void)SDL_GL_MakeCurrent(state->window, state->context);
		if ((state->filter_chain != nullptr) &&
		    (state->librashader.gl_filter_chain_free != nullptr)) {
			(void)state->librashader.gl_filter_chain_free(&state->filter_chain);
		}
		if ((state->source_texture != 0) && (state->gl.delete_textures != nullptr)) {
			state->gl.delete_textures(1, &state->source_texture);
		}
		if ((state->output_texture != 0) && (state->gl.delete_textures != nullptr)) {
			state->gl.delete_textures(1, &state->output_texture);
		}
		if ((state->vertex_array != 0) && (state->gl.delete_vertex_arrays != nullptr)) {
			state->gl.delete_vertex_arrays(1, &state->vertex_array);
		}
		if ((state->program != 0) && (state->gl.delete_program != nullptr)) {
			state->gl.delete_program(state->program);
		}
		SDL_GL_DeleteContext(state->context);
	}
	free(state->upload_buffer);
	free(state);
}

extern "C" int vaeg_gl_bridge_initialize(void *host_window, const char *preset_path,
                                           int enable_filter, VAEG_GL_BRIDGE *bridge) {
	VAEG_GL_STATE *state;

	if ((host_window == nullptr) || (bridge == nullptr)) {
		return 0;
	}
	bridge->state = nullptr;
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	state = static_cast<VAEG_GL_STATE *>(calloc(1, sizeof(*state)));
	if (state == nullptr) {
		return 0;
	}
	state->window = static_cast<SDL_Window *>(host_window);
	state->context = SDL_GL_CreateContext(state->window);
	if ((state->context == nullptr) || (SDL_GL_MakeCurrent(state->window, state->context) != 0) ||
	    !vaeg_gl_load_functions(state) || !vaeg_gl_create_program(state)) {
		vaeg_gl_release_state(state);
		return 0;
	}
	state->gl.gen_vertex_arrays(1, &state->vertex_array);
	if (state->vertex_array == 0) {
		vaeg_gl_release_state(state);
		return 0;
	}
	state->gl.bind_vertex_array(state->vertex_array);
	if ((enable_filter != 0) && !vaeg_gl_create_filter_chain(state, preset_path)) {
		vaeg_gl_release_state(state);
		return 0;
	}
	bridge->state = state;
	return 1;
}

extern "C" VAEG_GL_BRIDGE_RESULT vaeg_gl_bridge_set_drawable_size(
	VAEG_GL_BRIDGE *bridge, uint32_t width, uint32_t height) {
	VAEG_GL_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return VAEG_GL_BRIDGE_INVALID_ARGUMENT;
	}
	if ((width == 0) || (height == 0)) {
		return VAEG_GL_BRIDGE_NO_OUTPUT;
	}
	state = static_cast<VAEG_GL_STATE *>(bridge->state);
	state->drawable_width = width;
	state->drawable_height = height;
	return VAEG_GL_BRIDGE_OK;
}

extern "C" VAEG_GL_BRIDGE_RESULT vaeg_gl_bridge_set_filter_enabled(
	VAEG_GL_BRIDGE *bridge, int enabled) {
	VAEG_GL_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return VAEG_GL_BRIDGE_INVALID_ARGUMENT;
	}
	state = static_cast<VAEG_GL_STATE *>(bridge->state);
	if ((enabled != 0) && (state->filter_chain == nullptr)) {
		return VAEG_GL_BRIDGE_RESOURCE_FAILURE;
	}
	if ((enabled != 0) && !state->filter_enabled) {
		state->filter_first_frame = true;
	}
	state->filter_enabled = (enabled != 0);
	return VAEG_GL_BRIDGE_OK;
}

extern "C" VAEG_GL_BRIDGE_RESULT vaeg_gl_bridge_set_filter_parameter(
	VAEG_GL_BRIDGE *bridge, const char *name, float value) {
	VAEG_GL_STATE *state;
	libra_error_t error;

	if ((bridge == nullptr) || (bridge->state == nullptr) || (name == nullptr)) {
		return VAEG_GL_BRIDGE_INVALID_ARGUMENT;
	}
	state = static_cast<VAEG_GL_STATE *>(bridge->state);
	if ((state->filter_chain == nullptr) || (state->librashader.gl_filter_chain_set_param == nullptr)) {
		return VAEG_GL_BRIDGE_RESOURCE_FAILURE;
	}
	error = state->librashader.gl_filter_chain_set_param(&state->filter_chain, name, value);
	if (error != nullptr) {
		vaeg_gl_report_librashader_error(state, error, "parameter update");
		return VAEG_GL_BRIDGE_RESOURCE_FAILURE;
	}
	state->filter_first_frame = true;
	return VAEG_GL_BRIDGE_OK;
}

extern "C" VAEG_GL_BRIDGE_RESULT vaeg_gl_bridge_present(VAEG_GL_BRIDGE *bridge,
                                                           const VAEG_FRAME_INPUT *frame) {
	VAEG_GL_STATE *state;
	VAEG_GL_SAVED_STATE saved;
	int drawable_width;
	int drawable_height;
	double output_aspect;
	double source_aspect;
	GLint viewport_x;
	GLint viewport_y;
	GLsizei viewport_width;
	GLsizei viewport_height;

	if ((bridge == nullptr) || (bridge->state == nullptr) || (frame == nullptr)) {
		return VAEG_GL_BRIDGE_INVALID_ARGUMENT;
	}
	if (vaeg_frame_input_validate(frame) != VAEG_FRAME_INPUT_OK) {
		return VAEG_GL_BRIDGE_INVALID_FRAME;
	}
	state = static_cast<VAEG_GL_STATE *>(bridge->state);
	SDL_GL_GetDrawableSize(state->window, &drawable_width, &drawable_height);
	if ((drawable_width <= 0) || (drawable_height <= 0)) {
		return VAEG_GL_BRIDGE_NO_OUTPUT;
	}
	state->drawable_width = static_cast<uint32_t>(drawable_width);
	state->drawable_height = static_cast<uint32_t>(drawable_height);
	vaeg_gl_save_state(state, &saved);
	if (!vaeg_gl_ensure_source(state, frame->width, frame->height) ||
	    (vaeg_frame_convert_rgba8888(frame, state->upload_buffer, state->upload_pitch,
	                                  state->upload_capacity) != VAEG_FRAME_CONVERSION_OK)) {
		vaeg_gl_restore_state(state, &saved);
		return VAEG_GL_BRIDGE_RESOURCE_FAILURE;
	}
	state->gl.active_texture(GL_TEXTURE0);
	state->gl.bind_texture(GL_TEXTURE_2D, state->source_texture);
	state->gl.pixel_storei(GL_UNPACK_ALIGNMENT, 1);
	state->gl.tex_sub_image_2d(GL_TEXTURE_2D, 0, 0, 0, frame->width, frame->height, GL_RGBA,
	                           GL_UNSIGNED_BYTE, state->upload_buffer);
	state->gl.bind_framebuffer(GL_FRAMEBUFFER, 0);
	state->gl.clear_color(0.0f, 0.0f, 0.0f, 1.0f);
	state->gl.clear(GL_COLOR_BUFFER_BIT);
	output_aspect = static_cast<double>(drawable_width) / static_cast<double>(drawable_height);
	source_aspect = static_cast<double>(frame->source_aspect_width) /
	                static_cast<double>(frame->source_aspect_height);
	viewport_x = 0;
	viewport_y = 0;
	viewport_width = drawable_width;
	viewport_height = drawable_height;
	if (source_aspect > output_aspect) {
		viewport_height = static_cast<GLsizei>(static_cast<double>(viewport_width) / source_aspect);
		viewport_y = (drawable_height - viewport_height) / 2;
	} else {
		viewport_width = static_cast<GLsizei>(static_cast<double>(viewport_height) * source_aspect);
		viewport_x = (drawable_width - viewport_width) / 2;
	}
	state->gl.viewport(viewport_x, viewport_y, viewport_width, viewport_height);
	state->gl.disable(GL_BLEND);
	state->gl.disable(GL_CULL_FACE);
	state->gl.disable(GL_DEPTH_TEST);
	state->gl.disable(GL_SCISSOR_TEST);
	if (state->filter_enabled) {
		libra_image_gl_t source_image;
		libra_image_gl_t output_image;
		libra_viewport_t libra_viewport;
		frame_gl_opt_t filter_options{};
		libra_error_t error;

		if (!vaeg_gl_ensure_output(state, state->drawable_width, state->drawable_height)) {
			vaeg_gl_restore_state(state, &saved);
			return VAEG_GL_BRIDGE_RESOURCE_FAILURE;
		}
		source_image.handle = state->source_texture;
		source_image.format = GL_RGBA8;
		source_image.width = state->source_width;
		source_image.height = state->source_height;
		output_image.handle = state->output_texture;
		output_image.format = GL_RGBA8;
		output_image.width = state->drawable_width;
		output_image.height = state->drawable_height;
		libra_viewport.x = static_cast<float>(viewport_x);
		libra_viewport.y = static_cast<float>(viewport_y);
		libra_viewport.width = static_cast<uint32_t>(viewport_width);
		libra_viewport.height = static_cast<uint32_t>(viewport_height);
		filter_options.version = LIBRASHADER_CURRENT_VERSION;
		filter_options.clear_history = state->filter_first_frame;
		filter_options.frame_direction = 1;
		filter_options.rotation = 0;
		filter_options.total_subframes = 1;
		filter_options.current_subframe = 1;
		filter_options.aspect_ratio = static_cast<float>(frame->source_aspect_width) /
		                              static_cast<float>(frame->source_aspect_height);
		filter_options.frames_per_second =
			static_cast<float>(frame->source_frame_rate_numerator) /
			static_cast<float>(frame->source_frame_rate_denominator);
		filter_options.frametime_delta =
			static_cast<uint32_t>(frame->frame_time_delta_ns / 1000000U);
		error = state->librashader.gl_filter_chain_frame(
			&state->filter_chain, 1, source_image, output_image, &libra_viewport, nullptr,
			&filter_options);
		if (error != nullptr) {
			vaeg_gl_report_librashader_error(state, error, "frame rendering");
			vaeg_gl_restore_state(state, &saved);
			return VAEG_GL_BRIDGE_FILTER_FAILURE;
		}
		state->filter_first_frame = false;
		state->gl.bind_framebuffer(GL_FRAMEBUFFER, 0);
		state->gl.viewport(viewport_x, viewport_y, viewport_width, viewport_height);
		state->gl.use_program(state->program);
		state->gl.bind_vertex_array(state->vertex_array);
		state->gl.active_texture(GL_TEXTURE0);
		state->gl.bind_texture(GL_TEXTURE_2D, state->output_texture);
		state->gl.uniform_1i(state->sampler_location, 0);
		state->gl.draw_arrays(GL_TRIANGLE_STRIP, 0, 4);
	} else {
		state->gl.use_program(state->program);
		state->gl.bind_vertex_array(state->vertex_array);
		state->gl.uniform_1i(state->sampler_location, 0);
		state->gl.draw_arrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	vaeg_gl_restore_state(state, &saved);
	SDL_GL_SwapWindow(state->window);
	return VAEG_GL_BRIDGE_OK;
}

extern "C" void vaeg_gl_bridge_shutdown(VAEG_GL_BRIDGE *bridge) {
	VAEG_GL_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return;
	}
	state = static_cast<VAEG_GL_STATE *>(bridge->state);
	bridge->state = nullptr;
	vaeg_gl_release_state(state);
}
