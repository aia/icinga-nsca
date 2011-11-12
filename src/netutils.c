/****************************************************************************
 *
 * NETUTILS.C - NSCA Network Utilities
 *
 * License: GPL
 * Copyright (c) 1999-2006 Ethan Galstad (nagios@nagios.org)
 *
 * Last Modified: 01-21-2006
 *
 * Description:
 *
 * This file contains common network functions used in nrpe and check_nrpe.
 *
 * License Information:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ****************************************************************************/

#include "../include/common.h"
#include "../include/netutils.h"



/* opens a connection to a remote host/tcp port */
int my_tcp_connect(char *host_name, char *port, int *sd6) {
	int result;
	int rval;
	int success = 0;
	struct addrinfo addrinfo;
	struct addrinfo *res, *r;

	memset(&addrinfo, 0, sizeof(addrinfo));
	addrinfo.ai_family = PF_UNSPEC;
	addrinfo.ai_socktype = SOCK_STREAM;
	addrinfo.ai_protocol = IPPROTO_TCP;
	if (rval = getaddrinfo(host_name, port, &addrinfo, &res) != 0) {
		printf("Invalid host name '%s'\n", host_name);
		return STATE_UNKNOWN;
	}

	for (r = res; r; r = r->ai_next) {
		*sd6 = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
		result = connect(*sd6, r->ai_addr, r->ai_addrlen);

		if (result < 0) {
			switch (errno) {
			case ECONNREFUSED:
				printf("Connection refused by host\n");
				break;
			case ETIMEDOUT:
				printf("Timeout while attempting connection\n");
				break;
			case ENETUNREACH:
				printf("Network is unreachable\n");
				break;
			default:
				printf("Connection refused or timed out\n");
			}
			(void) close(*sd6);
		} else {
			success++;
			break;
		}
	}
	if (success == 0) {
		printf("Socket creation failed\n");
		freeaddrinfo(res);
		return STATE_CRITICAL;
	}
	freeaddrinfo(res);
	return STATE_OK;
}



/* sends all data - thanks to Beej's Guide to Network Programming */
int sendall(int s, char *buf, int *len) {
	int total = 0;
	int bytesleft = *len;
	int n = 0;

	while (total < *len) {
		n = send(s, buf + total, bytesleft, 0);
		if (n == -1)
			break;
		total += n;
		bytesleft -= n;
	}

	/* return number of bytes actually send here */
	*len = total;

	/* return -1 on failure, 0 on success */
	return n == -1 ? -1 : 0;
}


/* receives all data - modelled after sendall() */
int recvall(int s, char *buf, int *len, int timeout) {
	int total = 0;
	int bytesleft = *len;
	int n = 0;
	time_t start_time;
	time_t current_time;

	/* clear the receive buffer */
	bzero(buf, *len);

	time(&start_time);

	/* receive all data */
	while (total < *len) {

		/* receive some data */
		n = recv(s, buf + total, bytesleft, 0);

		/* no data has arrived yet (non-blocking socket) */
		if (n == -1 && errno == EAGAIN) {
			time(&current_time);
			if (current_time - start_time > timeout)
				break;
			sleep(1);
			continue;
		}

		/* receive error or client disconnect */
		else if (n <= 0)
			break;

		/* apply bytes we received */
		total += n;
		bytesleft -= n;
	}

	/* return number of bytes actually received here */
	*len = total;

	/* return <=0 on failure, bytes received on success */
	return (n <= 0) ? n : total;
}
