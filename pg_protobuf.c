#include "decode_internal.h"
#include <postgres.h>
#include <port.h>
#include <catalog/pg_type.h>
#include <utils/builtins.h>
#include <utils/array.h>
#include <utils/lsyscache.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(protobuf_decode);
PG_FUNCTION_INFO_V1(protobuf_get_int_multi);
PG_FUNCTION_INFO_V1(protobuf_get_sfixed32_multi);
PG_FUNCTION_INFO_V1(protobuf_get_float);
PG_FUNCTION_INFO_V1(protobuf_get_sfixed64);
PG_FUNCTION_INFO_V1(protobuf_get_double);
PG_FUNCTION_INFO_V1(protobuf_get_bytes);

/* protobuf -> cstring */
Datum protobuf_decode(PG_FUNCTION_ARGS) {
	char temp_buff[128];
	uint32 i;
	int len = -1;

	bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
	Size protobuf_size = VARSIZE(protobuf_bytea) - VARHDRSZ;
	const uint8* protobuf_data = (const uint8*)VARDATA(protobuf_bytea);

	size_t result_buff_size = 1024;
	size_t result_buff_free = result_buff_size;
	char* result_buff = palloc(result_buff_size);

	ProtobufDecodeResult decode_result;
	protobuf_decode_internal(protobuf_data, protobuf_size, &decode_result);

	for(i = 0; i < decode_result.nfields; i++) {
		switch(decode_result.field_info[i].type) {
		case PROTOBUF_TYPE_INTEGER:
			len = snprintf(temp_buff, sizeof(temp_buff),
				"type = integer, tag = %u, value = %ld\n",
				decode_result.field_info[i].tag,
				decode_result.field_info[i].value_or_length);
			break;
		case PROTOBUF_TYPE_FIXED64:
			len = snprintf(temp_buff, sizeof(temp_buff),
				"type = fixed64, tag = %u, int value = %ld, float value = %.02f\n",
				decode_result.field_info[i].tag,
				decode_result.field_info[i].value_or_length,
				*(double*)&decode_result.field_info[i].value_or_length);
			break;

		case PROTOBUF_TYPE_BYTES:
			len = snprintf(temp_buff, sizeof(temp_buff),
				"type = bytes, tag = %u, length = %ld, offset = %u\n",
				decode_result.field_info[i].tag,
				decode_result.field_info[i].value_or_length,
				decode_result.field_info[i].offset);
			break;
		case PROTOBUF_TYPE_FIXED32:
			len = snprintf(temp_buff, sizeof(temp_buff),
				"type = fixed32, tag = %u, int value = %d, float value = %.02f\n",
				decode_result.field_info[i].tag,
				(int32)decode_result.field_info[i].value_or_length,
				*(float*)&decode_result.field_info[i].value_or_length);
			break;
		default:
			ereport(ERROR,
				(
				 errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Unable to format protobuf data."),
				 errdetail("protobuf_decode() - unable to display type with id %u.", decode_result.field_info[i].type),
				 errhint("Most likely this is a bug in pg_protobuf. Please contact the author.")
				));
		}

		if(len < 0)
			ereport(ERROR,
				(
				 errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Unable to format protobuf data."),
				 errdetail("protobuf_decode() - snprintf() returned %d.", len),
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

/* protobuf -> int32[] / int64[] / etc */
Datum protobuf_get_int_multi(PG_FUNCTION_ARGS) {
	bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
	int32 i, tag = PG_GETARG_INT32(1);
	Size protobuf_size = VARSIZE(protobuf_bytea) - VARHDRSZ;
	const uint8* protobuf_data = (const uint8*)VARDATA(protobuf_bytea);
    Oid element_type = INT8OID;
    int  dims[MAXDIM], lbs[MAXDIM], nelements = 0, ndims = 1; // one dimension
    bool nulls[PROTOBUF_RESULT_MAX_FIELDS], typbyval;
    Datum elements[PROTOBUF_RESULT_MAX_FIELDS];
    ArrayType *result;
    int16 typlen;
    char typalign;
	ProtobufDecodeResult decode_result;

    /* get required info about the array element type */
    get_typlenbyvalalign(element_type, &typlen, &typbyval, &typalign);

	/* decode the data */
	protobuf_decode_internal(protobuf_data, protobuf_size, &decode_result);

	/* fill the array */
	for(i = 0; i < decode_result.nfields; i++) {
		if(decode_result.field_info[i].tag != tag)
			continue;

		if(decode_result.field_info[i].type != PROTOBUF_TYPE_INTEGER)
			/* if necessary, we can easily implement prtobuf_get_*_strict that whould throw an error */
			continue;

		elements[nelements] = Int64GetDatum((int64)decode_result.field_info[i].value_or_length);
		nulls[nelements] = false;
		nelements++;
	}

    /* build the PostgreSQL array representation */
    dims[0] = nelements; // number of elements
    lbs[0] = 1; // lower bound is 1
    result = construct_md_array(elements, nulls, ndims, dims, lbs,
                                element_type, typlen, typbyval, typalign);

    PG_RETURN_POINTER(result);
}

/* protobuf -> sfixed32 */
Datum protobuf_get_sfixed32_multi(PG_FUNCTION_ARGS) {
	bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
	int32 i, tag = PG_GETARG_INT32(1);
	Size protobuf_size = VARSIZE(protobuf_bytea) - VARHDRSZ;
	const uint8* protobuf_data = (const uint8*)VARDATA(protobuf_bytea);
    Oid element_type = INT4OID; // TODO changed
    int  dims[MAXDIM], lbs[MAXDIM], nelements = 0, ndims = 1; // one dimension
    bool nulls[PROTOBUF_RESULT_MAX_FIELDS], typbyval;
    Datum elements[PROTOBUF_RESULT_MAX_FIELDS];
    ArrayType *result;
    int16 typlen;
    char typalign;
	ProtobufDecodeResult decode_result;

    /* get required info about the array element type */
    get_typlenbyvalalign(element_type, &typlen, &typbyval, &typalign);

	/* decode the data */
	protobuf_decode_internal(protobuf_data, protobuf_size, &decode_result);

	/* fill the array */
	for(i = 0; i < decode_result.nfields; i++) {
		if(decode_result.field_info[i].tag != tag)
			continue;

		if(decode_result.field_info[i].type != PROTOBUF_TYPE_FIXED32) // TODO changed
			/* if necessary, we can easily implement prtobuf_get_*_strict that whould throw an error */
			continue;

		elements[nelements] = Int32GetDatum((int64)decode_result.field_info[i].value_or_length); // TODO changed
		nulls[nelements] = false;
		nelements++;
	}

    /* build the PostgreSQL array representation */
    dims[0] = nelements; // number of elements
    lbs[0] = 1; // lower bound is 1
    result = construct_md_array(elements, nulls, ndims, dims, lbs,
                                element_type, typlen, typbyval, typalign);

    PG_RETURN_POINTER(result);
}

/* protobuf -> float */
Datum protobuf_get_float(PG_FUNCTION_ARGS) {
	bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
	int32 i, tag = PG_GETARG_INT32(1);
	Size protobuf_size = VARSIZE(protobuf_bytea) - VARHDRSZ;
	const uint8* protobuf_data = (const uint8*)VARDATA(protobuf_bytea);
	ProtobufDecodeResult decode_result;

	protobuf_decode_internal(protobuf_data, protobuf_size, &decode_result);

	for(i = 0; i < decode_result.nfields; i++) {
		if(decode_result.field_info[i].tag == tag) {
			if(decode_result.field_info[i].type != PROTOBUF_TYPE_FIXED32)
				/* if necessary, we can easily implement prtobuf_get_*_strict that whould throw an error */
				PG_RETURN_NULL();

			PG_RETURN_FLOAT4(*(float*)&decode_result.field_info[i].value_or_length);
		}
	}

	/* not found */
	PG_RETURN_NULL();
}

/* protobuf -> sfixed64 */
Datum protobuf_get_sfixed64(PG_FUNCTION_ARGS) {
	bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
	int32 i, tag = PG_GETARG_INT32(1);
	Size protobuf_size = VARSIZE(protobuf_bytea) - VARHDRSZ;
	const uint8* protobuf_data = (const uint8*)VARDATA(protobuf_bytea);
	ProtobufDecodeResult decode_result;

	protobuf_decode_internal(protobuf_data, protobuf_size, &decode_result);

	for(i = 0; i < decode_result.nfields; i++) {
		if(decode_result.field_info[i].tag == tag) {
			if(decode_result.field_info[i].type != PROTOBUF_TYPE_FIXED64)
				/* if necessary, we can easily implement prtobuf_get_*_strict that whould throw an error */
				PG_RETURN_NULL();

			PG_RETURN_INT64(decode_result.field_info[i].value_or_length);
		}
	}

	/* not found */
	PG_RETURN_NULL();
}

/* protobuf -> double */
Datum protobuf_get_double(PG_FUNCTION_ARGS) {
	bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
	int32 i, tag = PG_GETARG_INT32(1);
	Size protobuf_size = VARSIZE(protobuf_bytea) - VARHDRSZ;
	const uint8* protobuf_data = (const uint8*)VARDATA(protobuf_bytea);
	ProtobufDecodeResult decode_result;

	protobuf_decode_internal(protobuf_data, protobuf_size, &decode_result);

	for(i = 0; i < decode_result.nfields; i++) {
		if(decode_result.field_info[i].tag == tag) {
			if(decode_result.field_info[i].type != PROTOBUF_TYPE_FIXED64)
				/* if necessary, we can easily implement prtobuf_get_*_strict that whould throw an error */
				PG_RETURN_NULL();

			PG_RETURN_FLOAT8(*(double*)&decode_result.field_info[i].value_or_length);
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

