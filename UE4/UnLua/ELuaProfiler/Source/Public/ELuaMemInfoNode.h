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

	//FELuaMemInfoNode(const FELuaMemInfoNode& Copy)
	//{
	//	name = Copy.name;
	//	desc = Copy.desc;
	//	size = Copy.size;
	//	level = Copy.level;
	//	count = Copy.count;
	//	type = Copy.type;
	//	address = Copy.address;
	//	parent = Copy.parent;
	//	children = Copy.children;
	//}

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
