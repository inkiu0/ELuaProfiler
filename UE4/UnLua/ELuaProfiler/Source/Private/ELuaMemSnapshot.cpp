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

#include "ELuaMemSnapshot.h"

FELuaMemSnapshot::FELuaMemSnapshot()
{

}

FELuaMemSnapshot::~FELuaMemSnapshot()
{

}

TSharedPtr<FELuaMemInfoNode> FELuaMemSnapshot::GetMemNode(const void* LuaObjAddress)
{
	if (LuaObjectMemNodeMap.Contains(LuaObjAddress))
	{
		return LuaObjectMemNodeMap[LuaObjAddress];
	}
	else
	{
		return nullptr;
	}
}

const void* FELuaMemSnapshot::Record(const void* Address, const char* Type, int32 Size, const char* Desc, int Level, const void* Parent)
{
	if (TSharedPtr<FELuaMemInfoNode> pnode = GetMemNode(Parent))
	{
		if (TSharedPtr<FELuaMemInfoNode> node = GetMemNode(Address))
		{
			if (node->parent != pnode)
			{
				node->count += 1;
			}
			node->parent = pnode;
			node->desc = Desc;
			node->level = Level;
			node->parents.Add(Parent, pnode);
			return nullptr;				// stop expanding tree
		}
		else
		{
			TSharedPtr<FELuaMemInfoNode> nnode = TSharedPtr<FELuaMemInfoNode>(new FELuaMemInfoNode());
			nnode->parent = pnode;
			nnode->desc = Desc;
			nnode->level = Level;
			nnode->size = Size;
			nnode->count = 1;
			nnode->address = Address;
			nnode->type = Type;
			pnode->children.Add(nnode);
			LuaObjectMemNodeMap.Add(Address, nnode);
		}
	}
	return Address;					// continue to expanding this object
}

const void* FELuaMemSnapshot::Record(lua_State* L, const char* Desc, int32 Level, const void* Parent)
{
	int32 t = lua_type(L, -1);
	const void* ObjAddress = lua_topointer(L, -1);
	int32 Size = lua_sizeof(L, -1);
	const char* Type = lua_typename(L, t);

	return Record(ObjAddress, Type, Size, Desc, Level, Parent);
}

int32 FELuaMemSnapshot::RecountNode(TSharedPtr<FELuaMemInfoNode> Node)
{
	int32 Size = 0;
	if (Node)
	{
		Size = Node->size;
		for (int32 i = 0; i < Node->children.Num(); i++)
		{
			Size += RecountNode(Node->children[i]);
		}
	}
	return Size;
}

int32 FELuaMemSnapshot::GetTotalSize()
{
	int32 size = 0;
	for (TPair<const void*, TSharedPtr<FELuaMemInfoNode>> Entry : LuaObjectMemNodeMap)
	{
		TSharedPtr<FELuaMemInfoNode> node = Entry.Value;
		if (node)
		{
			size += node->size;
		}
	}
	Root->size = size;
	return size;
}

int32 FELuaMemSnapshot::RecountSize()
{
	/* travel all nodes*/
	RecountNode(Root);

	return GetTotalSize();
}
