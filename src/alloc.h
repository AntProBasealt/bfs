// Copyright © Tavian Barnes <tavianator@tavianator.com>
// SPDX-License-Identifier: 0BSD

/**
 * Memory allocation.
 */

#ifndef BFS_ALLOC_H
#define BFS_ALLOC_H

#include "config.h"
#include <stddef.h>

/** Round down to a multiple of an alignment. */
static inline size_t align_floor(size_t align, size_t size) {
	return size & ~(align - 1);
}

/** Round up to a multiple of an alignment. */
static inline size_t align_ceil(size_t align, size_t size) {
	return align_floor(align, size + align - 1);
}

/**
 * Saturating array size.
 *
 * @param align
 *         Array element alignment.
 * @param size
 *         Array element size.
 * @param count
 *         Array element count.
 * @return
 *         size * count, saturating to the maximum aligned value on overflow.
 */
static inline size_t array_size(size_t align, size_t size, size_t count) {
	size_t ret = size * count;
	return ret / size == count ? ret : ~(align - 1);
}

/** Saturating array sizeof. */
#define sizeof_array(type, count) \
	array_size(alignof(type), sizeof(type), count)

/** Size of a struct/union field. */
#define sizeof_member(type, member) \
	sizeof(((type *)NULL)->member)

/**
 * Saturating flexible struct size.
 *
 * @param align
 *         Struct alignment.
 * @param min
 *         Minimum struct size.
 * @param offset
 *         Flexible array member offset.
 * @param size
 *         Flexible array element size.
 * @param count
 *         Flexible array element count.
 * @return
 *         The size of the struct with count flexible array elements.  Saturates
 *         to the maximum aligned value on overflow.
 */
static inline size_t flex_size(size_t align, size_t min, size_t offset, size_t size, size_t count) {
	size_t ret = size * count;
	size_t overflow = ret / size != count;

	size_t extra = offset + align - 1;
	ret += extra;
	overflow |= ret < extra;
	ret |= -overflow;
	ret = align_floor(align, ret);

	// Make sure flex_sizeof(type, member, 0) >= sizeof(type), even if the
	// type has more padding than necessary for alignment
	if (min > align_ceil(align, offset)) {
		ret = ret < min ? min : ret;
	}

	return ret;
}

/**
 * Computes the size of a flexible struct.
 *
 * @param type
 *         The type of the struct containing the flexible array.
 * @param member
 *         The name of the flexible array member.
 * @param count
 *         The length of the flexible array.
 * @return
 *         The size of the struct with count flexible array elements.  Saturates
 *         to the maximum aligned value on overflow.
 */
#define sizeof_flex(type, member, count) \
	flex_size(alignof(type), sizeof(type), offsetof(type, member), sizeof_member(type, member[0]), count)

/**
 * General memory allocator.
 *
 * @param align
 *         The required alignment.
 * @param size
 *         The size of the allocation.
 * @return
 *         The allocated memory, or NULL on failure.
 */
void *alloc(size_t align, size_t size);

/**
 * Zero-initialized memory allocator.
 *
 * @param align
 *         The required alignment.
 * @param size
 *         The size of the allocation.
 * @return
 *         The allocated memory, or NULL on failure.
 */
void *zalloc(size_t align, size_t size);

/** Allocate memory for the given type. */
#define ALLOC(type) \
	(type *)alloc(alignof(type), sizeof(type))

/** Allocate zeroed memory for the given type. */
#define ZALLOC(type) \
	(type *)zalloc(alignof(type), sizeof(type))

/** Allocate memory for an array. */
#define ALLOC_ARRAY(type, count) \
	(type *)alloc(alignof(type), sizeof_array(type, count));

/** Allocate zeroed memory for an array. */
#define ZALLOC_ARRAY(type, count) \
	(type *)zalloc(alignof(type), sizeof_array(type, count));

/** Allocate memory for a flexible struct. */
#define ALLOC_FLEX(type, member, count) \
	(type *)alloc(alignof(type), sizeof_flex(type, member, count))

/** Allocate zeroed memory for a flexible struct. */
#define ZALLOC_FLEX(type, member, count) \
	(type *)zalloc(alignof(type), sizeof_flex(type, member, count))

/**
 * An arena allocator for fixed-size types.
 *
 * Arena allocators are intentionally not thread safe.
 */
struct arena {
	/** The list of free chunks. */
	void *chunks;
	/** The number of allocated slabs. */
	size_t nslabs;
	/** The array of slabs. */
	void **slabs;
	/** Chunk alignment. */
	size_t align;
	/** Chunk size. */
	size_t size;
};

/**
 * Initialize an arena for chunks of the given size and alignment.
 */
void arena_init(struct arena *arena, size_t align, size_t size);

/**
 * Initialize an arena for the given type.
 */
#define ARENA_INIT(arena, type) \
	arena_init((arena), alignof(type), sizeof(type))

/**
 * Allocate an object out of the arena.
 */
void *arena_alloc(struct arena *arena);

/**
 * Free an object from the arena.
 */
void arena_free(struct arena *arena, void *ptr);

/**
 * Destroy an arena, freeing all allocations.
 */
void arena_destroy(struct arena *arena);

/**
 * An arena allocator for flexibly-sized types.
 */
struct varena {
	/** The alignment of the struct. */
	size_t align;
	/** The offset of the flexible array. */
	size_t offset;
	/** The size of the flexible array elements. */
	size_t size;
	/** Shift amount for the smallest size class. */
	size_t shift;
	/** The number of arenas of different sizes. */
	size_t narenas;
	/** The array of differently-sized arenas. */
	struct arena *arenas;
};

/**
 * Initialize a varena for a struct with the given layout.
 *
 * @param varena
 *         The varena to initialize.
 * @param align
 *         alignof(type)
 * @param min
 *         sizeof(type)
 * @param offset
 *         offsetof(type, flexible_array)
 * @param size
 *         sizeof(flexible_array[i])
 */
void varena_init(struct varena *varena, size_t align, size_t min, size_t offset, size_t size);

/**
 * Initialize a varena for the given type and flexible array.
 *
 * @param varena
 *         The varena to initialize.
 * @param type
 *         A struct type containing a flexible array.
 * @param member
 *         The name of the flexible array member.
 */
#define VARENA_INIT(varena, type, member) \
	varena_init(varena, alignof(type), sizeof(type), offsetof(type, member), sizeof_member(type, member[0]))

/**
 * Arena-allocate a flexible struct.
 *
 * @param varena
 *         The varena to allocate from.
 * @param count
 *         The length of the flexible array.
 * @return
 *         The allocated struct, or NULL on failure.
 */
void *varena_alloc(struct varena *varena, size_t count);

/**
 * Resize a flexible struct.
 *
 * @param varena
 *         The varena to allocate from.
 * @param ptr
 *         The object to resize.
 * @param old_count
 *         The old array lenth.
 * @param new_count
 *         The new array length.
 * @return
 *         The resized struct, or NULL on failure.
 */
void *varena_realloc(struct varena *varena, void *ptr, size_t old_count, size_t new_count);

/**
 * Free an arena-allocated flexible struct.
 *
 * @param varena
 *         The that allocated the object.
 * @param ptr
 *         The object to free.
 * @param count
 *         The length of the flexible array.
 */
void varena_free(struct varena *varena, void *ptr, size_t count);

/**
 * Destroy a varena, freeing all allocations.
 */
void varena_destroy(struct varena *varena);

#endif // BFS_ALLOC_H
