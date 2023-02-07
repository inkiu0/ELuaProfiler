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

const void* FELuaMemSnapshot::Record(lua_State* L, const char* Desc, int32 Level, const void* Parent)
{
    const void* ObjAddress = lua_getaddr(L, -1);
    int32 Size = lua_sizeof(L, -1);
    const char* Type = lua_typename(L, lua_type(L, -1));

    TSharedPtr<FELuaMemInfoNode> pnode = GetMemNode(Parent);
    if (TSharedPtr<FELuaMemInfoNode> node = GetMemNode(ObjAddress))
    {
        if (node->parent != pnode)
        {
            node->count += 1;
        }
        //node->parent = pnode;
        node->desc = Desc;
        node->level = Level;
        node->parents.Add(Parent, pnode);
        lua_pop(L, 1);
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
        nnode->address = ObjAddress;
        nnode->type = Type;
        if (pnode)
        {
            nnode->parents.Add(Parent, pnode);
            pnode->children.Add(nnode);
        }
        if (!Root)
        {
            Root = nnode;
        }
        LuaObjectMemNodeMap.Add(ObjAddress, nnode);
    }
    return ObjAddress;					// continue to expanding this object
}

int32 FELuaMemSnapshot::RecountNode(TSharedPtr<FELuaMemInfoNode> Node)
{
    int32 Size = 0;
    if (Node)
    {
        if (Node->state == White)
        {
            Size = Node->size;
        }
        for (int32 i = 0; i < Node->children.Num(); i++)
        {
            Size += RecountNode(Node->children[i]);
        }
    }
    Node->totalsize = Size;
    return Size;
}

