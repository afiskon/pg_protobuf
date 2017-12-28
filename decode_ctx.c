#include "decode_ctx.h"
#include <postgres.h>
#include <port.h>

/* Init `ProtobufDecodeCtx` structure */
void protobuf_decode_ctx_init(ProtobufDecodeCtx* ctx, const uint8* protobuf_data, Size protobuf_size) {
	ctx->protobuf_data = protobuf_data;
	ctx->protobuf_size = protobuf_size;
	ctx->protobuf_orig_size = protobuf_size;
}

/* Shift `ProtobufDecodeCtx` to `bytes` bytes */
void protobuf_decode_ctx_shift(ProtobufDecodeCtx* ctx, uint64 bytes) {
	if(ctx->protobuf_size < bytes)
		ereport(ERROR,
			(
			 errcode(ERRCODE_INTERNAL_ERROR),
			 errmsg("Unexpected end of the protobuf data."),
			 errdetail("protobuf_decode_ctx_shift() - %lu bytes left while trying to skip next %lu bytes.",
				ctx->protobuf_size, bytes),
			 errhint("Protobuf data is corrupted or there is a bug in pg_protobuf.")
			));

	ctx->protobuf_data += bytes;
	ctx->protobuf_size -= bytes;
}

/* Calculate current offset */
Size protobuf_decode_ctx_offset(const ProtobufDecodeCtx* ctx) {
	return ctx->protobuf_orig_size - ctx->protobuf_size;
}
