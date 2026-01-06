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

#ifndef AVUTIL_VAAPI_LOADER_H
#define AVUTIL_VAAPI_LOADER_H

#include "config.h"

#if CONFIG_VAAPI_DLOPEN

#include <va/va.h>
#if HAVE_VAAPI_DRM
#include <va/va_drm.h>
#endif
#if HAVE_VAAPI_X11
#include <va/va_x11.h>
#endif
#if HAVE_VAAPI_WIN32
#include <va/va_win32.h>
#endif

/* Function pointer typedefs for libva functions */

/* Core libva functions */
typedef VAStatus (*pfn_vaInitialize)(VADisplay dpy, int *major_version, int *minor_version);
typedef VAStatus (*pfn_vaTerminate)(VADisplay dpy);
typedef const char *(*pfn_vaErrorStr)(VAStatus error_status);
typedef const char *(*pfn_vaQueryVendorString)(VADisplay dpy);

/* Profile/entrypoint queries */
typedef int (*pfn_vaMaxNumProfiles)(VADisplay dpy);
typedef int (*pfn_vaMaxNumEntrypoints)(VADisplay dpy);
typedef VAStatus (*pfn_vaQueryConfigProfiles)(VADisplay dpy, VAProfile *profile_list, int *num_profiles);
typedef VAStatus (*pfn_vaQueryConfigEntrypoints)(VADisplay dpy, VAProfile profile, VAEntrypoint *entrypoint_list, int *num_entrypoints);
typedef VAStatus (*pfn_vaGetConfigAttributes)(VADisplay dpy, VAProfile profile, VAEntrypoint entrypoint, VAConfigAttrib *attrib_list, int num_attribs);

/* Config management */
typedef VAStatus (*pfn_vaCreateConfig)(VADisplay dpy, VAProfile profile, VAEntrypoint entrypoint, VAConfigAttrib *attrib_list, int num_attribs, VAConfigID *config_id);
typedef VAStatus (*pfn_vaDestroyConfig)(VADisplay dpy, VAConfigID config_id);

/* Surface management */
typedef VAStatus (*pfn_vaQuerySurfaceAttributes)(VADisplay dpy, VAConfigID config, VASurfaceAttrib *attrib_list, unsigned int *num_attribs);
typedef VAStatus (*pfn_vaCreateSurfaces)(VADisplay dpy, unsigned int format, unsigned int width, unsigned int height, VASurfaceID *surfaces, unsigned int num_surfaces, VASurfaceAttrib *attrib_list, unsigned int num_attribs);
typedef VAStatus (*pfn_vaDestroySurfaces)(VADisplay dpy, VASurfaceID *surfaces, int num_surfaces);

/* Context management */
typedef VAStatus (*pfn_vaCreateContext)(VADisplay dpy, VAConfigID config_id, int picture_width, int picture_height, int flag, VASurfaceID *render_targets, int num_render_targets, VAContextID *context);
typedef VAStatus (*pfn_vaDestroyContext)(VADisplay dpy, VAContextID context);

/* Buffer management */
typedef VAStatus (*pfn_vaCreateBuffer)(VADisplay dpy, VAContextID context, VABufferType type, unsigned int size, unsigned int num_elements, void *data, VABufferID *buf_id);
typedef VAStatus (*pfn_vaDestroyBuffer)(VADisplay dpy, VABufferID buffer_id);
typedef VAStatus (*pfn_vaMapBuffer)(VADisplay dpy, VABufferID buf_id, void **pbuf);
typedef VAStatus (*pfn_vaUnmapBuffer)(VADisplay dpy, VABufferID buf_id);

/* Encoding/rendering */
typedef VAStatus (*pfn_vaBeginPicture)(VADisplay dpy, VAContextID context, VASurfaceID render_target);
typedef VAStatus (*pfn_vaRenderPicture)(VADisplay dpy, VAContextID context, VABufferID *buffers, int num_buffers);
typedef VAStatus (*pfn_vaEndPicture)(VADisplay dpy, VAContextID context);
typedef VAStatus (*pfn_vaSyncSurface)(VADisplay dpy, VASurfaceID render_target);

