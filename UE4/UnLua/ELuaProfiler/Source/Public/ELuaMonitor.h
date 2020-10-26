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
#include "ELuaTraceInfoTree.h"

enum EMonitorState : uint32
{
	CREATED = 0,
	STARTED = 1 << 0,
	INITED = 1 << 1,
	RUNING = STARTED | INITED,
	PAUSE = 1 << 2,
	//PAUSE = 1 << 3,
};

class ELUAPROFILER_API FELuaMonitor
{
public:
	FELuaMonitor();
	~FELuaMonitor();

	static FELuaMonitor* GetInstance()
	{
		if (!SingletonInstance)
		{
			SingletonInstance = new FELuaMonitor();
		}
		return SingletonInstance;
	}

	void OnForward();

	void Tick(float DeltaTime);

	void SetMaxDepth(uint32 Depth) { MaxDepth = Depth; }

	TSharedPtr<FELuaTraceInfoNode> GetRoot(uint32 Index = 0);

	bool IsRuning() { return State == RUNING; }

	void LoadFile(const FString& Path);

	void PrintToFile(const FString& Path);
private:
	static void OnHook(lua_State* L, lua_Debug* ar);

	void OnHookCall(lua_State* L, lua_Debug* ar);

	void OnHookReturn(lua_State* L);

	void Init();

	void Start();

	void Stop();

	void Pause();

	void Resume();

private:
	/* max depth of hook  tracking */
	uint32 MaxDepth = 1;

	/* current depth of hook tracking */
	uint32 CurDepth = 0;

	TSharedPtr<FELuaTraceInfoTree> CurTraceTree;

	bool Started = false;

	bool Inited = false;

	uint32 State = CREATED;

	static FELuaMonitor* SingletonInstance;
};
