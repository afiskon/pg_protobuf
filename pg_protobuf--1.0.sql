-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_protobuf" to load this file. \quit

-- vim: set ai et ts=4 sw=4:
CREATE FUNCTION protobuf_decode(bytea)
    RETURNS cstring
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION protobuf_get_int_multi(bytea, int)
    RETURNS bigint[]
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION protobuf_get_sfixed32_multi(bytea, int)
    RETURNS int[]
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION protobuf_get_float_multi(bytea, int)
    RETURNS real[]
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION protobuf_get_sfixed64_multi(bytea, int)
    RETURNS bigint[]
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION protobuf_get_double_multi(bytea, int)
    RETURNS double precision[]
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION protobuf_get_bytes_multi(bytea, int)
    RETURNS bytea[]
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION protobuf_get_int(data bytea, tag int) RETURNS bigint AS $$
BEGIN
    RETURN (protobuf_get_int_multi(data, tag))[1];
END
$$ LANGUAGE 'plpgsql' IMMUTABLE;

CREATE FUNCTION protobuf_get_sfixed32(data bytea, tag int) RETURNS int AS $$
BEGIN
    RETURN (protobuf_get_sfixed32_multi(data, tag))[1];
END
$$ LANGUAGE 'plpgsql' IMMUTABLE;

CREATE FUNCTION protobuf_get_float(data bytea, tag int) RETURNS real AS $$
BEGIN
    RETURN (protobuf_get_float_multi(data, tag))[1];
END
$$ LANGUAGE 'plpgsql' IMMUTABLE;

CREATE FUNCTION protobuf_get_sfixed64(data bytea, tag int) RETURNS bigint AS $$
BEGIN
    RETURN (protobuf_get_sfixed64_multi(data, tag))[1];
END
$$ LANGUAGE 'plpgsql' IMMUTABLE;


CREATE FUNCTION protobuf_get_double(data bytea, tag int) RETURNS double precision AS $$
BEGIN
    RETURN (protobuf_get_double_multi(data, tag))[1];
END
$$ LANGUAGE 'plpgsql' IMMUTABLE;

CREATE FUNCTION protobuf_get_bytes(data bytea, tag int) RETURNS bytea AS $$
BEGIN
    RETURN (protobuf_get_bytes_multi(data, tag))[1];
END
$$ LANGUAGE 'plpgsql' IMMUTABLE;

CREATE FUNCTION protobuf_get_string(data bytea, tag int) RETURNS text AS $$
BEGIN
    RETURN convert_from(protobuf_get_bytes(data, tag), 'utf-8');
END
$$ LANGUAGE 'plpgsql' IMMUTABLE;

CREATE FUNCTION protobuf_get_string_multi(data bytea, tag int) RETURNS text AS $$
DECLARE
    bytes bytea[];
    strings text[];
BEGIN
    bytes := protobuf_get_bytes_multi(data, tag);

    FOR i IN array_lower(bytes, 1) .. array_upper(bytes, 1)
    LOOP
        strings[i] = convert_from(bytes[i], 'utf-8');
    END LOOP;

    RETURN strings;
END
$$ LANGUAGE 'plpgsql' IMMUTABLE;

CREATE FUNCTION protobuf_get_bool(data bytea, tag int) RETURNS boolean AS $$
BEGIN
    RETURN coalesce(protobuf_get_int(data, tag), 0) = 1;
END
$$ LANGUAGE 'plpgsql' IMMUTABLE;

