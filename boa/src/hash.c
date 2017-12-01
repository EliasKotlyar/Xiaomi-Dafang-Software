/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Copyright (C) 1996-1999 Larry Doolittle <ldoolitt@boa.org>
 *  Copyright (C) 1997-2004 Jon Nelson <jnelson@boa.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* $Id: hash.c,v 1.14.4.15 2005/02/22 14:11:29 jnelson Exp $*/

#include "boa.h"

#ifndef MAX_HASH_LENGTH
#define MAX_HASH_LENGTH 4
#endif

/*
 * There are two hash tables used, each with a key/value pair
 * stored in a hash_struct.  They are:
 *
 * mime_hashtable:
 *     key = file extension
 *   value = mime type
 *
 * passwd_hashtable:
 *     key = username
 *   value = home directory
 *
 */

struct _hash_struct_ {
    char *key;
    char *value;
    struct _hash_struct_ *next;
};

typedef struct _hash_struct_ hash_struct;

static hash_struct *mime_hashtable[MIME_HASHTABLE_SIZE];
static hash_struct *passwd_hashtable[PASSWD_HASHTABLE_SIZE];
void add_mime_type(const char *extension, const char *type);
static unsigned get_homedir_hash_value(const char *name);

#ifdef WANT_ICKY_HASH
static unsigned four_char_hash(const char *buf);
#define _boa_hash four_char_hash
#else
#ifdef WANT_SDBM_HASH
static unsigned sdbm_hash(const char *str);
#define _boa_hash sdbm_hash
#else
#ifdef WANT_DJB2_HASH
static unsigned djb2_hash(const char *str);
#define _boa_hash djb2_hash
#else
static unsigned fnv1a_hash(const char *str);
#define _boa_hash fnv1a_hash
#endif
#endif
#endif

static unsigned boa_hash(const char *str)
{
    if (str == NULL || str[0] == '\0') {
        log_error_time();
        fprintf(stderr,
                "Attempt to hash NULL or empty string! [boa_hash]!\n");
        return 0;
    }
    return _boa_hash(str);
}

#ifdef WANT_ICKY_HASH
static unsigned four_char_hash(const char *buf)
{
    unsigned int hash = (buf[0] +
                         (buf[1] ? buf[1] : 241 +
                          (buf[2] ? buf[2] : 251 +
                           (buf[3] ? buf[3] : 257))));
    DEBUG(DEBUG_HASH) {
        log_error_time();
        fprintf(stderr, "four_char_hash(%s) = %u\n", buf, hash);
    }
    return hash;
}

/* The next two hashes taken from
 * http://www.cs.yorku.ca/~oz/hash.html
 *
 * In my (admittedly) very brief testing, djb2_hash performed
 * very slightly better than sdbm_hash.
 */

#else
#ifdef WANT_SDBM_HASH
static unsigned sdbm_hash(const char *str)
{
    unsigned hash = 0;
    int c;
    short count = MAX_HASH_LENGTH;

    DEBUG(DEBUG_HASH) {
        log_error_time();
        fprintf(stderr, "sdbm_hash(%s) = ", str);
    }

    while ((c = *str++) && count--)
        hash = c + (hash << 6) + (hash << 16) - hash;

    DEBUG(DEBUG_HASH) {
        fprintf(stderr, "%u\n", hash);
    }
    return hash;
}
#else
#ifdef WANT_DJB2_HASH
static unsigned djb2_hash(const char *str)
{
    unsigned hash = 5381;
    int c;
    short count = MAX_HASH_LENGTH;

    DEBUG(DEBUG_HASH) {
        log_error_time();
        fprintf(stderr, "djb2_hash(%s) = ", str);
    }

    while ((c = *(str++)) && count--)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    DEBUG(DEBUG_HASH) {
        fprintf(stderr, "%u\n", hash);
    }
    return hash;
}
#else
/*
 * magic 32 bit FNV1a hash constants
 *
 * See: http://www.isthe.com/chongo/tech/comp/fnv/index.html
 */
#define OFFSET_BASIS 2166136261U
#define FNV_PRIME 16777619U
/*
 *
 * hash.c:152: warning: width of integer constant may change on other systems with -traditional
 */

