-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_protobuf" to load this file. \quit

CREATE FUNCTION protobuf_decode(bytea)
    RETURNS cstring
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION protobuf_get_integer(bytea, int)
    RETURNS int
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION protobuf_get_bytes(bytea, int)
    RETURNS bytea
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION protobuf_get_string(data bytea, tag int) RETURNS text AS $$
BEGIN
	RETURN convert_from(protobuf_get_bytes(data, tag), 'utf-8');
END
$$ LANGUAGE 'plpgsql' IMMUTABLE;
