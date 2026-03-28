#include "Logger.h"
#include <cassert>
#include <glad/gl.h>

enum class AttribType
{
    Float,
    Float2,
    Float3,
};

struct AttribElement
{
    AttribType Type;
    bool Normalized = false;
};

struct AttribInfo
{
    GLenum GlType;
    GLint Components;
    GLsizei Size;

    static AttribInfo FromType(AttribType type)
    {
        switch (type)
        {
        case AttribType::Float:
            return {GL_FLOAT, 1, 4};
        case AttribType::Float2:
            return {GL_FLOAT, 2, 8};
        case AttribType::Float3:
            return {GL_FLOAT, 3, 12};
        default:
            LOG_WARNING("AttribInfo::FromType got invalid type");
            return {};
        }
    }
};
