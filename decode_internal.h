/* vim: set ai et ts=4 sw=4: */

#ifndef _DECODE_INTERNAL_H_
#define _DECODE_INTERNAL_H_

#include <postgres.h>
#include <port.h>

#define PROTOBUF_TYPE_INTEGER 0
#define PROTOBUF_TYPE_FIXED64 1
#define PROTOBUF_TYPE_BYTES   2
#define PROTOBUF_TYPE_FIXED32 5

#define PROTOBUF_RESULT_MAX_FIELDS 256

typedef struct {
    uint32 tag;
    uint8 type;
    int64 value_or_length;
    uint32 offset; // for PROTOBUF_TYPE_BYTES only
} ProtobufFieldInfo;

typedef struct {
    uint32 nfields;
    ProtobufFieldInfo field_info[PROTOBUF_RESULT_MAX_FIELDS];
} ProtobufDecodeResult;

void protobuf_decode_internal(const uint8* protobuf_data, Size protobuf_size, ProtobufDecodeResult* result);

#endif // _DECODE_INTERNAL_H_ 
