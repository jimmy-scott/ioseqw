/*
 * Copyright (C) 2012 Jimmy Scott #jimmy#inet-solutions#be#. Belgium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  3. The names of the authors may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include "common.h"
#include "setup.h"
#include "record.h"

/*
 * Handle main program flow.
 */

int
main(int argc, char **argv)
{
	char *record;
	int fd, oflags;
	uint32_t written;
	struct config_t config;
	
	/* parse arguments and config file */
	if (setup(&config, argc, argv) != SETUP_OK) {
		usage(stderr);
		return EX_USAGE;
	}
	
	/* create record buffer */
	if ((record = mkrecord(config.rsize, '1')) == NULL) {
		PRERROR("failed to create record buffer");
		return EX_OSERR;
	}
	
	/* default set of open flags */
	oflags = O_WRONLY | O_CREAT | O_TRUNC;
	
	/* optional open flags */
	if (config.opt_sync) oflags |= O_SYNC;
	if (config.opt_dsync) oflags |= O_DSYNC;
	
	/* open file for writing */
	if ((fd = open(config.filename, oflags, 0644)) == -1) {
		PRERROR("failed to open '%s'", config.filename);
		return EX_CANTCREAT;
	}
	
	/* start writing records */
	for (written = 0; written < config.rcount; ++written)
	{
		/* write record - cleanup and return on failure */
		if (write(fd, record, config.rsize) != config.rsize) {
			PRERROR("failed to write to '%s'", config.filename);
			free(record);
			close(fd);
			return -1;
		}
		
		/* (full)fsync if requested */
		if (config.opt_fsync) fsync(fd);
#ifdef __APPLE__
		if (config.opt_ffsync) fcntl(fd, F_FULLFSYNC);
#endif /* __APPLE__ */
	}
	
	/* cleanup */
	free(record);
	close(fd);
	
	return EX_OK;
}
