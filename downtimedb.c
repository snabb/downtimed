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

/* _BSD_SOURCE is required to enable BSD style htobe64(3) and be64toh(3)
 * functions on GNU/Linux.
 */

#ifdef __linux__
#define _BSD_SOURCE
#endif

/* Include config.h in case we use autoconf. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif
#include <sys/types.h>
#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif
#include <errno.h>
#include <paths.h>
#include <stdint.h>
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
	    sizeof(struct downtimedb))
		if (ret == 0)
			return 0;	/* eof */
		else
			return -1;	/* some sort of error */

	buf->when = (int64_t) be64toh((uint64_t) buf->when);

	return 1;	/* 1 record read */
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
		return -1;

	return 0;
}

/* eof */
