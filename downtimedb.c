/*-
 * Copyright (c) 2009-2011 EPIPE Communications. All rights reserved.
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

#include <sys/types.h>

#include <errno.h>
#include <inttypes.h>
#include <paths.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "downtimedb.h"

/*
 * Swap bytes of uint64_t.
 *
 * We used to be trying to use htobe64() and be64toh().
 * Or htobe64() and betoh64().
 * Or OSSwapHostToBigInt64() and OSSwapBigToHostInt64().
 *
 * But the reality is that due to lack of standardization this became
 * just a big mess as there is no portable function to do this. Some
 * systems (for example RHEL/CentOS 5.5) lack the corresponding functions
 * altogether.
 *
 * Therefore we ignore whatever is available and just define our own
 * MY_BSWAP64() macro which is used on little endian architectures.
 *
 * The implementation is stolen from crypt-sha512.c released into the
 * Public Domain by Ulrich Drepper <drepper@redhat.com>.
 */

#ifndef WORDS_BIGENDIAN
#define MY_BSWAP64(n)			\
	(((n) << 56)			\
	| (((n) & 0xff00) << 40)	\
	| (((n) & 0xff0000) << 24)	\
	| (((n) & 0xff000000) << 8)	\
	| (((n) >> 8) & 0xff000000)	\
	| (((n) >> 24) & 0xff0000)	\
	| (((n) >> 40) & 0xff00)	\
	| ((n) >> 56))
#endif

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

#ifndef WORDS_BIGENDIAN
	buf->when = (int64_t) MY_BSWAP64((uint64_t) buf->when);
#endif

	return (1);	/* 1 record read */
}

/* The write function modifies the buffer on the fly to match
 * the endianness required in the database. Thus, the supplied
 * struct downtimedb will be invalid after calling this function.
 */

int
downtimedb_write(int fd, struct downtimedb *buf)
{
#ifndef WORDS_BIGENDIAN
	buf->when = (int64_t) MY_BSWAP64((uint64_t) buf->when);
#endif

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
