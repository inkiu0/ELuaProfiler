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

#include "ELuaCommonUtil.h"
#include "UnLuaBase.h"
extern "C"
{
#include "lstate.h"
#include "lfunc.h"
#include "lstring.h"
}

#define isdummy(t)		((t)->lastfree == NULL)

/* value at a non-valid index */
#define NONVALIDVALUE		nullptr//cast(TValue *, luaO_nilobject)

/* test for pseudo index */
#define ispseudo(i)		((i) <= LUA_REGISTRYINDEX)

static TValue* lua_index2addr(lua_State* L, int idx)
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

int32 lua_sizeof(lua_State* L, int32 idx)
{
	TValue* o = lua_index2addr(L, idx);
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

int32 GetStateMemB()
{
	if (lua_State* L = UnLua::GetState())
	{
		return cast_int(gettotalbytes(G(L)));
	}
	return 0;
}

int32 GetStateMemKb()
{
	if (lua_State* L = UnLua::GetState())
	{
		return cast_int(gettotalbytes(G(L))) * 0.001;
	}
	return 0;
}
