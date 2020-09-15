#include "gpuprogram.h"

#ifndef NDEBUG
#include <fstream>
#endif
#include <sstream>
#include <assert.h>
#include "spdlog/logger.h"

namespace {
static std::string replaceAll(std::string subject, const std::string &search, const std::string &replace) {
    std::size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}
}

namespace graphics {

GpuProgram::GpuProgram(app::AppContext &context)
    : context(context)
    , program_id(glCreateProgram())
{
    graphics::GL::catchErrors();
}

GpuProgram::~GpuProgram() {
    glDeleteProgram(program_id);
    graphics::GL::catchErrors();

    std::vector<GLuint>::const_iterator i = shaders.cbegin();
    while (i != shaders.cend())
    {
        glDeleteShader(*i);
        graphics::GL::catchErrors();
        i++;
    }
}

void GpuProgram::attachShader(GLenum type, std::string &&source, const Defines &defines) {
    static thread_local const std::string defines_directive = "#defines";
    std::size_t defines_pos = source.find(defines_directive);
    if (defines_pos != std::string::npos) {
        std::string defines_str;
        defines.writeInto(defines_str);
        source.replace(defines_pos, defines_directive.length(), defines_str);
    }

    static thread_local const std::string repeat_directive = "#repeat";
    while (true) {
        std::size_t repeat_pos = source.find(repeat_directive);
        if (repeat_pos == std::string::npos) { break; }

        std::size_t end_pos = source.find('\n', repeat_pos);
        if (end_pos == std::string::npos) {
            context.get<spdlog::logger>().error("#repeat must be eventually followed by a newline");
            throw Exception("Could not compile GLSL shader");
        }

        const char *str = source.data() + repeat_pos + repeat_directive.length();
        const char *end = source.data() + end_pos;
        long long int a = std::strtoll(str, const_cast<char **>(&str), 10);
        if (str >= end) {
            context.get<spdlog::logger>().error("#repeat must be followed by a start index");
            throw Exception("Could not compile GLSL shader");
        }
        long long int b = std::strtoll(str, const_cast<char **>(&str), 10);
        if (str >= end) {
            context.get<spdlog::logger>().error("#repeat must be followed by an end index");
            throw Exception("Could not compile GLSL shader");
        }

        std::string repeat_str;
        for (long long int i = a; i < b; i++) {
            repeat_str += replaceAll(std::string(str, end), "%", std::to_string(i));
        }

        source.replace(repeat_pos, end_pos - repeat_pos, repeat_str);
    }

    GLuint shader = glCreateShader(type);
    assert(shader != 0);
    graphics::GL::catchErrors();
    shaders.push_back(shader);

    const GLchar *srcStringData = source.c_str();
    glShaderSource(shader, 1, &srcStringData, nullptr);
    graphics::GL::catchErrors();
    glCompileShader(shader);
    graphics::GL::catchErrors();

    GLint is_compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
    if (is_compiled == GL_FALSE) {
        GLint max_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

        GLchar *error_log = new GLchar[static_cast<std::size_t>(max_length)];
        glGetShaderInfoLog(shader, max_length, &max_length, error_log);

        context.get<spdlog::logger>().error("Could not compile GLSL shader:");
        context.get<spdlog::logger>().error(std::string(error_log, static_cast<std::size_t>(max_length)));

#ifndef NDEBUG
        std::ofstream file;
        file.open("bad_shader.glsl");
        file << source;
        file.close();
#endif

        delete[] error_log;

        throw Exception("Could not compile GLSL shader");
    }

    glAttachShader(program_id, shader);
    graphics::GL::catchErrors();
}

void GpuProgram::Defines::writeInto(std::string &str) const {
    std::map<std::string, std::string>::const_iterator i = define_map.cbegin();
    while (i != define_map.cend()) {
        str += "#define ";
        str += i->first;
        str += " ";
        str += i->second;
        str += "\n";
        i++;
    }
}

void GpuProgram::link() {
    glLinkProgram(program_id);
    graphics::GL::catchErrors();

    GLint linkStatus;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLint max_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &max_length);

        GLchar *info_log = new GLchar[static_cast<std::size_t>(max_length)];
        glGetProgramInfoLog(program_id, max_length, &max_length, info_log);

        context.get<spdlog::logger>().error("Could not link GLSL program:");
        context.get<spdlog::logger>().error(std::string(info_log, static_cast<std::size_t>(max_length)));

        delete[] info_log;

        throw Exception("Could not link GLSL program");
    }
    graphics::GL::catchErrors();

    GLint max_length = 0;
    glGetProgramiv(getProgramId(), GL_INFO_LOG_LENGTH, &max_length);

    GLchar *info_log = new GLchar[static_cast<std::size_t>(max_length)];
    glGetProgramInfoLog(getProgramId(), max_length, &max_length, info_log);

    context.get<spdlog::logger>().info("GLSL program info log:");
    context.get<spdlog::logger>().info(std::string(info_log, static_cast<std::size_t>(max_length)));

    graphics::GL::catchErrors();

    isLinked = true;
}

void GpuProgram::assertLinked() const {
    assert(isLinked);
}

void GpuProgram::bind() {
    assertLinked();
    glUseProgram(program_id);
    graphics::GL::catchErrors();
}

void GpuProgram::assertBound() const {
#ifdef ISOSURFACESCENE_ASSERT_PROGRAMS
    GLint current_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    assert(static_cast<GLuint>(current_program) == program_id);
#endif
}

