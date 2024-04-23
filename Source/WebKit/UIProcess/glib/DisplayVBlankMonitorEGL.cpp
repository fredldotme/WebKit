/*
 * Copyright (C) 2024 Alfred Neumayer
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DisplayVBlankMonitorEGL.h"

#include <WebCore/AnimationFrameRate.h>
#include <chrono>
#include <thread>

namespace WebKit {

std::unique_ptr<DisplayVBlankMonitor> DisplayVBlankMonitorEGL::create()
{
    // libepoxy might have a different idea about the EGL implementation's ability
    // to provide these extensions, hence side-step it in order to retain functionality.
    // Effectively works around "No provider of eglCreateSyncKHR found" errors.
    auto create = reinterpret_cast<PFNEGLCREATESYNCPROC>(eglGetProcAddress("eglCreateSyncKHR"));
    auto wait = reinterpret_cast<PFNEGLCLIENTWAITSYNCPROC>(eglGetProcAddress("eglClientWaitSyncKHR"));
    auto destroy = reinterpret_cast<PFNEGLDESTROYSYNCPROC>(eglGetProcAddress("eglDestroySyncKHR"));
    
    if (!(create && wait && destroy))
        return nullptr;

    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY)
        return nullptr;

    return makeUnique<DisplayVBlankMonitorEGL>(display, create, wait, destroy);
}

DisplayVBlankMonitorEGL::DisplayVBlankMonitorEGL(
        EGLDisplay display,
        PFNEGLCREATESYNCPROC create,
        PFNEGLCLIENTWAITSYNCPROC wait,
        PFNEGLDESTROYSYNCPROC destroy)
    : DisplayVBlankMonitor(WebCore::FullSpeedFramesPerSecond)
    , m_eglDisplay(display)
    , m_eglCreateSyncKHR(create)
    , m_eglClientWaitSyncKHR(wait)
    , m_eglDestroySyncKHR(destroy)
{
}

bool DisplayVBlankMonitorEGL::waitForVBlank() const
{
    auto sync = m_eglCreateSyncKHR(m_eglDisplay, EGL_SYNC_FENCE, nullptr);
	m_eglClientWaitSyncKHR(m_eglDisplay, sync, EGL_SYNC_FLUSH_COMMANDS_BIT, EGL_FOREVER);
	m_eglDestroySyncKHR(m_eglDisplay, sync);
    return true;
}

} // namespace WebKit
