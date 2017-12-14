# pg\_protobuf

Protobuf support for PostgreSQL.

How to install:

```
make
make install
make installcheck
```

Usage example:

```
create extension pg_protobuf;

create table heroes (x bytea);

insert into heroes values
  (E'\\x0a0365617810321880022202100f'),
  (E'\\x0a07616669736b6f6e10191880082a060a0200011064')
  -- ...
  ;

create function hero_name(x bytea) returns text as $$
begin
return protobuf_get_string(x, 1);
end;
$$ language 'plpgsql' immutable;

create function hero_hp(x bytea) returns text as $$
begin
return protobuf_get_integer(x, 2);
end;
$$ language 'plpgsql' immutable;

create function hero_xp(x bytea) returns text as $$
begin
return protobuf_get_integer(x, 3);
end;
$$ language 'plpgsql' immutable;

create index hero_name_idx on heroes using btree(hero_name(x));

select protobuf_decode(x) from heroes where hero_name(x) = 'afiskon';
```
