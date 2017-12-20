/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Authorization "module" (c) 1998,99 Martin Hinner <martin@tdp.cz>
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

#include <stdio.h>
#include <fcntl.h>
#ifdef __UC_LIBC__
#include <unistd.h>
#else
#include <crypt.h>
#endif
#include <syslog.h>
#include "boa.h"
#ifdef SHADOW_AUTH
#include <shadow.h>
#endif
#ifdef OLD_CONFIG_PASSWORDS
#include <crypt_old.h>
#endif
#ifdef EMBED
#include <sys/types.h>
#include <pwd.h>
#include <config/autoconf.h>
#endif

#ifdef CONFIG_AMAZON
#include "../../login/logcnt.c"
#endif

#ifdef USE_AUTH

struct _auth_dir_ {
	char *directory;
	char *type;
	FILE *authfile;
	int dir_len;
	struct _auth_dir_ *next;
};

typedef struct _auth_dir_ auth_dir;

static auth_dir *auth_hashtable [AUTH_HASHTABLE_SIZE];
#ifdef OLD_CONFIG_PASSWORDS
char auth_old_password[16];
#endif

/*
 * Name: get_auth_hash_value
 *
 * Description: adds the ASCII values of the file letters
 * and mods by the hashtable size to get the hash value
 */
inline int get_auth_hash_value_to_end(char *dir, char *end)
{
#ifdef EMBED
	return 0;
#else
	unsigned int hash = 0;
	unsigned char c;

	hash = *dir++;
	while ( (end==NULL || dir < end) && (c = *dir++))
	{
		if(c == '/') continue;
		hash += (unsigned int) c;
	}

	return hash % AUTH_HASHTABLE_SIZE;
#endif
}

inline int get_auth_hash_value(char *dir)
{ return get_auth_hash_value_to_end(dir, NULL); }

/*
 * Name: auth_add
 *
 * Description: adds 
 */
void auth_add(char *directory,char *file, char *type)
{
	auth_dir *new_a,*old;
	
	old = auth_hashtable [get_auth_hash_value(directory)];
	while (old)
	{
		if (!strcmp(directory,old->directory))
			return;
		old = old->next;
	}
	
	new_a = (auth_dir *)malloc(sizeof(auth_dir));
	/* success of this call will be checked later... */
	if(file == NULL)
		new_a->authfile = NULL;
	else
	{
		new_a->authfile = fopen(file,"rt");
		if(new_a->authfile == NULL) type = "Error";
	}
	new_a->type = strdup(type);
	new_a->directory = strdup(directory);
	new_a->dir_len = strlen(directory);
	new_a->next = auth_hashtable [get_auth_hash_value(directory)];
	auth_hashtable [get_auth_hash_value(directory)] = new_a;
}

void auth_check()
{
	int hash;
	auth_dir *cur;
	
	for (hash=0;hash<AUTH_HASHTABLE_SIZE;hash++)
	{
  	cur = auth_hashtable [hash];
	  while (cur)
		{
			if (!cur->authfile)
			{
				log_error_time();
				fprintf(stderr,"Authentication password file for %s not found!\n",
						cur->directory);
			}
			cur = cur->next;
		}
	}
}

#ifdef LDAP_HACK
#include <lber.h>
#include <ldap.h>
#include <sg_configdd.h>
#include <sg_confighelper.h>
#include <sg_users.h>

/* Return a positive, negative or not possible result to the LDAP auth for
 * the specified user.
 */
