#ifndef PCH_H
#define PCH_H

//----------------------------------
// 引用头文件
//----------------------------------
#define WIN32_LEAN_AND_MEAN
// Windows 头文件
#include <windows.h>
// C++标准库 头文件
#include <iostream>
#include <string>
#include <unordered_map>
#include <chrono>
#include <map>
#include <set>
#include <utility>
// 微软 Detours 库 头文件
#include "./Detours/include/detours.h"

//----------------------------------
// 基本类型定义
//----------------------------------
using VA = unsigned __int64;
using RVA = unsigned int;
template<typename Type>
using Ptr = Type*;
//----------------------------------
// 其他
//----------------------------------
#define REF(PTR)										(&PTR)
#define DEREF(PTR)										(*PTR)
#define OBJECT(TYPE, VALUE)								reinterpret_cast<TYPE>(VALUE)

#define POINTER(PTR_TYPE, PTR)							OBJECT(PTR_TYPE, PTR)
#define POINTER_ADD_OFFSET(TYPE, PTR, OFFSET)			POINTER(TYPE, POINTER(VA, PTR)+OFFSET)
#define CLASS_OBJECT(TYPE, THISPTR, OFFSET)				DEREF(POINTER_ADD_OFFSET(Ptr<TYPE>, THISPTR, OFFSET))
#define CLASS_VTABLE_OBJECT(TYPE, THISPTR, OFFSET)		DEREF(POINTER(Ptr<TYPE>, DEREF(POINTER(Ptr<VA>, THISPTR))+OFFSET))

template<typename T_RET, typename... Args>
T_RET Symcall(RVA sym_rva, Args... args) {
	using FnType = T_RET (*)(Args...);
	FnType p = POINTER_ADD_OFFSET(FnType, GetModuleHandle(NULL), sym_rva);
	return p(args...);
}
#define SYMCALL(RETURN_TYPE, SYM_RVA, ...)				(Symcall<RETURN_TYPE>(SYM_RVA, ##__VA_ARGS__))
#define SYM_OBJECT(TYPE, SYM_RVA)						DEREF(POINTER_ADD_OFFSET(Ptr<TYPE>, GetModuleHandle(NULL), SYM_RVA))
#endif //PCH_H