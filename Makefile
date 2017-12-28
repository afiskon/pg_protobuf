EXTENSION = pg_protobuf
MODULE_big = pg_protobuf
OBJS = pg_protobuf.o decode_ctx.o decode_internal.o
DATA = pg_protobuf--1.0.sql
REGRESS = pg_protobuf

PG_CPPFLAGS = -g -O2
SHLIB_LINK =

ifndef PG_CONFIG
	PG_CONFIG := pg_config
endif
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