static int ldap_auth(const char *const user, const char *const pswd) {
	static time_t last;
	static char *prev_user;
	LDAP *ld;
	int ldap_ver, r;
	char f[256];
	ConfigHandleType *c = config_load(amazon_ldap_config_dd);

	/* Don't repeat query too often if the user name hasn't changed */
	if (last && prev_user &&
			time(NULL) < (last + config_get_int(c, AMAZON_LDAP_CACHET)) &&
			strcmp(prev_user, user) == 0) {
		config_free(c);
		last = time(NULL);
		return 1;
	}
	if (prev_user != NULL)   { free(prev_user);	prev_user = NULL;   }
	last = 0;

	if ((ld = ldap_init(config_get_string(c, AMAZON_LDAP_HOST),
			config_get_int(c, AMAZON_LDAP_PORT))) == NULL) {
		syslog(LOG_ERR, "unable to initialise LDAP");
		config_free(c);
		return 0;
	}
	if (ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF) != LDAP_SUCCESS) {
		syslog(LOG_ERR, "unable to set LDAP referrals off");
		config_free(c);
		ldap_unbind(ld);
		return 0;
	}
	ldap_ver = config_get_int(c, AMAZON_LDAP_VERSION);
	if (ldap_ver > 0 && ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &ldap_ver) != LDAP_SUCCESS) {
		syslog(LOG_ERR, "unable to set LDAP version %d", ldap_ver);
		config_free(c);
		ldap_unbind(ld);
		return 0;
	}
	snprintf(f, sizeof f, config_get_string(c, AMAZON_LDAP_BIND_DN), user);
	r = ldap_simple_bind_s(ld, f, pswd);
	if (r != LDAP_SUCCESS) {
		syslog(LOG_ERR, "unable to connect to LDAP (%s)", ldap_err2string(r));
		config_free(c);
		ldap_unbind(ld);
		return (r==LDAP_INVALID_CREDENTIALS)?-1:0;
	}
	config_free(c);
	ldap_unbind(ld);
	/* Caching timing details for next time through */
	prev_user = strdup(user);
	last = time(NULL);
	return 1;
}
static unsigned char ldap_succ;
#endif


/*
 * Name: auth_check_userpass
 *
 * Description: Checks user's password. Returns 0 when sucessful and password
 * 	is ok, else returns nonzero; As one-way function is used RSA's MD5 w/
 *  BASE64 encoding.
#ifdef EMBED
 * On embedded environments we use crypt(), instead of MD5.
#endif
 */
int auth_check_userpass(char *user,char *pass,FILE *authfile)
{
#ifdef LDAP_HACK
	/* Yeah, code before declarations will fail on older compilers... */
	switch (ldap_auth(user, pass)) {
	case -1:	ldap_succ = 0;				return 1;
	case 0:		ldap_succ = strcmp(user, "root")?0:1;	break;
	case 1:
		ldap_succ = 1;
		if (start_user_update(0) == 0)
			done_user_update(set_user_password(user, pass, 0)==0?1:0);
		return 0;
	}
#endif
#ifdef SHADOW_AUTH
	struct spwd *sp;

	sp = getspnam(user);
	if (!sp)
		return 2;

	if (!strcmp(crypt(pass, sp->sp_pwdp), sp->sp_pwdp))
		return 0;

#else

#ifndef EMBED
	char temps[0x100],*pwd = NULL;

	DBG(printf("auth_check_userpass(%s,%s,...);\n",user,pass);)

	/* no file means use system passwd */
	if(!authfile)
	{
		struct passwd *pwp;

		pwp = getpwnam(user);
		if (pwp != NULL) {
			if (strcmp(crypt(pass, pwp->pw_passwd), pwp->pw_passwd) == 0)
				return 0;
		}
		return 2;
	}

	fseek(authfile,0,SEEK_SET);
	while (fgets(temps,0x100,authfile))
	{
		if (temps[strlen(temps)-1]=='\n')
			temps[strlen(temps)-1] = 0;
		pwd = strchr(temps,':');
		if (pwd)
		{
			*pwd++=0;
			if (!strcmp(temps,user))
			{
				break;
			}
		}
	}
#ifdef USE_INTERNAL_MD5
  /* Encode password ('pass') using one-way function and then use base64
		 encoding. */
	{
		struct MD5Context mc;
	 	unsigned char final[16];
		char encoded_passwd[0x40];
		MD5Init(&mc);
		MD5Update(&mc, pass, strlen(pass));
		MD5Final(final, &mc);
		strcpy(encoded_passwd,"$1$");
		base64encode(final, encoded_passwd+3, 16);
		if(strcmp(pass, encoded_passwd))
			return 2;
		else
			return 0;
	}
#else
	if (pwd == NULL || strlen(pwd) < 3 )
		return 2;

	if (strcmp(crypt(pass, pwd), pwd))
		return 2;
	else
		return 0;
#endif

#else
	struct passwd *pwp;

	pwp = getpwnam(user);
	if (pwp != NULL) {
		if (strcmp(crypt(pass, pwp->pw_passwd), pwp->pw_passwd) == 0)
			return 0;
	} else 
		return 2;

#endif	/* ! EMBED */
#endif	/* SHADOW_AUTH */
	return 1;
}

