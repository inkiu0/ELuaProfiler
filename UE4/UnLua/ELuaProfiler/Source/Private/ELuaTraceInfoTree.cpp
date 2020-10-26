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

#include "ELuaTraceInfoTree.h"

FELuaTraceInfoTree::FELuaTraceInfoTree()
{

}

FELuaTraceInfoTree::~FELuaTraceInfoTree()
{
	Root = nullptr;
	CurNode = nullptr;
}

void FELuaTraceInfoTree::Init()
{
	FString RootName("Root");
	Root = TSharedPtr<FELuaTraceInfoNode>(new FELuaTraceInfoNode(nullptr, RootName, "Root", 0));
	Root->BeginInvoke();
	CurNode = Root;
}

void FELuaTraceInfoTree::OnHookCall(lua_State* L, lua_Debug* ar)
{
	if (Root)
	{
		lua_getinfo(L, "nS", ar);
		TSharedPtr<FELuaTraceInfoNode> Child = GetChild(ar);
		Child->BeginInvoke();
		CurNode = Child;
		++CurDepth;
	}
}

void FELuaTraceInfoTree::OnHookReturn()
{
	if (Root)
	{
		CurNode->EndInvoke();
		CurNode = CurNode->Parent;
		--CurDepth;
	}
}

TSharedPtr <FELuaTraceInfoNode> FELuaTraceInfoTree::GetChild(lua_Debug* ar)
{
	FString ID = FString::Printf(TEXT("%s:%d"), UTF8_TO_TCHAR(ar->short_src), ar->linedefined);
	TSharedPtr<FELuaTraceInfoNode> Child = CurNode->GetChild(ID);
	if (!Child)
	{
		Child = TSharedPtr<FELuaTraceInfoNode>(new FELuaTraceInfoNode(CurNode, ID, ar->name, ar->event));
		CurNode->AddChild(Child);
	}
	return Child;
}

void FELuaTraceInfoTree::CountSelfTime()
{
	if (Root)
	{
		Root->EndInvoke();
		CountNodeSelfTime(Root);
		Root->BeginInvoke();
		Root->Count -= 1;
	}
}

void FELuaTraceInfoTree::CountNodeSelfTime(TSharedPtr<FELuaTraceInfoNode> Node)
{
	if (Node)
	{
		Node->Children.Sort([](const TSharedPtr<FELuaTraceInfoNode>& A, const TSharedPtr <FELuaTraceInfoNode>& B) {
			return A->TotalTime > B->TotalTime;
		});

		// ifdef CORRECT_TIME sub profiler's own time overhead
		Node->SelfTime = Node->TotalTime - DEVIATION * Node->Count;
		for (int32 i = 0; i < Node->Children.Num(); i++)
		{
			Node->SelfTime -= Node->Children[i]->TotalTime;
			CountNodeSelfTime(Node->Children[i]);
		}
	}
}
