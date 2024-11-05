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

#include "ELuaBase.h"
#include "ELuaTraceInfoNode.h"

#define CORRECT_TIME false

#if CORRECT_TIME
#define DEVIATION 10	// default deviation 10ns
#else
#define DEVIATION 0
#endif // CORRECT_TIME


class ELUAMONITOR_API FELuaTraceInfoTree
{
public:
    FELuaTraceInfoTree();
    ~FELuaTraceInfoTree();

    void Init();

    void OnHookCall(lua_State* L, void const* p, FString ID, bool IsStatistics = false);

    void OnHookReturn();

    void OnHookError();

    void OnAlloc(void* nptr, void* ptr, size_t osize, size_t nsize);

    bool IsOnRoot() { return CurNode == Root; }

    void CountSelfTime(EMonitorSortMode SortMode);

    TSharedPtr<FELuaTraceInfoNode> GetRoot() { return Root; }

    TSharedPtr<FELuaTraceInfoNode> Statisticize(EMonitorSortMode SortMode);

    void StatisticizeNode(TSharedPtr<FELuaTraceInfoNode> Node, TSharedPtr<FELuaTraceInfoNode> StatisticsNode);
    
    void Deserialize(TSharedPtr<FELuaTraceInfoNode>& TreeRoot);

private:
    TSharedPtr<FELuaTraceInfoNode> GetChild(void const* p, FString ID);

    void CountNodeSelfTime(TSharedPtr<FELuaTraceInfoNode> Node, EMonitorSortMode SortMode);

private:
    TSharedPtr<FELuaTraceInfoNode> Root;
    TSharedPtr<FELuaTraceInfoNode> CurNode;
    TMap<void*, FELuaTraceInfoNode*> MemMap;
    uint32 CurDepth = 0;
};

FArchive& operator<<(FArchive& Ar, TSharedPtr<FELuaTraceInfoTree>& Tree);
