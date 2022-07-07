// The MIT License (MIT)

// Copyright 2020 HankShu inkiu0@gmail.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <chrono>
#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "lua.hpp"

#if PLATFORM_WINDOWS
#define ELUA_PRINTF sprintf_s
#else
#define ELUA_PRINTF snprintf
#endif

typedef std::chrono::high_resolution_clock Clock;

int64 GetTimeNs() { return Clock::now().time_since_epoch().count(); }

double GetTimeMs() { return Clock::now().time_since_epoch().count() * 0.000001; }

int32 GetStateMemB();

float GetStateMemKb();

int32 lua_sizeof(lua_State* L, int32 idx);

const void* lua_getaddr(lua_State* L, int32 idx);

namespace ELuaProfiler
{
	static const FName ELuaMonitorTabName(TEXT("ELuaMonitor"));

	static const FName ELuaMemAnalyzerTabName(TEXT("ELuaMemAnalyzer"));

	const int HookMask = LUA_MASKCALL | LUA_MASKRET;
	
    static uint32 AllocSize = 0;
    
    static uint32 GCSize = 0;
}

enum ELuaMonitorMode : uint8
{
	PerFrame,		// Deep Copy TraceInfoTree PerFrame
	Total,			// Only one TraceInfoTree
	Statistics,		// Unfold TraceInfoTree
	MAX
};

enum EMonitorSortMode : uint8
{
	TotalTime,
	SelfTime,
	Average,
	Alloc,
	GC,
	Calls
};

enum ESnapshotOp : uint8
{
	SOP_None = 1 << 0,
	SOP_AND = 1 << 1,
	SOP_OR = 1 << 2,
	SOP_XOR = 1 << 3
};
