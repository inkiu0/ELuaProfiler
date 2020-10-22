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

FELuaMonitor* FELuaMonitor::SingletonInstance = nullptr;

void FELuaMonitor::Start()
{
	lua_sethook(UnLua::GetState(), OnHook, ELuaProfiler::HookMask, 0);
}

void FELuaMonitor::Stop()
{
	lua_sethook(UnLua::GetState(), nullptr, 0, 0);
}

void FELuaMonitor::Pause()
{
	lua_sethook(UnLua::GetState(), nullptr, 0, 0);
}

void FELuaMonitor::Resume()
{
	lua_sethook(UnLua::GetState(), OnHook, ELuaProfiler::HookMask, 0);
}

/*static*/ void FELuaMonitor::OnHook(lua_State* L, lua_Debug* ar)
{
	switch (ar->event)
	{
	case LUA_HOOKCALL:
		FELuaMonitor::GetInstance()->OnHookCall(L, ar);
		break;
	case LUA_MASKRET:
		FELuaMonitor::GetInstance()->OnHookReturn(L);
		break;
	default:
		break;
	}
}

void FELuaMonitor::OnHookCall(lua_State* L, lua_Debug* ar)
{
	if (CurDepth < MaxDepth)
	{
		CurTraceTree->OnHookCall(L, ar);
	}
	CurDepth++;
}

void FELuaMonitor::OnHookReturn(lua_State* L)
{
	if (CurDepth <= MaxDepth && !CurTraceTree->IsOnRoot())
	{
		CurTraceTree->OnHookReturn();
	}
	CurDepth--;
}