void FELuaMemSnapshot::Dump()
{
    const FString Content = DumpNode(Root);
    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("MemDump.txt"));
    FFileHelper::SaveStringToFile(Content, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

FString FELuaMemSnapshot::DumpNode(TSharedPtr<FELuaMemInfoNode> Node)
{
    FString Content;
    if (Node)
    {
        if (Node->state == White)
        {
            for (int32 i = 0; i < Node->level; ++i)
            {
                Content.AppendChar(TEXT(' '));
            }
            Content.Append(FString::Printf(TEXT(" %s %s %p"), *Node->desc, *Node->type, Node->address));
            for (const TPair<const void*, TSharedPtr<FELuaMemInfoNode>> Entry : Node->parents)
            {
                Content.Append(FString::Printf(TEXT(" | %s %p |"), *Node->desc, Entry.Value->address));
            }
            Content.AppendChar(TEXT('\n'));
        }

        for (int32 i = 0; i < Node->children.Num(); i++)
        {
            Content += DumpNode(Node->children[i]);
        }
    }

    return Content;
}

//int32 FELuaMemSnapshot::GetTotalSize()
//{
//	int32 size = 0;
//	for (TPair<const void*, TSharedPtr<FELuaMemInfoNode>> Entry : LuaObjectMemNodeMap)
//	{
//		TSharedPtr<FELuaMemInfoNode> node = Entry.Value;
//		if (node)
//		{
//			size += node->size;
//		}
//	}
//	Root->size = size;
//	return size;
//}

int32 FELuaMemSnapshot::RecountSize()
{
    /* travel all nodes*/
    TotalSize = RecountNode(Root);
    return TotalSize;
}

void FELuaMemSnapshot::GenTimeStamp()
{
    SnapTimeStr = FDateTime::Now().ToString(TEXT("%H:%M:%S"));	// %H:%M:%S.%s
}

void FELuaMemSnapshot::RecordLinkedList(const TSharedPtr<FELuaMemSnapshot> Snapshot, const TSharedPtr<FELuaMemInfoNode> InNewnode)
{
    if (Snapshot->LuaObjectMemNodeMap.Contains(InNewnode->address))
    {
        TSharedPtr<FELuaMemInfoNode> wnode = Snapshot->LuaObjectMemNodeMap[InNewnode->address];
        wnode->state = White;
    }
    else
    {
        TSharedPtr<FELuaMemInfoNode> onode = InNewnode;	// origin node
        TSharedPtr<FELuaMemInfoNode> nnode = TSharedPtr<FELuaMemInfoNode>(new FELuaMemInfoNode(*onode));
        Snapshot->LuaObjectMemNodeMap.Add(nnode->address, nnode);
        while (nnode && onode->parent)
        {
            if (!Snapshot->LuaObjectMemNodeMap.Contains(onode->parent->address))
            {
                TSharedPtr<FELuaMemInfoNode> parent = TSharedPtr<FELuaMemInfoNode>(new FELuaMemInfoNode(*onode->parent));
                parent->state = Gray;
                parent->children.Add(nnode);
                nnode->parent = parent;
                nnode->parents.Add(parent->address, parent);
                nnode = parent;
                onode = onode->parent;
                Snapshot->LuaObjectMemNodeMap.Add(nnode->address, nnode);
            }
            else
            {
                TSharedPtr<FELuaMemInfoNode> parent = Snapshot->LuaObjectMemNodeMap[onode->parent->address];
                parent->children.Add(nnode);
                nnode->parent = parent;
                nnode->parents.Add(parent->address, parent);
                nnode = nullptr;
            }
        }
    }
}

TSharedPtr<FELuaMemSnapshot> FELuaMemSnapshot::LogicOperate(const FELuaMemSnapshot& Other, ESnapshotOp ESOP)
{
    switch (ESOP)
    {
    case SOP_AND:
        return *this & Other;
    case SOP_OR:
        return *this | Other;
    case SOP_XOR:
        return *this ^ Other;
    case SOP_None:
    default:
        return nullptr;
    }
}

TSharedPtr<FELuaMemSnapshot> FELuaMemSnapshot::operator&(const FELuaMemSnapshot& Other)
{
    TSharedPtr<FELuaMemSnapshot> Snapshot = TSharedPtr<FELuaMemSnapshot>(new FELuaMemSnapshot());
    Snapshot->Root = TSharedPtr<FELuaMemInfoNode>(new FELuaMemInfoNode(*Root));
    Snapshot->LuaObjectMemNodeMap.Add(Snapshot->Root->address, Snapshot->Root);
    for (TPair<const void*, TSharedPtr<FELuaMemInfoNode>> Iter : LuaObjectMemNodeMap)
    {
        if (Other.LuaObjectMemNodeMap.Contains(Iter.Key))
        {
            RecordLinkedList(Snapshot, Iter.Value);
        }
    }
    Snapshot->GenTimeStamp();
    Snapshot->RecountSize();
    return Snapshot;
}

TSharedPtr<FELuaMemSnapshot> FELuaMemSnapshot::operator|(const FELuaMemSnapshot& Other)
{
    TSharedPtr<FELuaMemSnapshot> Snapshot = TSharedPtr<FELuaMemSnapshot>(new FELuaMemSnapshot());
    Snapshot->Root = TSharedPtr<FELuaMemInfoNode>(new FELuaMemInfoNode(*Root));
    Snapshot->LuaObjectMemNodeMap.Add(Snapshot->Root->address, Snapshot->Root);
    for (TPair<const void*, TSharedPtr<FELuaMemInfoNode>> Iter : LuaObjectMemNodeMap)
    {
        RecordLinkedList(Snapshot, Iter.Value);
    }

    for (TPair<const void*, TSharedPtr<FELuaMemInfoNode>> Iter : Other.LuaObjectMemNodeMap)
    {
        if (!LuaObjectMemNodeMap.Contains(Iter.Key))
        {
            RecordLinkedList(Snapshot, Iter.Value);
        }
    }
    Snapshot->GenTimeStamp();
    Snapshot->RecountSize();
    return Snapshot;
}

TSharedPtr<FELuaMemSnapshot> FELuaMemSnapshot::operator^(const FELuaMemSnapshot& Other)
{
    TSharedPtr<FELuaMemSnapshot> Snapshot = TSharedPtr<FELuaMemSnapshot>(new FELuaMemSnapshot());
    Snapshot->Root = TSharedPtr<FELuaMemInfoNode>(new FELuaMemInfoNode(*Root));
    Snapshot->LuaObjectMemNodeMap.Add(Snapshot->Root->address, Snapshot->Root);
    for (TPair<const void*, TSharedPtr<FELuaMemInfoNode>> Iter : LuaObjectMemNodeMap)
    {
        if (!Other.LuaObjectMemNodeMap.Contains(Iter.Key))
        {
            RecordLinkedList(Snapshot, Iter.Value);
        }
    }

    for (TPair<const void*, TSharedPtr<FELuaMemInfoNode>> Iter : Other.LuaObjectMemNodeMap)
    {
        if (!LuaObjectMemNodeMap.Contains(Iter.Key))
        {
            RecordLinkedList(Snapshot, Iter.Value);
        }
    }
    Snapshot->GenTimeStamp();
    Snapshot->RecountSize();
    return Snapshot;
}
