/* Copyright (c) Zilong Tan (eric.zltan@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, subject to the conditions listed in the Metis LICENSE
 * file. These conditions include: you must preserve this copyright
 * notice, and you cannot mention the copyright holders in advertising
 * related to the Software without their permission.  The Software is
 * provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This notice is a
 * summary of the RPCLIB LICENSE file; the license in that file is
 * legally binding.
 */

// Version 1.2

#include <sys/time.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <algorithm>
#include <ulib/util_log.h>
#include "rpclib.h"

using namespace std;

namespace rpclib
{

// return a pointer to the body of the pack
template <typename T>
inline static T * pack_body(void *pak)
{
	return (T *)((char *)pak + 4);
}

template <typename T>
inline static T * pack_body(const void *pak)
{
	return (T *)((const char *)pak + 4);
}

// return the i-th element of the pack
template <typename T>
inline static T & pack_elem(void *pak, int i)
{
	return pack_body<T>(pak)[i];
}

// return the i-th element of the pack
template <typename T>
inline static T & pack_elem(const void *pak, int i)
{
	return pack_body<T>(pak)[i];
}

// return the size of the pack body
template <typename T>
static uint32_t pack_bodylen(const T &pak)
{
	return pak.size() - 4;
}

int set_nonblocking(int fd)
{
	int fl;

	if ((fl = fcntl(fd, F_GETFL)) < 0) {
		ULIB_WARNING("couldn't get fd flags:%s", strerror(errno));
		return -1;
	}
	if (fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0) {
		ULIB_WARNING("couldn't set fd flags:%s", strerror(errno));
		return -1;
	}
	fl = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&fl, sizeof(fl)) < 0) {
		ULIB_WARNING("couldn't set keep-alive flag");
		return -1;
	}
	return 0;
}

// optimization for small writes
int set_tcp_nodelay(int fd)
{
	int n = 1;

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&n, sizeof (n)) < 0) {
		ULIB_WARNING("couldn't set TCP_NODELAY:%s", strerror(errno));
		return -1;
	}
	if (setsockopt(fd, IPPROTO_IP, IP_TOS, (char *)&n, sizeof (n)) < 0) {
		ULIB_WARNING("couldn't set IP_TOS:%s", strerror(errno));
		return -1;
	}
	return 0;
}

// do not check if all data is transmitted upon close
int set_tcp_abort(int fd)
{
	linger l;

	l.l_onoff  = 1;
	l.l_linger = 0;

	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&l, sizeof(l)) < 0) {
		ULIB_WARNING("couldn't set SO_LINGER:%s", strerror(errno));
		return -1;
	}

	return 0;
}

int rpc_socket()
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		ULIB_WARNING("socket failed:%s", strerror(errno));
		return -1;
	}
	int v = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&v, sizeof(v)) < 0) {
		ULIB_WARNING("couldn't set SO_REUSEADDR:%s", strerror(errno));
		return -1;
	}
	if (set_tcp_nodelay(s) || set_tcp_abort(s))
		return -1;
	return s;
}

// RPC worker thread parameters
struct rpc_worker_param {
	int cfd;
	rpc_server *server;
};

pack::pack(void *buf, size_t len)
{
	reinit(buf, len);
}

pack::pack()
{
	reinit(NULL, 0);
}

void pack::reinit(void *buf, size_t len)
{
	if (buf && len >= 4 && *(uint32_t *) buf <= len) {
		// must point to an valid pack
		_paklen = *(uint32_t *)buf;  // current pack size
		_pos    = buf;  // start of the current pack
		_left   = len - _paklen;  // remaining buffer size
	} else {
		_paklen = 0;
		_left   = 0;
		_pos    = NULL;
	}
}

pack::operator void *()
{
	return _pos;
}

pack::operator const void *() const
{
	return _pos;
}

void *pack::body()
{
	return pack_body<void>(_pos);
}

