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
#define LUA_PROTO LUA_VPROTO
#define LUA_UPVAL LUA_VUPVAL
#define LUA_TABLE LUA_VTABLE
#define LUA_THREAD LUA_VTHREAD
#define LUA_USERDATA LUA_VUSERDATA
#define LUA_LIGHTUSERDATA LUA_VLIGHTUSERDATA
#define LUA_SHRSTR LUA_VSHRSTR
#define LUA_LNGSTR LUA_VLNGSTR
#define LUA_LCL LUA_VLCL
#define LUA_CCL LUA_VCCL
#define LUA_LCF LUA_VLCF
#else
#define GETTVALUE(v) v
#define tvtype(o) ttnov(o)
#define LUA_PROTO LUA_TPROTO
#define LUA_UPVAL LUA_TUPVAL
#define LUA_TABLE LUA_TTABLE
#define LUA_THREAD LUA_TTHREAD
#define LUA_USERDATA LUA_TUSERDATA
#define LUA_LIGHTUSERDATA LUA_TLIGHTUSERDATA
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
        return GETTVALUE(L->top + idx);
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

#if 504 == LUA_VERSION_NUM

int32 lua_sizeof(lua_State* L, int32 idx)
{
    TValue* t = lua_index2addr(L, idx);
    if (!t) return 0;

    GCObject* o = obj2gco(t);
    if (!o) return 0;

    switch (o->tt)
    {

    case LUA_TABLE:
    {
        luaL_checkstack(L, LUA_MINSTACK, NULL);
        Table* h = gco2t(o);
        unsigned int sizearray = 0;
        if (h->alimit > 0)
        {
            if (isrealasize(h))
            {
                sizearray = h->alimit;
            }
            else
            {
                unsigned int limit = h->alimit;
                limit |= (limit >> 1);
                limit |= (limit >> 2);
                limit |= (limit >> 4);
                limit |= (limit >> 8);
                limit |= (limit >> 16);
                limit++;
                sizearray = limit;
            }
        }

        return	(sizeof(h->node) * (1ULL << h->lsizenode) +
            sizeof(h->array) * sizearray +
            sizeof(h));
    }
    case LUA_LCL:
    {
        LClosure* cl = gco2lcl(o);
        return sizeLclosure(cl->nupvalues);
    }
    case LUA_CCL:
    {
        CClosure* cl = gco2ccl(o);
        return sizeCclosure(cl->nupvalues);
    }
    case LUA_TTHREAD:
    {
        lua_State* th = gco2th(o);

        return (sizeof(lua_State) + sizeof(lu_byte) * LUA_EXTRASPACE +
            sizeof(th->stack) * (stacksize(th) + EXTRA_STACK) +
            sizeof(th->ci) * th->nci);
        break;
    }
    case LUA_PROTO:
    {
        Proto* p = gco2p(o);
        return	sizeof(p) +
            sizeof(p->code) * p->sizecode +
            sizeof(p->p) * p->sizep +
            sizeof(p->k) * p->sizek +
            sizeof(p->lineinfo) * p->sizelineinfo +
            sizeof(p->abslineinfo) * p->sizeabslineinfo +
            sizeof(p->locvars) * p->sizelocvars +
            sizeof(p->upvalues) * p->sizeupvalues;
    }

    case LUA_USERDATA:
    {

        Udata* u = gco2u(o);
        return sizeudata(u->nuvalue, u->len);
    }
    case LUA_SHRSTR:
    {
        TString* ts = gco2ts(o);
        return sizelstring(ts->shrlen);
    }
    case LUA_LNGSTR:
    {
        TString* ts = gco2ts(o);
        return sizelstring(ts->u.lnglen);
    }
    case LUA_TNUMBER:
    {
        return sizeof(lua_Number);
    }
    case LUA_TBOOLEAN:
    {
        return sizeof(int);
    }
    case LUA_LIGHTUSERDATA:
    {
        return sizeof(void*);
    }
    default: return 0;
    }
}

#else

int32 lua_sizeof(lua_State* L, int32 idx)
{
    TValue* o = lua_index2addr(L, idx);
    if (!o)
        return 0;

    switch (tvtype(o))
    {

    case LUA_TABLE:
    {
        luaL_checkstack(L, LUA_MINSTACK, NULL);
        Table* h = hvalue(o);
        return (sizeof(Table) + sizeof(TValue) * h->sizearray +
            sizeof(Node) * (isdummy(h) ? 0 : sizenode(h)));
    }
    case LUA_LCL:
    {
        LClosure* cl = clLvalue(o);
        return sizeLclosure(cl->nupvalues);
    }
    case LUA_CCL:
    {
        CClosure* cl = clCvalue(o);
        return sizeCclosure(cl->nupvalues);
    }
    case LUA_TTHREAD:
    {
        lua_State* th = thvalue(o);

        return (sizeof(lua_State) + sizeof(TValue) * th->stacksize +
            sizeof(CallInfo) * th->nci);
    }
    case LUA_PROTO:
    {
        Proto* p = (Proto*)pvalue(o);
        return (sizeof(Proto) + 
                sizeof(Instruction) * p->sizecode +
                sizeof(Proto*) * p->sizep +
                sizeof(TValue) * p->sizek +
                sizeof(int) * p->sizelineinfo +
                sizeof(LocVar) * p->sizelocvars +
                sizeof(TString*) * p->sizeupvalues);
    }

    case LUA_USERDATA:
    {
        return sizeudata(uvalue(o));
    }
    case LUA_SHRSTR:
    {
        TString* ts = gco2ts(o);
        return sizelstring(ts->shrlen);
    }
    case LUA_LNGSTR:
    {
        TString* ts = gco2ts(o);
        return sizelstring(ts->u.lnglen);
    }
    case LUA_TNUMBER:
    {
        return sizeof(lua_Number);
    }
    case LUA_TBOOLEAN:
    {
        return sizeof(int);
    }
    case LUA_LIGHTUSERDATA:
    {
        return sizeof(void*);
    }
    default: return 0;
    }
}
#endif

int64 GetTimeNs()
{
	return Clock::now().time_since_epoch().count();
}

double GetTimeMs()
{
	return Clock::now().time_since_epoch().count() * 0.000001;
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
