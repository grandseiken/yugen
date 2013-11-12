# Targets:
#   yugen - the Yugen game binary
#   yedit - the Yedit editor binary
#   clean - delete all outputs
# Pass DBG=1 to make for debug binaries.

# Compilers and interpreters.
SHELL= \
	/bin/sh
CXX= \
	g++-4.8
PROTOC= \
	./depend/protobuf_2_5_0/bin/protoc
.SUFFIXES:

# Final outputs.
ifeq ($(DBG), 1)
OUTDIR=dbg
else
OUTDIR=bin
endif
YUGEN= \
	./$(OUTDIR)/yugen
YEDIT= \
	./$(OUTDIR)/yedit

# Compiler flags.
CFLAGS= \
	-isystem ./depend/boost_1_53_0/include \
	-isystem ./depend/sfml_2_0/include \
	-isystem ./depend/luajit_2_0_2/include \
	-isystem ./depend/protobuf_2_5_0/include \
	\
	-std=c++11
LFLAGS= \
	-L./depend/boost_1_53_0/lib \
	-L./depend/sfml_2_0/lib \
	-L./depend/luajit_2_0_2/lib \
	-L./depend/protobuf_2_5_0/lib \
	\
	-Wl,-Bstatic \
	-lboost_filesystem -lboost_iostreams -lboost_system \
	-lsfml-graphics-s -lsfml-window-s -lsfml-system-s \
	-lprotobuf \
	-lluajit-5.1 \
	-lz -lbz2 \
	\
	-Wl,-Bdynamic \
	-lGLEW -lGL -lX11	-lXrandr -ljpeg	-lpthread -ldl
PFLAGS= \
	-I=./source/proto \
	--cpp_out=./source/proto
ifeq ($(DBG), 1)
CFLAGS += -O3
else
CFLAGS += -Og \
	-Werror -Wall -Wextra -Wpedantic \
	-DLUA_DEBUG -DGL_DEBUG
endif

# File listings.
SUBDIRS= \
	$(shell find source/* -type d)
SUBDIR_OUTS= \
	$(subst source/,$(OUTDIR)/, \
	$(addsuffix /.out,$(addprefix ./,$(SUBDIRS))))
PROTOS= \
	$(wildcard ./source/proto/*.proto)
PROTO_SOURCES= \
	$(PROTOS:.proto=.pb.cc)
PROTO_HEADERS= \
	$(PROTOS:.proto=.pb.h)
SOURCES= \
	$(wildcard ./source/*.cpp) \
	$(wildcard ./source/*/*.cpp) \
	$(wildcard ./source/*.cc) \
	$(wildcard ./source/*/*.cc) \
	$(PROTO_SOURCES)
HEADERS= \
	$(wildcard ./source/*.h) \
	$(wildcard ./source/*/*.h) \
	$(PROTO_HEADERS)
DEPFILES= \
  $(subst /source/,/$(OUTDIR)/, \
	$(addsuffix .deps,$(SOURCES) $(HEADERS)))
OBJECTS= \
	$(subst /source/,/$(OUTDIR)/, \
	$(patsubst %.cc,%.cc.o,$(patsubst %.cpp,%.cpp.o,$(SOURCES))))
YUGEN_OBJECTS= \
	$(filter-out ./$(OUTDIR)/editor/yedit.cpp.o,$(OBJECTS))
YEDIT_OBJECTS= \
	$(filter-out ./$(OUTDIR)/yugen.cpp.o,$(OBJECTS))

# Master targets.
# Note: .INTERMEDIATE does not function correctly in GNU make 3.81. This isn't
# a huge problem, but it leaves generated proto files around, which sometimes
# causes extra dependency-generation. As a workaround, we delete them in each
# of the main targets.
.PHONY: all
all: \
	$(YUGEN) $(YEDIT)
	@rm -rf ./source/proto/*.pb.*
.PHONY: yugen
yugen: \
	$(YUGEN)
	@rm -rf ./source/proto/*.pb.*
.PHONY: yedit
yedit: \
	$(YEDIT)
	@rm -rf ./source/proto/*.pb.*
.PHONY: clean
clean:
	@echo Removing $(OUTDIR)
	@rm -rf ./$(OUTDIR)
	@rm -rf ./source/proto/*.pb.*

# Dependency generation.
./$(OUTDIR)/%.deps: \
	./source/% $(OUTDIR)/.out
	@echo Generating dependencies for $<
	@./deps.sh $@ $(@:.deps=.BUILD) $(OUTDIR) $<

ifneq ('$(MAKECMDGOALS)', 'clean')
-include $(DEPFILES)
endif

# Build steps.
$(YUGEN): \
	$(YUGEN_OBJECTS)
	@echo Linking yugen
	@$(CXX) -o $@ $^ $(LFLAGS)
$(YEDIT): \
	$(YEDIT_OBJECTS)
	@echo Linking yedit
	@$(CXX) -o $@ $^ $(LFLAGS)
./$(OUTDIR)/%.o: \
	./$(OUTDIR)/%.BUILD $(OUTDIR)/.out
	@echo Compiling $(subst $(OUTDIR)/,source/,$(<:.BUILD=))
	@$(CXX) -c $(CFLAGS) -o $@ $(subst $(OUTDIR)/,source/,$(<:.BUILD=))
./source/proto/%.pb.h: \
	./source/proto/%.pb.cc
	@# Noop for intermediate file
	@touch $@ $<
./source/proto/%.pb.cc: \
	./source/proto/%.proto
	@echo Compiling $<
	@$(PROTOC) $(PFLAGS) $<
./$(OUTDIR)/.out: \
	$(SUBDIR_OUTS)
	@echo Creating $(OUTDIR)/
	@mkdir -p ./$(OUTDIR)/
	@touch ./$(OUTDIR)/.out
./$(OUTDIR)/%/.out:
	@echo Creating $(dir $@)
	@mkdir -p $(dir $@)
	@touch $@

