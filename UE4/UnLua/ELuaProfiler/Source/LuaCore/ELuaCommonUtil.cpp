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

#if 504 == LUA_VERSION_NUM
#define GETTVALUE(v) &((v)->val)
#define tvtype(o) ttype(o)
#define LUA_SHRSTR LUA_VSHRSTR
#define LUA_LNGSTR LUA_VLNGSTR
#define LUA_LCL LUA_VLCL
#define LUA_CCL LUA_VCCL
#define LUA_LCF LUA_VLCF
#else
#define GETTVALUE(v) v
#define tvtype(o) ttnov(o)
#define LUA_SHRSTR LUA_TSHRSTR
#define LUA_LNGSTR LUA_TLNGSTR
#define LUA_LCL LUA_TLCL
#define LUA_CCL LUA_TCCL
#define LUA_LCF LUA_TLCF
#endif

static TValue* lua_index2addr(lua_State* L, int idx)
{
	CallInfo* ci = L->ci;
	if (idx > 0)
	{
		TValue* o = GETTVALUE(ci->func + idx);
		api_check(L, idx <= ci->top - (ci->func + 1), "unacceptable index");
		if (o >= GETTVALUE(L->top)) return NONVALIDVALUE;
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
		if (ttislcf(GETTVALUE(ci->func)))
			return NONVALIDVALUE;
		else
		{
			CClosure* func = clCvalue(GETTVALUE(ci->func));
			return (idx <= func->nupvalues) ? &func->upvalue[idx - 1] : NONVALIDVALUE;
		}
	}
}

const void* lua_getaddr(lua_State* L, int32 idx)
{
	TValue* o = lua_index2addr(L, idx);
	if (!o)
		return lua_topointer(L, -1);

	switch (tvtype(o))
	{
	case LUA_TPROTO:
	{
		return pvalue(o);
	}
	case LUA_SHRSTR:
	{
		return tsvalue(o);
	}
	case LUA_LNGSTR:
	{
		return tsvalue(o);
	}
	//case LUA_TNUMBER:
	//{
	//	return (const void*)nvalue(o);
	//}
	//case LUA_TBOOLEAN:
	//{
	//	return sizeof(int);
	//}
	case LUA_TTABLE:
	case LUA_LCL:
	case LUA_CCL:
	case LUA_LCF:
	case LUA_TTHREAD:
	case LUA_TUSERDATA:
	case LUA_TLIGHTUSERDATA:
	default:
	{
		return lua_topointer(L, -1);
		break;
	}
	}
}

int32 lua_sizeof(lua_State* L, int32 idx)
{
	TValue* o = lua_index2addr(L, idx);
	if (!o)
		return 0;

	switch (tvtype(o))
	{

	case LUA_TTABLE:
	{
		luaL_checkstack(L, LUA_MINSTACK, NULL);
		Table* h = hvalue(o);
#if 504 == LUA_VERSION_NUM
		unsigned int sizearray = 0;
		if (h->alimit > 0)
		{
			if (isrealasize(h))
			{
				sizearray = h->alimit;
			}
			else
			{
				sizearray = 1;
				while(sizearray < h->alimit)
				{
					sizearray = sizearray << 1;
				}
			}
		}

		return (sizeof(Table) + sizeof(TValue) * sizearray +
			sizeof(Node) * (isdummy(h) ? 0 : sizenode(h)));
#else
		return (sizeof(Table) + sizeof(TValue) * h->sizearray +
			sizeof(Node) * (isdummy(h) ? 0 : sizenode(h)));
#endif
	}
	case LUA_LCL:
	{
		LClosure* cl = clLvalue(o);
		return sizeLclosure(cl->nupvalues);
		break;
	}
	case LUA_CCL:
	{
		CClosure* cl = clCvalue(o);
		return sizeCclosure(cl->nupvalues);
		break;
	}
	case LUA_TTHREAD:
	{
		lua_State* th = thvalue(o);

#if 504 == LUA_VERSION_NUM
		return (sizeof(lua_State) + sizeof(TValue) * stacksize(th) +
			sizeof(CallInfo) * th->nci);
#else
		return (sizeof(lua_State) + sizeof(TValue) * th->stacksize +
			sizeof(CallInfo) * th->nci);
#endif
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

#if 504 == LUA_VERSION_NUM
		Udata* u = gco2u(o);
		return sizeudata(u->nuvalue, u->len);
#else
		return sizeudata(uvalue(o));
#endif
	}
	case LUA_TSTRING:
	{
		return sizelstring(vslen(o));
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

float GetStateMemKb()
{
	if (lua_State* L = UnLua::GetState())
	{
		return cast_int(gettotalbytes(G(L))) * 0.001f;
	}
	return 0.f;
}
