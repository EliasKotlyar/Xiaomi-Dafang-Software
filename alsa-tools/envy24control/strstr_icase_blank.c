/*
 *  Function to search string in string with ignoring case sensitivity
 *  and times of blank
 *  Copyright (c) by Dirk Kalis<dirk.kalis@t-online.de>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <string.h>
#include <ctype.h>

#ifndef MAX_SEARCH_FILED_LENGTH
#define MAX_SEARCH_FIELD_LENGTH 1024
#endif
#ifndef SEP_CHAR
#define SEP_CHAR ' '
#endif
#ifndef NOTFOUND
#define NOTFOUND -1
#endif

/*
 * Search string in string with ignoring case sensitivity and times of blank
 * and comment lines.
 * Blanks will be replaced with SEP_CHAR before compare.
 * Comment lines are lines beginning with first non blank character '#'.
 * Return value is the position in string1 relative to the begin of string1.
 */
int strstr_icase_blank(const char * const string1, const char * const string2)
{
	int position, i, j;
	char line[MAX_SEARCH_FIELD_LENGTH];
	char cmp_line[MAX_SEARCH_FIELD_LENGTH];
	char search_string[MAX_SEARCH_FIELD_LENGTH];
	char *pstr;
	int pos_first_non_blank;

	strncpy(search_string, string2, MAX_SEARCH_FIELD_LENGTH);
	search_string[MAX_SEARCH_FIELD_LENGTH - 1] = '\0';

	pos_first_non_blank = -1;
	// convert search string in upper case
	for (i = 0; i < strlen(search_string); i++)
	{
		if ((pos_first_non_blank < 0) && (!isblank(search_string[i])))
			pos_first_non_blank = i;
		search_string[i] = (char)toupper(search_string[i]);
	}

	// replace blanks in search string with SEP_CHAR to compare without blanks
	i = pos_first_non_blank;
	j = 0;
	while(i < strlen(search_string))
	{
		if (j > 0) {
			cmp_line[j] = SEP_CHAR;
			j++;
		}
		sscanf(&search_string[i], "%s", cmp_line + j);
		i += strlen(cmp_line + j) + 1;
		j += strlen(cmp_line + j);
		for(; i < strlen(search_string); i++)
		{
			if (isblank(search_string[i])) {
				continue
				;
			} else {
				break
				;
			}
		}
	}
	strncpy(search_string, cmp_line, strlen(search_string));

	position = 0;
	while (position < strlen(string1))
	{
		strncpy(line, (string1 + (position * sizeof(char))), MAX_SEARCH_FIELD_LENGTH);
		line[MAX_SEARCH_FIELD_LENGTH - 1] = '\0';
		pos_first_non_blank = -1;
		for (i = 0; i < strlen(line); i++)
		{
			if ((pos_first_non_blank < 0) && (!isblank(line[i])))
				pos_first_non_blank = i;
			line[i] = (char)toupper(line[i]);
			if (line[i] == '\n') {
				line[i] = '\0';
				break
				;
			}
		}
		// no compare with comment lines or empty lines
		if ((line[pos_first_non_blank] != '#') && strlen(line) > 0) {
			// replace blanks between entities in input line with SEP_CHAR to compare without blanks
			i = pos_first_non_blank;
			j = 0;
			while(i < strlen(line))
			{
				if (j > 0) {
					cmp_line[j] = SEP_CHAR;
					j++;
				}
				sscanf(&line[i], "%s", cmp_line + j);
				i += strlen(cmp_line + j) + 1;
				j += strlen(cmp_line + j);
				for(; i < strlen(line); i++)
				{
					if (isblank(line[i])) {
						continue
						;
					} else {
						break
						;
					}
				}
			}
			if ((pstr = strstr(cmp_line, search_string)) != NULL) {
				position += (pstr - cmp_line)/sizeof(char) + pos_first_non_blank;
				break
				;
			}
		}
		position += strlen(line) + 1;
	}
	if (position >= strlen(string1)) {
		position = NOTFOUND;
	}

	return position;
}
