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

/*
 * The structure of the downtime database file.
 *
 * This should provide a Y2K38 bug, 64 bit and endianness safe file format.
 * We need a portable format, because everything in the system could
 * change during the downtime. Someone could change the machine from
 * big-endian 32 bit architecture to 64 bit little-endian computer.
 * Or someone might want to transfer the database from one computer to
 * another for further analysis. It would also be possible to transmit
 * the data in the same format to a remote collector through a network
 * socket.
 *
 * At the time when 128 bit computers are introduced, possibly some pack
 * #pragmas or similar should be inserted here to retain compatibility. XXX
 */

struct downtimedb {
	uint8_t	what;		/* Op code of the recorded event  */
	uint8_t	_padding[7];	/* Reserved for future extensions */
	int64_t	when;		/* UNIX time in big-endian format */
};

#define	DOWNTIMEDB_WHAT_NONE		0
#define	DOWNTIMEDB_WHAT_UP		1
#define	DOWNTIMEDB_WHAT_SHUTDOWN	2
#define	DOWNTIMEDB_WHAT_CRASH		3

#if defined(__linux__) || defined(__FreeBSD_kernel__) || !defined(_PATH_VARDB)
#define	PATH_DOWNTIMEDBDIR	"/var/lib/downtimed/"
#else
#define	PATH_DOWNTIMEDBDIR	_PATH_VARDB "downtimed/"
#endif

#define	PATH_DOWNTIMEDBFILE	PATH_DOWNTIMEDBDIR "downtimedb"

/* default absolute time format unless specified by user */

#define FMT_DATETIME		"%F %T"

/* Function prototypes */

int	downtimedb_read(int, struct downtimedb *);
int	downtimedb_write(int, struct downtimedb *);
char *	timestr_abs(time_t, const char *);
char *	timestr_int(time_t);

/* eof */