const void *pack::body() const
{
	return pack_body<const void>(this);
}

pack & pack::operator++()
{
	if (_left >= 4) {
		void *next = (void *)((char *)_pos + _paklen);
		uint32_t nextlen = *(uint32_t *)next;
		if (nextlen <= _left) {
			_paklen = nextlen;
			_left  -= nextlen;
			_pos    = next;
			return *this;
		}
	}
	if (_paklen > 0)
		_paklen = -_paklen;  // marking the end of pack, the _buf will remain valid
	return *this;
}

pack pack::operator++(int)
{
	pack old = *this;
	this->operator++();
	return old;
}

bool pack::operator==(const pack & other) const
{
	if (_paklen <= 0 && other._paklen <= 0)
		return true;
	return _paklen == other._paklen && _pos == other._pos;
}

bool pack::operator!=(const pack & other) const
{
	return !operator==(other);
}

uint32_t pack::size() const
{
	return _paklen > 0? _paklen: -_paklen;
}

uint32_t pack::body_size() const
{
	if (_paklen > 4)
		return _paklen - 4;
	return 0;
}

const_pack::const_pack(const void *buf, size_t len)
{
	reinit(buf, len);
}

const_pack::const_pack()
{
	reinit(NULL, 0);
}

const_pack::const_pack(const pack & it)
{
	_paklen = it._paklen;
	_left   = it._left;
	_pos    = it._pos;
}

void const_pack::reinit(const void *buf, size_t len)
{
	if (buf && len >= 4 && *(uint32_t *) buf <= len) {
		_paklen = *(uint32_t *) buf;
		_pos    = buf;
		_left   = len - _paklen;
	} else {
		_paklen = 0;
		_left   = 0;
		_pos    = NULL;
	}
}

const_pack::operator const void *() const
{
	return _pos;
}

const void *const_pack::body() const
{
	return pack_body<const void>(_pos);
}

const_pack & const_pack::operator++()
{
	if (_left >= 4) {
		void *next = (void *)((char *)_pos + _paklen);
		uint32_t nextlen = *(uint32_t *)next;
		if (nextlen <= _left) {
			_paklen = nextlen;
			_left  -= nextlen;
			_pos    = next;
			return *this;
		}
	}
	if (_paklen > 0)
		_paklen = -_paklen;  // marking the end of pack, the _buf will remain valid
	return *this;
}

const_pack const_pack::operator++(int)
{
	const_pack old = *this;
	this->operator++();
	return old;
}

bool const_pack::operator==(const const_pack & other) const
{
	if (_paklen <= 0 && other._paklen <= 0)
		return true;
	return _paklen == other._paklen && _pos == other._pos;
}

bool const_pack::operator!=(const const_pack & other) const
{
	return !operator==(other);
}

uint32_t const_pack::size() const
{
	return _paklen > 0? _paklen: -_paklen;
}

uint32_t const_pack::body_size() const
{
	if (_paklen > 4)
		return _paklen - 4;
	return 0;
}

pack_writer::pack_writer(void *buf, size_t len)
{
	_buf  = buf;
	_pos  = buf;
	_left = len;
}

pack_writer::operator void *()
{
	return _buf;
}

int pack_writer::put(const void *buf, size_t len)
{
	if (_pos && len + 4 <= _left) {
		*(uint32_t *)_pos = len + 4;
		memcpy((char *)_pos + 4, buf, len);
		_pos = ((char *)_pos + 4 + len);
		_left -= 4 + len;
		return 0;
	}
	return -1;
}

// return the number of bytes written so far
size_t pack_writer::size() const
{
	return (char *)_pos - (char *)_buf;
}

rpc_resp::rpc_resp(int cfd)
	: _cfd(cfd)
{
	_buf = new char [RESP_BUFLEN];
	_pos = _buf;
}

rpc_resp::~rpc_resp()
{
	if (flush())
		ULIB_WARNING("There were bytes left unflushed");
	delete [] _buf;
}

