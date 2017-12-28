#include "decode_ctx.h"
#include "decode_internal.h"
#include <postgres.h>
#include <port.h>

void protobuf_decode_type_and_tag(ProtobufDecodeCtx* ctx, uint8* type_res, uint32* tag_res);
uint64 protobuf_decode_value_or_length(ProtobufDecodeCtx* ctx);

/* Decode type and tag */
void protobuf_decode_type_and_tag(ProtobufDecodeCtx* ctx, uint8* type_res, uint32* tag_res) {
	uint8 cont = (*(ctx->protobuf_data)) >> 7;
	uint32 tag = ((*(ctx->protobuf_data)) >> 3) & 0x0F;
	uint8 type = (*(ctx->protobuf_data)) & 0x07;
	uint8 shift;

	protobuf_decode_ctx_shift(ctx, 1);

	shift = 4;
	while(cont) {
		if((shift / 8) >= sizeof(tag))
			ereport(ERROR,
				(
				 errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Too long tag encoded in the protobuf data."),
				 errdetail("protobuf_decode_internal() - tag variable is "
							"uint32 and your protobuf stores larger tags."),
				 errhint("Sorry for that :( Patches are welcome!")
				));

		cont = (*(ctx->protobuf_data)) >> 7;
		tag = tag | (((uint32)((*(ctx->protobuf_data)) & 0x7F)) << shift);
		shift += 7;
		protobuf_decode_ctx_shift(ctx, 1);
	}

	*type_res = type;
	*tag_res = tag;
}

/* Decode value_or_length (int32, uint64, string length, etc) */
uint64 protobuf_decode_value_or_length(ProtobufDecodeCtx* ctx) {
	uint64 value_or_length = 0;
	uint8 shift = 0;
	uint8 temp;

	for(;;) {
		if((shift / 8) >= sizeof(value_or_length))
			ereport(ERROR,
				(
				 errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Too long integer encoded in the protobuf data."),
				 errdetail("protobuf_decode_internal() - value_or_length variable is "
							"uint64 and your protobuf stores larger integers."),
				 errhint("That should have never happen. Your data is probably corrupted.")
				));

		temp = *ctx->protobuf_data;
		protobuf_decode_ctx_shift(ctx, 1);

		value_or_length = value_or_length | ( (((uint64)temp) & 0x7F) << shift);
		shift += 7;

		if(!(temp & 0x80)) // 0b0xxxxxxx - the end of the integer
			break;
	}

	return value_or_length;
}

/* Decode Protobuf structure. */
void protobuf_decode_internal(const uint8* protobuf_data, Size protobuf_size, ProtobufDecodeResult* result) {
	ProtobufDecodeCtx* ctx;
	ProtobufDecodeCtx decode_ctx;
	ctx = &decode_ctx;
	protobuf_decode_ctx_init(ctx, protobuf_data, protobuf_size);
	result->nfields = 0;

	while(ctx->protobuf_size > 0) {
		uint8 type;
		uint32 tag, offset = 0;
		uint64 value_or_length;

		if(result->nfields == PROTOBUF_RESULT_MAX_FIELDS)
			ereport(ERROR,
				(
				 errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Protobuf data contains to many fields."),
				 errdetail("protobuf_decode_internal() - ProtobufDecodeResult structure can't fit all the fields."),
				 errhint("You can increase PROTOBUF_RESULT_MAX_FIELDS constant. "
						"Also it's probably worth notifying the author regarding this issue.")
				));

		/* Decode type and tag */
		protobuf_decode_type_and_tag(ctx, &type, &tag);

		switch(type) {
		case PROTOBUF_TYPE_INTEGER:
			/* Decode int32, uint64, etc */
			value_or_length = protobuf_decode_value_or_length(ctx);
			break;
		case PROTOBUF_TYPE_BYTES:
			/* Decode string length */
			value_or_length = protobuf_decode_value_or_length(ctx);
			/* Calculate string offset */
			offset = protobuf_decode_ctx_offset(ctx);
			/* Skip `length` bytes */
			protobuf_decode_ctx_shift(ctx, value_or_length);
			break;
		default:
			ereport(ERROR,
				(
				 errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Usupported protobuf type."),
				 errdetail("pg_protobuf doesn't support type with id %u yet.", type),
				 errhint("Sorry for that :( Patches are welcome!")
				));
		}

		/* Save the field */
		result->field_info[result->nfields].tag = tag;
		result->field_info[result->nfields].type = type;
		result->field_info[result->nfields].value_or_length = (int64)value_or_length;
		result->field_info[result->nfields].offset = offset;
		result->nfields++;
	}
}

