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

		ULIB_DEBUG("Executing RPC call %d, message length:%d", id, len);

		print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, buf, len);

		// print the message from the client
		for (const_pack pak(buf, len); pak != const_pack(); ++pak) {
			printf("Message:\n");
			print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, pak.body(), pak.body_size());
		}

		pack_writer w(obuf, sizeof(obuf));
		w.put("Server", 6);
		w.put("says", 4);
		w.put("hello", 5);

		ULIB_WARNING("Response message size:%zu", w.size());

		return res.put(id, w, w.size());
	}
};

int main()
{
	char buf[100];
	hello_cb hcb;
	rpc_proto prot(buf, sizeof(buf));

	rpc_resp res(1);

	char ibuf[64];
	// pack messages into the input buffer
	pack_writer writer(ibuf, sizeof(ibuf));
	writer.put("Hello", 5);
	writer.put("world", 5);

	ULIB_DEBUG("Writer:");
	print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, writer, writer.size());

	prot.add_stub(2, RPC_HELLO, &hcb);
	prot.put(RPC_HELLO, writer, writer.size());
	ULIB_DEBUG("All RPC calls:");
	for (rpc_proto::const_iterator it = prot.begin(); it != prot.end(); ++it)
		print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, it.body(), it.body_size());

	rpc_proto prot1(buf, prot.size());
	ULIB_DEBUG("All RPC calls:");
	for (rpc_proto::const_iterator it = prot1.begin(); it != prot1.end(); ++it)
		print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, it.body(), it.body_size());

	ULIB_DEBUG("Protocol:");
	print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, buf, prot.size());
	ULIB_DEBUG("Processed:%d", prot(res));

	return 0;
}
