/*
 * Name: strstr and strdup
 *
 * These are the standard library utilities.  We define them here for
 * systems that don't have them.
 */

#ifndef HAVE_STRSTR
char *strstr(char *s1, char *s2)
{                               /* from libiberty */
    char *p;
    int len = strlen(s2);

    if (*s2 == '\0')            /* everything matches empty string */
        return s1;
    for (p = s1; (p = strchr(p, *s2)) != NULL; p = strchr(p + 1, *s2)) {
        if (strncmp(p, s2, len) == 0)
            return (p);
    }
    return NULL;
}
#endif

#ifndef HAVE_STRDUP
char *strdup(char *s)
{
    char *retval;

    retval = (char *) malloc(strlen(s) + 1);
    if (retval == NULL) {
        perror("boa: out of memory in strdup");
        exit(1);
    }
    return strcpy(retval, s);
}
#endif
