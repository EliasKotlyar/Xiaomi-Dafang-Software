#ifndef __PROFILES_H__
#define __PROFILES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "envy24control"
#endif

#ifndef MAX_PROFILES
#define MAX_PROFILES 4
#endif

#ifndef MAX_PROFILE_NAME_LENGTH
#define MAX_PROFILE_NAME_LENGTH 20
#endif

#ifndef DEFAULT_PROFILERC
#define DEFAULT_PROFILERC "~/"PROGRAM_NAME"/profiles.conf"
#endif

#ifndef SYS_PROFILERC
#define SYS_PROFILERC "/etc/"PROGRAM_NAME"/profiles.conf"
#endif

#define PROFILE_NAME_FIELD_LENGTH MAX_PROFILE_NAME_LENGTH + 1

#define PROFILE_HEADER_TEMPL "[ Profile # ]"
#define CARD_HEADER_TEMPL "< Card # >"
#define CARD_FOOTER_TEMPL "< /CARD # >"
#define PROFILE_NAME_TEMPL "{ /$/ }"

#define PLACE_HOLDER_NUM '#'
#define PLACE_HOLDER_STR '$'

/* max 32k for every profile */
#define MAX_PROFILE_SIZE 32768
#define MAX_SEARCH_FIELD_LENGTH 1024
#define MAX_FILE_NAME_LENGTH 1024
#define MAX_NUM_STR_LENGTH 10
#define TOKEN_SEP "|"
#define SEP_CHAR ' '

#ifndef NOTFOUND
#define NOTFOUND -1
#endif

#define ALSACTL_OP_STORE "store"
#define ALSACTL_OP_RESTORE "restore"

#define DIR_CREA_MODE "0755"	// this must be a string
#define FILE_CREA_MODE 0644	// this must be a octal number

/* max count of parameters for new_process
 * !first parameter will be the name of the external programm
 * - last parameter will be NULL
 */
#define MAX_PARAM 10

/* the place from mkdir */
#ifndef MKDIR
#define MKDIR "/bin/mkdir"
#endif

/* the place from alsactl */
#ifndef ALSACTL
#define ALSACTL "/usr/sbin/alsactl"
#endif

#ifndef __PROFILES_C__
extern int save_restore(const char * const operation, const int profile_number, const int card_number, char * cfgfile, const char * const profile_name);
extern char *get_profile_name(const int profile_number, const int card_number, char * cfgfile);
extern int get_profile_number(const char * const profile_name, const int card_number, char * cfgfile);
extern int delete_card(const int card_number, char * const cfgfile);
#endif

#endif /* __PROFILES_H__ */
