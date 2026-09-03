/*
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "librashader_gl_bridge.h"

#include <cstdio>
#include <cstring>
#include <new>
#include <algorithm>
#include <vector>

#ifndef APIENTRY
#define APIENTRY
#endif

struct GlApi {
    using ActiveTexture = void(APIENTRY *)(GLenum);
    using AttachShader = void(APIENTRY *)(GLuint, GLuint);
    using BindBuffer = void(APIENTRY *)(GLenum, GLuint);
    using BindFramebuffer = void(APIENTRY *)(GLenum, GLuint);
    using BindVertexArray = void(APIENTRY *)(GLuint);
    using BufferData = void(APIENTRY *)(GLenum, GLsizeiptr, const void *, GLenum);
    using CompileShader = void(APIENTRY *)(GLuint);
    using CreateProgram = GLuint(APIENTRY *)();
    using CreateShader = GLuint(APIENTRY *)(GLenum);
    using DeleteBuffers = void(APIENTRY *)(GLsizei, const GLuint *);
    using DeleteProgram = void(APIENTRY *)(GLuint);
    using DeleteShader = void(APIENTRY *)(GLuint);
    using DeleteVertexArrays = void(APIENTRY *)(GLsizei, const GLuint *);
    using DisableVertexAttribArray = void(APIENTRY *)(GLuint);
    using DrawArrays = void(APIENTRY *)(GLenum, GLint, GLsizei);
    using EnableVertexAttribArray = void(APIENTRY *)(GLuint);
    using GenBuffers = void(APIENTRY *)(GLsizei, GLuint *);
    using GenVertexArrays = void(APIENTRY *)(GLsizei, GLuint *);
    using GetProgramInfoLog = void(APIENTRY *)(GLuint, GLsizei, GLsizei *, GLchar *);
    using GetProgramiv = void(APIENTRY *)(GLuint, GLenum, GLint *);
    using GetShaderInfoLog = void(APIENTRY *)(GLuint, GLsizei, GLsizei *, GLchar *);
    using GetShaderiv = void(APIENTRY *)(GLuint, GLenum, GLint *);
    using GetUniformLocation = GLint(APIENTRY *)(GLuint, const GLchar *);
    using LinkProgram = void(APIENTRY *)(GLuint);
    using ShaderSource = void(APIENTRY *)(GLuint, GLsizei, const GLchar *const *,
                                          const GLint *);
    using Uniform1i = void(APIENTRY *)(GLint, GLint);
    using UseProgram = void(APIENTRY *)(GLuint);
    using VertexAttribPointer = void(APIENTRY *)(GLuint, GLint, GLenum, GLboolean,
                                                 GLsizei, const void *);

    ActiveTexture active_texture;
    AttachShader attach_shader;
    BindBuffer bind_buffer;
    BindFramebuffer bind_framebuffer;
    BindVertexArray bind_vertex_array;
    BufferData buffer_data;
    CompileShader compile_shader;
    CreateProgram create_program;
    CreateShader create_shader;
    DeleteBuffers delete_buffers;
    DeleteProgram delete_program;
    DeleteShader delete_shader;
    DeleteVertexArrays delete_vertex_arrays;
    DisableVertexAttribArray disable_vertex_attrib_array;
    DrawArrays draw_arrays;
    EnableVertexAttribArray enable_vertex_attrib_array;
    GenBuffers gen_buffers;
    GenVertexArrays gen_vertex_arrays;
    GetProgramInfoLog get_program_info_log;
    GetProgramiv get_program_iv;
    GetShaderInfoLog get_shader_info_log;
    GetShaderiv get_shader_iv;
    GetUniformLocation get_uniform_location;
    LinkProgram link_program;
    ShaderSource shader_source;
    Uniform1i uniform_1i;
    UseProgram use_program;
    VertexAttribPointer vertex_attrib_pointer;
    GLuint output_program = 0;
    GLuint output_vao = 0;
    GLuint output_vbo = 0;
};

namespace {

constexpr GLenum kSourceInternalFormat = GL_RGBA8;
constexpr GLenum kFramebuffer = 0x8d40;

template <typename T> T load_proc(const char *name) {
    return reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
}

bool load_gl_api(GlApi *api) {
#define LOAD_GL(field, type, symbol) \
    api->field = load_proc<GlApi::type>(symbol)
    LOAD_GL(active_texture, ActiveTexture, "glActiveTexture");
    LOAD_GL(attach_shader, AttachShader, "glAttachShader");
    LOAD_GL(bind_buffer, BindBuffer, "glBindBuffer");
    LOAD_GL(bind_framebuffer, BindFramebuffer, "glBindFramebuffer");
    LOAD_GL(bind_vertex_array, BindVertexArray, "glBindVertexArray");
    LOAD_GL(buffer_data, BufferData, "glBufferData");
    LOAD_GL(compile_shader, CompileShader, "glCompileShader");
    LOAD_GL(create_program, CreateProgram, "glCreateProgram");
    LOAD_GL(create_shader, CreateShader, "glCreateShader");
    LOAD_GL(delete_buffers, DeleteBuffers, "glDeleteBuffers");
    LOAD_GL(delete_program, DeleteProgram, "glDeleteProgram");
    LOAD_GL(delete_shader, DeleteShader, "glDeleteShader");
    LOAD_GL(delete_vertex_arrays, DeleteVertexArrays, "glDeleteVertexArrays");
    LOAD_GL(disable_vertex_attrib_array, DisableVertexAttribArray,
            "glDisableVertexAttribArray");
    LOAD_GL(draw_arrays, DrawArrays, "glDrawArrays");
    LOAD_GL(enable_vertex_attrib_array, EnableVertexAttribArray,
            "glEnableVertexAttribArray");
    LOAD_GL(gen_buffers, GenBuffers, "glGenBuffers");
    LOAD_GL(gen_vertex_arrays, GenVertexArrays, "glGenVertexArrays");
    LOAD_GL(get_program_info_log, GetProgramInfoLog, "glGetProgramInfoLog");
    LOAD_GL(get_program_iv, GetProgramiv, "glGetProgramiv");
    LOAD_GL(get_shader_info_log, GetShaderInfoLog, "glGetShaderInfoLog");
    LOAD_GL(get_shader_iv, GetShaderiv, "glGetShaderiv");
    LOAD_GL(get_uniform_location, GetUniformLocation, "glGetUniformLocation");
    LOAD_GL(link_program, LinkProgram, "glLinkProgram");
    LOAD_GL(shader_source, ShaderSource, "glShaderSource");
    LOAD_GL(uniform_1i, Uniform1i, "glUniform1i");
    LOAD_GL(use_program, UseProgram, "glUseProgram");
    LOAD_GL(vertex_attrib_pointer, VertexAttribPointer, "glVertexAttribPointer");
#undef LOAD_GL
    return api->active_texture && api->attach_shader && api->bind_buffer &&
           api->bind_framebuffer && api->bind_vertex_array && api->buffer_data &&
           api->compile_shader && api->create_program && api->create_shader &&
           api->delete_buffers && api->delete_program && api->delete_shader &&
           api->delete_vertex_arrays && api->disable_vertex_attrib_array &&
           api->draw_arrays && api->enable_vertex_attrib_array && api->gen_buffers &&
           api->gen_vertex_arrays && api->get_program_info_log && api->get_program_iv &&
           api->get_shader_info_log && api->get_shader_iv && api->link_program &&
           api->get_uniform_location &&
           api->shader_source && api->uniform_1i && api->use_program &&
           api->vertex_attrib_pointer;
}

void set_texture_parameters() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

bool gl_ok() { return glGetError() == GL_NO_ERROR; }

GLuint compile_shader(GlApi *api, GLenum type, const char *source) {
    const GLuint shader = api->create_shader(type);
    if (shader == 0) {
        return 0;
    }
    api->shader_source(shader, 1, &source, nullptr);
    api->compile_shader(shader);
    GLint compiled = 0;
    api->get_shader_iv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint length = 0;
        api->get_shader_iv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<GLchar> log(static_cast<size_t>((length > 0) ? length : 1));
        api->get_shader_info_log(shader, static_cast<GLsizei>(log.size()), nullptr,
                                 log.data());
        std::fprintf(stderr, "librashader: output shader compile failed: %s\n",
                     log.data());
        api->delete_shader(shader);
        return 0;
    }
    return shader;
}

bool create_output_program(GlApi *api) {
    static const char vertex_source[] =
        "#version 330 core\n"
        "layout(location = 0) in vec2 Position;\n"
        "layout(location = 1) in vec2 TexCoord;\n"
        "out vec2 vTexCoord;\n"
        "void main() { gl_Position = vec4(Position, 0.0, 1.0); "
        "vTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y); }\n";
    static const char fragment_source[] =
        "#version 330 core\n"
        "in vec2 vTexCoord;\n"
        "uniform sampler2D Output;\n"
        "out vec4 FragColor;\n"
        "void main() { FragColor = texture(Output, vTexCoord); }\n";
    const GLuint vertex = compile_shader(api, GL_VERTEX_SHADER, vertex_source);
    const GLuint fragment = compile_shader(api, GL_FRAGMENT_SHADER, fragment_source);
    if ((vertex == 0) || (fragment == 0)) {
        if (vertex != 0) {
            api->delete_shader(vertex);
        }
        if (fragment != 0) {
            api->delete_shader(fragment);
        }
        return false;
    }

    api->output_program = api->create_program();
    if (api->output_program == 0) {
        api->delete_shader(vertex);
        api->delete_shader(fragment);
        return false;
    }
    api->attach_shader(api->output_program, vertex);
    api->attach_shader(api->output_program, fragment);
    api->link_program(api->output_program);
    api->delete_shader(vertex);
    api->delete_shader(fragment);
    GLint linked = 0;
    api->get_program_iv(api->output_program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint length = 0;
        api->get_program_iv(api->output_program, GL_INFO_LOG_LENGTH, &length);
        std::vector<GLchar> log(static_cast<size_t>((length > 0) ? length : 1));
        api->get_program_info_log(api->output_program, static_cast<GLsizei>(log.size()),
                                  nullptr, log.data());
        std::fprintf(stderr, "librashader: output shader link failed: %s\n", log.data());
        api->delete_program(api->output_program);
        api->output_program = 0;
        return false;
    }

    static const GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,  1.0f, 1.0f,  1.0f, 1.0f,
    };
    api->gen_vertex_arrays(1, &api->output_vao);
    api->gen_buffers(1, &api->output_vbo);
    if ((api->output_vao == 0) || (api->output_vbo == 0)) {
        return false;
    }
    api->bind_vertex_array(api->output_vao);
    api->bind_buffer(GL_ARRAY_BUFFER, api->output_vbo);
    api->buffer_data(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    api->enable_vertex_attrib_array(0);
    api->enable_vertex_attrib_array(1);
    api->vertex_attrib_pointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                               nullptr);
    api->vertex_attrib_pointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                               reinterpret_cast<const void *>(2 * sizeof(GLfloat)));
    api->bind_buffer(GL_ARRAY_BUFFER, 0);
    api->bind_vertex_array(0);
    return gl_ok();
}

void report_error(const libra_instance_t *instance, const char *operation,
                  libra_error_t error) {
    if ((instance == nullptr) || (error == nullptr)) {
        std::fprintf(stderr, "librashader: %s failed\n", operation);
        return;
    }
    std::fprintf(stderr, "librashader: %s failed (error=%d):\n", operation,
                 static_cast<int>(instance->error_errno(error)));
    (void)instance->error_print(error);
    (void)instance->error_free(&error);
}

} // namespace

LibrashaderGlBridge::LibrashaderGlBridge(SDL_Renderer *renderer)
    : initialized_(false), source_texture_(0), output_texture_(0), source_width_(0),
      source_height_(0), output_width_(0), output_height_(0), renderer_(renderer),
      loader_(), chain_(nullptr), chain_ready_(false), api_(nullptr) {
    (void)initialize_context();
}

bool LibrashaderGlBridge::initialize_context() {
    SDL_RendererInfo info{};
    if ((renderer_ == nullptr) || (SDL_GetRendererInfo(renderer_, &info) != 0) ||
        (info.name == nullptr) || (std::strcmp(info.name, "opengl") != 0) ||
        (SDL_GL_GetCurrentContext() == nullptr)) {
        return false;
    }

    api_ = new (std::nothrow) GlApi{};
    if ((api_ == nullptr) || !load_gl_api(api_)) {
        destroy_gl_objects();
        return false;
    }
    if (!create_output_program(api_)) {
        destroy_gl_objects();
        return false;
    }

    glGetError();
    glGenTextures(1, &source_texture_);
    glGenTextures(1, &output_texture_);
    if ((source_texture_ == 0) || (output_texture_ == 0) || !gl_ok()) {
        destroy_textures();
        destroy_gl_objects();
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, source_texture_);
    set_texture_parameters();
    glBindTexture(GL_TEXTURE_2D, output_texture_);
    set_texture_parameters();
    glBindTexture(GL_TEXTURE_2D, 0);
    initialized_ = gl_ok();
    if (!initialized_) {
        destroy_textures();
        destroy_gl_objects();
    }
    return initialized_;
}

LibrashaderGlBridge::~LibrashaderGlBridge() {
    disable_filter();
    destroy_textures();
    destroy_gl_objects();
    loader_.unload();
}

void LibrashaderGlBridge::destroy_gl_objects() {
    const bool context_available = SDL_GL_GetCurrentContext() != nullptr;

    if (api_ != nullptr) {
        if (context_available && (api_->output_vbo != 0)) {
            api_->delete_buffers(1, &api_->output_vbo);
        }
        if (context_available && (api_->output_vao != 0)) {
            api_->delete_vertex_arrays(1, &api_->output_vao);
        }
        if (context_available && (api_->output_program != 0)) {
            api_->delete_program(api_->output_program);
        }
        delete api_;
        api_ = nullptr;
    }
}

void LibrashaderGlBridge::destroy_textures() {
    if (SDL_GL_GetCurrentContext() == nullptr) {
        source_texture_ = 0;
        output_texture_ = 0;
        initialized_ = false;
        return;
    }
    if (source_texture_ != 0) {
        glDeleteTextures(1, &source_texture_);
        source_texture_ = 0;
    }
    if (output_texture_ != 0) {
        glDeleteTextures(1, &output_texture_);
        output_texture_ = 0;
    }
    initialized_ = false;
}

bool LibrashaderGlBridge::initialized() const { return initialized_; }

bool LibrashaderGlBridge::upload_source(const void *pixels, int pitch,
                                         uint32_t width, uint32_t height) {
    if (!initialized_ || (pixels == nullptr) || (pitch < 0) || (width == 0) ||
        (height == 0) || ((pitch & 1) != 0) ||
        (static_cast<uint32_t>(pitch / 2) < width)) {
        return false;
    }

    glGetError();
    glBindTexture(GL_TEXTURE_2D, source_texture_);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
    if ((source_width_ != width) || (source_height_ != height)) {
        glTexImage2D(GL_TEXTURE_2D, 0, kSourceInternalFormat,
                     static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                     0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
        source_width_ = width;
        source_height_ = height;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(width),
                        static_cast<GLsizei>(height), GL_RGB,
                        GL_UNSIGNED_SHORT_5_6_5, pixels);
    }
    // librashader 0.12.0 binds its source sampler with a mipmapped minification
    // mode even when the preset does not request mipmap generation. Keep the
    // externally-owned source texture complete so those samplers cannot return
    // an incomplete-texture black value.
    glGenerateMipmap(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return gl_ok();
}

bool LibrashaderGlBridge::ensure_output(uint32_t width, uint32_t height) {
    if (!initialized_ || (width == 0) || (height == 0)) {
        return false;
    }
    if ((output_width_ == width) && (output_height_ == height)) {
        return true;
    }

    glGetError();
    glBindTexture(GL_TEXTURE_2D, output_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, kSourceInternalFormat,
                 static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (!gl_ok()) {
        return false;
    }
    output_width_ = width;
    output_height_ = height;
    return true;
}

bool LibrashaderGlBridge::load_preset(const char *path) {
    if (!initialized_ || (path == nullptr) || (path[0] == '\0')) {
        return false;
    }
    disable_filter();
    if (!loader_.load()) {
        std::fprintf(stderr, "librashader: runtime unavailable or incompatible "
                             "(status=%d abi=%zu api=%zu)\n",
                     static_cast<int>(loader_.status()), loader_.abi_version(),
                     loader_.api_version());
        return false;
    }
    const libra_instance_t *instance = loader_.instance();
    libra_preset_opt_t preset_options{};
    preset_options.version = LIBRASHADER_CURRENT_VERSION;
    preset_options.original_aspect_uniforms = true;
    libra_shader_preset_t preset = nullptr;
    libra_error_t error = instance->preset_create_with_options(
        path, nullptr, &preset_options, &preset);
    if (error != nullptr) {
        report_error(instance, "preset creation", error);
        return false;
    }

    libra_preset_param_list_t parameter_list{};
    error = instance->preset_get_runtime_params(&preset, &parameter_list);
    if (error != nullptr) {
        report_error(instance, "preset parameter enumeration", error);
        (void)instance->preset_free(&preset);
        return false;
    }
    for (uint64_t i = 0; i < parameter_list.length; ++i) {
        const libra_preset_param_t &source = parameter_list.parameters[i];
        if ((source.name == nullptr) || (source.description == nullptr)) {
            continue;
        }
        parameters_.push_back({source.name, source.description, source.initial,
                               source.minimum, source.maximum, source.step,
                               source.initial});
    }
    (void)instance->preset_free_runtime_params(parameter_list);

    filter_chain_gl_opt_t chain_options{};
    chain_options.version = LIBRASHADER_CURRENT_VERSION;
    chain_options.glsl_version = 330;
    chain_options.use_dsa = false;
    chain_options.force_no_mipmaps = true;
    chain_options.disable_cache = false;
    error = instance->gl_filter_chain_create(
        &preset, reinterpret_cast<libra_gl_loader_t>(SDL_GL_GetProcAddress),
        &chain_options, &chain_);
    if (error != nullptr) {
        report_error(instance, "filter-chain creation", error);
        (void)instance->preset_free(&preset);
        chain_ = nullptr;
        return false;
    }
    chain_ready_ = (chain_ != nullptr);
    if (!chain_ready_) {
        std::fprintf(stderr, "librashader: filter-chain creation returned no chain\n");
    }
    return chain_ready_;
}

bool LibrashaderGlBridge::render_frame(uint32_t frame_count,
                                       uint32_t frame_time_ms, uint32_t width,
                                       uint32_t height) {
    if (!chain_ready_ || !ensure_output(width, height) ||
        (source_width_ == 0) || (source_height_ == 0)) {
        return false;
    }
    if (renderer_ != nullptr) {
        (void)SDL_RenderFlush(renderer_);
    }
    const libra_instance_t *instance = loader_.instance();
    libra_viewport_t viewport{0.0f, 0.0f, width, height};
    frame_gl_opt_t frame_options{};
    frame_options.version = LIBRASHADER_CURRENT_VERSION;
    frame_options.frame_direction = 1;
    frame_options.total_subframes = 1;
    frame_options.current_subframe = 1;
    frame_options.aspect_ratio = 4.0f / 3.0f;
    frame_options.frames_per_second = 60.0f;
    frame_options.frametime_delta = frame_time_ms;
    frame_options.color_space = LIBRA_COLOR_SPACE_SDR;
    frame_options.brightness_nits = 200.0f;
    libra_error_t error = instance->gl_filter_chain_frame(
        &chain_, frame_count, source_image(), output_image(), &viewport, nullptr,
        &frame_options);
    if (error != nullptr) {
        report_error(instance, "filter-chain frame", error);
        return false;
    }
    return true;
}

bool LibrashaderGlBridge::draw_output(int drawable_width, int drawable_height,
                                      int x, int y, uint32_t width,
                                      uint32_t height) {
    if (!chain_ready_ || (api_ == nullptr) || (api_->output_program == 0) ||
        (api_->output_vao == 0) || (output_width_ != width) ||
        (output_height_ != height) || (drawable_width <= 0) ||
        (drawable_height <= 0)) {
        return false;
    }
    glGetError();
    api_->bind_framebuffer(kFramebuffer, 0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, drawable_width, drawable_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(x, drawable_height - y - static_cast<int>(height),
               static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    api_->use_program(api_->output_program);
    api_->active_texture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, output_texture_);
    const GLint sampler = api_->get_uniform_location(api_->output_program, "Output");
    if (sampler >= 0) {
        api_->uniform_1i(sampler, 0);
    }
    api_->bind_vertex_array(api_->output_vao);
    api_->draw_arrays(GL_TRIANGLE_STRIP, 0, 4);
    api_->bind_vertex_array(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    api_->use_program(0);
    return gl_ok();
}

void LibrashaderGlBridge::disable_filter() {
    if (chain_ != nullptr) {
        if (SDL_GL_GetCurrentContext() != nullptr) {
            const libra_instance_t *instance = loader_.instance();
            libra_error_t error = instance->gl_filter_chain_free(&chain_);
            if (error != nullptr) {
                report_error(instance, "filter-chain destruction", error);
            }
        }
        chain_ = nullptr;
    }
    chain_ready_ = false;
    parameters_.clear();
}

uint32_t LibrashaderGlBridge::parameter_count() const {
    return static_cast<uint32_t>(parameters_.size());
}

bool LibrashaderGlBridge::parameter_info(
    uint32_t index, VAEG_LIBRASHADER_PARAMETER *info) const {
    if ((info == nullptr) || (index >= parameters_.size())) {
        return false;
    }
    const Parameter &parameter = parameters_[index];
    std::snprintf(info->name, sizeof(info->name), "%s", parameter.name.c_str());
    std::snprintf(info->description, sizeof(info->description), "%s",
                  parameter.description.c_str());
    info->initial = parameter.initial;
    info->minimum = parameter.minimum;
    info->maximum = parameter.maximum;
    info->step = parameter.step;
    info->value = parameter.value;
    return true;
}

bool LibrashaderGlBridge::set_parameter(const char *name, float value) {
    if (!chain_ready_ || (name == nullptr)) {
        return false;
    }
    Parameter *selected = nullptr;
    for (Parameter &parameter : parameters_) {
        if (parameter.name == name) {
            selected = &parameter;
            break;
        }
    }
    if (selected == nullptr) {
        return false;
    }
    value = std::max(selected->minimum, std::min(selected->maximum, value));
    const libra_instance_t *instance = loader_.instance();
    libra_error_t error = instance->gl_filter_chain_set_param(
        &chain_, selected->name.c_str(), value);
    if (error != nullptr) {
        report_error(instance, "filter-chain parameter update", error);
        return false;
    }
    selected->value = value;
    return true;
}

libra_image_gl_t LibrashaderGlBridge::source_image() const {
    return {source_texture_, kSourceInternalFormat, source_width_, source_height_};
}

libra_image_gl_t LibrashaderGlBridge::output_image() const {
    return {output_texture_, kSourceInternalFormat, output_width_, output_height_};
}

extern "C" void *librashader_gl_bridge_create(SDL_Renderer *renderer) {
    LibrashaderGlBridge *bridge = new (std::nothrow) LibrashaderGlBridge(renderer);
    return bridge;
}

extern "C" void librashader_gl_bridge_destroy(void *bridge) {
    delete static_cast<LibrashaderGlBridge *>(bridge);
}

extern "C" int librashader_gl_bridge_upload_source(void *bridge, const void *pixels,
                                                     int pitch, uint32_t width,
                                                     uint32_t height) {
    return (bridge != nullptr) &&
                   static_cast<LibrashaderGlBridge *>(bridge)->upload_source(
                       pixels, pitch, width, height)
               ? 1
               : 0;
}

extern "C" int librashader_gl_bridge_load_preset(void *bridge, const char *path) {
    return (bridge != nullptr) &&
                   static_cast<LibrashaderGlBridge *>(bridge)->load_preset(path)
               ? 1
               : 0;
}

extern "C" int librashader_gl_bridge_render_frame(void *bridge, uint32_t frame_count,
                                                   uint32_t frame_time_ms,
                                                   uint32_t width, uint32_t height) {
    return (bridge != nullptr) &&
                   static_cast<LibrashaderGlBridge *>(bridge)->render_frame(
                       frame_count, frame_time_ms, width, height)
               ? 1
               : 0;
}

extern "C" int librashader_gl_bridge_draw_output(void *bridge, int drawable_width,
                                                  int drawable_height, int x, int y,
                                                  uint32_t width, uint32_t height) {
    return (bridge != nullptr) &&
                   static_cast<LibrashaderGlBridge *>(bridge)->draw_output(
                       drawable_width, drawable_height, x, y, width, height)
               ? 1
               : 0;
}

extern "C" void librashader_gl_bridge_disable_filter(void *bridge) {
    if (bridge != nullptr) {
        static_cast<LibrashaderGlBridge *>(bridge)->disable_filter();
    }
}

extern "C" uint32_t librashader_gl_bridge_parameter_count(void *bridge) {
    return (bridge != nullptr)
               ? static_cast<LibrashaderGlBridge *>(bridge)->parameter_count()
               : 0;
}

extern "C" int librashader_gl_bridge_parameter_info(
    void *bridge, uint32_t index, VAEG_LIBRASHADER_PARAMETER *info) {
    return (bridge != nullptr) &&
                   static_cast<LibrashaderGlBridge *>(bridge)->parameter_info(index,
                                                                                info)
               ? 1
               : 0;
}

extern "C" int librashader_gl_bridge_set_parameter(void *bridge, const char *name,
                                                    float value) {
    return (bridge != nullptr) &&
                   static_cast<LibrashaderGlBridge *>(bridge)->set_parameter(name,
                                                                               value)
               ? 1
               : 0;
}
