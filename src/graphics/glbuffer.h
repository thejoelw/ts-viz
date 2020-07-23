#pragma once

#include <cstring>
#include <ratio>

#include "jw_util/baseexception.h"
#include "jw_util/intervalset.h"

#include "defs/GLBUFFER_MAX_MERGE_GAP.h"
#include "defs/GLBUFFER_FLUSH_EXPLICIT.h"

#include "graphics/glbufferbase.h"

namespace graphics {

// TODO: Try glWriteBufferSubData
// http://www.bfilipek.com/2015/01/persistent-mapped-buffers-in-opengl.html

template <typename Type, typename alloc_ratio = std::ratio<3, 2>>
class GlBuffer : public GlBufferBase {
public:
    class Exception : public jw_util::BaseException {
        friend class GlBuffer;

    private:
        Exception(const std::string &msg)
            : BaseException(msg)
        {}
    };

    GlBuffer(GLenum target, GLenum buffer_hint)
        : GlBufferBase(target, buffer_hint)
        , vbo_size(0)
    {}

    typedef unsigned int Index;

    void flag_index(Index offset)
    {
        flags.insert(offset, offset + 1);
    }

    void flag_range(Index offset, Index limit)
    {
        flags.insert(offset, limit);
    }

    bool needs_resize(Index size) const
    {
        return size > vbo_size;
    }

    void update_size_nocopy(Index size) {
        if (needs_resize(size))
        {
            // Need to allocate a larger vbo

            static_assert(alloc_ratio::num && alloc_ratio::den, "Cannot change the size of this buffer");
            Index new_vbo_size = size * alloc_ratio::num / alloc_ratio::den;

            glBindBuffer(target, vbo_id);
            glBufferData(target, new_vbo_size * sizeof(Type), 0, buffer_hint);
            GL::catchErrors();

            vbo_size = new_vbo_size;
        }
    }

    void update_size(Index size)
    {
        if (needs_resize(size))
        {
            // Need to allocate a larger vbo

            static_assert(alloc_ratio::num && alloc_ratio::den, "Cannot change the size of this buffer");
            Index new_vbo_size = size * alloc_ratio::num / alloc_ratio::den;
            if (vbo_size)
            {
                GLuint new_vbo_id;
                glGenBuffers(1, &new_vbo_id);

                glBindBuffer(target, new_vbo_id);
                glBufferData(target, new_vbo_size * sizeof(Type), 0, buffer_hint);

                glBindBuffer(GL_COPY_READ_BUFFER, vbo_id);
                glCopyBufferSubData(GL_COPY_READ_BUFFER, target, 0, 0, vbo_size * sizeof(Type));
                glBindBuffer(GL_COPY_READ_BUFFER, 0);

                glDeleteBuffers(1, &vbo_id); // I don't know why this undoes the new_vbo_id binding
                vbo_id = new_vbo_id;

                // bind(); // I don't know why this is necessary
                // Apparently it isn't
            }
            else
            {
                glBindBuffer(target, vbo_id);
                glBufferData(target, new_vbo_size * sizeof(Type), 0, buffer_hint);
            }

            GL::catchErrors();

            vbo_size = new_vbo_size;
        }
    }

    template <typename TypeIterator>
    void update_flags(TypeIterator source)
    {
        if (flags.empty()) {
            return;
        }

        constexpr unsigned int merge_elements = GLBUFFER_MAX_MERGE_GAP / sizeof(Type);
        if (merge_elements)
        {
            flags.merge(merge_elements);
        }

#if GLBUFFER_FLUSH_EXPLICIT
        Type *dst = map(0, vbo_size, map_write_access | GL_MAP_FLUSH_EXPLICIT_BIT);
        GL::catchErrors();
#endif

        jw_util::IntervalSet<Index>::Set::const_iterator i = flags.get_set().cbegin();
        while (i != flags.get_set().cend())
        {
            assert(i->offset < i->limit);
            assert(i->limit <= vbo_size);

#if GLBUFFER_FLUSH_EXPLICIT
            std::copy(source + i->offset, source + i->limit, dst + i->offset);
            glFlushMappedBufferRange(target, i->offset * sizeof(Type), (i->limit - i->offset) * sizeof(Type));
#else
            Type *dst = map(i->offset, i->limit - i->offset, map_write_access);
            std::copy(source + i->offset, source + i->limit, dst);
            unmap();
#endif

            i++;
        }

#if GLBUFFER_FLUSH_EXPLICIT
        unmap();
#endif

        flags.clear();

        GL::catchErrors();
    }

