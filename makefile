# Makefile for GrayCore
# sample from https://www.softwaretestinghelp.com/cpp-makefile-tutorial/
# https://stackoverflow.com/questions/2908057/can-i-compile-all-cpp-files-in-src-to-os-in-obj-then-link-to-binary-in

NAME = GrayCore

# standard

CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g $(shell root-config --cflags)
# CXXFLAGS
LDFLAGS=-g $(shell root-config --ldflags)
LDLIBS=$(shell root-config --libs)

BASEDIR = ../..
SRC_DIR := $(BASEDIR)/src
OBJ_DIR := $(BASEDIR)/lib
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# targets
 
all: tool

tool: $(OBJS)
    $(CXX) $(LDFLAGS) -o tool $(OBJS) $(LDLIBS)

clean:
    $(RM) $(OBJS)

distclean: clean
    $(RM) tool

# specific h dependencies.

#  GrayCore.o: GrayCore.cpp GrayCore.h
 
