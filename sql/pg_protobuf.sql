CREATE EXTENSION pg_protobuf;

SELECT protobuf_decode(null);

SELECT protobuf_decode('garbage' :: bytea);

SELECT protobuf_decode(E'\\x0a0365617810321880022202100f' :: bytea);

SELECT protobuf_decode(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea);

SELECT protobuf_get_integer(null, 1);

SELECT protobuf_get_integer(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea, 1);

SELECT protobuf_get_integer(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea, 2);

SELECT protobuf_get_integer(E'\\x0a07616669736b6f6e10191880082a060a0200011064' :: bytea, 3);

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

SELECT protobuf_get_integer(E'\\x0a036561781088ef99abc5e88c91111880022202100f', 2);

SELECT protobuf_decode(E'\\x0a0365617810c0bbf8ffffffffffff0118f890e6d4ba97f3eeee012202100f');

SELECT protobuf_get_integer(E'\\x0a0365617810c0bbf8ffffffffffff0118f890e6d4ba97f3eeee012202100f', 2);

SELECT protobuf_get_integer(E'\\x0a0365617810c0bbf8ffffffffffff0118f890e6d4ba97f3eeee012202100f', 3);

SELECT protobuf_get_boolean(E'\\x0a03656178107b18c8032202100f', 6);

SELECT protobuf_get_boolean(E'\\x0a07616669736b6f6e10191880082a060a02000110643001', 6);

DROP EXTENSION pg_protobuf;
