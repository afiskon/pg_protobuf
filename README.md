# pg\_protobuf

Protobuf support for PostgreSQL.

## Quick start

How to install:

```
# On Ubuntu:
# sudo apt install postgresql-server-dev-13
make
make install
make installcheck
```

Usage example:

```
create extension pg_protobuf;

create table heroes (x bytea);

-- fill table with random Protobuf data
insert into heroes
  select E'\\x10191880082a060a0200011064822007' ||
         convert_to(substring(md5('' || random() || random()), 0, 8), 'utf-8')
  from generate_series(1,10000);

create function hero_name(x bytea) returns text as $$
begin
return protobuf_get_string(x, 512);
end
$$ language 'plpgsql' immutable;

create function hero_hp(x bytea) returns text as $$
begin
return protobuf_get_int(x, 2);
end
$$ language 'plpgsql' immutable;

create function hero_xp(x bytea) returns text as $$
begin
return protobuf_get_int(x, 3);
end
$$ language 'plpgsql' immutable;

create index hero_name_idx on heroes using btree(hero_name(x));

select hero_name(x) from heroes order by hero_name(x) limit 10;

-- make sure index is used
explain select hero_name(x) from heroes order by hero_name(x) limit 10;
```

## Limitations

Current limitations and possible workarounds:

* Modification of the Protobuf data is not supported.
* Enums are not directly supported. However, you can read them using
  `protobuf_get_int` and `protobuf_get_int_multi` procedures;
* Unsigned Protobuf types (`uint`, `fixed32`, `fixed64`) are not directly
  supported due to lack of unsigned integer types in PostgreSQL;
* Attribute `[packed=true]` for repeated values is not supported by `*_multi`
  procedures. However these values can be decoded as raw byte arrays using
  `protobuf_get_bytes*` procedures. **Warning!** In Protobuf 3 values are
  packed by default unless specified otherwise;
