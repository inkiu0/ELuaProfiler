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

#include "ELuaMemAnalyzer.h"
#include <stdio.h>
#include "lauxlib.h"
#include "lua.h"
#include "lfunc.h"
#include "lapi.h"
#include "lstring.h"

#define isdummy(t)		((t)->lastfree == NULL)

/* value at a non-valid index */
#define NONVALIDVALUE		nullptr//cast(TValue *, luaO_nilobject)

/* test for pseudo index */
#define ispseudo(i)		((i) <= LUA_REGISTRYINDEX)

FELuaMemAnalyzer::FELuaMemAnalyzer()
{

}

FELuaMemAnalyzer::~FELuaMemAnalyzer()
{
	cur_snapshoot_root = nullptr;
	snapshoots.Empty();
}

TValue* FELuaMemAnalyzer::index2addr(lua_State* L, int idx)
{
	CallInfo* ci = L->ci;
	if (idx > 0)
	{
		TValue* o = ci->func + idx;
		api_check(L, idx <= ci->top - (ci->func + 1), "unacceptable index");
		if (o >= L->top) return NONVALIDVALUE;
		else return o;
	}
	else if (!ispseudo(idx))
	{
		/* negative index */
		api_check(L, idx != 0 && -idx <= L->top - (ci->func + 1), "invalid index");
		return L->top + idx;
	}
	else if (idx == LUA_REGISTRYINDEX)
		return &G(L)->l_registry;
	else
	{
		/* upvalues */
		idx = LUA_REGISTRYINDEX - idx;
		api_check(L, idx <= MAXUPVAL + 1, "upvalue index too large");
		if (ttislcf(ci->func))
			return NONVALIDVALUE;
		else
		{
			CClosure* func = clCvalue(ci->func);
			return (idx <= func->nupvalues) ? &func->upvalue[idx - 1] : NONVALIDVALUE;
		}
	}
}

size_t FELuaMemAnalyzer::lua_sizeof(lua_State* L, int idx)
{
	TValue* o = index2addr(L, idx);
	if (!o)
		return 0;

	switch (ttnov(o))
	{

	case LUA_TTABLE:
	{
		luaL_checkstack(L, LUA_MINSTACK, NULL);
		Table* h = hvalue(o);
		return (sizeof(Table) + sizeof(TValue) * h->sizearray +
			sizeof(Node) * (isdummy(h) ? 0 : sizenode(h)));
		break;
	}
	case LUA_TLCL:
	{
		LClosure* cl = clLvalue(o);
		return sizeLclosure(cl->nupvalues);
		break;
	}
	case LUA_TCCL:
	{
		CClosure* cl = clCvalue(o);
		return sizeCclosure(cl->nupvalues);
		break;
	}
	case LUA_TTHREAD:
	{
		lua_State* th = thvalue(o);
		return (sizeof(lua_State) + sizeof(TValue) * th->stacksize +
			sizeof(CallInfo) * th->nci);
		break;
	}
	case LUA_TPROTO:
	{
		Proto* p = (Proto*)pvalue(o);
		return (sizeof(Proto) + sizeof(Instruction) * p->sizecode +
			sizeof(Proto*) * p->sizep +
			sizeof(TValue) * p->sizek +
			sizeof(int) * p->sizelineinfo +
			sizeof(LocVar) * p->sizelocvars +
			sizeof(TString*) * p->sizeupvalues);
		break;
	}

	case LUA_TUSERDATA:
	{
		return sizeudata(uvalue(o));
		break;
	}
	case LUA_TSHRSTR:
	{
		TString* ts = tsvalue(o);
		return sizelstring(ts->shrlen);
		break;
	}
	case LUA_TLNGSTR:
	{
		TString* ts = tsvalue(o);
		return sizelstring(ts->u.lnglen);
		break;
	}
	case LUA_TNUMBER:
	{
		return sizeof(lua_Number);
	}
	case LUA_TBOOLEAN:
	{
		return sizeof(int);
	}
	case LUA_TLIGHTUSERDATA:
	{
		return sizeof(void*);
	}
	default: return 0;
	}
}

TSharedPtr<FELuaMemInfoNode> FELuaMemAnalyzer::getnode(const void* p)
{
	if (cur_object_node_map.Contains(p))
	{
		return cur_object_node_map[p];
	} 
	else
	{
		return nullptr;
	}
}

const char* FELuaMemAnalyzer::key_tostring(lua_State* L, int index, char* buffer)
{
	int t = lua_type(L, index);
	switch (t)
	{
	case LUA_TSTRING:
		return lua_tostring(L, index);
	case LUA_TNUMBER:
		sprintf(buffer, "[%lg]", lua_tonumber(L, index));
		break;
	case LUA_TBOOLEAN:
		sprintf(buffer, "[%s]", lua_toboolean(L, index) ? "true" : "false");
		break;
	case LUA_TNIL:
		sprintf(buffer, "[nil]");
		break;
	default:
		sprintf(buffer, "[%s:%p]", lua_typename(L, t), lua_topointer(L, index));
		break;
	}
	return buffer;
}