int rpc_resp::put(unsigned char id, const void *buf, int len)
{
	const char *p = (const char *)buf;

	ULIB_DEBUG("Pushing %d bytes of response", len);

	if (_pos - _buf > RESP_BUFLEN - 5) {
		// not enought space for the size and id info
		if (flush())
			return -1;
	}
	*(uint32_t *)_pos = len + 5;
	_pos[4] = id;
	_pos += 5;

	int left = RESP_BUFLEN - (_pos - _buf);
	while (len > left) {
		memcpy(_pos, p, left);
		len  -= left;
		p    += left;
		_pos += left;
		if (flush())
			return -1;
		left = RESP_BUFLEN;
	}

	memcpy(_pos, p, len);
	_pos += len;

	return 0;
}

int rpc_resp::flush()
{
	int size = _pos - _buf;
	int nw = 0;

	ULIB_DEBUG("Flushing %d bytes", size);

	while (nw < size) {
		int nb = write(_cfd, _buf + nw, size - nw);
		if (nb <= 0) {
			if (errno == EINTR)
				continue;
			ULIB_WARNING("write failed:%s", strerror(errno));
			return -1;
		}
		nw += nb;
	}

	_pos = _buf;

	return 0;
}

rpc_proto::rpc_proto(void *buf, size_t len)
{
	_buf  = buf;
	_pos  = buf;
	_len  = len;
	_left = len;
	_rpc_cbs.assign(256, (rpc_callback *)NULL);
}

void rpc_proto::reinit(void *buf, size_t len)
{
	_buf  = buf;
	_pos  = buf;
	_len  = len;
	_left = len;
}

rpc_proto::iterator rpc_proto::begin()
{
	return rpc_proto::iterator(_buf, _len);
}

rpc_proto::const_iterator rpc_proto::begin() const
{
	return rpc_proto::const_iterator(_buf, _len);
}

rpc_proto::iterator rpc_proto::end()
{
	return rpc_proto::iterator();
}

rpc_proto::const_iterator rpc_proto::end() const
{
	return rpc_proto::const_iterator();
}

void rpc_proto::add_stub(int num, ...)
{
	va_list ap;

	if (num & 1) {
		ULIB_FATAL("num should be even, RPC id and callback should be paired");
		return;
	}

	va_start(ap, num);

	num >>= 1;
	while (num-- > 0) {
		unsigned char id = va_arg(ap, int);
		_rpc_cbs[id] = va_arg(ap, rpc_callback *);
		ULIB_DEBUG("added stub %d", id);
	}

	va_end(ap);
}

// Execute RPC calls provided in the buf, RPC stubs must be
// set appropriately using add_stub().
// RPC calls use the writer to output results.
// Return the number of bytes processed, which can be less than @len.
int rpc_proto::operator()(rpc_resp &res, const void *buf, size_t len) const
{
	rpc_proto::const_iterator it(buf, len);

	for (; it != end(); ++it) {
		if (((const_pack)it).body_size() == 0) {
			ULIB_WARNING("corrupted RPC stubs data stream");
			return -1;
		}
		unsigned char id = it.get_id();
		if (_rpc_cbs[id] == NULL) {
			ULIB_WARNING("RPC stub %u isn't set", id);
			continue;
		}
		ULIB_DEBUG("Invoking RPC stub %u", id);
		if ((*_rpc_cbs[id])(id, it.body(), it.body_size(), res)) {
			ULIB_WARNING("RPC stub %u failed", id);
			return -1;
		}
	}

	if (res.flush()) {
		ULIB_WARNING("failed to flush response");
		return -1;
	}

	return it? (const char *)(const void *)it + it.size() - (const char *)buf: 0;
}

// Execute all packed RPC calls
int rpc_proto::operator()(rpc_resp &res) const
{
	return (*this)(res, _buf, size());
}

