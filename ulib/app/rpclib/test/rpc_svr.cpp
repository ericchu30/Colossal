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

// RPC server implementation. The server receives the message from a
// client and then sends back a hello message.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ulib/util_hexdump.h>
#include <ulib/util_log.h>
#include "../rpclib.h"
#include "interface.h"

using namespace rpclib;

class hello_cb : public rpc_callback {
public:
	virtual int
	operator()(unsigned char id, const void *buf, int len, rpc_resp &res)
	{
		char obuf[64];

		if (id != RPC_HELLO) {
			ULIB_FATAL("RPC call id mismatches:got %u, expect %d", id, RPC_HELLO);
			return -1;
		}

		ULIB_DEBUG("Executing RPC call %d", id);

		// print the message from the client
		for (const_pack pak(buf, len); pak != const_pack(); ++pak) {
			printf("Message:\n");
			print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, pak.body(), pak.body_size());
		}

		pack_writer w(obuf, sizeof(obuf));
		w.put("Server", 6);
		w.put("says", 4);
		w.put("hello", 5);

		return res.put(id, w, w.size());
	}
};

class olleh_cb : public rpc_callback {
public:
	virtual int
	operator()(unsigned char id, const void *buf, int len, rpc_resp &res)
	{
		char obuf[64];

		if (id != RPC_OLLEH) {
			ULIB_FATAL("RPC call id mismatches:got %u, expect %d", id, RPC_OLLEH);
			return -1;
		}

		ULIB_DEBUG("Executing RPC call %d", id);

		// print the message from the client
		for (const_pack pak(buf, len); pak != const_pack(); ++pak) {
			printf("Message:\n");
			print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, pak.body(), pak.body_size());
		}

		pack_writer w(obuf, sizeof(obuf));
		w.put("hello", 5);
		w.put("says", 4);
		w.put("Server", 6);

		return res.put(id, w, w.size());
	}
};

class nop_cb : public rpc_callback {
public:
	virtual int
	operator()(unsigned char id, const void *buf, int len, rpc_resp &res)
	{
		if (id != RPC_NOP) {
			ULIB_FATAL("RPC call id mismatches:got %u, expect %d", id, RPC_NOP);
			return -1;
		}

		if (len != 0) {
			ULIB_FATAL("expected no input for NOP RPC call");
			return -1;
		}

		ULIB_NOTICE("NOP RPC called");

		return 0;
	}
};

int main(int argc, char *argv[])
{
	char req[4096];
	hello_cb hcb;
	olleh_cb ocb;
	nop_cb ncb;
	rpc_proto prot(req, sizeof(req));

	if (argc != 3) {
		fprintf(stderr, "usage: %s nworker port\n", argv[0]);
		return -1;
	}

	prot.add_stub(6, RPC_HELLO, &hcb, RPC_OLLEH, &ocb, RPC_NOP, &ncb);

	rpc_server svr(atoi(argv[1]), atoi(argv[2]), prot);

	svr.start();
	getchar();
//	svr.join();
	svr.stop_and_join();
	ULIB_WARNING("exiting...");

	return 0;
}