void GpuProgram::bindUniformBlock(const char *name, GLuint bindingPointIndex) const {
    GLuint index = glGetUniformBlockIndex(program_id, name);
    graphics::GL::catchErrors();
    assert(index != GL_INVALID_INDEX);

    glUniformBlockBinding(program_id, index, bindingPointIndex);
    graphics::GL::catchErrors();
}

void GpuProgram::printExtensions(app::AppContext &context) {
    (void) context;

    /*
    GLEW_ARB_shader_atomic_counters
    GLEW_ARB_fragment_shader
    GLEW_ARB_compute_shader
    GLEW_ARB_shader_storage_buffer_object
    GLEW_ARB_buffer_storage
    GLEW_ARB_clear_buffer_object
    GLEW_ARB_copy_buffer
    GLEW_ARB_draw_buffers
    GLEW_ARB_draw_indirect
    GLEW_ARB_explicit_attrib_location
    GLEW_ARB_explicit_uniform_location
    GLEW_ARB_shadow
    GLEW_ARB_timer_query
    GLEW_ARB_uniform_buffer_object
    GLEW_ARB_shader_image_load_store // For early_fragment_tests
    GLEW_ARB_vertex_array_object
    GLEW_ARB_vertex_buffer_object
    GLEW_ARB_vertex_shader
    GLEW_ARB_map_buffer_range
    */

    /*
    context.get<spdlog::logger>().debug("GLEW_ARB_shader_atomic_counters: {}", static_cast<unsigned long long int>(GLEW_ARB_shader_atomic_counters));
    context.get<spdlog::logger>().debug("GLEW_ARB_fragment_shader: {}", static_cast<unsigned long long int>(GLEW_ARB_fragment_shader));
    context.get<spdlog::logger>().debug("GLEW_ARB_compute_shader: {}", static_cast<unsigned long long int>(GLEW_ARB_compute_shader));
    context.get<spdlog::logger>().debug("GLEW_ARB_shader_storage_buffer_object: {}", static_cast<unsigned long long int>(GLEW_ARB_shader_storage_buffer_object));
    context.get<spdlog::logger>().debug("GLEW_ARB_buffer_storage: {}", static_cast<unsigned long long int>(GLEW_ARB_buffer_storage));
    context.get<spdlog::logger>().debug("GLEW_ARB_clear_buffer_object: {}", static_cast<unsigned long long int>(GLEW_ARB_clear_buffer_object));
    context.get<spdlog::logger>().debug("GLEW_ARB_copy_buffer: {}", static_cast<unsigned long long int>(GLEW_ARB_copy_buffer));
    context.get<spdlog::logger>().debug("GLEW_ARB_draw_buffers: {}", static_cast<unsigned long long int>(GLEW_ARB_draw_buffers));
    context.get<spdlog::logger>().debug("GLEW_ARB_draw_indirect: {}", static_cast<unsigned long long int>(GLEW_ARB_draw_indirect));
    context.get<spdlog::logger>().debug("GLEW_ARB_explicit_attrib_location: {}", static_cast<unsigned long long int>(GLEW_ARB_explicit_attrib_location));
    context.get<spdlog::logger>().debug("GLEW_ARB_explicit_uniform_location: {}", static_cast<unsigned long long int>(GLEW_ARB_explicit_uniform_location));
    context.get<spdlog::logger>().debug("GLEW_ARB_shadow: {}", static_cast<unsigned long long int>(GLEW_ARB_shadow));
    context.get<spdlog::logger>().debug("GLEW_ARB_timer_query: {}", static_cast<unsigned long long int>(GLEW_ARB_timer_query));
    context.get<spdlog::logger>().debug("GLEW_ARB_uniform_buffer_object: {}", static_cast<unsigned long long int>(GLEW_ARB_uniform_buffer_object));
    context.get<spdlog::logger>().debug("GLEW_ARB_shader_image_load_store: {}", static_cast<unsigned long long int>(GLEW_ARB_shader_image_load_store));
    context.get<spdlog::logger>().debug("GLEW_ARB_vertex_array_object: {}", static_cast<unsigned long long int>(GLEW_ARB_vertex_array_object));
    context.get<spdlog::logger>().debug("GLEW_ARB_vertex_buffer_object: {}", static_cast<unsigned long long int>(GLEW_ARB_vertex_buffer_object));
    context.get<spdlog::logger>().debug("GLEW_ARB_vertex_shader: {}", static_cast<unsigned long long int>(GLEW_ARB_vertex_shader));
    context.get<spdlog::logger>().debug("GLEW_ARB_map_buffer_range: {}", static_cast<unsigned long long int>(GLEW_ARB_map_buffer_range));
    */

    /*
    GLEW_ARB_shader_atomic_counters: 0
    GLEW_ARB_fragment_shader: 0
    GLEW_ARB_compute_shader: 0
    GLEW_ARB_shader_storage_buffer_object: 0
    GLEW_ARB_buffer_storage: 0
    GLEW_ARB_clear_buffer_object: 0
    GLEW_ARB_copy_buffer: 1
    GLEW_ARB_draw_buffers: 1
    GLEW_ARB_draw_indirect: 1
    GLEW_ARB_explicit_attrib_location: 0
    GLEW_ARB_explicit_uniform_location: 0
    GLEW_ARB_shadow: 0
    GLEW_ARB_timer_query: 1
    GLEW_ARB_uniform_buffer_object: 1
    GLEW_ARB_shader_image_load_store: 0
    GLEW_ARB_vertex_array_object: 1
    GLEW_ARB_vertex_buffer_object: 1
    GLEW_ARB_vertex_shader: 1
    GLEW_ARB_map_buffer_range: 1
    */
}

GpuProgram::Exception::Exception(const std::string &msg)
    : BaseException(msg)
{}

}