char *strrchr_to_end(const char *start, int c, char *end)
{
	if(end == start) return NULL;
	if(!end) end = (char *)start + strlen((char *)start) + 1;
	do
	{
		end--;
		if(*end == c) return end;
	} while(end > start);
	return NULL;
}

int auth_authorize(request * req)
{
	auth_dir *current;
	int hash;
	char *pwd;
	static char current_client[20];
	char auth_userpass[0x80];
	char *end = NULL;
					
  do
  {
	hash = get_auth_hash_value_to_end(req->request_uri, end);
	current = auth_hashtable[hash];
	if(current) break;
	end = strrchr_to_end(req->request_uri, '/', end);
	if(!end) break;
  } while (1);
					
  while (current) {
		if (!memcmp(req->request_uri, current->directory,
								current->dir_len)) {
			if (current->directory[current->dir_len - 1] != '/' &&
				        req->request_uri[current->dir_len] != '/' &&
								req->request_uri[current->dir_len] != '\0') {
				break;
			}
			if(current->type == NULL || !strcmp(current->type, "Error"))
			{
				send_r_error(req);
				return 0;
			}
			if(!strcmp(current->type, "None"))
				return 1;
			if (req->authorization)
			{
				int denied = 1;
				if (strncasecmp(req->authorization,"Basic ",6))
				{
					syslog(LOG_ERR, "Can only handle Basic auth\n");
					send_r_bad_request(req);
					return 0;
				}
				
				base64decode(auth_userpass,req->authorization+6,sizeof(auth_userpass));
				
				if ( (pwd = strchr(auth_userpass,':')) == 0 )
				{
					syslog(LOG_ERR, "No user:pass in Basic auth\n");
					send_r_bad_request(req);
					return 0;
				}
				
				*pwd++=0;

				denied = auth_check_userpass(auth_userpass,pwd,current->authfile);
#ifdef CONFIG_AMAZON
				if (strncmp(get_mime_type(req->request_uri),"image/",6))
					access__attempted(denied, auth_userpass);
#endif
				if (denied) {
					switch (denied) {
						case 1:
							syslog(LOG_ERR, "Authentication attempt failed for %s from %s because: Bad Password\n",
									auth_userpass, req->remote_ip_addr);
							break;
						case 2:
							syslog(LOG_ERR, "Authentication attempt failed for %s from %s because: Invalid Username\n",
									auth_userpass, req->remote_ip_addr);
					}
					bzero(current_client, sizeof(current_client));
					send_r_unauthorized(req,server_name);
					return 0;
				}
				if (strcmp(current_client, req->remote_ip_addr) != 0) {
					strncpy(current_client, req->remote_ip_addr, sizeof(current_client));
					syslog(LOG_INFO, "Authentication successful for %s from %s\n", auth_userpass, req->remote_ip_addr);
				}
				/* Copy user's name to request structure */
#ifdef LDAP_HACK
				if (!ldap_succ) {
					strcpy(req->user, "noldap");
					syslog(LOG_INFO, "Access granted as noldap");
				} else
#endif
				strncpy(req->user, auth_userpass, 15);
				req->user[15] = '\0';
				return 1;
			}else
			{
				/* No credentials were supplied. Tell them that some are required */
				if(!strcmp(current->type, "None"))
					return 1;
				send_r_unauthorized(req,server_name);
				return 0;
			}
		}
	  current = current->next;
  }
						
	return 1;
}

void dump_auth(void)
{
	int i;
	auth_dir *temp;

	for (i = 0; i < AUTH_HASHTABLE_SIZE; ++i) {
		if (auth_hashtable[i]) {
			temp = auth_hashtable[i];
			while (temp) {
				auth_dir *temp_next;

				if (temp->directory)
					free(temp->directory);
				if (temp->type)
					free(temp->type);
				if (temp->authfile)
					fclose(temp->authfile);
				temp_next = temp->next;
				free(temp);
				temp = temp_next;
			}
			auth_hashtable[i] = NULL;
		}
	}
}

#endif
