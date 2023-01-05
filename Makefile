project := pg++
summary := An asynchronous PostgreSQL client for C++

STD := c++20

common.libs := fmt netcore timber

library := lib$(project)
$(library).type := shared
$(library).libs := $(common.libs)

examples := examples
$(examples).type := executable
$(examples).deps := $(library)
$(examples).libs := \
  $(project) \
  commline \
  fmt \
  netcore \
  timber

test.deps := lib$(project)
test.libs := \
  $(project) \
  $(common.libs) \
  gtest

install.directories = $(include)/$(project)

files = $(include) $(src) Makefile Version

install := $(library)
targets := $(install) $(examples)

include mkbuild/base.mk
