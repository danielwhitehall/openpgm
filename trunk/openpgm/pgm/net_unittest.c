/* vim:ts=8:sts=8:sw=4:noai:noexpandtab
 *
 * unit tests for network send wrapper.
 *
 * Copyright (c) 2009 Miru Limited.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <check.h>

#include <pgm/transport.h>


/* mock state */

static
pgm_transport_t*
generate_transport (void)
{
	pgm_transport_t* transport = g_malloc0 (sizeof(pgm_transport_t));
	return transport;
}

static
char*
flags_string (
	int		flags
	)
{
	static char s[1024];

	s[0] = '\0';
	if (flags & MSG_OOB)
		strcat (s, "MSG_OOB");
#define MSG(flag) \
	do { \
		if (flags & flag) { \
			strcat (s, s[0] ? ("|" #flag) : (#flag)); \
		} \
	} while (0)
#ifdef MSG_PEEK
	MSG(MSG_PEEK);
#endif
#ifdef MSG_DONTROUTE
	MSG(MSG_DONTROUTE);
#endif
#ifdef MSG_CTRUNC
	MSG(MSG_CTRUNC);
#endif
#ifdef MSG_PROXY
	MSG(MSG_PROXY);
#endif
#ifdef MSG_TRUNC
	MSG(MSG_TRUNC);
#endif
#ifdef MSG_DONTWAIT
	MSG(MSG_DONTWAIT);
#endif
#ifdef MSG_EOR
	MSG(MSG_EOR);
#endif
#ifdef MSG_WAITALL
	MSG(MSG_WAITALL);
#endif
#ifdef MSG_FIN
	MSG(MSG_FIN);
#endif
#ifdef MSG_SYN
	MSG(MSG_SYN);
#endif
#ifdef MSG_CONFIRM
	MSG(MSG_CONFIRM);
#endif
#ifdef MSG_RST
	MSG(MSG_RST);
#endif
#ifdef MSG_ERRQUEUE
	MSG(MSG_ERRQUEUE);
#endif
#ifdef MSG_NOSIGNAL
	MSG(MSG_NOSIGNAL);
#endif
#ifdef MSG_MORE
	MSG(MSG_MORE);
#endif
#ifdef MSG_CMSG_CLOEXEC
	MSG(MSG_CMSG_CLOEXEC);
#endif
	if (!s[0]) {
		if (flags)
			sprintf (s, "0x%x", flags);
		else
			strcpy (s, "0");
	}
	return s;
}


/* mock functions for external references */

static
gboolean
mock__pgm_rate_check (
	gpointer		bucket,
	const guint		data_size,
	const int		flags		/* MSG_DONTWAIT = non-blocking */
	)
{
	g_debug ("mock__pgm_rate_check (bucket:%p data-size:%u flags:%s)",
		bucket, data_size, flags_string (flags));
	return TRUE;
}

static
ssize_t
mock_sendto (
	int			s,
	const void*		buf,
	size_t			len,
	int			flags,
	const struct sockaddr*	to,
	socklen_t		tolen
	)
{
	char saddr[INET6_ADDRSTRLEN];
	pgm_sockaddr_ntop (to, saddr, sizeof(saddr));
	g_debug ("mock_sendto (s:%i buf:%p len:%d flags:%s to:%s tolen:%d)",
		s, buf, len, flags_string (flags), saddr, tolen);
	return len;
}

#ifdef CONFIG_HAVE_POLL
static
int
mock_poll (
	struct pollfd*		fds,
	nfds_t			nfds,
	int			timeout
	)
{
	g_debug ("mock_poll (fds:%p nfds:%d timeout:%d)",
		(gpointer)fds, (int)nfds, timeout);
	return 0;
}
#else
static
int
mock_select (
	int			nfds,
	fd_set*			readfds,
	fd_set*			writefds,
	fd_set*			exceptfds,
	struct timeval*		timeout
	)
{
	g_debug ("mock_select (nfds:%d readfds:%p writefds:%p exceptfds:%p timeout:%p)",
		nfds, (gpointer)readfds, (gpointer)writefds, (gpointer)exceptfds, (gpointer)timeout);
	return 0;
}
#endif

static
int
mock_fcntl (
	int			fd,
	int			cmd,
	...
	)
{
	long arg;
	va_list args;
	if (F_GETFL == cmd) {
		g_debug ("mock_fcntl (fd:%d cmd:F_GETFL)", fd);
		return 0;
	}
	if (F_SETFL == cmd) {
		va_start (args, cmd);
		arg = va_arg (args, long);
		va_end (args);
		g_debug ("mock_fcntl (fd:%d cmd:F_SETFL arg:%d)", fd, arg);
		return arg;
	}
	g_assert_not_reached();
}

#define _pgm_rate_check		mock__pgm_rate_check
#define sendto			mock_sendto
#define poll			mock_poll
#define select			mock_select
#define fcntl			mock_fcntl

#define NET_DEBUG
#include "net.c"


/* target:
 *	gssize
 *	_pgm_sendto (
 *		pgm_transport_t*	transport,
 *		gboolean		use_rate_limit,
 *		gboolean		use_router_alert,
 *		const void*		buf,
 *		gsize			len,
 *		int			flags,
 *		const struct sockaddr*	to,
 *		gsize			tolen
 *	)
 */

START_TEST (test_sendto_pass_001)
{
	pgm_transport_t* transport = generate_transport ();
	const char buf[] = "i am not a string";
	struct sockaddr_in addr = {
		.sin_family		= AF_INET,
		.sin_addr.s_addr	= inet_addr ("172.12.90.1")
	};
	gssize len = _pgm_sendto (transport, FALSE, FALSE, buf, sizeof(buf), 0, (struct sockaddr*)&addr, sizeof(addr));
}
END_TEST

START_TEST (test_sendto_fail_001)
{
	const char buf[] = "i am not a string";
	struct sockaddr_in addr = {
		.sin_family		= AF_INET,
		.sin_addr.s_addr	= inet_addr ("172.12.90.1")
	};
	gssize len = _pgm_sendto (NULL, FALSE, FALSE, buf, sizeof(buf), 0, (struct sockaddr*)&addr, sizeof(addr));
	fail ();
}
END_TEST

START_TEST (test_sendto_fail_002)
{
	pgm_transport_t* transport = generate_transport ();
	const char buf[] = "i am not a string";
	struct sockaddr_in addr = {
		.sin_family		= AF_INET,
		.sin_addr.s_addr	= inet_addr ("172.12.90.1")
	};
	gssize len = _pgm_sendto (transport, FALSE, FALSE, NULL, sizeof(buf), 0, (struct sockaddr*)&addr, sizeof(addr));
	fail ();
}
END_TEST

START_TEST (test_sendto_fail_003)
{
	pgm_transport_t* transport = generate_transport ();
	const char buf[] = "i am not a string";
	struct sockaddr_in addr = {
		.sin_family		= AF_INET,
		.sin_addr.s_addr	= inet_addr ("172.12.90.1")
	};
	gssize len = _pgm_sendto (transport, FALSE, FALSE, buf, 0, 0, (struct sockaddr*)&addr, sizeof(addr));
	fail ();
}
END_TEST

START_TEST (test_sendto_fail_004)
{
	pgm_transport_t* transport = generate_transport ();
	const char buf[] = "i am not a string";
	struct sockaddr_in addr = {
		.sin_family		= AF_INET,
		.sin_addr.s_addr	= inet_addr ("172.12.90.1")
	};
	gssize len = _pgm_sendto (transport, FALSE, FALSE, buf, sizeof(buf), 0, NULL, sizeof(addr));
	fail ();
}
END_TEST

START_TEST (test_sendto_fail_005)
{
	pgm_transport_t* transport = generate_transport ();
	const char buf[] = "i am not a string";
	struct sockaddr_in addr = {
		.sin_family		= AF_INET,
		.sin_addr.s_addr	= inet_addr ("172.12.90.1")
	};
	gssize len = _pgm_sendto (transport, FALSE, FALSE, buf, sizeof(buf), 0, (struct sockaddr*)&addr, 0);
	fail ();
}
END_TEST

/* target:
 * 	int
 * 	_pgm_set_nonblocking (
 * 		int			filedes[2]
 * 	)
 */

START_TEST (test_set_nonblocking_pass_001)
{
	int fd = fileno (stdout);
	int retval = _pgm_set_nonblocking (&fd);
}
END_TEST

START_TEST (test_set_nonblocking_fail_001)
{
	int retval = _pgm_set_nonblocking (NULL);
	fail ();
}
END_TEST


static
Suite*
make_test_suite (void)
{
	Suite* s;

	s = suite_create (__FILE__);

	TCase* tc_sendto = tcase_create ("sendto");
	suite_add_tcase (s, tc_sendto);
	tcase_add_test (tc_sendto, test_sendto_pass_001);
	tcase_add_test_raise_signal (tc_sendto, test_sendto_fail_001, SIGABRT);
	tcase_add_test_raise_signal (tc_sendto, test_sendto_fail_002, SIGABRT);
	tcase_add_test_raise_signal (tc_sendto, test_sendto_fail_003, SIGABRT);
	tcase_add_test_raise_signal (tc_sendto, test_sendto_fail_004, SIGABRT);
	tcase_add_test_raise_signal (tc_sendto, test_sendto_fail_005, SIGABRT);

	TCase* tc_set_nonblocking = tcase_create ("set-nonblocking");
	suite_add_tcase (s, tc_set_nonblocking);
	tcase_add_test (tc_set_nonblocking, test_set_nonblocking_pass_001);
	tcase_add_test_raise_signal (tc_set_nonblocking, test_set_nonblocking_fail_001, SIGABRT);
	return s;
}

static
Suite*
make_master_suite (void)
{
	Suite* s = suite_create ("Master");
	return s;
}

int
main (void)
{
	SRunner* sr = srunner_create (make_master_suite ());
	srunner_add_suite (sr, make_test_suite ());
	srunner_run_all (sr, CK_ENV);
	int number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* eof */
