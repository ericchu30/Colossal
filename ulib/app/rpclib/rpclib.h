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

/*
 * RPCLIB, version 1.2
 *
 * This is a simple multi-threaded asynchronous RPC toolbox which
 * provides:
 *   1 C++ classes for developing efficient RPC clients and servers.
 *   2 Abstraction of the asynchronous socket programming and
 *     threading.
 *   3 Helper classes for packing and unpacking messages between
 *     RPC clients and servers.
 */

#ifndef _RPCLIB_H
#define _RPCLIB_H

#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h>
#include <stdarg.h>
#include <vector>

namespace rpclib {

// TCP socket options
extern int set_nonblocking(int fd);
extern int set_tcp_nodelay(int fd);
extern int set_tcp_abort(int fd);
// Create an RPC socket with appropriate flags set
extern int rpc_socket();

class const_pack;

class pack {
public:
	pack();
	pack(void *buf, size_t len);

	void reinit(void *buf, size_t len);

	// get the buffer for the pack
	virtual operator void *();
	virtual operator const void *() const;

	// return a pointer to the body
	virtual void *body();
	virtual const void *body() const;

	// advance to the next adjacent pack
	pack&operator++();
	pack operator++(int);

	// equality check for the current pack
	// empty packs are considered equal to each other
	bool operator==(const pack &other) const;
	bool operator!=(const pack &other) const;

	uint32_t size() const;

	virtual uint32_t body_size() const;

	friend const_pack;

protected:
	ssize_t _paklen;
	size_t  _left;
	void *  _pos;
};

class const_pack {
public:
	const_pack();
	const_pack(const void *buf, size_t len);
	const_pack(const pack &it);

	void reinit(const void *buf, size_t len);

	// get the buffer for the pack
	virtual operator const void *() const;
	virtual const void *body() const;

	const_pack&operator++();
	const_pack operator++(int);

	// equality check for the current pack
	// empty packs are considered equal to each other
	bool operator==(const const_pack &other) const;
	bool operator!=(const const_pack &other) const;

	uint32_t size() const;

	virtual uint32_t body_size() const;

protected:
	ssize_t      _paklen;
	size_t       _left;
	const void * _pos;
};

// pack bytes into an pack
class pack_writer {
public:
	pack_writer(void *buf, size_t len);

	// return the start of the buffer, which is NOT necessarily
	// the current pack
	operator void *();

	// append a data buffer to the pack
	int put(const void *buf, size_t len);

	// return the number of bytes written, which is NOT
	// necessarily the size of the current pack
	size_t size() const;

protected:
	void * _buf;
	void * _pos;
	size_t _left;
};

// The RPC request structure, which is used in rpc_client class to
// issue RPC calls. Upon the completion of a successful RPC call, the
// @obuf will be filled with the message from the RPC server, and @len
// will be set to the size of the message. Note that @olen must first
// initialized to the size of @obuf buffer.
struct rpc_call {
	int   id;    // rpc id
	const void *ibuf;  // input  buffer
	int   ilen;  // input  buffer len
	void *obuf;  // output buffer
	int   olen;  // output buffer len

	rpc_call(int i = 0, const void *pi = NULL, int ni = 0,
		 void *po = NULL, int no = 0)
	: id(i), ibuf(pi), ilen(ni), obuf(po), olen(no) { }

	// whether the RPC call returns
	bool returnable() const
	{ return obuf && olen > 0; }
};

// One-way RPC call that does not return
struct rpc_call1 : public rpc_call {
	rpc_call1(int i = 0, const void *pi = NULL, int ni = 0)
	: rpc_call(i, pi, ni, NULL, 0) { }
};

// Round-trip RPC call that returns
struct rpc_call2 : public rpc_call {
	rpc_call2(int i = 0, const void *pi = NULL, int ni = 0,
		  void *po = NULL, int no = 0)
	: rpc_call(i, pi, ni, po, no) { }
};

// A buffered writer used in RPC callbacks to write response message
// to the client. This provides support for asynchronous communication
// between RPC clients and servers.
class rpc_resp {
public:
	static const int RESP_BUFLEN = 32;

	// @cfd is a socket to a connected client
	rpc_resp(int cfd = -1);

	virtual ~rpc_resp();

	// add a response message
	int put(unsigned char id, const void *buf, int len);

	// flush all remaining data, this is automatically called when
	// the class instance destructs.
	int flush();

private:
	int   _cfd;
	char *_buf;
	char *_pos;
};

// RPC callback base class.
// Any RPC callback function must derive from this class and implement
// its operator(). Inside the RPC callback, response should be written
// using @res.
class rpc_callback {
public:
	virtual int
	operator()(unsigned char id, const void *buf, int len, rpc_resp &res) = 0;
};

// RPC protocol encoder.
class rpc_proto {
public:
	rpc_proto(void *buf = NULL, size_t len = 0);

