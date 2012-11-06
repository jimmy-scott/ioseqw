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

#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "common.h"
#include "setup.h"

/*
 * Macros for validating input.
 */

#define STRTOX_INVALID_NUMBER(i, str, ep, min, max) \
	(*ep || ep == str || i < min || i > max)

/*
 * Static function prototypes.
 */

static void config_init(struct config_t *config);
static int config_validate(struct config_t *config);
static int parse_args(struct config_t *config, int argc, char **argv);

/*
 * Print "short" usage information to stream.
 */

void
usage(FILE *stream)
{
	fputs(
	"usage: "PROGRAM" [opts] -s <record-size> -c <record-count>\n\n"
	"Mandatory:\n"
	"  -s <record-size>   Size of records to write\n"
	"  -c <record-count>  Number of records to write\n\n"
	"Optional:\n"
	"  -f <filename>      Name of the file to write to\n"
	"  -t <sync-type>     Sync option to use for writing\n"
	"  -h                 Show this help\n"
	"  -v                 Show version\n\n"
	"Sync types:\n"
	"  -t sync            Use the O_SYNC open option\n"
	"  -t dsync           Use the O_DSYNC open option\n"
	"  -t fsync           Use the fsync() system call\n"
#ifdef __APPLE__
	"  -t ffsync          Use the F_FULLSYNC file control\n"
#endif /* __APPLE__ */
	, stream);
}

/*
 * Parse arguments.
 *
 * Values are stored in the config_t structured which is passed as a
 * pointer to the function. Function will return SETUP_OK if all was
 * OK or SETUP_ERROR in case of an error.
 */

int
setup(struct config_t *config, int argc, char **argv)
{	
	/* set defaults */
	config_init(config);
	
	/* parse arguments */
	if (parse_args(config, argc, argv) != 0)
		return SETUP_ERROR;
	
	/* check mandatory/conflicting settings */
	if (config_validate(config) != 0)
		return SETUP_ERROR;
	
	return SETUP_OK;
}

/*
 * Initialize config_t structure to default values.
 */

static void
config_init(struct config_t *config)
{
	/* set everything to 0/NULL */
	memset(config, 0, sizeof(config_t));
	
	/* default output filename */
	config->filename = DEFAULT_FILENAME;
}

/*
 * Verify if mandatory values are set.
 *
 * Returns 0 if OK, -1 on error.
 */

static int
config_validate(struct config_t *config)
{
	/* check if mandatory options are set */
	if (!config->rsize) {
		PRWARN("missing parameter: record size");
		return -1;
	}
	if (!config->rcount) {
		PRWARN("missing parameter: record count");
		return -1;
	}
	
	return 0; /* ok */
}

/*
 * Parse command-line arguments, and set the values in *config.
 *
 * Parses both options and non-option arguments. Will validate input
 * and amount of arguments.
 *
 * Returns 0 if OK, -1 on error. Exits on help/version.
 */

static int
parse_args(struct config_t *config, int argc, char **argv)
{
	int c;
	uintmax_t num;
	char *endptr = NULL;
	
	/* options */
	static char *shortopts = "hvf:s:c:t:";
	static struct option longopts[] = {
		{ "filename",      required_argument, NULL, 'f' },
		{ "record-size",   required_argument, NULL, 's' },
		{ "record-count",  required_argument, NULL, 'c' },
		{ "sync-type",     required_argument, NULL, 't' },
		{ "help",          no_argument,       NULL, 'h' },
		{ "version",       no_argument,       NULL, 'v' },
		{ NULL, 0, NULL, 0 }
	};
	
	/* get options from arguments */
	while ((c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
	{
		switch (c)
		{
		case 'f':
			config->filename = optarg;
			break;
		case 's':
			num = strtoumax(optarg, &endptr, 10);
			if (STRTOX_INVALID_NUMBER(num, optarg, endptr, 1, UINT32_MAX)) {
				PRWARN("invalid record size: %s", optarg);
				return -1;
			}
			config->rsize = (uint32_t)num;
			break;
		case 'c':
			num = strtoumax(optarg, &endptr, 10);
			if (STRTOX_INVALID_NUMBER(num, optarg, endptr, 1, UINT64_MAX)) {
				PRWARN("invalid record count: %s", optarg);
				return -1;
			}
			config->rcount = (uint64_t)num;
			break;
		case 't':
			if (strcmp(optarg, "sync") == 0)
				config->opt_sync = 1;
			else if (strcmp(optarg, "dsync") == 0)
				config->opt_dsync = 1;
			else if (strcmp(optarg, "fsync") == 0)
				config->opt_fsync = 1;
#ifdef __APPLE__
			else if (strcmp(optarg, "ffsync") == 0)
				config->opt_ffsync = 1;
#endif /* __APPLE__ */
			else {
				PRWARN("invalid sync type: %s", optarg);
				return -1;
			}
			break;
		case 'h':
			usage(stdout);
			exit(EX_OK);
			break;
		case 'v':
			/* too lame to make a function for this */
			puts(PROGRAM " " BUILD_VERSION);
			exit(EX_OK);
			break;
		default:
			return -1;
			break;
		}
	}
	
	/* all arguments must be processed */
	if (argc - optind != 0) {
		PRWARN("too many arguments");
		return -1;
	}
	
	/* all ok */
	return 0;
}
