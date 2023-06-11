project := pg++
summary := An asynchronous PostgreSQL client for C++

STD := c++20

common.libs := crypto ext++ fmt netcore ssl timber uuid++
executable.libs := \
  $(project) \
  commline \
  crypto \
  dotenv \
  ext++ \
  fmt \
  netcore \
  ssl \
  timber

library := lib$(project)
$(library).type := shared
$(library).deps = $(errcodes.object)
$(library).libs := $(common.libs)

gen := codegen
$(gen).type := executable
$(gen).libs := commline ext++ fmt

examples := examples
$(examples).type := executable
$(examples).deps := $(library)
$(examples).libs := $(executable.libs)

utilities := util
$(utilities).type := executable
$(utilities).deps := $(library)
$(utilities).libs := $(executable.libs)

test.deps := $(library)
test.libs := \
  $(project) \
  $(common.libs) \
  dotenv \
  gtest

errcodes.header = $(include)/$(project)/except/errcodes.gen.hpp
errcodes.source = $(src)/$(library)/except/errcodes.gen.cpp
errcodes.object = $(obj)/$(library)/except/errcodes.gen.o
errcodes.options =\
 --header $(errcodes.header)\
 --source $(errcodes.source)\
 $(shell pg_config --sharedir)/errcodes.txt

mk.gen.headers += $(errcodes.header)

CLEAN += $(errcodes.header) $(errcodes.source)

install.directories = $(include)/$(project)

files = $(include) $(src) Makefile VERSION

install := $(library)
targets := $(install) $(gen) $(examples) $(utilities)

include mkbuild/base.mk

$(obj)/$(gen)/main.o: CXXFLAGS +=\
 -DNAME='"$(gen)"'\
 -DVERSION='"$(version)"'\
 -DDESCRIPTION='"code generator for $(project)"'

$(errcodes.source): $($(gen))
	$($(gen)) errcodes $(errcodes.options)

gen: $(errcodes.source)
