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
 * summary of the Metis LICENSE file; the license in that file is
 * legally binding.
 */

// Define the RPC stub table which is shared among both the client and
// the server.

#ifndef _INTERFACE_H
#define _INTERFACE_H

typedef enum {
	RPC_HELLO,
	RPC_NOP,
	RPC_OLLEH
} rpc_id_t;

#endif  /* _INTERFACE_H */
