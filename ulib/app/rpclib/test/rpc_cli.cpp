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

// RPC client implementation. The client sends a hello message to the
// server, and then the server sends a hello message back.

#include <stdio.h>
#include <string.h>
#include <ulib/util_hexdump.h>
#include <ulib/util_log.h>
#include "../rpclib.h"
#include "interface.h"

using namespace rpclib;

int main(int argc, char *argv[])
{
	rpc_client rpc_cli;
	rpc_call1  nop_stub(RPC_NOP, NULL, 0);
	rpc_call2  hello_stub;
	rpc_call2  olleh_stub;

	// input and output message buffers
	char ibuf[64];
	char obuf[64];

	char ibuf1[64];
	char obuf1[64];

	if (argc != 3) {
		fprintf(stderr, "usage: %s host port\n", argv[0]);
		return -1;
	}

	// pack messages into the input buffer
	pack_writer writer(ibuf, sizeof(ibuf));
	writer.put("Hello", 5);
	writer.put("world", 5);

	// fill a hello message
	hello_stub.id   = RPC_HELLO; // RPC id defined in interface.h
	hello_stub.ibuf = ibuf;
	hello_stub.ilen = writer.size(); // number of bytes written
	hello_stub.obuf = obuf;
	hello_stub.olen = sizeof(obuf1);

	// pack messages into the input buffer
	pack_writer writer1(ibuf1, sizeof(ibuf1));
	writer1.put("world", 5);
	writer1.put("Hello", 5);

	olleh_stub.id   = RPC_OLLEH; // RPC id defined in interface.h
	olleh_stub.ibuf = ibuf1;
	olleh_stub.ilen = writer1.size(); // number of bytes written
	olleh_stub.obuf = obuf1;
	olleh_stub.olen = sizeof(obuf1);

	const char *host = argv[1];
	const char *port = argv[2];

	for (int i = 0; i < 10000; ++i) {
		hello_stub.olen = sizeof(obuf);
		olleh_stub.olen = sizeof(obuf1);

		if (rpc_cli(host, port, 5,
			    &nop_stub,  &hello_stub, &nop_stub,
			    &olleh_stub, &nop_stub)) {
			ULIB_WARNING("Executing five RPC failed");
			return -1;
		}

		// reuse the address in the next loop
		host = NULL;
		port = NULL;

		ULIB_NOTICE("RPC call was successful");

		ULIB_DEBUG("Response length=%d", hello_stub.olen);
		if (hello_stub.olen >= (int)sizeof(obuf)) {
			ULIB_FATAL("Response length is invalid");
			return -1;
		}

		// Parse response
		const_pack pak(obuf, hello_stub.olen);
		while (pak != const_pack()) {
			printf("Message:\n");
			print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, pak.body(), pak.body_size());
			++pak;
		}

		// Parse response
		const_pack pak1(obuf1, olleh_stub.olen);
		while (pak1 != const_pack()) {
			printf("Message:\n");
			print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, pak1.body(), pak1.body_size());
			++pak1;
		}
	}

	printf("10000 queries!\n");

	return 0;
}