// pack an RPC call into the buffer
int rpc_proto::put(unsigned char id, const void *data, size_t size)
{
	if (_pos && size + 5 <= _left) {
		*(uint32_t *)_pos = size + 5;
		*pack_body<unsigned char>(_pos) = id;
		memcpy(&pack_elem<unsigned char>(_pos, 1), data, size);
		_pos = (void *)((char *)_pos + 5 + size);
		_left -= 5 + size;
		return 0;
	}
	return -1;
}

size_t rpc_proto::size() const
{
	return (char *)_pos - (char *)_buf;
}

rpc_client::rpc_client()
	: _addr(NULL)
{
	signal(SIGPIPE, SIG_IGN);
	_reqbuf = new char [MAX_REQ_LEN];
	_resbuf = new char [MAX_RES_LEN];
}

rpc_client::~rpc_client()
{
	if (_addr)
		freeaddrinfo(_addr);

	delete [] _reqbuf;
	delete [] _resbuf;
}

int rpc_client::resolve(const char *host, const char *port)
{
	addrinfo hints;
	addrinfo *rp;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	int s = getaddrinfo(host, port, &hints, &rp);
	if (s) {
		ULIB_WARNING("couldn't resolve host=%s,port=%s:%s",
		             host, port, gai_strerror(s));
		return -1;
	}

	if (_addr)
		freeaddrinfo(_addr);
	_addr = rp;

	return 0;
}

int rpc_client::connect()
{
	if (_addr == NULL) {
		ULIB_WARNING("resolve() must be called first to get host address");
		return -1;
	}

	int sfd = rpc_socket();
	if (sfd == -1) {
		ULIB_FATAL("couldn't create RPC socket");
		return -1;
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sfd, (sockaddr *)&addr, sizeof(addr)) < 0) {
		ULIB_WARNING("bind failed:%s", strerror(errno));
		close(sfd);
		return -1;
	}

	addrinfo *p;
	for (p = _addr; p; p = p->ai_next) {
		if (::connect(sfd, p->ai_addr, p->ai_addrlen) != -1)
			break;
	}

	if (p == NULL) {
		ULIB_WARNING("failed to connect to any of the addresses");
		close(sfd);
		return -1;
	}

	return sfd;
}

int rpc_client::connect(const char *host, const char *port)
{
	addrinfo hints;
	addrinfo *rp, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	int s = getaddrinfo(host, port, &hints, &rp);
	if (s) {
		ULIB_WARNING("couldn't resolve host=%s,port=%s:%s",
		             host, port, gai_strerror(s));
		return -1;
	}

	int sfd = rpc_socket();
	if (sfd == -1) {
		ULIB_FATAL("couldn't create RPC socket");
		return -1;
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sfd, (sockaddr *)&addr, sizeof(addr)) < 0) {
		ULIB_WARNING("bind failed:%s", strerror(errno));
		close(sfd);
		freeaddrinfo(rp);
		return -1;
	}

	for (p = rp; p; p = p->ai_next) {
		if (::connect(sfd, p->ai_addr, p->ai_addrlen) != -1)
			break;
	}

	if (p == NULL) {
		ULIB_WARNING("couldn't resolve the address");
		close(sfd);
		sfd = -1;
	}

	freeaddrinfo(rp);
	return sfd;
}

