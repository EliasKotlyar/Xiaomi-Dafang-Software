/*
 * Purpose: OSS memory block allocation and management routines.
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

typedef struct _oss_memblk_t oss_memblk_t;

extern oss_memblk_t *oss_global_memblk;

extern void *oss_memblk_malloc(oss_memblk_t **, int size);
extern void oss_memblk_free(oss_memblk_t **, void *addr);
extern void oss_memblk_unalloc(oss_memblk_t **);