    void update_fill(Index start, Index length, const Type value) const
    {
        assert(start <= vbo_size);
        assert(start + length <= vbo_size);

        if (GLEW_ARB_clear_buffer_object && sizeof(Type) <= 16)
        {
            assert_bound();

            GLenum dst_format;
            GLenum src_format;
            GLenum src_type;
            load_fill_formats(sizeof(Type), dst_format, src_format, src_type);

            glClearBufferSubData(target, dst_format, start * sizeof(Type), length * sizeof(Type), src_format, src_type, &value);
        }
        else
        {
            Type *dst = map(start, length, map_write_access);
            std::fill_n(dst, length, value);
            unmap();
        }

        GL::catchErrors();
    }

    void read(Index start, Index length, Type *data) const
    {
        Type *dst = map(start, length, map_read_access);
        std::copy(dst, dst + length, data);
        unmap();
    }

    void write(Index start, Index length, const Type *data) const
    {
        Type *dst = map(start, length, map_write_access);
        std::copy(data, data + length, dst);
        unmap();
    }

    void swap(Index start, Index length, const Type *write_data, Type *read_data) const
    {
        Type *dst = map(start, length, map_read_access | map_write_access);
        std::copy(dst, dst + length, read_data);
        std::copy(write_data, write_data + length, dst);
        unmap();
    }

    Type *map(Index start, Index length, GLbitfield access) const
    {
        assert_bound();
        assert(length);
        assert(start + length <= vbo_size);

        void *dst = glMapBufferRange(target, start * sizeof(Type), length * sizeof(Type), access);
        if (!dst)
        {
            throw Exception("Cannot map buffer range of " + std::to_string(length) + " bytes: " + GL::getErrors());
        }
        GL::catchErrors();
        return static_cast<Type*>(dst);
    }

    void unmap() const
    {
        assert_bound();

        bool success = glUnmapBuffer(target);
        if (!success)
        {
            throw Exception("Cannot unmap buffer!" + GL::getErrors());
        }
        GL::catchErrors();
    }

    Index capacity() const {return vbo_size;}

    const jw_util::IntervalSet<Index> &get_flags() const {return flags;}

    static constexpr GLbitfield map_write_access = GL_MAP_WRITE_BIT /* | GL_MAP_UNSYNCHRONIZED_BIT */;
    static constexpr GLbitfield map_read_access = GL_MAP_READ_BIT;

private:
    Index vbo_size;

    jw_util::IntervalSet<Index> flags;

    void load_fill_formats(unsigned int stride, GLenum &dst_format, GLenum &src_format, GLenum &src_type) const
    {
        switch (stride)
        {
        case 1:
            dst_format = GL_R8UI;
            src_format = GL_RED_INTEGER;
            src_type = GL_UNSIGNED_BYTE;
            break;

        case 2:
            dst_format = GL_R16UI;
            src_format = GL_RED_INTEGER;
            src_type = GL_UNSIGNED_SHORT;
            break;

        case 4:
            dst_format = GL_R32UI;
            src_format = GL_RED_INTEGER;
            src_type = GL_UNSIGNED_INT;
            break;

        case 8:
            dst_format = GL_RG32UI;
            src_format = GL_RG_INTEGER;
            src_type = GL_UNSIGNED_INT;
            break;

        case 16:
            dst_format = GL_RGBA32UI;
            src_format = GL_RGBA_INTEGER;
            src_type = GL_UNSIGNED_INT;
            break;

        default:
            assert(false);
        }
    }
};

}