const void* FELuaMemAnalyzer::record(lua_State* L, const char* desc, int level, const void* parent)
{
	int t = lua_type(L, -1);
	const void* p = lua_topointer(L, -1);
	int size = cast_int(lua_sizeof(L, -1));
	if (TSharedPtr<FELuaMemInfoNode> pnode = getnode(parent))
	{
		if (TSharedPtr<FELuaMemInfoNode> node = getnode(p))
		{
			if (node->parent != pnode)
			{
				node->count += 1;
			}
			node->parent = pnode;
			node->desc = desc;
			node->level = level;
			node->parents.Add(parent, pnode);
			return nullptr;		// stop expanding tree
		}
		else
		{
			TSharedPtr<FELuaMemInfoNode> nnode = TSharedPtr<FELuaMemInfoNode>(new FELuaMemInfoNode());
			nnode->parent = pnode;
			nnode->level = level;
			nnode->desc = desc;
			nnode->size = size;
			nnode->count = 1;
			nnode->address = p;
			nnode->type = lua_typename(L, t);
			pnode->children.Add(nnode);
			cur_object_node_map.Add(p, nnode);
		}
	}
	return p;			// continue to expanding this object
}

void FELuaMemAnalyzer::travel_table(lua_State* L, const char* desc, int level, const void* parent)
{
	const void* p = record(L, desc, level, parent);						// [table]
	if (p == NULL)
		return;

	bool weakk = false;
	bool weakv = false;
	if (lua_getmetatable(L, -1))										// [metatable, table] push metatable
	{
		lua_pushliteral(L, "__mode");									// ["__mode", metatable, table] push "__mode" string
		lua_rawget(L, -2);												// [metatable.__mode, metatable, table] pop "__mode"; push metatable.__mode
		if (lua_isstring(L, -1))
		{
			const char* mode = lua_tostring(L, -1);
			if (strchr(mode, 'k'))
			{
				weakk = true;
			}
			if (strchr(mode, 'v'))
			{
				weakv = true;
			}
		}
		lua_pop(L, 1);													// [metatable, table] pop metatable.__mode

		luaL_checkstack(L, LUA_MINSTACK, NULL);
		travel_table(L, "[table]metatable", level + 1, p);				// [table]  pop metatable
	}

	lua_pushnil(L);														// [nil, table] push nil as key slot on stack
	while (lua_next(L, -2) != 0)										// [(value, key), table]pop key slot; push (-2)key object and (-1)value object to the top of the stack if exists
	{
		if (weakv)
		{
			lua_pop(L, 1);												// [key, table] pop and skip weak value
		}
		else
		{
			char tmp[32];
			const char* desc = key_tostring(L, -2, tmp);
			travel_object(L, desc, level + 1, p);						// [key, table] travel and pop value object
		}
		if (!weakk)
		{
			lua_pushvalue(L, -1);										// [key, key, object] copy key slot to the top of the stack
			travel_object(L, "key", level + 1, p);						// [key, table] travel and pop key object
		}
	}																	// [table] last time call lua_net will pop key slot, do not push anything
	lua_pop(L, 1);														// [] pop table
}

void FELuaMemAnalyzer::travel_userdata(lua_State* L, const char* desc, int level, const void* parent)
{
	const void* p = record(L, desc, level, parent);						// [userdata]
	if (p == NULL)
		return;

	if (lua_getmetatable(L, -1))										// [metatable, userdata] push metatable
	{
		travel_table(L, "[userdata]metatable", level + 1, p);			// [userdata] pop metatable
	}

	lua_getuservalue(L, -1);											// [uservalue, userdata] push uservalue
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 2);													// [] pop userdata and uservalue on stack
	}
	else
	{
		travel_table(L, "uservalue", level + 1, p);						// [userdata] pop uservalue
		lua_pop(L, 1);													// [] pop userdata on stack
	}
}

void FELuaMemAnalyzer::travel_function(lua_State* L, const char* desc, int level, const void* parent)
{
	const void* p = record(L, desc, level, parent);						// [function]
	if (p == NULL)
		return;

	int i;
	const char* name;
	for (i = 1;; i++)
	{
		name = lua_getupvalue(L, -1, i);								// [upvalue, function] push upvalue
		if (name == NULL)
			break;
		travel_object(L, name[0] ? name : "[upvalue]", level + 1, p);	// [function] travel and pop upvalue
	}

	if (lua_iscfunction(L, -1))
	{
		if (i == 1)
		{
			/* update special description of lua function */
			update_node_desc(p, "cfunction");
		}
		lua_pop(L, 1);													// [] pop function on stack
	}
	else
	{
		lua_Debug ar;
		lua_getinfo(L, ">S", &ar);										// [] will pop function on stack
		char tmp[64];
		sprintf(tmp, "%s:%d~%d", ar.short_src, ar.linedefined, ar.lastlinedefined);
		/* update special description of lua function */
		update_node_desc(p, tmp);
	}
}

