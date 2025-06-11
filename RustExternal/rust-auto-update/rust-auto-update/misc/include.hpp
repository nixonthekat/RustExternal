#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <TlHelp32.h>
#include <Zydis/Zydis.h>
#include <unordered_map>

#include "../memory/driver.hpp"
#include "../memory/zydis.hpp"

#include "../cheat/structures.h"
#include "../cheat/offsets.hpp"
#include "../cheat/decryption.hpp"
typedef unsigned int    uint32;
#define _DWORD uint32

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
#  define LOW_IND(x,part_type)   LAST_IND(x,part_type)
#  define HIGH_IND(x,part_type)  0
#else
#  define HIGH_IND(x,part_type)  LAST_IND(x,part_type)
#  define LOW_IND(x,part_type)   0
#endif

#define TEST_BITD(value, bit) (((value) & (1 << (bit))) != 0)
#define DWORDn(x, n)  (*((_DWORD*)&(x)+n))
#define LODWORD(x) DWORDn(x,LOW_IND(x,_DWORD))