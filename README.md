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