static unsigned fnv1a_hash(const char *str)
{
    unsigned int hash = OFFSET_BASIS;
    short count = MAX_HASH_LENGTH;

    /*
     * compute FNV1a hash of the first file component
     */
    do {
        hash ^= *str++;
        hash *= FNV_PRIME;
    } while (*str != '\0' && count--);

    return hash;
}

#endif
#endif
#endif

/*
 * Name: hash_insert
 * Description: Adds a key/value pair to the provided hashtable
 */

static
hash_struct *hash_insert(hash_struct * table[], const unsigned int hash,
                         const char *key, const char *value)
{
    hash_struct *current, *trailer;

    if (!key) {
        log_error_time();
        fprintf(stderr, "Yipes! Null value sent as key! [hash_insert]!\n");
        return NULL;
    } else if (key[0] == '\0') {
        log_error_time();
        fprintf(stderr,
                "Attempt to hash NULL or empty key! [hash_insert]!\n");
        return NULL;
    }

    if (!value) {
        log_error_time();
        fprintf(stderr, "Yipes! Null value sent as value! [hash_insert]!\n");
        return NULL;
    } else if (value[0] == '\0') {
        log_error_time();
        fprintf(stderr,
                "Attempt to hash NULL or empty value! [hash_insert]!\n");
        return NULL;
    }

    /* should we bother to check table, hash, and key for NULL/0 ? */

    DEBUG(DEBUG_HASH) {
        fprintf(stderr,
                "Adding key \"%s\" for value \"%s\" (hash=%u)\n",
                key, value, hash);
    }

    current = table[hash];
    while (current) {
        if (!strcmp(current->key, key))
            return current;     /* don't add extension twice */
        if (current->next)
            current = current->next;
        else
            break;
    }

    /* not found */
    trailer = (hash_struct *) malloc(sizeof (hash_struct));
    if (trailer == NULL) {
        WARN("unable to allocate memory for hash_insert");
        return NULL;
    }

    trailer->key = strdup(key);
    trailer->value = strdup(value);
    trailer->next = NULL;

    if (trailer->key == NULL || trailer->value == NULL) {
        int errno_save = errno;

        if (trailer->key)
            free(trailer->key);
        if (trailer->value)
            free(trailer->value);
        free(trailer);
        trailer = NULL;
        errno = errno_save;
        WARN("allocated key or value is NULL");
        return NULL;
    }

    /* successfully allocated and built new hash structure */
    if (!current) {
        /* no entries for this hash value */
        table[hash] = trailer;
    } else {
        current->next = trailer;
    }

    return trailer;
}

static
hash_struct *hash_find(hash_struct * table[], const char *key,
                          const unsigned int hash)
{
    hash_struct *current;

    current = table[hash];

    if (!key) {
        log_error_time();
        fprintf(stderr, "Yipes! Null value sent as key! [hash_find]!\n");
        return NULL;
    } else if (key[0] == '\0') {
        log_error_time();
        fprintf(stderr,
                "Attempt to locate empty string in hash! [hash_find]!\n");
        return NULL;
    }

    while (current) {
        if (!strcmp(current->key, key)) /* hit */
            return current;
        current = current->next;
    }

    return NULL;
}

static
void hash_clear(hash_struct * table[], int size)
{
    int i;
    hash_struct *temp;
    for (i = 0; i < size; ++i) { /* these limits OK? */
        temp = table[i];
        while (temp) {
            hash_struct *temp_next;

            temp_next = temp->next;
            free(temp->key);
            free(temp->value);
            free(temp);

            temp = temp_next;
        }
        table[i] = NULL;
    }
}

