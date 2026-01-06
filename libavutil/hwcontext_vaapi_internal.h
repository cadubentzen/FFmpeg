/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVUTIL_HWCONTEXT_VAAPI_INTERNAL_H
#define AVUTIL_HWCONTEXT_VAAPI_INTERNAL_H

#include "config.h"
#include "hwcontext.h"

#if CONFIG_VAAPI_DLOPEN
#include "vaapi_loader.h"

/**
 * Get the VAAPILoader from an AVHWDeviceContext.
 * Returns NULL if the device context is not VAAPI or if dlopen is disabled.
 */
VAAPILoader *ff_vaapi_get_loader(AVHWDeviceContext *hwdev);

/* Macro for calling VA functions through the loader */
#define VA_CALL(loader, func, ...) ((loader)->func(__VA_ARGS__))

#else /* !CONFIG_VAAPI_DLOPEN */

/* Direct function calls when linking against libva */
#define VA_CALL(loader, func, ...) ((void)(loader), func(__VA_ARGS__))

static inline void *ff_vaapi_get_loader(AVHWDeviceContext *hwdev)
{
    return NULL;
}

#endif /* CONFIG_VAAPI_DLOPEN */

#endif /* AVUTIL_HWCONTEXT_VAAPI_INTERNAL_H */
