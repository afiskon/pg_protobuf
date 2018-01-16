CREATE EXTENSION pg_protobuf;

SELECT protobuf_decode(null);

SELECT protobuf_decode('garbage' :: bytea);

SELECT protobuf_decode(E'\\x0a0365617810321880022202100f' :: bytea);

SELECT protobuf_decode(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea);

SELECT protobuf_get_int(null, 1);

SELECT protobuf_get_int(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea, 1);

SELECT protobuf_get_int(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea, 2);

SELECT protobuf_get_int(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea, 3);

SELECT protobuf_get_bytes(null, 1);

SELECT protobuf_get_bytes(E'\\x0a0365617810321880022202100f' :: bytea, 1);

SELECT protobuf_get_bytes(E'\\x0a0365617810321880022202100f' :: bytea, 4);

SELECT protobuf_get_bytes(E'\\x0a0365617810321880022202100f' :: bytea, 5);

SELECT protobuf_get_bytes(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea, 1);

SELECT protobuf_get_bytes(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea, 4);

SELECT protobuf_get_bytes(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea, 5);

SELECT protobuf_get_string(E'\\x0a0365617810321880022202100f' :: bytea, 1);

SELECT protobuf_get_string(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea, 1);

SELECT protobuf_get_string(E'\\x10191880082a060a0200011064822007616669736b6f6e' :: bytea, 512);

SELECT protobuf_get_string(E'\\x10321880022202100ffaa3e80303656178', 999999);

SELECT protobuf_get_string(E'\\x10321880022202100f82200ad0a8d183d180d0b8d0ba' :: bytea, 512);

SELECT length(protobuf_get_string(E'\\x10321880022202100f82208e02' || convert_to(repeat('a', 270), 'utf-8'), 512));

SELECT protobuf_decode(E'\\x0a036561781088ef99abc5e88c91111880022202100f');

SELECT protobuf_get_int(E'\\x0a036561781088ef99abc5e88c91111880022202100f', 2);

SELECT protobuf_decode(E'\\x0a0365617810c0bbf8ffffffffffff0118f890e6d4ba97f3eeee012202100f');

SELECT protobuf_get_int(E'\\x0a0365617810c0bbf8ffffffffffff0118f890e6d4ba97f3eeee012202100f', 2);

SELECT protobuf_get_int(E'\\x0a0365617810c0bbf8ffffffffffff0118f890e6d4ba97f3eeee012202100f', 3);

SELECT protobuf_get_int(E'\\x0a0365617810321880022202100f306f30de0130cd02', 6);

SELECT protobuf_get_int_multi(E'\\x0a0365617810321880022202100f306f30de0130cd02', 6);

SELECT protobuf_get_sfixed32(E'\\x0a0365617815320000001d00ffffff2202100f', 2);

SELECT protobuf_get_sfixed32(E'\\x0a0365617815320000001d00ffffff2202100f', 3);

SELECT protobuf_get_sfixed32(E'\\x0a0365617810321880022202100f35bc01000035d5fdffff359a020000', 6);

SELECT protobuf_get_sfixed32_multi(E'\\x0a0365617810321880022202100f35bc01000035d5fdffff359a020000', 6);

SELECT protobuf_get_float(E'\\x0a036561781500004a421d333380c32202100f', 2);

SELECT protobuf_get_float(E'\\x0a036561781500004a421d333380c32202100f', 3);

SELECT protobuf_get_float(E'\\x0a0365617810321880022202100f35000048413500000ac23500000000', 6);

SELECT protobuf_get_float_multi(E'\\x0a0365617810321880022202100f35000048413500000ac23500000000', 6);

SELECT protobuf_get_sfixed64(E'\\x0a036561781132000000000000001966666666660670c02202100f', 2);

SELECT protobuf_get_sfixed64(E'\\x0a07616669736b6f6e11e7ffffffffffffff1933333333330390402a060a0200011064', 2);

SELECT protobuf_get_sfixed64(E'\\x0a0365617810321880022202100f310b0000000000000031eaffffffffffffff', 6);

SELECT protobuf_get_sfixed64_multi(E'\\x0a0365617810321880022202100f310b0000000000000031eaffffffffffffff', 6);

SELECT protobuf_get_double(E'\\x0a036561781132000000000000001966666666660670c02202100f', 3);

SELECT protobuf_get_double(E'\\x0a07616669736b6f6e11e7ffffffffffffff1933333333330390402a060a0200011064', 3);

SELECT protobuf_get_double_multi(E'\\x0a0365617810321880022202100f319a99999999d95e40316666666666867cc0', 6);

SELECT protobuf_get_bool(E'\\x0a03656178107b18c8032202100f', 6);

SELECT protobuf_get_bool(E'\\x0a07616669736b6f6e10191880082a060a02000110643001', 6);

DROP EXTENSION pg_protobuf;
