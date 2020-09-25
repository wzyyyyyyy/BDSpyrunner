
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

#endif //PCH_H

