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

#include "ELuaMonitor.h"

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
    Root = MakeShared<FELuaTraceInfoNode>(nullptr, RootName);
    CurNode = Root;
}

void FELuaTraceInfoTree::OnHookCall(lua_State* L, void const* p, FString ID, bool IsStatistics/* = false */)
{
    if (Root)
    {
        TSharedPtr<FELuaTraceInfoNode> Child = GetChild(p, ID);
        if (Child->IsTwigs())
        {
			return FELuaMonitor::GetInstance()->OnPurning(p);
        }

        if (Child->Parent == Root)
        {
            Root->FakeBeginInvoke();
        }
        Child->BeginInvoke();
        CurNode = Child;
        ++CurDepth;
    }
}

void FELuaTraceInfoTree::OnHookReturn()
{
    if (Root && CurNode)
    {
        CurNode->EndInvoke();
        CurNode = CurNode->Parent;
        --CurDepth;
        if (CurNode == Root)
        {
            Root->FakeEndInvoke();
        }
    }
}

TSharedPtr <FELuaTraceInfoNode> FELuaTraceInfoTree::GetChild(void const* p, FString ID)
{
    TSharedPtr<FELuaTraceInfoNode> Child = CurNode->GetChild(p);
    if (!Child)
    {
        Child = TSharedPtr<FELuaTraceInfoNode>(new FELuaTraceInfoNode(CurNode, ID));
        CurNode->AddChild(p, Child);
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
        CurNode = Root;
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
                return A->SelfTime == B->SelfTime ? A->CallTime < B->CallTime : A->SelfTime > B->SelfTime;
                break;
            case Average:
                return A->Average == B->Average ? A->CallTime < B->CallTime : A->Average > B->Average;
                break;
            case Alloc:
                return A->AllocSize == B->AllocSize ? A->CallTime < B->CallTime : A->AllocSize > B->AllocSize;
                break;
            case GC:
                return A->GCSize == B->GCSize ? A->CallTime < B->CallTime : A->GCSize > B->GCSize;
                break;
            case Calls:
                return A->Count == B->Count ? A->CallTime < B->CallTime : A->Count > B->Count;
                break;
            case TotalTime:
            default:
                return A->TotalTime == B->TotalTime ? A->CallTime < B->CallTime : A->TotalTime > B->TotalTime;
                break;
            }
            return A->TotalTime == B->TotalTime ? A->CallTime < B->CallTime : A->TotalTime > B->TotalTime;
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
        for (TPair<void const*, TSharedPtr<FELuaTraceInfoNode>> Entry : Node->ChildPtrMap)
        {
            StatisticsNode->StatisticizeOtherNode(Entry.Key, Entry.Value);
            StatisticizeNode(Entry.Value, StatisticsNode);
        }
    }
}

TSharedPtr<FELuaTraceInfoNode> FELuaTraceInfoTree::Statisticize()
{
    TSharedPtr<FELuaTraceInfoNode> StatisticsNode = MakeShared<FELuaTraceInfoNode>(Root);
    StatisticizeNode(Root, StatisticsNode);
    return StatisticsNode;
}

void FELuaTraceInfoTree::Deserialize(TSharedPtr<FELuaTraceInfoNode>& TreeRoot)
{
    CurNode = TreeRoot;
}

FArchive& operator<<(FArchive& Ar, TSharedPtr<FELuaTraceInfoTree>& Tree)
{
    if (Ar.IsLoading())
    {
        Tree->Init();
        TSharedPtr<FELuaTraceInfoNode> TreeRoot = Tree->GetRoot();
        Ar << TreeRoot;
        Tree->Deserialize(TreeRoot);
    }
    else
    {
        TSharedPtr<FELuaTraceInfoNode> TreeRoot = Tree->GetRoot();
        if(TreeRoot.IsValid())
        {
			Ar << TreeRoot;
        }
    }
    return Ar;
}
