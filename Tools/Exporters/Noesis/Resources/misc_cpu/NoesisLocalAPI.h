#pragma once

#ifndef _NOESISLOCALAPI_H
#define _NOESISLOCALAPI_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <vector>

#include <assert.h>
#ifndef NoeAssert
	#define NoeAssert assert
#endif
#ifndef NoeHeapAlloc 
	#define NoeHeapAlloc malloc
#endif
#ifndef NoeHeapFree 
	#define NoeHeapFree free
#endif
#ifndef NoeStrCpy
	#define NoeStrCpy strcpy_s
#endif
#ifndef NoeSPrintF
	#define NoeSPrintF sprintf_s
#endif
#ifndef NoeGetFirstLastBitSet64
	#ifdef _NOESIS_INTERNAL
		extern void Math_GetFirstLastBitSet64(unsigned int *pFirstBit, unsigned int *pLastBitPlusOne, const unsigned __int64 val);
		#define NoeGetFirstLastBitSet64 Math_GetFirstLastBitSet64
	#else
		#include "../pluginshare.h"
		extern mathImpFn_t *g_mfn;
		#define NoeGetFirstLastBitSet64 g_mfn->Math_GetFirstLastBitSet64
	#endif
#endif

#endif //_NOESISLOCALAPI_H