	// Reinitialize the internal buffer. This does not affect the
	// stub table
	void reinit(void *buf, size_t len);

	class iterator : public pack {
	public:
		iterator(void *buf, size_t len)
		: pack(buf, len) { }

		iterator() { }

		unsigned char
		get_id() const  // get the id for the RPC call
		{ return *(unsigned char *)pack::body(); }

		// return a pointer to the RPC call message
		virtual	void *body()
		{ return (char *)pack::body() + 1; }

		virtual	const void *body() const
		{ return (const char *)pack::body() + 1; }

		virtual uint32_t body_size() const
		{ return pack::body_size() > 0? pack::body_size() - 1: 0; }
	};

	class const_iterator : public const_pack {
	public:
		const_iterator(const void *buf, size_t len)
		: const_pack(buf, len) { }

		const_iterator() { }

		const_iterator(const iterator &it)
		{ *(const_pack *)this = (const pack)it; }

		unsigned char
		get_id() const
		{ return *(unsigned char *)const_pack::body(); }

		virtual	const void *body() const
		{ return (const char *)const_pack::body() + 1; }

		virtual uint32_t body_size() const
		{ return const_pack::body_size() > 0? const_pack::body_size() - 1: 0; }
	};

	iterator begin();  // return an iterator of the encoded message
	const_iterator begin() const;

	iterator end();  // return an iterator denoting the end of the
			 // encoded message
	const_iterator end() const;

	// add RPC stubs, which should be used in conjunction with the
	// operator().
	// @num is TWICE the number of rpc_callbacks to add
	// Ex: add_stub(4, id1, rpc_cb1, id2, rpc_cb2)
	void add_stub(int num, ...);

	// Execute RPC calls provided in the buf, RPC stubs must be
	// set appropriately using add_stub().
	// RPC calls use the writer to output results.
	// Return the number of bytes processed, which can be less than @len.
	int operator()(rpc_resp &res, const void *buf, size_t len) const;

	// Execute all packed RPC calls
	int operator()(rpc_resp &res) const;

	// Encode an RPC call into the protocol buffer.
	int put(unsigned char id, const void *data, size_t size);

	size_t size() const;

private:
	void * _buf;
	void * _pos;
	size_t _len;
	size_t _left;
	std::vector<rpc_callback *> _rpc_cbs;
};

class rpc_client {
public:
	static const int MAX_REQ_LEN = 1024 * 1024;
	static const int MAX_RES_LEN = 1024 * 1024;

	// Timeout for a single RPC call in microseconds
	static const uint64_t RPC_TIMEOUT = 4000000;  // max bandwidth ~4Mbit/s

	rpc_client();

	virtual ~rpc_client();

	// Resolve host by name
	int resolve(const char *host, const char *port);

	// TCP connect, which uses the resolved address
	// Returns a socket fd
	int connect();

	// TCP connect, returns a socket fd
	static int connect(const char *host, const char *port);

	// Issue RPC calls.
	// @s: a connected socket to the server
	// @num: number of RPC calls follow. Each RPC call is provided
	// as an rpc_call * pointer.
	int operator()(int s, int num, ...);

	// This is basically a copy of operator(int s, int num, ...)
	// since C doesn't support forwarding invocation of a variadic
	// function.
	int operator()(const char *host, const char *port, int num, ...);

	// Timed talk to the remote server, the connection will be
	// shuted down after the talk
	// Return the number of bytes received in _resbuf
	int nonblock_talk(int s, uint32_t reqlen);

	// Extract data in the stub given by an iterator
	// Return the number of bytes copied to buf
	static int extract_stub(rpc_proto::const_iterator &stub, void *buf, int len);

private:
	char *     _reqbuf;
	char *     _resbuf;
	addrinfo * _addr;
};

class rpc_server {
public:
	// accept() timeout in microseconds. This constant is used to
	// enable stop_and_join() while the server is waiting for
	// incomining connections.
	static const int ACCEPT_TIMEOUT = 1000000;

	// @nworker specifies the number of concurrent worker threads
	// @port specifies the local port to use for the server
	// @prot provides stubs to execute RPC calls
	rpc_server(int nworker, int port, rpc_proto &prot);
	virtual ~rpc_server();

	// start accepting RPC calls
	int start();

	// wait for the dispatcher to stop, this can only happen when
	// an error occurs
	int join();

	// signal and wait for the dispatcher to stop
	int stop_and_join();

	friend void dispatcher(void *);
	friend void worker(void *);

private:
	int serv_fd();

	static void *dispatcher(void *param);  // listening thread
	static void *worker(void *param);  // worker thread

	unsigned short _port;
	int            _sfd;
	bool volatile  _stopped;
	rpc_proto &    _prot;
	sem_t          _sem_worker;
	pthread_t      _disp_tid;
};

}

#endif  /* _RPCLIB_H */