// Issue RPC calls.
// @s: a connected socket to the server
// @num: number of RPC calls follow. Each RPC call is provided
// as an rpc_call *.
int rpc_client::operator()(int s, int num, ...)
{
	int ret = -1;
	int arg;
	rpc_proto prot(_reqbuf, MAX_REQ_LEN);
	rpc_proto::const_iterator it;
	va_list ap;

	if (num < 1) {
		ULIB_WARNING("no RPC was provided");
		return 0;
	}

	// enter nonblocking mode to speed up performance
	if (set_nonblocking(s)) {
		ULIB_WARNING("couldn't set nonblocking mode");
		return -1;
	}

	ULIB_DEBUG("Packing %d RPC calls ...", num);
	va_start(ap, num);
	for (arg = 0; arg < num; ++arg) {
		rpc_call *rpc = va_arg(ap, rpc_call *);
		ULIB_DEBUG("Packing RPC %u,ilen=%d", rpc->id, rpc->ilen);
		if (prot.put(rpc->id, rpc->ibuf, rpc->ilen)) {
			ULIB_FATAL("failed to pack RPC call %u,ilen=%d",
			           rpc->id, rpc->ilen);
			goto done;
		}
	}
	ULIB_DEBUG("RPC pack size=%zu", prot.size());
	int reslen;
	if ((reslen = nonblock_talk(s, prot.size())) < 0) {
		ULIB_WARNING("RPC talk failed");
		goto done;
	}
	ULIB_DEBUG("Received %d bytes of response", reslen);
	// unpack the response
	va_start(ap, num);
	prot.reinit(_resbuf, reslen);
	it = prot.begin();
	for (arg = 0;; ++arg, ++it) {
		rpc_call *rpc = va_arg(ap, rpc_call *);
		while (arg < num && !rpc->returnable()) {
			ULIB_DEBUG("Skipped nonreturnable RPC call %d", arg);
			++arg;
			rpc = va_arg(ap, rpc_call *);
		}
		if (arg == num)
			break;
		if (it == prot.end()) {
			ULIB_WARNING("missing response for the %d-th RPC call", arg);
			goto done;
		}
		if (((const_pack)it).body_size() == 0) {
			ULIB_WARNING("missing RPC id for the %d-th RPC call", arg);
			goto done;
		}
		if (it.get_id() != rpc->id) {
			ULIB_WARNING("RPC id mismatches: expected %u, got %u",
			             rpc->id, it.get_id());
			goto done;;
		}
		rpc->olen = extract_stub(it, rpc->obuf, rpc->olen);
		ULIB_DEBUG("Extracted results for RPC %d,olen=%d", rpc->id, rpc->olen);
	}
	if (it != prot.end()) {
		ULIB_WARNING("RPC server returned more than requested");
		goto done;
	}
	ULIB_DEBUG("RPC completed,reslen=%d", reslen);
	ret = 0;
done:
	va_end(ap);
	return ret;
}

