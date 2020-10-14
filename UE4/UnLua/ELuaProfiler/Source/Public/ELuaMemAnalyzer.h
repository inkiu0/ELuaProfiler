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
#include "lua.hpp"
#include "lstate.h"
#include "lobject.h"
#include <string.h>

struct ELUAPROFILER_API FELuaMemInfoNode
{
	/* show name */
	FString name;
	/* detail description */
	FString desc;
	/* self size */
	int32 size;
	/* the depth of this node */
	int32 level;
	/* the reference count of this lua object */
	int32 count;
	/* the type name of this lua object */
	FString type;
	/* the address of lua object */
	const void* address = nullptr;
	/* last recorded parent node */
	TSharedPtr<FELuaMemInfoNode> parent = nullptr;
	/* all child nodes */
	TArray<TSharedPtr<FELuaMemInfoNode>> children;
	/* all parent nodes. a node may be referenced by multi other nodes */
	TMap<const void*, TSharedPtr<FELuaMemInfoNode>> parents;

	void Empty()
	{
		name.Empty();
		desc.Empty();
		size = 0;
		level = 0;
		count = 0;
		type = 0;
		address = nullptr;
		parent = nullptr;
		children.Empty();
	}
};

class ELUAPROFILER_API FELuaMemAnalyzer
{
public:
	FELuaMemAnalyzer();
	~FELuaMemAnalyzer();

private:
	static TValue* index2addr(lua_State* L, int idx);
	static size_t lua_sizeof(lua_State* L, int idx);

	TSharedPtr<FELuaMemInfoNode> getnode(const void* p);
	const char* key_tostring(lua_State* L, int index, char* buffer);
	const void* record(lua_State* L, const char* desc, int level, const void* parent);

	/* count the node size */
	int32 sizeofnode(TSharedPtr<FELuaMemInfoNode> node);
	/* count total size */
	int32 sizeoftree();

	void travel_table(lua_State* L, lua_State* dL, const char* desc, int level, const void* parent);
	void travel_userdata(lua_State* L, lua_State* dL, const char* desc, int level, const void* parent);
	void travel_function(lua_State* L, lua_State* dL, const char* desc, int level, const void* parent);
	void travel_thread(lua_State* L, lua_State* dL, const char* desc, int level, const void* parent);

public:
	void travel_object(lua_State* L, lua_State* dL, const char* desc, int level, const void* parent);
	
	void snapshot(lua_State* L);

private:
	lua_State* sL = nullptr;
	TSharedPtr<FELuaMemInfoNode> mem_info_root;
	TMap<const void*, TSharedPtr<FELuaMemInfoNode>> object_node_map;
};
