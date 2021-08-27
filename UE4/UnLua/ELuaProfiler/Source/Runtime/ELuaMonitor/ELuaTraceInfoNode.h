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

#include "LuaCore/ELuaBase.h"

struct ELUAPROFILER_API FELuaTraceInfoNode
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

	/* parent node */
	TSharedPtr<FELuaTraceInfoNode> Parent = nullptr;

	/* all child nodes */
	TArray<TSharedPtr<FELuaTraceInfoNode>> Children;

	/* id map to FELuaTraceInfoNode */
	TMap<FString, TSharedPtr<FELuaTraceInfoNode>> ChildIDMap;

	FELuaTraceInfoNode(TSharedPtr<FELuaTraceInfoNode> InParent, FString& InID, const char* InName, int32 InEvent)
	{
		ID = InID;
		if (InName)
		{
			Name = InName;
		}
		Event = InEvent;
		Parent = InParent;
	}

	void AddChild(TSharedPtr<FELuaTraceInfoNode> Child)
	{
		Children.Add(Child);
		ChildIDMap.Add(Child->ID, Child);
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

	int32 EndInvoke()
	{
		TotalTime += GetTimeMs() - CallTime;

        AllocSize += (ELuaProfiler::AllocSize - CallAllocSize) * 0.001f;
        GCSize += (ELuaProfiler::GCSize - CallGCSize) * 0.001f;
	}

	TSharedPtr<FELuaTraceInfoNode> GetChild(const FString& InID)
	{
		if (ChildIDMap.Contains(InID))
		{
			return ChildIDMap[InID];
		}
		return nullptr;
	}

	void Empty()
	{
		Name.Empty();
		CallTime = 0.f;
		SelfTime = 0.f;
		TotalTime = 0.f;
		Count = 0;
		Event = 0;
		ID.Empty();
		Parent = nullptr;
		Children.Empty();
		ChildIDMap.Empty();
	}

	FELuaTraceInfoNode(TSharedPtr<FELuaTraceInfoNode> Other)
	{
		ID = Other->ID;
		Name = Other->Name;
		CallTime = Other->CallTime;
		SelfTime = Other->SelfTime;
		TotalTime = Other->TotalTime;
		CallAllocSize = Other->CallAllocSize;
        CallGCSize = Other->CallGCSize;
		AllocSize = Other->AllocSize;
		GCSize = Other->GCSize;
		Count = Other->Count;
		Event = Other->Event;
		Parent = Other->Parent;
	}

	void StatisticizeOtherNode(TSharedPtr<FELuaTraceInfoNode> Other)
	{
		if (ChildIDMap.Contains(Other->ID))
		{
			TSharedPtr<FELuaTraceInfoNode> CNode = ChildIDMap[Other->ID];
			CNode->SelfTime += Other->SelfTime;
			CNode->TotalTime += Other->TotalTime;
			CNode->AllocSize += Other->AllocSize;
			CNode->GCSize += Other->GCSize;
			CNode->Count += Other->Count;
		}
		else
		{
			AddChild(TSharedPtr<FELuaTraceInfoNode>(new FELuaTraceInfoNode(Other)));
		}
		TotalTime += Other->TotalTime;
		AllocSize += Other->AllocSize;
		GCSize += Other->GCSize;
	}
};