// This is basically a copy of operator(int s, int num, ...)
// since C doesn't support forwarding invocation of a variadic
// function.
int rpc_client::operator()(const char *host, const char *port, int num, ...)
{
	int ret = -1;
	int arg;
	rpc_proto prot(_reqbuf, MAX_REQ_LEN);
	rpc_proto::const_iterator it;
	va_list ap;

	if (num < 1) {
		ULIB_WARNING("no RPC was provided");
		return 0;
	}

	if (host && port) {
		if (resolve(host, port)) {
			ULIB_WARNING("failed to resolve %s:%s", host, port);
			return -1;
		}
	}

	int s = connect();
	if (s == -1) {
		ULIB_WARNING("couldn't connect to the server");
		return -1;
	}

	// enter nonblocking mode to speed up performance
	if (set_nonblocking(s)) {
		ULIB_WARNING("couldn't set nonblocking mode");
		close(s);
		return -1;
	}

	ULIB_DEBUG("Packing %d RPC calls ...", num);
	va_start(ap, num);
	for (arg = 0; arg < num; ++arg) {
		rpc_call *rpc = va_arg(ap, rpc_call *);
		ULIB_DEBUG("Packing RPC %u,ilen=%d", rpc->id, rpc->ilen);
		if (prot.put(rpc->id, rpc->ibuf, rpc->ilen)) {
			ULIB_FATAL("Failed to pack RPC call %u,ilen=%d",
			           rpc->id, rpc->ilen);
			goto done;
		}
	}
	ULIB_DEBUG("RPC pack size=%zu", prot.size());
	int reslen;
	if ((reslen = nonblock_talk(s, prot.size())) < 0) {
		ULIB_WARNING("RPC talk failed");
		goto done;
	}
	ULIB_DEBUG("Received %d bytes of response", reslen);
	// unpack the response
	va_start(ap, num);
	prot.reinit(_resbuf, reslen);
	it = prot.begin();
	for (arg = 0;; ++arg, ++it) {
		rpc_call *rpc = va_arg(ap, rpc_call *);
		while (arg < num && !rpc->returnable()) {
			ULIB_DEBUG("Skipped nonreturnable RPC call %d", arg);
			++arg;
			rpc = va_arg(ap, rpc_call *);
		}
		if (arg == num)
			break;
		if (it == prot.end()) {
			ULIB_WARNING("missing response for the %d-th RPC call", arg);
			goto done;
		}
		if (((const_pack)it).body_size() == 0) {
			ULIB_WARNING("missing RPC id for the %d-th RPC call", arg);
			goto done;
		}
		if (it.get_id() != rpc->id) {
			ULIB_WARNING("RPC id mismatches: expected %u, got %u",
			             rpc->id, it.get_id());
			goto done;;
		}
		rpc->olen = extract_stub(it, rpc->obuf, rpc->olen);
		ULIB_DEBUG("Extracted results for RPC %d,olen=%d", rpc->id, rpc->olen);
	}
	if (it != prot.end()) {
		ULIB_WARNING("RPC server returned more than requested");
		goto done;
	}
	ULIB_DEBUG("RPC completed,reslen=%d", reslen);
	ret = 0;
done:
	close(s);
	va_end(ap);
	return ret;
}

// Timed talk to the remote server, the connection will be
// shuted down after the talk
// Return the number of bytes received in _resbuf
int rpc_client::nonblock_talk(int s, uint32_t reqlen)
{
	char *p = _reqbuf;
	char *q = _resbuf;
	int reslen = 0;
	int r, nb;
	fd_set rfs, wfs, trfs, twfs;
	timeval tv, ftv;

	if (reqlen > MAX_REQ_LEN) {
		ULIB_WARNING("RPC request is too long(%u), expected <= %d",
		             reqlen, MAX_REQ_LEN);
		shutdown(s, SHUT_RDWR);
		return -1;
	}

	FD_ZERO(&rfs);
	FD_ZERO(&wfs);
	FD_SET(s, &rfs);
	FD_SET(s, &wfs);
	ftv.tv_sec  = RPC_TIMEOUT / 1000000;
	ftv.tv_usec = RPC_TIMEOUT % 1000000;

	for (; reqlen || reslen < MAX_RES_LEN;) {
		tv   = ftv;
		trfs = rfs;
		twfs = wfs;
		r = select(s + 1, &trfs, &twfs, NULL, &tv);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			shutdown(s, SHUT_RDWR);
			ULIB_WARNING("select failed:%s", strerror(errno));
			break;
		}
		if (r == 0) {
			// must be a result of time out; otherwise the loop wouldn't have started
			ULIB_WARNING("RPC cancelled due to timeout");
			shutdown(s, SHUT_RDWR);
			break;
		}
		if (FD_ISSET(s, &twfs)) {
			nb = write(s, p, reqlen);
			if (nb <= 0) {
				if (errno == EINTR)
					continue;
				shutdown(s, SHUT_RDWR);
				ULIB_WARNING("write failed:%s", strerror(errno));
				break;
			}
			p += nb;
			reqlen -= nb;
			if (reqlen == 0) {
				FD_CLR(s, &wfs);
				if (shutdown(s, SHUT_WR) < 0)
					ULIB_WARNING("shutdown write side failed:%s", strerror(errno));
			}
		}
		if (FD_ISSET(s, &trfs)) {
			nb = read(s, q, MAX_RES_LEN - reslen);
			if (nb < 0) {
				if (errno == EINTR)
					continue;
				shutdown(s, SHUT_RDWR);
				ULIB_WARNING("read failed:%s", strerror(errno));
				break;
			}
			if (nb == 0)
				break;
			q += nb;
			reslen += nb;
			if (reslen == MAX_RES_LEN) {
				FD_CLR(s, &rfs);
				if (shutdown(s, SHUT_RD) < 0)
					ULIB_WARNING("shutdown read side failed:%s", strerror(errno));
			}
		}
	}

	return reslen;
}

