// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include "GFFX.h"


// TODO: reference additional headers your program requires here
#define SAFE_DELETE_OBJ(obj) \
if (NULL != obj)                                  \
{                                           \
    delete obj;                        \
    obj = NULL;                      \
}

#define SAFE_DELETE_ARRAY(arrayObj) \
if (NULL != arrayObj)                                  \
{                                           \
    delete[] arrayObj;                        \
    arrayObj = NULL;                      \
}