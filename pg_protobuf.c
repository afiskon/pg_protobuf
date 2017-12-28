#include "decode_internal.h"
#include <postgres.h>
#include <port.h>
#include <utils/builtins.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(protobuf_decode);
PG_FUNCTION_INFO_V1(protobuf_get_integer);
PG_FUNCTION_INFO_V1(protobuf_get_bytes);

/* protobuf -> cstring */
Datum protobuf_decode(PG_FUNCTION_ARGS) {
	char temp_buff[128];
	uint32 i;
	int len;

	bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
	Size protobuf_size = VARSIZE(protobuf_bytea) - VARHDRSZ;
	const uint8* protobuf_data = (const uint8*)VARDATA(protobuf_bytea);

	size_t result_buff_size = 1024;
	size_t result_buff_free = result_buff_size;
	char* result_buff = palloc(result_buff_size);

	ProtobufDecodeResult decode_result;
	protobuf_decode_internal(protobuf_data, protobuf_size, &decode_result);

	for(i = 0; i < decode_result.nfields; i++) {
		len = snprintf(temp_buff, sizeof(temp_buff),
			"field tag = %u, field_type = %s, %s = %ld, offset = %u\n",
			decode_result.field_info[i].tag,
			( decode_result.field_info[i].type == PROTOBUF_TYPE_INTEGER ? "integer" : "bytes"),
			( decode_result.field_info[i].type == PROTOBUF_TYPE_INTEGER ? "value" : "length"),
			decode_result.field_info[i].value_or_length,
			decode_result.field_info[i].offset);

		if(len < 0)
			ereport(ERROR,
				(
				 errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Unable to format protobuf data."),
				 errdetail("protobuf_to_string() - snprintf() returned %d.", len),
				 errhint("Most likely this is a bug in pg_protobuf. Please contact the author.")
				));

		if(len >= result_buff_free) {
			result_buff_free += result_buff_size;
			result_buff_size *= 2;
			repalloc(result_buff, result_buff_size);
		}

		memcpy(result_buff + (result_buff_size - result_buff_free), temp_buff, len+1);
		result_buff_free -= len;
	}

	PG_RETURN_CSTRING((Datum)result_buff);
}

/* protobuf -> integer */
Datum protobuf_get_integer(PG_FUNCTION_ARGS) {
	bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
	int32 i, tag = PG_GETARG_INT32(1);
	Size protobuf_size = VARSIZE(protobuf_bytea) - VARHDRSZ;
	const uint8* protobuf_data = (const uint8*)VARDATA(protobuf_bytea);
	ProtobufDecodeResult decode_result;

	protobuf_decode_internal(protobuf_data, protobuf_size, &decode_result);

	for(i = 0; i < decode_result.nfields; i++) {
		if(decode_result.field_info[i].tag == tag) {
			if(decode_result.field_info[i].type != PROTOBUF_TYPE_INTEGER)
				/* if necessary, we can easily implement prtobuf_get_*_strict that whould throw an error */
				PG_RETURN_NULL();

			PG_RETURN_INT64((int64)decode_result.field_info[i].value_or_length);
		}
	}	

	/* not found */
	PG_RETURN_NULL();
}

/* protobuf -> bytea */
Datum protobuf_get_bytes(PG_FUNCTION_ARGS) {
	bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
	int32 i, tag = PG_GETARG_INT32(1);
	Size protobuf_size = VARSIZE(protobuf_bytea) - VARHDRSZ;
	const uint8* protobuf_data = (const uint8*)VARDATA(protobuf_bytea);
	ProtobufDecodeResult decode_result;

	protobuf_decode_internal(protobuf_data, protobuf_size, &decode_result);

	for(i = 0; i < decode_result.nfields; i++) {
		if(decode_result.field_info[i].tag == tag) {
			if(decode_result.field_info[i].type != PROTOBUF_TYPE_BYTES)
				/* if necessary, we can easily implement prtobuf_get_*_strict that whould throw an error */
				PG_RETURN_NULL();
			else {
				uint64 length = (uint64)decode_result.field_info[i].value_or_length;
				uint8* buffer = palloc(VARHDRSZ + length);
				SET_VARSIZE(buffer, VARHDRSZ + length);
				memcpy(VARDATA(buffer), protobuf_data + decode_result.field_info[i].offset, length);
				PG_RETURN_BYTEA_P(buffer);
			}	
		}
	}	

	/* not found */
	PG_RETURN_NULL();
}