// return the number of bytes copied to buf
int rpc_client::extract_stub(rpc_proto::const_iterator &stub, void *buf, int len)
{
	if (buf == NULL || len <= 0)
		return 0;

	// data size for the pack
	int size = stub.body_size();

	ULIB_DEBUG("RPC pack size=%d, buffer size=%d", size, len);

	if (size > len) {
		ULIB_WARNING("Pack size %d is larger than buffer size %d", size, len);
		size = len;
	}

	memcpy(buf, stub.body(), size);

	return size;
}

rpc_server::rpc_server(int nworker, int port, rpc_proto &prot)
	: _port(port), _sfd(-1), _stopped(true), _prot(prot)
{
	signal(SIGPIPE, SIG_IGN);
	sem_init(&_sem_worker, 0, nworker);
}

rpc_server::~rpc_server()
{
	stop_and_join();
	sem_destroy(&_sem_worker);
}

int rpc_server::start()
{
	if (_stopped) {
		int s = pthread_create(&_disp_tid, NULL, dispatcher, this);
		if (s < 0) {
			ULIB_WARNING("pthread_create failed with code %d", s);
			return -1;
		}
	}
	return 0;
}

int rpc_server::serv_fd()
{
	int s;
	sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	s = rpc_socket();
	if (s == -1) {
		ULIB_WARNING("couldn't create server socket");
		return -1;
	}

	/* resolve address in use error */
	int v = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&v, sizeof(v)) < 0) {
		ULIB_WARNING("setsockopt failed:%s", strerror(errno));
		close(s);
		return -1;
	}

	if (bind(s, (sockaddr *)&addr, sizeof(addr)) < 0) {
		ULIB_WARNING("bind failed:%s", strerror(errno));
		close(s);
		return -1;
	}

	if (::listen(s, 128) < 0) {
		ULIB_WARNING("listen failed:%s", strerror(errno));
		close(s);
		return -1;
	}

	return s;

}

int rpc_server::join()
{
	// let the dispatcher update the _stopped flag
	int s = pthread_join(_disp_tid, NULL);
	if (s < 0) {
		ULIB_WARNING("pthread_join failed with code %d", s);
		return -1;
	}
	return 0;
}

int rpc_server::stop_and_join()
{
	if (!_stopped) {
		_stopped = true;  // notify the dispatcher to exit
		return join();
	}
	return 0;
}

