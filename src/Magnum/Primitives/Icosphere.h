#ifndef Magnum_Primitives_Icosphere_h
#define Magnum_Primitives_Icosphere_h
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

/** @file
 * @brief Function @ref Magnum::Primitives::icosphereSolid()
 */

#include "Magnum/Primitives/visibility.h"
#include "Magnum/Trade/Trade.h"

#ifdef MAGNUM_BUILD_DEPRECATED
#include <Corrade/Utility/Macros.h>
#endif

namespace Magnum { namespace Primitives {

/**
@brief Solid 3D icosphere
@param subdivisions      Number of subdivisions

Sphere with radius @cpp 1.0f @ce. Indexed @ref MeshPrimitive::Triangles with
normals.

@image html primitives-icospheresolid.png width=256px

The @p subdivisions parameter describes how many times is each icosphere
triangle subdivided, recursively. Specifying @cpp 0 @ce will result in an
icosphere with 20 faces, saying @cpp 1 @ce will result in an icosphere with 80
faces (each triangle subdivided into four smaller), saying @cpp 2 @ce will
result in 320 faces and so on. In particular, this is different from the
`subdivisions` parameter in @ref grid3DSolid() or @ref grid3DWireframe().
@see @ref uvSphereSolid(), @ref uvSphereWireframe()
*/
MAGNUM_PRIMITIVES_EXPORT Trade::MeshData3D icosphereSolid(UnsignedInt subdivisions);

#ifdef MAGNUM_BUILD_DEPRECATED
/**
@brief 3D icosphere
@deprecated Use @ref icosphereSolid() instead.
*/
struct MAGNUM_PRIMITIVES_EXPORT Icosphere {
    /** @brief @copybrief icosphereSolid()
     * @deprecated Use @ref icosphereSolid() instead.
     */
    CORRADE_DEPRECATED("use icosphereSolid() instead") static Trade::MeshData3D solid(UnsignedInt subdivisions);
};
#endif

}}

#endif
