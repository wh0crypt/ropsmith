#ifndef _WINDEF_
#define _WINDEF_
#pragma once

#define _WINDEF_H // wine ...

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4255)
#endif

#include "minwindef.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINVER
#define WINVER 0x0502
#endif

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#ifndef __declspec
#define __declspec(e) __attribute__((e))
#endif
#ifndef _declspec
#define _declspec(e) __attribute__((e))
#endif
#elif defined(__WATCOMC__)
#define PACKED
#else
#define PACKED
#endif

typedef unsigned int *LPUINT; // FIXME: not in native headers


// HACK for MinGW headers
typedef int WINBOOL;

typedef DWORD COLORREF;
typedef DWORD *LPCOLORREF;

#ifdef __cplusplus
}
#endif

#endif /* _WINDEF_ */