void * rpc_server::dispatcher(void *param)
{
	rpc_server *svr = (rpc_server *)param;

	svr->_stopped = false;

	int sfd = svr->serv_fd();
	if (sfd == -1) {
		ULIB_WARNING("unable to get server fd");
		goto done;
	}

	if (set_nonblocking(sfd)) {
		ULIB_WARNING("unable to set nonblocking");
		goto done;
	}

	fd_set rfs;
	FD_ZERO(&rfs);
	FD_SET(sfd, &rfs);

	timeval ftv;
	ftv.tv_sec  = ACCEPT_TIMEOUT / 1000000;
	ftv.tv_usec = ACCEPT_TIMEOUT % 1000000;

	ULIB_DEBUG("Wait for connections ...");
	for (; !svr->_stopped;) {
		fd_set trfs = rfs;
		timeval tv  = ftv;
		int s = select(sfd + 1, &trfs, NULL, NULL, &tv);
		if (s < 0) {
			if (errno == EINTR)
				continue;
			ULIB_WARNING("select failed:%s", strerror(errno));
			goto done;
		}
		if (s == 0)  // accept timeout
			continue;
		sockaddr_in addr;
		socklen_t addrlen = sizeof(addr);
		int cfd = accept(sfd, (sockaddr *)&addr, &addrlen);
		if (cfd == -1) {
			ULIB_WARNING("accept failed:%s", strerror(errno));
			goto done;
		}
		ULIB_DEBUG("Got connection, worker creation is pending");
		if (sem_wait(&svr->_sem_worker) < 0) {
			ULIB_WARNING("sem_wait failed:%s", strerror(errno));
			close(cfd);
			goto done;
		}
		rpc_worker_param *param = new rpc_worker_param;
		param->cfd = cfd;
		param->server = svr;
		pthread_t t;
		ULIB_DEBUG("creating RPC worker");
		s = pthread_create(&t, NULL, worker, param);
		if (s < 0) {
			close(cfd);
			delete param;
			ULIB_WARNING("unable to pthread_create worker:%d", s);
			goto done;
		}
	}

	ULIB_DEBUG("RPC server stop requested");
done:
	close(sfd);
	svr->_stopped = true;
	pthread_exit(NULL);
}

void * rpc_server::worker(void *param)
{
	pthread_detach(pthread_self());
	rpc_worker_param *wp = (rpc_worker_param *)param;

	ULIB_DEBUG("worker %zu started", pthread_self());
	char *reqbuf = new char [rpc_client::MAX_REQ_LEN];
	char *pos    = reqbuf;
	int reqlen   = 0;
	rpc_proto prot = wp->server->_prot;
	rpc_resp res(wp->cfd);

	fd_set rfs;
	FD_ZERO(&rfs);
	FD_SET(wp->cfd, &rfs);

	timeval ftv;
	ftv.tv_sec  = rpc_client::RPC_TIMEOUT / 1000000;
	ftv.tv_usec = rpc_client::RPC_TIMEOUT % 1000000;

	for (;;) {
		fd_set trfs = rfs;
		timeval tv  = ftv;
		int s = select(wp->cfd + 1, &trfs, NULL, NULL, &tv);
		if (s < 0) {
			if (errno == EINTR)
				continue;
			ULIB_WARNING("select failed:%s", strerror(errno));
			goto done;
		}
		if (s == 0) {
			ULIB_WARNING("RPC read request time out");
			goto done;
		}
		int nb = read(wp->cfd, reqbuf + reqlen, rpc_client::MAX_REQ_LEN - reqlen);
		if (nb < 0) {
			if (errno == EINTR)
				continue;
			ULIB_WARNING("read failed:%s", strerror(errno));
			goto done;
		}
		if (nb == 0) {
			ULIB_DEBUG("request read finished, reqlen=%d", reqlen);
			break;
		}
		reqlen += nb;
		int nr = prot(res, pos, reqlen - (pos - reqbuf));
		if (nr < 0) {
			ULIB_WARNING("failed to execute RPC calls");
			goto done;
		}
		ULIB_DEBUG("RPC call was successful, %d bytes processed", nr);
		pos += nr;
		if (reqlen == rpc_client::MAX_REQ_LEN) {
			ULIB_WARNING("Shutdown the read side due to full req buf");
			shutdown(wp->cfd, SHUT_RD);
			break; // can't read more
		}
	}

	// don't need to flush, every RPC execution flushes in the end
	ULIB_DEBUG("Successfully handled RPC requests");

done:
	delete [] reqbuf;
	shutdown(wp->cfd, SHUT_RDWR);
	close(wp->cfd);
	if (sem_post(&wp->server->_sem_worker) < 0)  // allow for another worker creation
		ULIB_WARNING("sem_post failed:%s", strerror(errno));
	delete wp;
	pthread_exit(NULL);
}

}
