/* vim: set ai et ts=4 sw=4: */

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
PG_FUNCTION_INFO_V1(protobuf_get_float_multi);
PG_FUNCTION_INFO_V1(protobuf_get_sfixed64_multi);
PG_FUNCTION_INFO_V1(protobuf_get_double);
PG_FUNCTION_INFO_V1(protobuf_get_bytes);

typedef Datum (*FieldInfoToDatumCallback)(ProtobufFieldInfo*);

Datum protobuf_get_int_or_sfixed64_multi_callback(ProtobufFieldInfo* field_info);
Datum protobuf_get_sfixed32_multi_callback(ProtobufFieldInfo* field_info);
Datum protobuf_get_float_multi_callback(ProtobufFieldInfo* field_info);

Datum protobuf_get_any_multi_internal(
    bytea* protobuf_bytea, int32 tag, Oid element_type, uint8 expected_protobuf_type,
    FieldInfoToDatumCallback field_info_to_datum_cb);

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

/* Common code used in all protobuf_get_*_multi procedures */
Datum protobuf_get_any_multi_internal(
        bytea* protobuf_bytea, int32 tag, Oid element_type, uint8 expected_protobuf_type,
        FieldInfoToDatumCallback field_info_to_datum_cb) {
    Size protobuf_size = VARSIZE(protobuf_bytea) - VARHDRSZ;
    const uint8* protobuf_data = (const uint8*)VARDATA(protobuf_bytea);
    int  dims[MAXDIM], lbs[MAXDIM], nelements = 0, ndims = 1; /* one dimension */
    bool nulls[PROTOBUF_RESULT_MAX_FIELDS], typbyval;
    Datum elements[PROTOBUF_RESULT_MAX_FIELDS];
    int32 i;
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

        if(decode_result.field_info[i].type != expected_protobuf_type)
            /* if necessary, we can easily implement prtobuf_get_*_strict that whould throw an error */
            continue;

        elements[nelements] = field_info_to_datum_cb(&decode_result.field_info[i]);
        nulls[nelements] = false;
        nelements++;
    }

    /* build and return a pointer to ArrayType, PostgreSQL's array representation */
    dims[0] = nelements; /* number of elements */
    lbs[0] = 1; /* lower bound is 1 */
    PG_RETURN_POINTER(
        construct_md_array(elements, nulls, ndims, dims, lbs, element_type, typlen, typbyval, typalign)
    );
}

/* For internal usage in protobuf_get_{int,sfixed64}_multi procedures only */
Datum protobuf_get_int_or_sfixed64_multi_callback(ProtobufFieldInfo* field_info) {
    return Int64GetDatum(field_info->value_or_length);
}

/* protobuf -> int32[] / int64[] / etc */
Datum protobuf_get_int_multi(PG_FUNCTION_ARGS) {
    bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
    int32 tag = PG_GETARG_INT32(1);

    return protobuf_get_any_multi_internal(
        protobuf_bytea, tag, INT8OID, PROTOBUF_TYPE_INTEGER,
        protobuf_get_int_or_sfixed64_multi_callback);
}

/* For internal usage in protobuf_get_sfixed32_multi procedure only */
Datum protobuf_get_sfixed32_multi_callback(ProtobufFieldInfo* field_info) {
    return Int32GetDatum(field_info->value_or_length);
}

/* protobuf -> sfixed32[] */
Datum protobuf_get_sfixed32_multi(PG_FUNCTION_ARGS) {
    bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
    int32 tag = PG_GETARG_INT32(1);

    return protobuf_get_any_multi_internal(
        protobuf_bytea, tag, INT4OID, PROTOBUF_TYPE_FIXED32,
        protobuf_get_sfixed32_multi_callback);
}

/* For internal usage in protobuf_get_float_multi procedure only */
Datum protobuf_get_float_multi_callback(ProtobufFieldInfo* field_info) {
    return Float4GetDatum(*(float*)&(field_info->value_or_length));
}

/* protobuf -> float[] */
Datum protobuf_get_float_multi(PG_FUNCTION_ARGS) {
    bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
    int32 tag = PG_GETARG_INT32(1);

    return protobuf_get_any_multi_internal(
        protobuf_bytea, tag, FLOAT4OID, PROTOBUF_TYPE_FIXED32,
        protobuf_get_float_multi_callback);
}

/* protobuf -> sfixed64[] */
Datum protobuf_get_sfixed64_multi(PG_FUNCTION_ARGS) {
    bytea* protobuf_bytea = PG_GETARG_BYTEA_P(0);
    int32 tag = PG_GETARG_INT32(1);

    return protobuf_get_any_multi_internal(
        protobuf_bytea, tag, INT8OID, PROTOBUF_TYPE_FIXED64,
        protobuf_get_int_or_sfixed64_multi_callback);
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

