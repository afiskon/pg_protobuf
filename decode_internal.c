#include "decode_ctx.h"
#include "decode_internal.h"
#include <postgres.h>
#include <port.h>

/* Decode Protobuf structure. */
void protobuf_decode_internal(const uint8* protobuf_data, Size protobuf_size, ProtobufDecodeResult* result) {
	ProtobufDecodeCtx* ctx;
	ProtobufDecodeCtx decode_ctx;
	ctx = &decode_ctx;
	protobuf_decode_ctx_init(ctx, protobuf_data, protobuf_size);
	result->nfields = 0;

	while(ctx->protobuf_size > 0) {
		uint8 cont = (*(ctx->protobuf_data)) >> 7;
		uint32 tag = ((*(ctx->protobuf_data)) >> 3) & 0x0F;
		uint8 type = (*(ctx->protobuf_data)) & 0x07;
		uint64 value_or_length = 0;
		uint8 shift;

		if((type != PROTOBUF_TYPE_INTEGER) && (type != PROTOBUF_TYPE_BYTES))
			ereport(ERROR,
				(
				 errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Usupported protobuf type."),
				 errdetail("protobuf_decode_internal() doesn't support type %u yet.", type),
				 errhint("Sorry for that :( Patches are welcome!")
				));

		protobuf_decode_ctx_shift(ctx, 1);

		/* Read the tag */
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

		/* Read value_or_length */
		shift = 0;
		for(;;) {
			uint8 temp;

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

		if(result->nfields == PROTOBUF_RESULT_MAX_FIELDS)
			ereport(ERROR,
				(
				 errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Protobuf data contains to many fields."),
				 errdetail("protobuf_decode_internal() - ProtobufDecodeResult structure can't fit all the fields."),
				 errhint("You can increase PROTOBUF_RESULT_MAX_FIELDS constant. "
						"Also it's probably worth notifying the author regarding this issue.")
				));

		/* Save the field */
		result->field_info[result->nfields].tag = tag;
		result->field_info[result->nfields].type = type;
		result->field_info[result->nfields].value_or_length = (int64)value_or_length;
		if(type == PROTOBUF_TYPE_BYTES) {
			result->field_info[result->nfields].offset = protobuf_decode_ctx_offset(ctx);
			protobuf_decode_ctx_shift(ctx, value_or_length);
		} else
			result->field_info[result->nfields].offset = 0;

		result->nfields++;
	}
}

