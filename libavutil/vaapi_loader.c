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

#include "config.h"

#if CONFIG_VAAPI_DLOPEN

#include "vaapi_loader.h"
#include "mem.h"
#include "log.h"
#include "error.h"

#ifdef _WIN32
#include "compat/w32dlfcn.h"
#else
#include <dlfcn.h>
#endif

#include <pthread.h>
#include <stdatomic.h>

static VAAPILoader *g_vaapi_loader = NULL;
static atomic_int g_vaapi_refcount = 0;
static pthread_mutex_t g_vaapi_mutex = PTHREAD_MUTEX_INITIALIZER;

#define LOAD_SYM(handle, name) do {                                     \
    loader->name = (pfn_##name)dlsym(handle, #name);                    \
    if (!loader->name) {                                                \
        av_log(NULL, AV_LOG_ERROR,                                      \
               "VAAPI loader: failed to load symbol %s: %s\n",          \
               #name, dlerror());                                       \
        goto fail;                                                      \
    }                                                                   \
} while (0)

#define LOAD_SYM_OPTIONAL(handle, name) do {                            \
    loader->name = (pfn_##name)dlsym(handle, #name);                    \
} while (0)

static void vaapi_loader_unload(VAAPILoader *loader)
{
    if (!loader)
        return;

#if HAVE_VAAPI_WIN32
    if (loader->libva_win32_handle)
        dlclose(loader->libva_win32_handle);
#endif
#if HAVE_VAAPI_X11
    if (loader->libva_x11_handle)
        dlclose(loader->libva_x11_handle);
#endif
#if HAVE_VAAPI_DRM
    if (loader->libva_drm_handle)
        dlclose(loader->libva_drm_handle);
#endif
    if (loader->libva_handle)
        dlclose(loader->libva_handle);

    av_free(loader);
}

int ff_vaapi_loader_init(VAAPILoader **out_loader)
{
    VAAPILoader *loader;
    int ret = 0;

    pthread_mutex_lock(&g_vaapi_mutex);

    if (g_vaapi_loader) {
        atomic_fetch_add(&g_vaapi_refcount, 1);
        *out_loader = g_vaapi_loader;
        pthread_mutex_unlock(&g_vaapi_mutex);
        return 0;
    }

    loader = av_mallocz(sizeof(*loader));
    if (!loader) {
        ret = AVERROR(ENOMEM);
        goto fail_unlock;
    }

    /* Load libva core library */
    loader->libva_handle = dlopen("libva.so.2", RTLD_NOW | RTLD_LOCAL);
    if (!loader->libva_handle)
        loader->libva_handle = dlopen("libva.so", RTLD_NOW | RTLD_LOCAL);
    if (!loader->libva_handle) {
        av_log(NULL, AV_LOG_VERBOSE,
               "VAAPI loader: failed to load libva: %s\n", dlerror());
        ret = AVERROR(ENOSYS);
        goto fail;
    }

    /* Load required symbols from libva */
    LOAD_SYM(loader->libva_handle, vaInitialize);
    LOAD_SYM(loader->libva_handle, vaTerminate);
    LOAD_SYM(loader->libva_handle, vaErrorStr);
    LOAD_SYM(loader->libva_handle, vaQueryVendorString);

    LOAD_SYM(loader->libva_handle, vaMaxNumProfiles);
    LOAD_SYM(loader->libva_handle, vaMaxNumEntrypoints);
    LOAD_SYM(loader->libva_handle, vaQueryConfigProfiles);
    LOAD_SYM(loader->libva_handle, vaQueryConfigEntrypoints);
    LOAD_SYM(loader->libva_handle, vaGetConfigAttributes);

    LOAD_SYM(loader->libva_handle, vaCreateConfig);
    LOAD_SYM(loader->libva_handle, vaDestroyConfig);

    LOAD_SYM(loader->libva_handle, vaQuerySurfaceAttributes);
    LOAD_SYM(loader->libva_handle, vaCreateSurfaces);
    LOAD_SYM(loader->libva_handle, vaDestroySurfaces);

    LOAD_SYM(loader->libva_handle, vaCreateContext);
    LOAD_SYM(loader->libva_handle, vaDestroyContext);

    LOAD_SYM(loader->libva_handle, vaCreateBuffer);
    LOAD_SYM(loader->libva_handle, vaDestroyBuffer);
    LOAD_SYM(loader->libva_handle, vaMapBuffer);
    LOAD_SYM(loader->libva_handle, vaUnmapBuffer);

    LOAD_SYM(loader->libva_handle, vaBeginPicture);
    LOAD_SYM(loader->libva_handle, vaRenderPicture);
    LOAD_SYM(loader->libva_handle, vaEndPicture);
    LOAD_SYM(loader->libva_handle, vaSyncSurface);

    LOAD_SYM(loader->libva_handle, vaMaxNumImageFormats);
    LOAD_SYM(loader->libva_handle, vaQueryImageFormats);
    LOAD_SYM(loader->libva_handle, vaCreateImage);
    LOAD_SYM(loader->libva_handle, vaDeriveImage);
    LOAD_SYM(loader->libva_handle, vaDestroyImage);
    LOAD_SYM(loader->libva_handle, vaGetImage);
    LOAD_SYM(loader->libva_handle, vaPutImage);

    /* Load optional symbols */
    LOAD_SYM_OPTIONAL(loader->libva_handle, vaSetDriverName);
    LOAD_SYM_OPTIONAL(loader->libva_handle, vaSyncBuffer);
#if VA_CHECK_VERSION(0, 36, 0)
    LOAD_SYM_OPTIONAL(loader->libva_handle, vaAcquireBufferHandle);
    LOAD_SYM_OPTIONAL(loader->libva_handle, vaReleaseBufferHandle);
#endif
#if VA_CHECK_VERSION(1, 0, 0)
    LOAD_SYM_OPTIONAL(loader->libva_handle, vaSetErrorCallback);
    LOAD_SYM_OPTIONAL(loader->libva_handle, vaSetInfoCallback);
    LOAD_SYM_OPTIONAL(loader->libva_handle, vaProfileStr);
    LOAD_SYM_OPTIONAL(loader->libva_handle, vaEntrypointStr);
#endif
#if VA_CHECK_VERSION(1, 1, 0)
    LOAD_SYM_OPTIONAL(loader->libva_handle, vaExportSurfaceHandle);
#endif
#if VA_CHECK_VERSION(1, 21, 0)
    LOAD_SYM_OPTIONAL(loader->libva_handle, vaMapBuffer2);
#endif

    /* Load backend-specific libraries and symbols */
#if HAVE_VAAPI_DRM
    loader->libva_drm_handle = dlopen("libva-drm.so.2", RTLD_NOW | RTLD_LOCAL);
    if (!loader->libva_drm_handle)
        loader->libva_drm_handle = dlopen("libva-drm.so", RTLD_NOW | RTLD_LOCAL);
    if (loader->libva_drm_handle) {
        LOAD_SYM(loader->libva_drm_handle, vaGetDisplayDRM);
    } else {
        av_log(NULL, AV_LOG_VERBOSE,
               "VAAPI loader: failed to load libva-drm: %s\n", dlerror());
    }
#endif

#if HAVE_VAAPI_X11
    loader->libva_x11_handle = dlopen("libva-x11.so.2", RTLD_NOW | RTLD_LOCAL);
    if (!loader->libva_x11_handle)
        loader->libva_x11_handle = dlopen("libva-x11.so", RTLD_NOW | RTLD_LOCAL);
    if (loader->libva_x11_handle) {
        LOAD_SYM(loader->libva_x11_handle, vaGetDisplay);
    } else {
        av_log(NULL, AV_LOG_VERBOSE,
               "VAAPI loader: failed to load libva-x11: %s\n", dlerror());
    }
#endif

#if HAVE_VAAPI_WIN32
    loader->libva_win32_handle = dlopen("va_win32.dll", RTLD_NOW | RTLD_LOCAL);
    if (!loader->libva_win32_handle)
        loader->libva_win32_handle = dlopen("libva_win32.dll", RTLD_NOW | RTLD_LOCAL);
    if (loader->libva_win32_handle) {
        LOAD_SYM(loader->libva_win32_handle, vaGetDisplayWin32);
    } else {
        av_log(NULL, AV_LOG_VERBOSE,
               "VAAPI loader: failed to load libva-win32: %s\n", dlerror());
    }
#endif

    g_vaapi_loader = loader;
    atomic_store(&g_vaapi_refcount, 1);
    *out_loader = loader;

    pthread_mutex_unlock(&g_vaapi_mutex);
    return 0;

fail:
    vaapi_loader_unload(loader);
fail_unlock:
    pthread_mutex_unlock(&g_vaapi_mutex);
    return ret;
}

void ff_vaapi_loader_uninit(VAAPILoader **loader_ptr)
{
    if (!loader_ptr || !*loader_ptr)
        return;

    pthread_mutex_lock(&g_vaapi_mutex);

    if (atomic_fetch_sub(&g_vaapi_refcount, 1) == 1) {
        vaapi_loader_unload(g_vaapi_loader);
        g_vaapi_loader = NULL;
    }

    *loader_ptr = NULL;

    pthread_mutex_unlock(&g_vaapi_mutex);
}

#endif /* CONFIG_VAAPI_DLOPEN */
