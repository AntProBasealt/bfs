// Copyright © Tavian Barnes <tavianator@tavianator.com>
// SPDX-License-Identifier: 0BSD

/**
 * Asynchronous I/O queues.
 */

#ifndef BFS_IOQ_H
#define BFS_IOQ_H

#include <stddef.h>

/**
 * An queue of asynchronous I/O operations.
 */
struct ioq;

/**
 * I/O queue operations.
 */
enum ioq_op {
	/** ioq_close(). */
	IOQ_CLOSE,
	/** ioq_opendir(). */
	IOQ_OPENDIR,
	/** ioq_closedir(). */
	IOQ_CLOSEDIR,
};

/**
 * An I/O queue entry.
 */
struct ioq_ent {
	/** The I/O operation. */
	enum ioq_op op;

	/** The return value of the operation. */
	int ret;
	/** The error code, if the operation failed. */
	int error;

	/** Arbitrary user data. */
	void *ptr;

	/** Operation-specific arguments. */
	union {
		/** ioq_close() args. */
		struct ioq_close {
			int fd;
		} close;
		/** ioq_opendir() args. */
		struct ioq_opendir {
			struct bfs_dir *dir;
			int dfd;
			const char *path;
		} opendir;
		/** ioq_closedir() args. */
		struct ioq_closedir {
			struct bfs_dir *dir;
		} closedir;
	};
};

/**
 * Create an I/O queue.
 *
 * @param depth
 *         The maximum depth of the queue.
 * @param nthreads
 *         The maximum number of background threads.
 * @return
 *         The new I/O queue, or NULL on failure.
 */
struct ioq *ioq_create(size_t depth, size_t nthreads);

/**
 * Check the remaining capacity of a queue.
 */
size_t ioq_capacity(const struct ioq *ioq);

/**
 * Asynchronous close().
 *
 * @param ioq
 *         The I/O queue.
 * @param fd
 *         The fd to close.
 * @param ptr
 *         An arbitrary pointer to associate with the request.
 * @return
 *         0 on success, or -1 on failure.
 */
int ioq_close(struct ioq *ioq, int fd, void *ptr);

/**
 * Asynchronous bfs_opendir().
 *
 * @param ioq
 *         The I/O queue.
 * @param dir
 *         The allocated directory.
 * @param dfd
 *         The base file descriptor.
 * @param path
 *         The path to open, relative to dfd.
 * @param ptr
 *         An arbitrary pointer to associate with the request.
 * @return
 *         0 on success, or -1 on failure.
 */
int ioq_opendir(struct ioq *ioq, struct bfs_dir *dir, int dfd, const char *path, void *ptr);

/**
 * Asynchronous bfs_closedir().
 *
 * @param ioq
 *         The I/O queue.
 * @param dir
 *         The directory to close.
 * @param ptr
 *         An arbitrary pointer to associate with the request.
 * @return
 *         0 on success, or -1 on failure.
 */
int ioq_closedir(struct ioq *ioq, struct bfs_dir *dir, void *ptr);

/**
 * Pop a response from the queue.
 *
 * @param ioq
 *         The I/O queue.
 * @return
 *         The next response, or NULL.
 */
struct ioq_ent *ioq_pop(struct ioq *ioq);

/**
 * Pop a response from the queue, without blocking.
 *
 * @param ioq
 *         The I/O queue.
 * @return
 *         The next response, or NULL.
 */
struct ioq_ent *ioq_trypop(struct ioq *ioq);

/**
 * Free a queue entry.
 *
 * @param ioq
 *         The I/O queue.
 * @param ent
 *         The entry to free.
 */
void ioq_free(struct ioq *ioq, struct ioq_ent *ent);

/**
 * Cancel any pending I/O operations.
 */
void ioq_cancel(struct ioq *ioq);

/**
 * Stop and destroy an I/O queue.
 */
void ioq_destroy(struct ioq *ioq);

#endif // BFS_IOQ_H
