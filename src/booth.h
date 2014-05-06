/* 
 * Copyright (C) 2011 Jiaju Zhang <jjzhang@suse.de>
 * Copyright (C) 2013-2014 Philipp Marek <philipp.marek@linbit.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _BOOTH_H
#define _BOOTH_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#define BOOTH_RUN_DIR "/var/run/booth/"
#define BOOTH_LOG_DIR "/var/log"
#define BOOTH_LOGFILE_NAME "booth.log"
#define BOOTH_DEFAULT_CONF_DIR "/etc/booth/"
#define BOOTH_DEFAULT_CONF_NAME "booth"
#define BOOTH_DEFAULT_CONF_EXT ".conf"
#define BOOTH_DEFAULT_CONF \
	BOOTH_DEFAULT_CONF_DIR BOOTH_DEFAULT_CONF_NAME BOOTH_DEFAULT_CONF_EXT

#define DAEMON_NAME		"boothd"
#define BOOTH_PATH_LEN		127

#define BOOTH_DEFAULT_PORT		9929

/* TODO: remove */
#define BOOTH_PROTO_FAMILY	AF_INET

#define BOOTHC_MAGIC		0x5F1BA08C
#define BOOTHC_VERSION		0x00010002


/** Timeout value for poll().
 * Determines frequency of periodic jobs, eg. when send-retries are done.
 * See process_tickets(). */
#define POLL_TIMEOUT	1000


/** @{ */
/** The on-network data structures and constants. */

#define BOOTH_NAME_LEN		64

#define CHAR2CONST(a,b,c,d) ((a << 24) | (b << 16) | (c << 8) | d)


/* Says that the ticket shouldn't be active anywhere.
 * NONE wouldn't be specific enough. */
#define NO_ONE ((uint32_t)-1)
/* Says that another one should recover. */
#define TICKET_LOST CHAR2CONST('L', 'O', 'S', 'T')


typedef unsigned char boothc_site  [BOOTH_NAME_LEN];
typedef unsigned char boothc_ticket[BOOTH_NAME_LEN];


struct boothc_header {
	/** Authentication data; not used now. */
	uint32_t iv;
	uint32_t auth1;
	uint32_t auth2;


	/** BOOTHC_MAGIC */
	uint32_t magic;
	/** BOOTHC_VERSION */
	uint32_t version;

	/** Packet source; site_id. See add_site(). */
	uint32_t from;

	/** Length including header */
	uint32_t length;

	/** The command respectively protocol state. See cmd_request_t. */
	uint32_t cmd;
	/** Result of operation. 0 == OK */
	uint32_t result;

	char data[0];
} __attribute__((packed));


struct ticket_msg {
	/** Ticket name. */
	boothc_ticket id;

	/** Current leader. May be NO_ONE. See add_site().
	 * For a OP_REQ_VOTE this is  */
	uint32_t leader;

	/** Current term. */
	uint32_t term;
	uint32_t term_valid_for;

	/* Perhaps we need to send a status along, too - like
	 *  starting, running, stopping, error, ...? */

	uint32_t leader_commit; // TODO: NEEDED?
} __attribute__((packed));


struct boothc_ticket_msg {
	struct boothc_header header;
	struct ticket_msg ticket;
} __attribute__((packed));


typedef enum {
	/* 0x43 = "C"ommands */
	CMD_LIST    = CHAR2CONST('C', 'L', 's', 't'),
	CMD_GRANT   = CHAR2CONST('C', 'G', 'n', 't'),
	CMD_REVOKE  = CHAR2CONST('C', 'R', 'v', 'k'),

	/* Replies */
	CMR_GENERAL = CHAR2CONST('G', 'n', 'l', 'R'), // Increase distance to CMR_GRANT
	CMR_LIST    = CHAR2CONST('R', 'L', 's', 't'),
	CMR_GRANT   = CHAR2CONST('R', 'G', 'n', 't'),
	CMR_REVOKE  = CHAR2CONST('R', 'R', 'v', 'k'),

	/* get status from another server */
	OP_STATUS   = CHAR2CONST('S', 't', 'a', 't'),
	OP_MY_INDEX = CHAR2CONST('M', 'I', 'd', 'x'), /* Answer to status */

	/* Raft */
	OP_REQ_VOTE = CHAR2CONST('R', 'V', 'o', 't'),
	OP_VOTE_FOR = CHAR2CONST('V', 't', 'F', 'r'),
	OP_HEARTBEAT= CHAR2CONST('H', 'r', 't', 'B'), /* Heartbeat */
	OP_UPDATE   = CHAR2CONST('U', 'p', 'd', 'E'), /* Update ticket */
	OP_REVOKE   = CHAR2CONST('R', 'e', 'v', 'k'), /* Revoke ticket */
	OP_REJECTED = CHAR2CONST('R', 'J', 'C', '!'),
} cmd_request_t;


