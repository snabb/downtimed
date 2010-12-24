/*-
 * Copyright (c) 2009-2010 EPIPE Communications. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of EPIPE Communications.
 *
 *
 * Software web site:
 *   http://dist.epipe.com/downtimed/
 *
 * Author contact information:
 *   opensource@epipe.com
 */

/* Include config.h in case we use autoconf. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* _BSD_SOURCE is required to enable BSD style htobe64(3) and be64toh(3)
 * functions on GNU/Linux. It probably needs to be defined before including
 * any system headers:
 */

#ifdef __linux__
#define _BSD_SOURCE
#endif

#include <sys/types.h>

/* This should pull in the 64 bit byte swapping stuff on *BSD and Linux: */

#ifdef HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif
#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif

/* ...except on OpenBSD the name of be64toh() is different: */

#ifdef __OpenBSD__
#define be64toh(x) betoh64(x)
#endif

/* MacOS X has it's own 64 bit byte swapping functions: */

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define be64toh(x) OSSwapBigToHostInt64(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#endif

/* Finally now we should have working be64toh() and htobe64(). At
 * least *BSD, GNU/Linux and MacOS X should be covered. What a mess!
 */

#include <errno.h>
#include <inttypes.h>
#include <paths.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "downtimedb.h"

/*
 * Functions for reading from and writing to the downtime database.
 */

int
downtimedb_read(int fd, struct downtimedb *buf)
{
	int ret;

	errno = 0;
	if ((ret = read(fd, (void *)buf, sizeof(struct downtimedb))) <
	    sizeof(struct downtimedb)) {
		if (ret == 0)
			return (0);	/* eof */
		else {
			if (ret != -1) {
				/* set errno when it is not a read error */
				errno = EILSEQ;
			}
			return (-1);	/* some sort of error */
		}
	}

	buf->when = (int64_t) be64toh((uint64_t) buf->when);

	return (1);	/* 1 record read */
}

/* The write function modifies the buffer on the fly to match
 * the endianness required in the database. Thus, the supplied
 * struct downtimedb will be invalid after calling this function.
 */

int
downtimedb_write(int fd, struct downtimedb *buf)
{
	buf->when = (int64_t) htobe64((uint64_t) buf->when);

	errno = 0;
	if (write(fd, (void *)buf, sizeof(struct downtimedb)) <
	    sizeof(struct downtimedb))
		return (-1);

	return (0);
}

/*
 * Return time string of absolute time in static buffer.
 * Certainly not thread-safe.
 */

char *
timestr_abs(time_t t)
{
	static char str[100];
	struct tm *lt;

	if (t != 0) {
		if ((lt = localtime(&t)) == NULL)
			goto err;
		if (strftime(str, sizeof(str), "%F %T", lt) == 0)
			goto err;

		return (str);
	}
err:
	/* we have the backslashes here to avoid interpretation as trigraphs */
	return ("?\?\?\?-?\?-?\? ?\?:?\?:?\?");
}

/*
 * Stolen from top.c. Return time interval in human-readable (?) static
 * string. Definitely not thread-safe.
 */

char *
timestr_int(time_t t)
{
	int days, hrs, mins, secs;
	static char str[100];

	days = t / 86400;
	t %= 86400;
	hrs = t / 3600;
	t %= 3600;
	mins = t / 60;
	secs = t % 60;

	if (days > 0)
		snprintf(str, sizeof(str), "%d+%02d:%02d:%02d",
		    days, hrs, mins, secs);
	else
		snprintf(str, sizeof(str), "%02d:%02d:%02d",
		    hrs, mins, secs);

	return (str);
}

/* eof */
