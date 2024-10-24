/*
 * Copyright (C) 2023 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformDisplayGBM.h"

#if USE(EGL) && USE(GBM)
#include "GLContext.h"
#include <epoxy/egl.h>
#include <fcntl.h>
#include <gbm.h>
#include <unistd.h>
#include <wtf/SafeStrerror.h>
#include <wtf/StdLibExtras.h>
#include <wtf/unix/UnixFileDescriptor.h>

namespace WebCore {

std::unique_ptr<PlatformDisplayGBM> PlatformDisplayGBM::create(const String& deviceFile)
{
    auto fd = UnixFileDescriptor { open(deviceFile.utf8().data(), O_RDWR | O_CLOEXEC), UnixFileDescriptor::Adopt };
    if (!fd) {
        WTFLogAlways("Failed to open DRM render device %s: %s", deviceFile.utf8().data(), safeStrerror(errno).data());
        return nullptr;
    }

    auto* device = gbm_create_device(fd.value());
    if (!device) {
        WTFLogAlways("Failed to create GBM device for render device: %s: %s", deviceFile.utf8().data(), safeStrerror(errno).data());
        return nullptr;
    }

    return std::unique_ptr<PlatformDisplayGBM>(new PlatformDisplayGBM(WTFMove(fd), device));
}

PlatformDisplayGBM::PlatformDisplayGBM(UnixFileDescriptor&& fd, struct gbm_device* device)
{
#if PLATFORM(GTK)
    PlatformDisplay::setSharedDisplayForCompositing(*this);
#endif

    m_gbm = { WTFMove(fd), device };

    const char* extensions = eglQueryString(nullptr, EGL_EXTENSIONS);
    if (GLContext::isExtensionSupported(extensions, "EGL_EXT_platform_base"))
        m_eglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, m_gbm.device.value(), nullptr);
    else if (GLContext::isExtensionSupported(extensions, "EGL_KHR_platform_base"))
        m_eglDisplay = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, m_gbm.device.value(), nullptr);

    PlatformDisplay::initializeEGLDisplay();
}

PlatformDisplayGBM::~PlatformDisplayGBM()
{
}

} // namespace WebCore

#endif // USE(EGL) && USE(GBM)
