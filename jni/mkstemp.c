/* $NiH: mkstemp.c,v 1.3 2006/04/23 14:51:45 wiz Exp $ */

/* Adapted from NetBSB libc by Dieter Baron */

/*	NetBSD: gettemp.c,v 1.13 2003/12/05 00:57:36 uebayasi Exp 	*/

/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define mode_t int
#define S_ISREG(m) (((m)&_S_IFMT) == _S_IFREG)
#define S_ISDIR(m) (((m)&_S_IFMT) == _S_IFDIR)
#define mkdir _mkdir
#define S_IRUSR  (_S_IREAD)
#define S_IWUSR  (_S_IWRITE)
#define ssize_t  SSIZE_T
/*
#define open _open
*/
#undef min
#define min _cpp_min
#undef max
#define max _cpp_max
#else
#include <unistd.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif



#ifdef _WIN32
#include <io.h>
#include <process.h>
#define getpid() _getpid()
typedef int pid_t;
//#define S_ISDIR(m) (((m) & (_S_IFMT)) == (_S_IFDIR))
#define open(p, f, m) _open((p), ((f) | _O_BINARY), _S_IREAD | _S_IWRITE)
#endif

int
_zip_mkstemp(char *path)
{
	int fd;   
	char *start, *trv;
	struct stat sbuf;
	pid_t pid;

	/* To guarantee multiple calls generate unique names even if
	   the file is not created. 676 different possibilities with 7
	   or more X's, 26 with 6 or less. */
	static char xtra[2] = {'a', 'a'};
	int xcnt = 0;

	pid = getpid();

	/* Move to end of path and count trailing X's. */
	for (trv = path; *trv; ++trv)
		if (*trv == 'X')
			xcnt++;
		else
			xcnt = 0;	

	/* Use at least one from xtra.  Use 2 if more than 6 X's. */
	if (*(trv - 1) == 'X')
		*--trv = xtra[0];
	if (xcnt > 6 && *(trv - 1) == 'X')
		*--trv = xtra[1];

	/* Set remaining X's to pid digits with 0's to the left. */
	while (*--trv == 'X') {
		*trv = (pid % 10) + '0';
		pid /= 10;
	}

	/* update xtra for next call. */
	if (xtra[0] != 'z')
		xtra[0]++;
	else {
		xtra[0] = 'a';
		if (xtra[1] != 'z')
			xtra[1]++;
		else
			xtra[1] = 'a';
	}

	/*
	 * check the target directory; if you have six X's and it
	 * doesn't exist this runs for a *very* long time.
	 */
	for (start = trv + 1;; --trv) {
		if (trv <= path)
			break;
		if (*trv == '/') {
			*trv = '\0';
			if (stat(path, &sbuf))
				return (0);
			if (!S_ISDIR(sbuf.st_mode)) {
				errno = ENOTDIR;
				return (0);
			}
			*trv = '/';
			break;
		}
	}

	for (;;) {
		if ((fd=open(path, O_CREAT|O_EXCL|O_RDWR|O_BINARY, 0600)) >= 0)
			return (fd);
		if (errno != EEXIST)
			return (0);

		/* tricky little algorithm for backward compatibility */
		for (trv = start;;) {
			if (!*trv)
				return (0);
			if (*trv == 'z')
				*trv++ = 'a';
			else {
				if (isdigit((unsigned char)*trv))
					*trv = 'a';
				else
					++*trv;
				break;
			}
		}
	}
	/*NOTREACHED*/
}
