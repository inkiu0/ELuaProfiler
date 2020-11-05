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

#include "ELuaMonitor.h"
#include "UnLuaBase.h"

FELuaMonitor::FELuaMonitor()
{
	CurTraceTree = TSharedPtr<FELuaTraceInfoTree>(new FELuaTraceInfoTree());
}

FELuaMonitor::~FELuaMonitor()
{
	CurTraceTree = nullptr;
}

void FELuaMonitor::Init()
{
	if (UnLua::GetState())
	{
		State |= INITED;
	}
	else
	{
		State &= (~INITED);
	}
	CurTraceTree->Init();
}

void FELuaMonitor::OnForward()
{
	if (State == RUNING)
	{
		Pause();
	}
	else if ((State & PAUSE) > 0)
	{
		Resume();
	}
	else
	{
		Start();
	}
}

void FELuaMonitor::Start()
{
	Init();
	if ((State & INITED) > 0)
	{
		lua_sethook(UnLua::GetState(), OnHook, ELuaProfiler::HookMask, 0);
	}
	State |= STARTED;
}

void FELuaMonitor::Stop()
{
	if (State == RUNING)
	{
		lua_sethook(UnLua::GetState(), nullptr, 0, 0);
	}
	State &= (~STARTED);
}

void FELuaMonitor::Pause()
{
	if (State == RUNING)
	{
		lua_sethook(UnLua::GetState(), nullptr, 0, 0);
	}
	State |= PAUSE;
}

void FELuaMonitor::Resume()
{
	if ((State & PAUSE) > 0)
	{
		State &= (~PAUSE);
		lua_sethook(UnLua::GetState(), OnHook, ELuaProfiler::HookMask, 0);
	}
}

void FELuaMonitor::OnClear()
{
	Stop();

	State = CREATED;

	FramesTraceTreeList.Empty();

	CurFrameIndex = 0;

	CurTraceTree = TSharedPtr<FELuaTraceInfoTree>(new FELuaTraceInfoTree());
}

/*static*/ void FELuaMonitor::OnHook(lua_State* L, lua_Debug* ar)
{
	switch (ar->event)
	{
	case LUA_HOOKCALL:
		FELuaMonitor::GetInstance()->OnHookCall(L, ar);
		break;
	case LUA_HOOKRET:
		FELuaMonitor::GetInstance()->OnHookReturn(L, ar);
		break;
	default:
		break;
	}
}

void FELuaMonitor::OnHookCall(lua_State* L, lua_Debug* ar)
{
	if (CurTraceTree)
	{
		if (CurDepth < MaxDepth)
		{
			CurTraceTree->OnHookCall(L, ar, MonitorMode == Statistics);
		}
		CurDepth++;
	}
}

void FELuaMonitor::OnHookReturn(lua_State* L, lua_Debug* ar)
{
	if (CurTraceTree)
	{
		if (CurDepth <= MaxDepth)
		{
			CurTraceTree->OnHookReturn(L, ar, MonitorMode == Statistics);
		}
		CurDepth--;
	}
}

TSharedPtr<FELuaTraceInfoNode> FELuaMonitor::GetRoot(uint32 FrameIndex /* = 0 */)
{
	if ((State & INITED) > 0)
	{
		if (MonitorMode == PerFrame)
		{
			int32 Index = GetCurFrameIndex() < GetTotalFrames() ? CurFrameIndex - 1 : CurFrameIndex - 2;
			return FramesTraceTreeList[Index]->GetRoot();
		}
		else
		{
			CurTraceTree->CountSelfTime();
			return CurTraceTree->GetRoot();
		}
	}
	return nullptr;
}

void FELuaMonitor::SetCurFrameIndex(int32 Index)
{
	if (Index > 0 && Index < GetTotalFrames())
	{
		CurFrameIndex = Index;
	}
	else
	{
		CurFrameIndex = GetTotalFrames();
	}
}

void FELuaMonitor::PerFrameModeUpdate(bool Manual /* = false */)
{
	CurTraceTree->CountSelfTime();
	FramesTraceTreeList.Add(CurTraceTree);
	CurTraceTree = TSharedPtr<FELuaTraceInfoTree>(new FELuaTraceInfoTree());
	CurTraceTree->Init();

	if (CurFrameIndex == FramesTraceTreeList.Num() - 1)
	{
		CurFrameIndex = FramesTraceTreeList.Num();
	}
}

void FELuaMonitor::Tick(float DeltaTime)
{
	if (State == RUNING)
	{
		// game stop
		if (!UnLua::GetState())
		{
			Stop();
		}

		if (MonitorMode == PerFrame && !ManualUpdated)
		{
			PerFrameModeUpdate();
		}
	}
	else if(State == STARTED)
	{
		// waiting game start
		if (UnLua::GetState())
		{
			OnForward();
		}
	}
}