/* TODO: make readable constants */
typedef enum {
	/* for compatibility with other functions */
	RLT_SUCCESS             = 0,
	RLT_ASYNC               = CHAR2CONST('A', 's', 'y', 'n'),
	RLT_SYNC_SUCC           = CHAR2CONST('S', 'c', 'c', 's'),
	RLT_SYNC_FAIL           = CHAR2CONST('F', 'a', 'i', 'l'),
	RLT_INVALID_ARG         = CHAR2CONST('I', 'A', 'r', 'g'),
	RLT_EXT_FAILED          = CHAR2CONST('X', 'P', 'r', 'g'),
	RLT_OVERGRANT           = CHAR2CONST('O', 'v', 'e', 'r'),
	RLT_PROBABLY_SUCCESS    = CHAR2CONST('S', 'u', 'c', '?'),
	RLT_BUSY                = CHAR2CONST('B', 'u', 's', 'y'),
	RLT_TERM_OUTDATED       = CHAR2CONST('T', 'O', 'd', 't'),
	RLT_TERM_STILL_VALID    = CHAR2CONST('T', 'V', 'l', 'd'),
	RLT_REDIRECT            = CHAR2CONST('R', 'e', 'd', 'r'),
} cmd_result_t;


/** @} */

/** @{ */

struct booth_site {
	/** Calculated ID. See add_site(). */
	int site_id;
	int type;
	int local;

	/** Roles, like ACCEPTOR, PROPOSER, or LEARNER. Not really used ATM. */
	int role;

	char addr_string[BOOTH_NAME_LEN];

	int tcp_fd;
	int udp_fd;

	/* 0-based, used for indexing into per-ticket weights */
	int index;
	uint64_t bitmask;

	unsigned short family;
	union {
		struct sockaddr_in  sa4;
		struct sockaddr_in6 sa6;
	};
	int saddrlen;
	int addrlen;
} __attribute__((packed));



extern struct booth_site *local;
extern struct booth_site * no_leader;

/** @} */

struct booth_transport;

struct client {
	int fd;
	const struct booth_transport *transport;
	void (*workfn)(int);
	void (*deadfn)(int);
};

extern struct client *clients;
extern struct pollfd *pollfds;


int client_add(int fd, const struct booth_transport *tpt,
		void (*workfn)(int ci), void (*deadfn)(int ci));
int do_read(int fd, void *buf, size_t count);
int do_write(int fd, void *buf, size_t count);
void process_connection(int ci);
void safe_copy(char *dest, char *value, size_t buflen, const char *description);


struct command_line {
	int type;		/* ACT_ */
	int op;			/* OP_ */
	char configfile[BOOTH_PATH_LEN];
	char lockfile[BOOTH_PATH_LEN];

	char site[BOOTH_NAME_LEN];
	struct boothc_ticket_msg msg;
};
extern struct command_line cl;



/* http://gcc.gnu.org/onlinedocs/gcc/Typeof.html */
#define min(a__,b__) \
	({ typeof (a__) _a = (a__); \
	 typeof (b__) _b = (b__); \
	 _a < _b ? _a : _b; })
#define max(a__,b__) \
	({ typeof (a__) _a = (a__); \
	 typeof (b__) _b = (b__); \
	 _a > _b ? _a : _b; })





#endif /* _BOOTH_H */
