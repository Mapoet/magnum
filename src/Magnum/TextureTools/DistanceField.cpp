/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "DistanceField.h"

#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Resource.h>

#include "Magnum/Math/Range.h"
#include "Magnum/GL/AbstractShaderProgram.h"
#include "Magnum/GL/Buffer.h"
#include "Magnum/GL/Context.h"
#include "Magnum/GL/Extensions.h"
#include "Magnum/GL/Framebuffer.h"
#include "Magnum/GL/Mesh.h"
#include "Magnum/GL/Shader.h"
#include "Magnum/GL/Texture.h"
#include "Magnum/Shaders/Implementation/CreateCompatibilityShader.h"

#ifdef MAGNUM_BUILD_STATIC
static void importTextureToolResources() {
    CORRADE_RESOURCE_INITIALIZE(MagnumTextureTools_RCS)
}
#endif

namespace Magnum { namespace TextureTools {

namespace {

class DistanceFieldShader: public GL::AbstractShaderProgram {
    public:
        typedef GL::Attribute<0, Vector2> Position;

        explicit DistanceFieldShader(Int radius);

        DistanceFieldShader& setScaling(const Vector2& scaling) {
            setUniform(scalingUniform, scaling);
            return *this;
        }

        DistanceFieldShader& setImageSizeInverted(const Vector2& size) {
            setUniform(imageSizeInvertedUniform, size);
            return *this;
        }

        DistanceFieldShader& bindTexture(GL::Texture2D& texture) {
            texture.bind(TextureUnit);
            return *this;
        }

    private:
        /* ES2 on iOS (apparently independent on the device) has only 8 texture
           units, so be careful to not step over that. ES3 on the same has 16. */
        enum: Int { TextureUnit = 7 };

        Int scalingUniform{0},
            imageSizeInvertedUniform;
};

DistanceFieldShader::DistanceFieldShader(Int radius) {
    #ifdef MAGNUM_BUILD_STATIC
    /* Import resources on static build, if not already */
    if(!Utility::Resource::hasGroup("MagnumTextureTools"))
        importTextureToolResources();
    #endif
    Utility::Resource rs("MagnumTextureTools");

    #ifndef MAGNUM_TARGET_GLES
    const GL::Version v = GL::Context::current().supportedVersion({GL::Version::GL320, GL::Version::GL300, GL::Version::GL210});
    #else
    const GL::Version v = GL::Context::current().supportedVersion({GL::Version::GLES300, GL::Version::GLES200});
    #endif

    GL::Shader vert = Shaders::Implementation::createCompatibilityShader(rs, v, GL::Shader::Type::Vertex);
    GL::Shader frag = Shaders::Implementation::createCompatibilityShader(rs, v, GL::Shader::Type::Fragment);

    vert.addSource(rs.get("FullScreenTriangle.glsl"))
        .addSource(rs.get("DistanceFieldShader.vert"));
    frag.addSource(Utility::formatString("#define RADIUS {}\n", radius))
        .addSource(rs.get("DistanceFieldShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, frag}));

    attachShaders({vert, frag});

    /* Older GLSL doesn't have gl_VertexID, vertices must be supplied explicitly */
    #ifndef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isVersionSupported(GL::Version::GL300))
    #else
    if(!GL::Context::current().isVersionSupported(GL::Version::GLES300))
    #endif
    {
        bindAttributeLocation(Position::Location, "position");
    }

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    #ifndef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::explicit_uniform_location>())
    #endif
    {
        scalingUniform = uniformLocation("scaling");

        #ifndef MAGNUM_TARGET_GLES
        if(!GL::Context::current().isVersionSupported(GL::Version::GL320))
        #else
        if(!GL::Context::current().isVersionSupported(GL::Version::GLES300))
        #endif
        {
            imageSizeInvertedUniform = uniformLocation("imageSizeInverted");
        }
    }

    #ifndef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::shading_language_420pack>())
    #endif
    {
        setUniform(uniformLocation("textureData"), TextureUnit);
    }
}

}
#ifndef MAGNUM_TARGET_GLES
void distanceField(GL::Texture2D& input, GL::Texture2D& output, const Range2Di& rectangle, const Int radius, const Vector2i&)
#else
void distanceField(GL::Texture2D& input, GL::Texture2D& output, const Range2Di& rectangle, const Int radius, const Vector2i& imageSize)
#endif
{
    #ifndef MAGNUM_TARGET_GLES
    MAGNUM_ASSERT_GL_EXTENSION_SUPPORTED(GL::Extensions::ARB::framebuffer_object);
    #endif

    /** @todo Disable depth test, blending and then enable it back (if was previously) */

    #ifndef MAGNUM_TARGET_GLES
    Vector2i imageSize = input.imageSize(0);
    #endif

    GL::Framebuffer framebuffer(rectangle);
    framebuffer.attachTexture(GL::Framebuffer::ColorAttachment(0), output, 0);
    framebuffer.bind();
    framebuffer.clear(GL::FramebufferClear::Color);

    const GL::Framebuffer::Status status = framebuffer.checkStatus(GL::FramebufferTarget::Draw);
    if(status != GL::Framebuffer::Status::Complete) {
        Error() << "TextureTools::distanceField(): cannot render to given output texture, unexpected framebuffer status"
                << status;
        return;
    }

    DistanceFieldShader shader{radius};
    shader.setScaling(Vector2(imageSize)/Vector2(rectangle.size()))
        .bindTexture(input);

    #ifndef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isVersionSupported(GL::Version::GL320))
    #else
    if(!GL::Context::current().isVersionSupported(GL::Version::GLES300))
    #endif
    {
        shader.setImageSizeInverted(1.0f/Vector2(imageSize));
    }

    GL::Mesh mesh;
    mesh.setPrimitive(GL::MeshPrimitive::Triangles)
        .setCount(3);

    /* Older GLSL doesn't have gl_VertexID, vertices must be supplied explicitly */
    GL::Buffer buffer;
    #ifndef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isVersionSupported(GL::Version::GL300))
    #else
    if(!GL::Context::current().isVersionSupported(GL::Version::GLES300))
    #endif
    {
        constexpr Vector2 triangle[] = {
            Vector2(-1.0,  1.0),
            Vector2(-1.0, -3.0),
            Vector2( 3.0,  1.0)
        };
        buffer.setData(triangle, GL::BufferUsage::StaticDraw);
        mesh.addVertexBuffer(buffer, 0, DistanceFieldShader::Position());
    }

    /* Draw the mesh */
    mesh.draw(shader);
}

}}