/* Image management */
typedef int (*pfn_vaMaxNumImageFormats)(VADisplay dpy);
typedef VAStatus (*pfn_vaQueryImageFormats)(VADisplay dpy, VAImageFormat *format_list, int *num_formats);
typedef VAStatus (*pfn_vaCreateImage)(VADisplay dpy, VAImageFormat *format, int width, int height, VAImage *image);
typedef VAStatus (*pfn_vaDeriveImage)(VADisplay dpy, VASurfaceID surface, VAImage *image);
typedef VAStatus (*pfn_vaDestroyImage)(VADisplay dpy, VAImageID image);
typedef VAStatus (*pfn_vaGetImage)(VADisplay dpy, VASurfaceID surface, int x, int y, unsigned int width, unsigned int height, VAImageID image);
typedef VAStatus (*pfn_vaPutImage)(VADisplay dpy, VASurfaceID surface, VAImageID image, int src_x, int src_y, unsigned int src_width, unsigned int src_height, int dest_x, int dest_y, unsigned int dest_width, unsigned int dest_height);

/* Optional functions (version-dependent) */
typedef VAStatus (*pfn_vaSetDriverName)(VADisplay dpy, char *driver_name);
typedef VAStatus (*pfn_vaSyncBuffer)(VADisplay dpy, VABufferID buf_id, uint64_t timeout_ns);

#if VA_CHECK_VERSION(0, 36, 0)
typedef VAStatus (*pfn_vaAcquireBufferHandle)(VADisplay dpy, VABufferID buf_id, VABufferInfo *buf_info);
typedef VAStatus (*pfn_vaReleaseBufferHandle)(VADisplay dpy, VABufferID buf_id);
#endif

#if VA_CHECK_VERSION(1, 0, 0)
typedef VAMessageCallback (*pfn_vaSetErrorCallback)(VADisplay dpy, VAMessageCallback callback, void *user_context);
typedef VAMessageCallback (*pfn_vaSetInfoCallback)(VADisplay dpy, VAMessageCallback callback, void *user_context);
typedef const char *(*pfn_vaProfileStr)(VAProfile profile);
typedef const char *(*pfn_vaEntrypointStr)(VAEntrypoint entrypoint);
#endif

#if VA_CHECK_VERSION(1, 1, 0)
typedef VAStatus (*pfn_vaExportSurfaceHandle)(VADisplay dpy, VASurfaceID surface_id,
                                              uint32_t mem_type, uint32_t flags,
                                              void *descriptor);
#endif

#if VA_CHECK_VERSION(1, 21, 0)
typedef VAStatus (*pfn_vaMapBuffer2)(VADisplay dpy, VABufferID buf_id, void **pbuf, uint32_t flags);
#endif

/* Display creation functions (backend-specific) */
#if HAVE_VAAPI_DRM
typedef VADisplay (*pfn_vaGetDisplayDRM)(int fd);
#endif
#if HAVE_VAAPI_X11
typedef VADisplay (*pfn_vaGetDisplay)(Display *dpy);
#endif
#if HAVE_VAAPI_WIN32
typedef VADisplay (*pfn_vaGetDisplayWin32)(const LUID *adapter_luid);
#endif

