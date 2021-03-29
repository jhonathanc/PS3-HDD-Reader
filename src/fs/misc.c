/*
 * Copyright (c) 2004 Nehal Mistry
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "misc.h"
#include "../types.h"




int Stricmp(const char *s1, const char *s2) {
  char f, l;

  do {
    f = ((*s1 <= 'Z') && (*s1 >= 'A')) ? *s1 + 'a' - 'A' : *s1;
    l = ((*s2 <= 'Z') && (*s2 >= 'A')) ? *s2 + 'a' - 'A' : *s2;
    s1++;
    s2++;
  } while ((f) && (f == l));

  return (int) (f - l);
}

int Strnicmp(const char *s1, const char *s2, size_t n) {
  int f, l;

  do {
    if (((f = (unsigned char)(*(s1++))) >= 'A') && (f <= 'Z')) f -= 'A' - 'a';
    if (((l = (unsigned char)(*(s2++))) >= 'A') && (l <= 'Z')) l -= 'A' - 'a';
  } while (--n && f && (f == l));

  return f - l;
}




// invalid names (case-insensitive), including an appended '.'
// omitting "CON"
char *reserved_names[] = {
	"PRN",
	"AUX",
	"NUL",
	"COM1",
	"COM2",
	"COM3",
	"COM4",
	"COM5",
	"COM6",
	"COM7",
	"COM8",
	"COM9",
	"LPT1",
	"LPT2",
	"LPT3",
	"LPT4",
	"LPT5",
	"LPT6",
	"LPT7",
	"LPT8",
	"LPT9",
	"CLOCK$"
};

// invalid chars, also 0-31
char reserved_chars[] = {
	'<',
	'>',
	':',
	'"',
	'/',
	'\\',
	'|'
};

/***********************************************************************
*
***********************************************************************/
char *basename(const char *path)
{
	char *base, *f;

	//wenn path leer oder erstes zeichen nullterminator...
	if(path == NULL || path[0] == '\0'){
		
		//"." in string base kopieren.
		base = strdup(".");
	}
	else{  //wenn nicht leer...
	
		//path in string base kopieren...
		base = strdup(path);
		
		//wenn ein '/' im string base gefunden wird...
		if((f = strrchr(base, '/'))){
			
			//
			base = strdup(++f);
		}
		else{  //wenn nicht, path komplet in base kopieren
			base = strdup(path);
		}
	}

	return base;
}

/***********************************************************************
*
***********************************************************************/
char *dirname(const char *path)
{
	char *dir;

	if (path == NULL || path[0] == '\0' || strchr(path, '/') == NULL) {
		dir = strdup(".");
	}
	else
	{
		char *s;
		dir = strdup(path);
		s = strrchr(dir, '/');  //sucht nach / im string
		while (*--s == '/');
		*++s = '\0';
	}

	return dir;
}


char *valid_filename(char *path, int allowcon)
{
	char *dir, *base;
	char *validname;
	char tmp[MISC_MAX_PATH];
	char invext[64];
	int i, j;
	int invalid;
	int skip;

	validname = malloc(MISC_MAX_PATH);

	dir = dirname(path);
	base = basename(path);

	strcpy(validname, dir);
	strcat(validname, "/");

	invalid = 0;
	for (i = 0; i < sizeof(reserved_names) / sizeof(*reserved_names)
	    && !invalid; ++i) {
		strcpy(invext, reserved_names[i]);
		strcat(invext, ".");

		if (!Stricmp(base, reserved_names[i]) ||
		    !Strnicmp(base, invext, strlen(invext))) {
			invalid = 1;
		}
	}

	if (!allowcon && (!Stricmp(base, "CON") || !(Strnicmp(base, "CON.", 4))))
		invalid = 1;

	tmp[0] = '\0';
	for (i = 0; base[i] != '\0'; ++i) {
		skip = 0;
		if (base[i] >= 0 && base[i] <= 31) {
			invalid = 1;
			skip = 1;
		}
		for (j = 0; !skip && j < sizeof(reserved_chars) /
		    sizeof(*reserved_chars); j++) {
			if (base[i] == reserved_chars[j] ||
			    (base[i] >= 0 && base[i] <= 31)) {
				invalid = 1;
				skip = 1;
			}
		}
		if (!skip) {
			strncat(tmp, &base[i], 1);
		}
	}

	if (invalid)
		strcat(validname, "__");
	strcat(validname, tmp);

	free(dir);
	free(base);

	return validname;
}

