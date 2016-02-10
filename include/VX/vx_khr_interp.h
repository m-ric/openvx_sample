/*
 * Copyright (c) 2012-2015 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#ifndef _VX_KHR_INTERP_H_
#define _VX_KHR_INTERP_H_

/*! \brief The Interpolation Type Query Extension.
 * \file
 */

#define OPENVX_KHR_INTERP   "vx_khr_interpolation"

#include <VX/vx.h>

/*! \brief Additional interpolation types */
enum vx_interpolation_type_ext_e {
    /*! \brief Bicubic interpolation method */
    VX_INTERPOLATION_BICUBIC = VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_INTERPOLATION) + 0x3,
    /*! \brief Mipmapping interpolation method */
    VX_INTERPOLATION_MIPMAP = VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_INTERPOLATION) + 0x4,
};

#endif

