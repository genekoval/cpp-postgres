project := pg++
summary := An asynchronous PostgreSQL client for C++

STD := c++20

common.libs := ext++ fmt netcore timber uuid++
executable.libs := $(project) commline dotenv fmt netcore timber

library := lib$(project)
$(library).type := shared
$(library).libs := $(common.libs)

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

install.directories = $(include)/$(project)

files = $(include) $(src) Makefile VERSION

install := $(library)
targets := $(install) $(examples) $(utilities)

include mkbuild/base.mk
