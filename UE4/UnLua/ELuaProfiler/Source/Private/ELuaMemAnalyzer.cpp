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

using namespace std;

#define ROOT 1
#define MARK 2

#define isdummy(t)		((t)->lastfree == NULL)

/* value at a non-valid index */
#define NONVALIDVALUE		nullptr//cast(TValue *, luaO_nilobject)

/* test for pseudo index */
#define ispseudo(i)		((i) <= LUA_REGISTRYINDEX)

FELuaMemAnalyzer::FELuaMemAnalyzer()
{
	mem_info_root = TSharedPtr<FELuaMemInfoNode>(new FELuaMemInfoNode());
}

FELuaMemAnalyzer::~FELuaMemAnalyzer()
{
	mem_info_root = nullptr;
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
	if (object_node_map.Contains(p))
	{
		return object_node_map[p];
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
		}
	}
	return p;			// continue to expanding this object
}

void FELuaMemAnalyzer::travel_table(lua_State* L, lua_State* dL, const char* desc, int level, const void* parent)
{
	luaL_checkstack(dL, LUA_MINSTACK, NULL);
	const void* p = record(L, desc, level, parent);
	if (p == NULL)
		return;

	bool weakk = false;
	bool weakv = false;
	if (lua_getmetatable(L, -1))
	{
		lua_pushliteral(L, "__mode");
		lua_rawget(L, -2);
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
		lua_pop(L, 1);

		luaL_checkstack(L, LUA_MINSTACK, NULL);
		travel_table(L, dL, "[table]metatable", level + 1, p);
	}

	lua_pushnil(L);
	while (lua_next(L, -2) != 0)
	{
		if (weakv)
		{
			lua_pop(L, 1);
		}
		else
		{
			char tmp[32];
			const char* desc = key_tostring(L, -2, tmp);
			travel_object(L, dL, desc, level + 1, p);
		}
		if (!weakk)
		{
			lua_pushvalue(L, -1);
			travel_object(L, dL, "key", level + 1, p);
		}
	}
	lua_pop(L, 1);
}

void FELuaMemAnalyzer::travel_userdata(lua_State* L, lua_State* dL, const char* desc, int level, const void* parent)
{
	luaL_checkstack(dL, LUA_MINSTACK, NULL);
	const void* p = record(L, desc, level, parent);
	if (p == NULL)
		return;
	if (lua_getmetatable(L, -1))
	{
		travel_table(L, dL, "[userdata]metatable", level + 1, p);
	}

	lua_getuservalue(L, -1);
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 2);
	}
	else
	{
		travel_table(L, dL, "uservalue", level + 1, p);
		lua_pop(L, 1);
	}
}

void FELuaMemAnalyzer::travel_function(lua_State* L, lua_State* dL, const char* desc, int level, const void* parent)
{
	luaL_checkstack(dL, LUA_MINSTACK, NULL);
	const void* p = record(L, desc, level, parent);
	if (p == NULL)
		return;

	int i;
	const char* name;
	for (i = 1;; i++)
	{
		name = lua_getupvalue(L, -1, i);
		if (name == NULL)
			break;
		travel_object(L, dL, name[0] ? name : "[upvalue]", level + 1, p);
	}

	if (lua_iscfunction(L, -1))
	{
		if (i == 1)
		{
			lua_rawgetp(dL, ROOT, p);
			lua_getfield(dL, -1, "infos");
			lua_pushstring(dL, "cfunction");
			lua_rawseti(dL, -2, 1);
			lua_setfield(dL, -2, "infos");
			lua_rawsetp(dL, ROOT, p);
		}
		lua_pop(L, 1);
	}
	else
	{
		lua_Debug ar;
		lua_getinfo(L, ">S", &ar);
		lua_rawgetp(dL, ROOT, p);
		lua_getfield(dL, -1, "infos");
		luaL_Buffer b;
		luaL_buffinit(dL, &b);
		luaL_addstring(&b, ar.short_src);
		char tmp[32];
		sprintf(tmp, ":%d~%d", ar.linedefined, ar.lastlinedefined);
		luaL_addstring(&b, tmp);
		luaL_pushresult(&b);
		lua_rawseti(dL, -2, 1);
		lua_setfield(dL, -2, "infos");
		lua_rawsetp(dL, ROOT, p);
	}
}

void FELuaMemAnalyzer::travel_thread(lua_State* L, lua_State* dL, const char* desc, int level, const void* parent)
{
	const void* p = record(L, desc, level, parent);
	if (p == NULL)
		return;
	int lv = 0;
	lua_State* cL = lua_tothread(L, -1);
	if (cL == L)
	{
		lv = 1;
	}
	else
	{
		int top = lua_gettop(cL);
		luaL_checkstack(cL, 1, NULL);
		int i;
		char tmp[32];
		for (i = 0; i < top; i++)
		{
			lua_pushvalue(cL, i + 1);
			sprintf(tmp, "[%d]", i + 1);
			travel_object(cL, dL, tmp, level + 1, p);
		}
	}

	lua_Debug ar;
	luaL_Buffer b;
	luaL_buffinit(dL, &b);
	while (lua_getstack(cL, lv, &ar))
	{
		char tmp[128];
		lua_getinfo(cL, "Sl", &ar);
		luaL_addstring(&b, ar.short_src);
		if (ar.linedefined >= 0)
		{
			char tmp[16];
			sprintf(tmp, ":%d~%d", ar.linedefined, ar.lastlinedefined);
			luaL_addstring(&b, tmp);
		}

		int i, j;
		for (i = 1; i > -1; i -= 2)
		{
			for (j = i;; j += i)
			{
				const char* name = lua_getlocal(cL, &ar, j);
				if (name == NULL)
					break;
				snprintf(tmp, sizeof(tmp), "%s : %s:%d", name, ar.short_src, ar.linedefined);
				travel_object(cL, dL, tmp, level + 1, p);
			}
		}
		++lv;
	}
	lua_rawgetp(dL, ROOT, p);
	lua_getfield(dL, -1, "infos");
	luaL_pushresult(&b);
	lua_rawseti(dL, -2, 1);
	lua_setfield(dL, -2, "infos");
	lua_rawsetp(dL, ROOT, p);
	lua_pop(L, 1);
}

void FELuaMemAnalyzer::travel_object(lua_State* L, lua_State* dL, const char* desc, int level, const void* parent)
{
	int t = lua_type(L, -1);
	switch (t)
	{
	case LUA_TTABLE:
		travel_table(L, dL, desc, level, parent);
		break;
	case LUA_TUSERDATA:
		travel_userdata(L, dL, desc, level, parent);
		break;
	case LUA_TFUNCTION:
		travel_function(L, dL, desc, level, parent);
		break;
	case LUA_TTHREAD:
		travel_thread(L, dL, desc, level, parent);
		break;
	default:
		lua_pop(L, 1);
		break;
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
	sizeofnode(mem_info_root);

	/* accurately count the total size */
	int32 size = 0;
	for (TPair<const void*, TSharedPtr<FELuaMemInfoNode>> Entry : object_node_map)
	{
		TSharedPtr<FELuaMemInfoNode> node = Entry.Value;
		if (node)
		{
			size += node->size;
		}
	}
	mem_info_root->size = size;
	return size;
}

void FELuaMemAnalyzer::snapshot(lua_State* L)
{
	lua_settop(L, 0);
	lua_pushglobaltable(L);

	travel_table(L, sL, "Global", 1, NULL);

	sizeoftree();
}
