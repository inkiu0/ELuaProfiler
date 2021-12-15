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
	Root = TSharedPtr<FELuaTraceInfoNode>(new FELuaTraceInfoNode(nullptr, RootName, TEXT("Root"), 0));
	CurNode = Root;
}

void FELuaTraceInfoTree::OnHookCall(lua_State* L, lua_Debug* ar, bool IsStatistics/* = false */)
{
	if (Root)
	{
		lua_getinfo(L, "nS", ar);
		TSharedPtr<FELuaTraceInfoNode> Child = GetChild(ar);
		if (Child->Parent == Root)
		{
			Root->FakeBeginInvoke();
		}
		Child->BeginInvoke();
		CurNode = Child;
		++CurDepth;
	}
}

void FELuaTraceInfoTree::OnHookReturn(lua_State* L, lua_Debug* ar, bool IsStatistics/* = false */)
{
	if (Root)
	{
		CurNode->EndInvoke();
		CurNode = CurNode->Parent;
		lua_getinfo(L, "nS", ar);
		--CurDepth;
		if (CurNode == Root)
		{
			Root->FakeEndInvoke();
		}
	}
}

TSharedPtr <FELuaTraceInfoNode> FELuaTraceInfoTree::GetChild(lua_Debug* ar)
{
	TCHAR* Name = UTF8_TO_TCHAR(ar->name);
	FString ID = FString::Printf(TEXT("%s:%d~%d %s"), UTF8_TO_TCHAR(ar->short_src), ar->linedefined, ar->lastlinedefined, Name ? Name : TEXT("anonymous"));
	TSharedPtr<FELuaTraceInfoNode> Child = CurNode->GetChild(ID);
	if (!Child)
	{
		Child = TSharedPtr<FELuaTraceInfoNode>(new FELuaTraceInfoNode(CurNode, ID, Name, ar->event));
		CurNode->AddChild(Child);
	}
	return Child;
}

void FELuaTraceInfoTree::CountSelfTime(EMonitorSortMode SortMode)
{
	if (CurNode != Root)
	{
		// 递归统计尚未返回的函数
		TSharedPtr<FELuaTraceInfoNode> Node = CurNode;
		while (Node)
		{
			Node->FakeEndInvoke();
			Node = Node->Parent;
		}
	}

	CountNodeSelfTime(Root, SortMode);
}

void FELuaTraceInfoTree::CountNodeSelfTime(TSharedPtr<FELuaTraceInfoNode> Node, EMonitorSortMode SortMode)
{
	if (Node)
	{
		Node->Children.Sort([SortMode](const TSharedPtr<FELuaTraceInfoNode>& A, const TSharedPtr <FELuaTraceInfoNode>& B) {
			switch (SortMode)
			{
			case SelfTime:
				return A->SelfTime > B->SelfTime;
				break;
			case Average:
				return A->Average > B->Average;
				break;
			case Alloc:
				return A->AllocSize > B->AllocSize;
				break;
			case GC:
				return A->GCSize < B->GCSize;
				break;
			case Calls:
				return A->Count > B->Count;
				break;
			case TotalTime:
			default:
				return A->TotalTime > B->TotalTime;
				break;
			}
			return A->TotalTime > B->TotalTime;
		});

		// ifdef CORRECT_TIME sub profiler's own time overhead
		Node->SelfTime = Node->TotalTime - DEVIATION * Node->Count;
		Node->Average = Node->Count > 0 ? Node->TotalTime / Node->Count : 0;
		for (int32 i = 0; i < Node->Children.Num(); i++)
		{
			Node->SelfTime -= Node->Children[i]->TotalTime;
			CountNodeSelfTime(Node->Children[i], SortMode);
		}
	}
}

void FELuaTraceInfoTree::StatisticizeNode(TSharedPtr<FELuaTraceInfoNode> Node, TSharedPtr<FELuaTraceInfoNode> StatisticsNode)
{
	if (Node)
	{
		for (int32 i = 0; i < Node->Children.Num(); i++)
		{
			StatisticsNode->StatisticizeOtherNode(Node->Children[i]);
			StatisticizeNode(Node->Children[i], StatisticsNode);
		}
	}
}

TSharedPtr<FELuaTraceInfoNode> FELuaTraceInfoTree::Statisticize()
{
	TSharedPtr<FELuaTraceInfoNode> StatisticsNode = TSharedPtr<FELuaTraceInfoNode>(new FELuaTraceInfoNode(Root));
	StatisticizeNode(Root, StatisticsNode);
	return StatisticsNode;
}
