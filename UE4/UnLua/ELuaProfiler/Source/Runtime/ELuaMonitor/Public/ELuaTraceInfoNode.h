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
#include <chrono>


struct ELUAMONITOR_API FELuaTraceInfoNode
{
    /* show name */
    FString Name = "anonymous";

    /* call time */
    double CallTime = 0;

    /* self time */
    double SelfTime = 0;

    /* total time */
    double TotalTime = 0;

    /* average time */
    double Average = 0;

    /* the allocated size of lua_State when this node invoke */
    float CallAllocSize = 0;

    /* the gced size of lua_State when this node return */
    float CallGCSize;

    /* the size of this node alloc */
    float AllocSize = 0;

    /* the size of this node release */
    float GCSize = 0;

    /* the num of calls */
    int32 Count = 0;

    /* debug info event 
     * LUA_HOOKCALL
     * LUA_HOOKRET 
     * LUA_HOOKTAILCALL
     * LUA_HOOKLINE
     * LUA_HOOKCOUNT
     */
    int32 Event = -1;

    /* node id */
    FString ID;

    /* is twigs
     * -1: not evaluated
     *  0: false
     *  1: true
     */
    int8 FlagTwigs = -1;

    /* parent node */
    TSharedPtr<FELuaTraceInfoNode> Parent = nullptr;

    /* all child nodes */
    TArray<TSharedPtr<FELuaTraceInfoNode>> Children;

    /* LuaFuncPtr map to FELuaTraceInfoNode */
    TMap<void const*, TSharedPtr<FELuaTraceInfoNode>> ChildPtrMap;
    
    FELuaTraceInfoNode(TSharedPtr<FELuaTraceInfoNode> InParent, FString& InID/*, const TCHAR* InName, int32 InEvent*/)
    {
        ID = InID;
        // if (InName)
        // {
        //     Name = InName;
        // }
        // Event = InEvent;
        Parent = InParent;
    }

    void AddChild(void const* p, TSharedPtr<FELuaTraceInfoNode> Child)
    {
        Children.Add(Child);
        if (p) { ChildPtrMap.Add(p, Child); }
    }

    // 只有Root才会调用
    void FakeBeginInvoke()
    {
        CallTime = GetTimeMs();
        CallAllocSize = ELuaProfiler::AllocSize;
        CallGCSize = ELuaProfiler::GCSize;
    }

    // 只有Root和一帧结束尚未返回的函数会调用
    void FakeEndInvoke()
    {
        double NowTime = GetTimeMs();
        TotalTime += NowTime - CallTime;
        CallTime = NowTime;

        AllocSize += (ELuaProfiler::AllocSize - CallAllocSize) * 0.001f;
        GCSize += (ELuaProfiler::GCSize - CallGCSize) * 0.001f;
    }

    void BeginInvoke()
    {
        CallTime = GetTimeMs();
        CallAllocSize = ELuaProfiler::AllocSize;
        CallGCSize = ELuaProfiler::GCSize;
        Count += 1;
    }

    void EndInvoke()
    {
        TotalTime += GetTimeMs() - CallTime;

        AllocSize += (ELuaProfiler::AllocSize - CallAllocSize) * 0.001f;
        GCSize += (ELuaProfiler::GCSize - CallGCSize) * 0.001f;
        // return Event;
    }

    TSharedPtr<FELuaTraceInfoNode> GetChild(void const* p)
    {
        if (ChildPtrMap.Contains(p))
        {
            return ChildPtrMap[p];
        }
        return nullptr;
    }

    void Empty()
    {
        // Name.Empty();
        CallTime = 0.f;
        SelfTime = 0.f;
        TotalTime = 0.f;
        Count = 0;
        // Event = 0;
        ID.Empty();
        Parent = nullptr;
        Children.Empty();
        ChildPtrMap.Empty();
    }

    bool IsTwigs()
    {
        if (FlagTwigs < 0 && Count >= 10)
        {
	        if (TotalTime > TOLERANCE * Count)
	        {
		        FlagTwigs = 0;
	        }
            else
            {
                FlagTwigs = 1;
            }
        }

	    return FlagTwigs == 1;
    }

    FELuaTraceInfoNode(TSharedPtr<FELuaTraceInfoNode> Other)
    {
        if (Other)
        {
            ID = Other->ID;
            // Name = Other->Name;
            CallTime = Other->CallTime;
            SelfTime = Other->SelfTime;
            TotalTime = Other->TotalTime;
            CallAllocSize = Other->CallAllocSize;
            CallGCSize = Other->CallGCSize;
            AllocSize = Other->AllocSize;
            GCSize = Other->GCSize;
            Count = Other->Count;
            // Event = Other->Event;
            Parent = Other->Parent;
        }
    }

    void StatisticizeOtherNode(void const* p, TSharedPtr<FELuaTraceInfoNode> Other)
    {
        if (ChildPtrMap.Contains(p))
        {
            TSharedPtr<FELuaTraceInfoNode> CNode = ChildPtrMap[p];
            CNode->SelfTime += Other->SelfTime;
            CNode->TotalTime += Other->TotalTime;
            CNode->AllocSize += Other->AllocSize;
            CNode->GCSize += Other->GCSize;
            CNode->Count += Other->Count;
        }
        else
        {
            AddChild(p, TSharedPtr<FELuaTraceInfoNode>(new FELuaTraceInfoNode(Other)));
        }
        TotalTime += Other->TotalTime;
        AllocSize += Other->AllocSize;
        GCSize += Other->GCSize;
    }
};

inline FArchive& operator<<(FArchive& Ar, TSharedPtr<FELuaTraceInfoNode>& Node)
{
    if (Ar.IsLoading())
    {
        Ar << Node->ID;
        Ar << Node->Name;
        Ar << Node->CallTime;
        Ar << Node->SelfTime;
        Ar << Node->TotalTime;
        Ar << Node->CallAllocSize;
        Ar << Node->CallGCSize;
        Ar << Node->AllocSize;
        Ar << Node->GCSize;
        Ar << Node->Count;
        Ar << Node->Event;
        int32 ChildrenNum = 0;
        Ar << ChildrenNum;
        for (int32 i = 0; i < ChildrenNum; i++)
        {
            Node->AddChild(nullptr, MakeShared<FELuaTraceInfoNode>(nullptr));
            Ar << Node->Children[i];
            Node->Children[i]->Parent = Node;
        }
    }
    else
    {
        Ar << Node->ID;
        Ar << Node->Name;
        Ar << Node->CallTime;
        Ar << Node->SelfTime;
        Ar << Node->TotalTime;
        Ar << Node->CallAllocSize;
        Ar << Node->CallGCSize;
        Ar << Node->AllocSize;
        Ar << Node->GCSize;
        Ar << Node->Count;
        Ar << Node->Event;
        int32 ChildrenNum = Node->Children.Num();
        Ar << ChildrenNum;
        for (int32 i = 0; i < Node->Children.Num(); i++)
        {
            Ar << Node->Children[i];
        }
    }
    return Ar;
}
