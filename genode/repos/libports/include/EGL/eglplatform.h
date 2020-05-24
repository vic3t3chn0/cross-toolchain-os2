/*
 * \brief  Genode-specific EGL platform definitions
 * \author Norman Feske
 * \date   2010-07-01
 */

/*
 * Copyright (C) 2010-2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef __eglplatform_h_  /* include-guard named as on the other platforms */
#define __eglplatform_h_

#include <KHR/khrplatform.h>

#ifndef EGLAPI
#define EGLAPI KHRONOS_APICALL
#endif

#ifndef EGLAPIENTRY
#define EGLAPIENTRY  KHRONOS_APIENTRY
#endif
#define EGLAPIENTRYP EGLAPIENTRY*

typedef long  EGLNativeDisplayType;
typedef void *EGLNativePixmapType;


enum Surface_type {
	WINDOW,
	PIXMAP
};

struct Genode_egl_window
{
	int width;
	int height;
	unsigned char *addr;
	enum Surface_type  type;
};

typedef struct Genode_egl_window *EGLNativeWindowType;

/* EGL 1.2 types, renamed for consistency in EGL 1.3 */
typedef EGLNativeDisplayType NativeDisplayType;
typedef EGLNativePixmapType  NativePixmapType;
typedef EGLNativeWindowType  NativeWindowType;

typedef khronos_int32_t EGLint;

/* C++ / C typecast macros for special EGL handle values */
#if defined(__cplusplus)
#define EGL_CAST(type, value) (static_cast<type>(value))
#else
#define EGL_CAST(type, value) ((type) (value))
#endif

#endif /* __eglplatform_h_ */
