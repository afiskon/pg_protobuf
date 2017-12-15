// TODO: cleanup includes
#include <postgres.h>
#include <port.h>
#include <catalog/pg_type.h>
#include <executor/spi.h>
#include <utils/builtins.h>
#include <utils/jsonb.h>
#include <sys/time.h>
#include <limits.h>
#include <string.h>

#define PROTOBUF_TYPE_INTEGER 0
#define PROTOBUF_TYPE_BYTES   2

#define PROTOBUF_RESULT_MAX_FIELDS 256

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(protobuf_decode);
PG_FUNCTION_INFO_V1(protobuf_get_integer);
PG_FUNCTION_INFO_V1(protobuf_get_bytes);

typedef struct {
	uint32 tag;
	uint8 type;
	uint32 value_or_length;
	uint32 offset; // for PROTOBUF_TYPE_BYTES only
} ProtobufFieldInfo;

typedef struct {
	uint32 nfields;
	ProtobufFieldInfo field_info[PROTOBUF_RESULT_MAX_FIELDS];
} ProtobufDecodeResult;

void protobuf_decode_internal(const uint8* protobuf_data, Size protobuf_size, ProtobufDecodeResult* result);

// protobuf -> cstring
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
			"field tag = %u, field_type = %s, %s = %u, offset = %u\n",
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
				// if necessary, we can easily implement prtobuf_get_*_strict that whould throw an error
				PG_RETURN_NULL();

			PG_RETURN_INT32(decode_result.field_info[i].value_or_length);
		}
	}	

	// not found
	PG_RETURN_NULL();
}

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
				// if necessary, we can easily implement prtobuf_get_*_strict that whould throw an error
				PG_RETURN_NULL();
			else {
				uint32 length = decode_result.field_info[i].value_or_length;
				uint8* buffer = palloc(VARHDRSZ + length);
				SET_VARSIZE(buffer, VARHDRSZ + length);
				memcpy(VARDATA(buffer), protobuf_data + decode_result.field_info[i].offset, length);
				PG_RETURN_BYTEA_P(buffer);
			}	
		}
	}	

	// not found
	PG_RETURN_NULL();
}

// decode protobuf structure
void protobuf_decode_internal(const uint8* protobuf_data, Size protobuf_size, ProtobufDecodeResult* result) {
	Size protobuf_orig_size = protobuf_size;
	result->nfields = 0;

	while(protobuf_size > 0) {
		uint8 cont = (*protobuf_data) >> 7;
		uint32 tag = ((*protobuf_data) >> 3) & 0x0F;
		uint8 type = (*protobuf_data) & 0x07;
		uint32 value_or_length = 0;
		uint8 bytes_read, shift;

		if((type != PROTOBUF_TYPE_INTEGER) && (type != PROTOBUF_TYPE_BYTES))
			ereport(ERROR,
				(
				 errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Usupported protobuf type."),
				 errdetail("protobuf_decode_internal() doesn't support type %u yet.", type),
				 errhint("Sorry for that :( Patches are welcome!")
				));

		protobuf_data++;
		protobuf_size--;

		shift = 4;
		bytes_read = 1;
		while(cont) {
			if(bytes_read == 4)
				ereport(ERROR,
					(
					 errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("Too long tag encoded in the protobuf data."),
					 errdetail("protobuf_decode_internal() - tag variable is "
								"uint32 and your protobuf stores larger tags."),
					 errhint("Sorry for that :( Patches are welcome!")
					));

			if(protobuf_size == 0)
				ereport(ERROR,
					(
					 errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("Unexpected end of the protobuf data."),
					 errdetail("protobuf_decode_internal() - data ended while still reading tag variable."),
					 errhint("Protobuf data is corrupted or there is a bug in pg_protobuf.")
					));
	
			cont = (*protobuf_data) >> 7;
			tag = tag | (((*protobuf_data) & 0x7F) << shift);
			shift += 7;
			bytes_read++;

			protobuf_data++;
			protobuf_size--;
		}

		shift = 0;
		bytes_read = 0;
		for(;;) {
			uint8 temp;

			if(bytes_read == 4)
				ereport(ERROR,
					(
					 errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("Too long integer encoded in the protobuf data."),
					 errdetail("protobuf_decode_internal() - value_or_length variable is "
								"uint32 and your protobuf stores larger integers."),
					 errhint("Sorry for that :( Patches are welcome!")
					));
	
			if(protobuf_size == 0)
				ereport(ERROR,
					(
					 errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("Unexpected end of the protobuf data."),
					 errdetail("protobuf_decode_internal() - data ended while still reading value_or_length variable."),
					 errhint("Protobuf data is corrupted or there is a bug in pg_protobuf.")
					));
		
			temp = *protobuf_data;
			protobuf_data++;
			protobuf_size--;
			bytes_read++;

			value_or_length = value_or_length | ( (((uint32)temp) & 0x7F) << shift);
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

		result->field_info[result->nfields].tag = tag;
		result->field_info[result->nfields].type = type;
		result->field_info[result->nfields].value_or_length = value_or_length;
		if(type == PROTOBUF_TYPE_INTEGER)
			result->field_info[result->nfields].offset = 0;
		else
			result->field_info[result->nfields].offset = protobuf_orig_size - protobuf_size;
		result->nfields++;

		if(type == PROTOBUF_TYPE_BYTES) {
			// skip next value_or_length bytes
			if(protobuf_size < value_or_length)
				ereport(ERROR,
					(
					 errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("Unexpected end of the protobuf data."),
					 errdetail("protobuf_decode_internal() - %lu bytes left while trying to skip next %u bytes.",
						protobuf_size, value_or_length),
					 errhint("Protobuf data is corrupted or there is a bug in pg_protobuf.")
					));

			protobuf_data += value_or_length;
			protobuf_size -= value_or_length;
		} // type == PROTOBUF_TYPE_BYTES
	}
}