void FELuaMemAnalyzer::travel_thread(lua_State* L, const char* desc, int level, const void* parent)
{
	const void* p = record(L, desc, level, parent);						// [thread]
	if (p == NULL)
		return;

	int lv = 0;
	lua_State* cL = lua_tothread(L, -1);								// [thread] ¡ú [[...]]convert thread to state
	if (cL == L)
	{
		lv = 1;
	}
	else
	{
		int top = lua_gettop(cL);										// [{top}] confirm the depth of substack
		luaL_checkstack(cL, 1, NULL);
		int i;
		char tmp[32];
		for (i = 0; i < top; i++)
		{
			lua_pushvalue(cL, i + 1);									// [i_obj, {top}] copy i_obj on substack
			sprintf(tmp, "[%d]", i + 1);
			travel_object(cL, tmp, level + 1, p);						// [{top}] pop i_obj
		}
	}

	lua_Debug ar;
	while (lua_getstack(cL, lv, &ar))									// copy lv_function debuginfo to ar
	{
		char short_src[64];
		char tmp[128];
		lua_getinfo(cL, "Sl", &ar);										// [{top - lv}] pop function on substack
		sprintf(short_src, "%s", ar.short_src);
		if (ar.linedefined >= 0)
		{
			sprintf(tmp, "%s:%d~%d", short_src, ar.linedefined, ar.lastlinedefined);
		}

		int i, j;
		for (i = 1; i > -1; i -= 2)
		{
			for (j = i;; j += i)
			{
				const char* name = lua_getlocal(cL, &ar, j);			// [localvalue, {top - lv}] push local value on stack
				if (name == NULL)
					break;
				snprintf(tmp, sizeof(tmp), "%s : %s:%d", name, ar.short_src, ar.linedefined);
				travel_object(cL, tmp, level + 1, p);					// [{top - lv}] travel and pop localvalue
			}
		}
		++lv;
	}
	lua_pop(L, 1);														// [[...]] ¡ú [] pop thread on stack
}

void FELuaMemAnalyzer::travel_object(lua_State* L, const char* desc, int level, const void* parent)
{
	int t = lua_type(L, -1);											// [object]
	switch (t)
	{
	case LUA_TTABLE:
		travel_table(L, desc, level, parent);							// [] pop object
		break;
	case LUA_TUSERDATA:
		travel_userdata(L, desc, level, parent);						// [] pop object
		break;
	case LUA_TFUNCTION:
		travel_function(L, desc, level, parent);						// [] pop object
		break;
	case LUA_TTHREAD:
		travel_thread(L, desc, level, parent);							// [] pop object
		break;
	default:
		lua_pop(L, 1);													// [] pop object
		break;
	}
}

void FELuaMemAnalyzer::update_node_desc(const void* p, const char* desc)
{
	if (TSharedPtr<FELuaMemInfoNode> node = getnode(p))
	{
		node->desc = desc;
	}
}

int32 FELuaMemAnalyzer::sizeofnode(TSharedPtr<FELuaMemInfoNode> node)
{
	int32 size = node->size;
	if (node)
	{
		for (int32 i = 0; i < node->children.Num(); i++)
		{
			size += sizeofnode(node->children[i]);
		}
	}
	return size;
}

int32 FELuaMemAnalyzer::sizeoftree()
{
	/* travel all nodes*/
	sizeofnode(cur_snapshoot_root);

	/* accurately count the total size */
	int32 size = 0;
	for (TPair<const void*, TSharedPtr<FELuaMemInfoNode>> Entry : cur_object_node_map)
	{
		TSharedPtr<FELuaMemInfoNode> node = Entry.Value;
		if (node)
		{
			size += node->size;
		}
	}
	cur_snapshoot_root->size = size;
	return size;
}

void FELuaMemAnalyzer::snapshot(lua_State* L)
{
	lua_settop(L, 0);
	lua_pushglobaltable(L);

	CreateSnapshootRoot();

	travel_table(L, "Global", 1, NULL);

	sizeoftree();
}

TSharedPtr<FELuaMemInfoNode> FELuaMemAnalyzer::CreateSnapshootRoot()
{
	cur_object_node_map = TMap<const void*, TSharedPtr<FELuaMemInfoNode>>();
	cur_snapshoot_root = TSharedPtr<FELuaMemInfoNode>(new FELuaMemInfoNode());
	snapshoots.Add(cur_snapshoot_root);
	all_snapshoot_node_maps.Add(cur_snapshoot_root, cur_object_node_map);
	return cur_snapshoot_root;
}

void FELuaMemAnalyzer::popsnapshoot()
{
	if (snapshoots.Num() > 0)
	{
		TSharedPtr<FELuaMemInfoNode> poproot = snapshoots.Pop();
		all_snapshoot_node_maps.Remove(poproot);
	}

	if (snapshoots.Num() > 0)
	{
		cur_snapshoot_root = snapshoots[0];
		cur_object_node_map = all_snapshoot_node_maps[cur_snapshoot_root];
	}
	else
	{
		cur_snapshoot_root = nullptr;
		cur_object_node_map = TMap<const void*, TSharedPtr<FELuaMemInfoNode>>();
	}
}