void hash_show_stats(void)
{
    int i;
    hash_struct *temp;
    int total = 0;
    int count;

    for (i = 0; i < MIME_HASHTABLE_SIZE; ++i) { /* these limits OK? */
        if (mime_hashtable[i]) {
            count = 0;
            temp = mime_hashtable[i];
            while (temp) {
                temp = temp->next;
                ++count;
            }
#ifdef NOISY_SIGALRM
            log_error_time();
            fprintf(stderr, "mime_hashtable[%d] has %d entries\n",
                    i, count);
#endif
            total += count;
        }
    }
    log_error_time();
    fprintf(stderr, "mime_hashtable has %d total entries\n", total);

    total = 0;
    for (i = 0; i < PASSWD_HASHTABLE_SIZE; ++i) { /* these limits OK? */
        if (passwd_hashtable[i]) {
            temp = passwd_hashtable[i];
            count = 0;
            while (temp) {
                temp = temp->next;
                ++count;
            }
#ifdef NOISY_SIGALRM
            log_error_time();
            fprintf(stderr, "passwd_hashtable[%d] has %d entries\n",
                    i, count);
#endif
            total += count;
        }
    }

    log_error_time();
    fprintf(stderr, "passwd_hashtable has %d total entries\n", total);

}


/*******************************************************************/
/*******************************************************************/
/******* Generic Hash Functions Above, Specific Below **************/
/*******************************************************************/
/*******************************************************************/

/*
 * Name: add_mime_type
 * Description: Adds a key/value pair to the mime_hashtable
 */

void add_mime_type(const char *extension, const char *type)
{
    unsigned int hash;

    hash = get_mime_hash_value(extension);
    if (hash_insert(mime_hashtable, hash, extension, type) == NULL)
	DIE("Failed to hash_insert mime type.");
}

/*
 * Name: get_mime_hash_value
 *
 * Description: adds the ASCII values of the file extension letters
 * and mods by the hashtable size to get the hash value
 */

unsigned get_mime_hash_value(const char *extension)
{
    return boa_hash(extension) % MIME_HASHTABLE_SIZE;
}

/*
 * Name: get_mime_type
 *
 * Description: Returns the mime type for a supplied filename.
 * Returns default type if not found.
 */

char *get_mime_type(const char *filename)
{
    char *extension;
    hash_struct *current;

    unsigned int hash;

    if (filename == NULL) {
        log_error_time();
        fprintf(stderr,
                "Attempt to hash NULL string! [get_mime_type]\n");
        return default_type;
    } else if (filename[0] == '\0') {
        log_error_time();
        fprintf(stderr,
                "Attempt to hash empty string! [get_mime_type]\n");
        return default_type;
    }

    extension = strrchr(filename, '.');

    /* remember, extension points to the *last* '.' in the filename,
     * which may be NULL or look like:
     *  foo.bar
     *  foo. (in which case extension[1] == '\0')
     */
    /* extension[0] *can't* be NIL */
    if (!extension || extension[1] == '\0')
        return default_type;

    /* make sure we hash on the 'bar' not the '.bar' */
    ++extension;

    hash = get_mime_hash_value(extension);
    current = hash_find(mime_hashtable, extension, hash);
    return (current ? current->value : default_type);
}

/*
 * Name: get_homedir_hash_value
 *
 * Description: adds the ASCII values of the username letters
 * and mods by the hashtable size to get the hash value
 */

static unsigned get_homedir_hash_value(const char *name)
{
    return boa_hash(name) % PASSWD_HASHTABLE_SIZE;
}


/*
 * Name: get_home_dir
 *
 * Description: Returns a point to the supplied user's home directory.
 * Adds to the hashtable if it's not already present.
 *
 */

char *get_home_dir(const char *name)
{
    hash_struct *current;

    unsigned int hash;

    hash = get_homedir_hash_value(name);

    current = hash_find(passwd_hashtable, name, hash);

    if (!current) {
        /* not found */
        struct passwd *passwdbuf;

        passwdbuf = getpwnam(name);

        if (!passwdbuf)         /* does not exist */
            return NULL;

        current =
            hash_insert(passwd_hashtable, hash, name, passwdbuf->pw_dir);
    }

    return (current ? current->value : NULL);
}

void dump_mime(void)
{
    hash_clear(mime_hashtable, MIME_HASHTABLE_SIZE);
}

void dump_passwd(void)
{
    hash_clear(passwd_hashtable, PASSWD_HASHTABLE_SIZE);
}
