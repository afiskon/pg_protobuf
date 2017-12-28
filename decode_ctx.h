#ifndef _DECODE_CTX_H_
#define _DECODE_CTX_H_

#include <postgres.h>
#include <port.h>

typedef struct {
	const uint8* protobuf_data;
	Size protobuf_size;
} ProtobufDecodeCtx; // for internal usage

void protobuf_decode_ctx_init(ProtobufDecodeCtx* ctx, const uint8* protobuf_data, Size protobuf_size);
void protobuf_decode_ctx_shift(ProtobufDecodeCtx* ctx, uint64 bytes);

#endif // _DECODE_CTX_H_