/* Structure to hold all function pointers */
typedef struct VAAPILoader {
    void *libva_handle;
    void *libva_drm_handle;
    void *libva_x11_handle;
    void *libva_win32_handle;

    /* Core functions */
    pfn_vaInitialize vaInitialize;
    pfn_vaTerminate vaTerminate;
    pfn_vaErrorStr vaErrorStr;
    pfn_vaQueryVendorString vaQueryVendorString;

    /* Profile/entrypoint queries */
    pfn_vaMaxNumProfiles vaMaxNumProfiles;
    pfn_vaMaxNumEntrypoints vaMaxNumEntrypoints;
    pfn_vaQueryConfigProfiles vaQueryConfigProfiles;
    pfn_vaQueryConfigEntrypoints vaQueryConfigEntrypoints;
    pfn_vaGetConfigAttributes vaGetConfigAttributes;

    /* Config management */
    pfn_vaCreateConfig vaCreateConfig;
    pfn_vaDestroyConfig vaDestroyConfig;

    /* Surface management */
    pfn_vaQuerySurfaceAttributes vaQuerySurfaceAttributes;
    pfn_vaCreateSurfaces vaCreateSurfaces;
    pfn_vaDestroySurfaces vaDestroySurfaces;

    /* Context management */
    pfn_vaCreateContext vaCreateContext;
    pfn_vaDestroyContext vaDestroyContext;

    /* Buffer management */
    pfn_vaCreateBuffer vaCreateBuffer;
    pfn_vaDestroyBuffer vaDestroyBuffer;
    pfn_vaMapBuffer vaMapBuffer;
    pfn_vaUnmapBuffer vaUnmapBuffer;

    /* Encoding/rendering */
    pfn_vaBeginPicture vaBeginPicture;
    pfn_vaRenderPicture vaRenderPicture;
    pfn_vaEndPicture vaEndPicture;
    pfn_vaSyncSurface vaSyncSurface;

    /* Image management */
    pfn_vaMaxNumImageFormats vaMaxNumImageFormats;
    pfn_vaQueryImageFormats vaQueryImageFormats;
    pfn_vaCreateImage vaCreateImage;
    pfn_vaDeriveImage vaDeriveImage;
    pfn_vaDestroyImage vaDestroyImage;
    pfn_vaGetImage vaGetImage;
    pfn_vaPutImage vaPutImage;

    /* Optional functions (may be NULL) */
    pfn_vaSetDriverName vaSetDriverName;
    pfn_vaSyncBuffer vaSyncBuffer;
#if VA_CHECK_VERSION(0, 36, 0)
    pfn_vaAcquireBufferHandle vaAcquireBufferHandle;
    pfn_vaReleaseBufferHandle vaReleaseBufferHandle;
#endif
#if VA_CHECK_VERSION(1, 0, 0)
    pfn_vaSetErrorCallback vaSetErrorCallback;
    pfn_vaSetInfoCallback vaSetInfoCallback;
    pfn_vaProfileStr vaProfileStr;
    pfn_vaEntrypointStr vaEntrypointStr;
#endif
#if VA_CHECK_VERSION(1, 1, 0)
    pfn_vaExportSurfaceHandle vaExportSurfaceHandle;
#endif
#if VA_CHECK_VERSION(1, 21, 0)
    pfn_vaMapBuffer2 vaMapBuffer2;
#endif

    /* Display creation (backend-specific) */
#if HAVE_VAAPI_DRM
    pfn_vaGetDisplayDRM vaGetDisplayDRM;
#endif
#if HAVE_VAAPI_X11
    pfn_vaGetDisplay vaGetDisplay;
#endif
#if HAVE_VAAPI_WIN32
    pfn_vaGetDisplayWin32 vaGetDisplayWin32;
#endif
} VAAPILoader;

/**
 * Load libva and backend libraries, populating the function pointers.
 * This function is thread-safe and reference-counted.
 *
 * @param loader Pointer to receive the loader structure
 * @return 0 on success, negative AVERROR on failure
 */
int ff_vaapi_loader_init(VAAPILoader **loader);

/**
 * Release a reference to the loaded libraries.
 * When the last reference is released, the libraries are unloaded.
 *
 * @param loader Pointer to the loader structure (set to NULL on return)
 */
void ff_vaapi_loader_uninit(VAAPILoader **loader);

#endif /* CONFIG_VAAPI_DLOPEN */

#endif /* AVUTIL_VAAPI_LOADER_H */
