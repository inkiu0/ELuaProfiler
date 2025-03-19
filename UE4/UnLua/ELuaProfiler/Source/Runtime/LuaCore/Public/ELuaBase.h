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

ELUACORE_API int64 GetTimeNs();

ELUACORE_API double GetTimeMs();

ELUACORE_API int32 GetStateMemB();

ELUACORE_API float GetStateMemKb();

ELUACORE_API int32 lua_sizeof(lua_State* L, int32 idx);

ELUACORE_API const void* lua_getaddr(lua_State* L, int32 idx);

namespace ELuaProfiler
{
    static const FName ELuaMonitorTabName(TEXT("ELuaMonitor"));

    static const FName ELuaMemAnalyzerTabName(TEXT("ELuaMemAnalyzer"));

    static const FString ELUA_PROF_FILE_SUFFIX(TEXT(".elprof"));

    const int HookMask = LUA_MASKCALL | LUA_MASKRET;
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

//#if PLATFORM_WINDOWS
//const FString SandBoxPath = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()), TEXT("Content/Script/"));
//#else
//const FString SandBoxPath = FPaths::ProjectDir() + TEXT("Content/Script/");
//#endif

#if PLATFORM_WINDOWS
constexpr double TOLERANCE = 0.0002;
#else
constexpr double TOLERANCE = 0.0005;
#endif

enum EPerfDataSerializeVersion : uint8
{
	Default,
    SymbolTable
};

constexpr  EPerfDataSerializeVersion E_SERIALIZE_VERSION = SymbolTable;

static EPerfDataSerializeVersion E_SERIALIZE_READING_PERF_DATA_VERSION = Default;

static TMap<void const*, FString> LuaFuncPtrMap;
