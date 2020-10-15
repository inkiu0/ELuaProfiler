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

#include "CoreMinimal.h"
#include "ELuaMemInfoNode.h"
#include "lobject.h"

class ELUAPROFILER_API FELuaMemSnapshot
{
public:
	FELuaMemSnapshot();
	~FELuaMemSnapshot();

public:
	/* recount size of all the nodes */
	int32 RecountSize();

	/* get FELuaMemInfoNode by LuaObjAddress */
	TSharedPtr<FELuaMemInfoNode> GetMemNode(const void* LuaObjAddress);

	/* record the object at the top of luavm stack */
	const void* Record(lua_State* L, const char* Desc, int32 Level, const void* Parent);

	/* record the object at the top of luavm stack */
	const void* Record(const void* Address, const char* Type, int32 Size, const char* Desc, int Level, const void* Parent);

	//void Sort();
	//void Compare(const FELuaMemSnapshot& OtherSnapshoot);

private:
	/* accurately count the total size */
	int32 GetTotalSize();

	/* count the node size */
	int32 RecountNode(TSharedPtr<FELuaMemInfoNode> Node);

private:
	TSharedPtr<FELuaMemInfoNode> Root = nullptr;
	TMap<const void*, TSharedPtr<FELuaMemInfoNode>> LuaObjectMemNodeMap;
};
