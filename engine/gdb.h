#pragma once
#ifdef NDEBUG
	#define GDB_CALLABLE
#else
	#define GDB_CALLABLE __attribute__((noinline, used))
#endif