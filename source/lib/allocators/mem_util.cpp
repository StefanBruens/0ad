/**
 * =========================================================================
 * File        : mem_util.cpp
 * Project     : 0 A.D.
 * Description : memory allocator helper routines.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "mem_util.h"

#include "lib/bits.h"				// round_up
#include "lib/posix/posix_mman.h"
#include "lib/sysdep/os_cpu.h"			// os_cpu_PageSize


bool mem_IsPageMultiple(uintptr_t x)
{
	return (x & (os_cpu_PageSize()-1)) == 0;
}

size_t mem_RoundUpToPage(size_t size)
{
	return round_up(size, os_cpu_PageSize());
}

size_t mem_RoundUpToAlignment(size_t size)
{
	// all allocators should align to at least this many bytes:
	const size_t alignment = 8;
	return round_up(size, alignment);
}


//-----------------------------------------------------------------------------

static inline LibError LibError_from_mmap(void* ret, bool warn_if_failed = true)
{
	if(ret != MAP_FAILED)
		return INFO::OK;
	return LibError_from_errno(warn_if_failed);
}

// "anonymous" effectively means mapping /dev/zero, but is more efficient.
// MAP_ANONYMOUS is not in SUSv3, but is a very common extension.
// unfortunately, MacOS X only defines MAP_ANON, which Solaris says is
// deprecated. workaround there: define MAP_ANONYMOUS in terms of MAP_ANON.
#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

static const int mmap_flags = MAP_PRIVATE|MAP_ANONYMOUS;

LibError mem_Reserve(size_t size, u8** pp)
{
	errno = 0;
	void* ret = mmap(0, size, PROT_NONE, mmap_flags|MAP_NORESERVE, -1, 0);
	*pp = (u8*)ret;
	return LibError_from_mmap(ret);
}

LibError mem_Release(u8* p, size_t size)
{
	errno = 0;
	int ret = munmap(p, size);
	return LibError_from_posix(ret);
}

LibError mem_Commit(u8* p, size_t size, int prot)
{
	// avoid misinterpretation by mmap.
	if(prot == PROT_NONE)
		WARN_RETURN(ERR::INVALID_PARAM);

	errno = 0;
	void* ret = mmap(p, size, prot, mmap_flags|MAP_FIXED, -1, 0);
	return LibError_from_mmap(ret);
}

LibError mem_Decommit(u8* p, size_t size)
{
	errno = 0;
	void* ret = mmap(p, size, PROT_NONE, mmap_flags|MAP_NORESERVE|MAP_FIXED, -1, 0);
	return LibError_from_mmap(ret);
}

LibError mem_Protect(u8* p, size_t size, int prot)
{
	errno = 0;
	int ret = mprotect(p, size, prot);
	return LibError_from_posix(ret);
}


//-----------------------------------------------------------------------------

// "freelist" is a pointer to the first unused element (0 if there are none);
// its memory holds a pointer to the next free one in list.

void mem_freelist_AddToFront(void*& freelist, void* el)
{
	debug_assert(el != 0);

	void* prev_el = freelist;
	freelist = el;
	*(void**)el = prev_el;
}


void* mem_freelist_Detach(void*& freelist)
{
	void* el = freelist;
	// nothing in list
	if(!el)
		return 0;
	freelist = *(void**)el;
	return el;
}
