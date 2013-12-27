# Targets:
#   yugen - the Yugen game binary
#   yedit - the Yedit editor binary
#   yang - the Yang standalone compiler/checker
#   clean - delete all outputs
#   clean_all - delete all outputs and clean dependencies
# Pass DBG=1 to make for debug binaries.
#
# Utilities:
#   todo - print todo lines in all code files
#   add - run git add on all code files
#   wc - print line counts of all code files
#
# External package dependencies:
#   make
#   libz-dev libbz2-dev
#   libglew-dev libgl-dev libx11-dev libjpeg-dev
# For SFML:
#   cmake
#   build-essential mesa-common-dev
#   libx11-dev libxrandr-dev libgl1-mesa-dev
#   libglu1-mesa-dev libfreetype6-dev
#   libopenal-dev libsndfile1-dev
.SUFFIXES:

# Final outputs.
ifeq ($(DBG), 1)
OUTDIR=./dbg
else
OUTDIR=./bin
endif
GEN= \
	./gen
SOURCE= \
	./source
YUGEN_BINARY= \
	$(OUTDIR)/yugen
YEDIT_BINARY= \
	$(OUTDIR)/editor/yedit
YANG_BINARY= \
	$(OUTDIR)/yang/yang
BINARIES= \
	$(YUGEN_BINARY) $(YEDIT_BINARY) $(YANG_BINARY)

# Dependency directories.
DEPEND_DIR= \
	./depend
BOOST_DIR= \
	$(DEPEND_DIR)/boost_1_55_0
BYACC_DIR= \
	$(DEPEND_DIR)/byacc_2013_09_25
FLEX_DIR= \
	$(DEPEND_DIR)/flex_2_5_37
LLVM_DIR= \
  $(DEPEND_DIR)/llvm_3_3
LUAJIT_DIR= \
	$(DEPEND_DIR)/luajit_2_0_2
PROTOBUF_DIR= \
	$(DEPEND_DIR)/protobuf_2_5_0
SFML_DIR= \
	$(DEPEND_DIR)/sfml_2_1
# Boost compiled libraries we depend on.
BOOST_LIBRARIES= \
	filesystem iostreams system

# Compilers and interpreters.
export SHELL= \
	/bin/sh
export CC= \
	/usr/bin/gcc-4.8
export CXX= \
	/usr/bin/g++-4.8
export PROTOC= \
	$(PROTOBUF_DIR)/bin/protoc
export FLEX= \
	$(FLEX_DIR)/flex
export YACC= \
	$(BYACC_DIR)/yacc

# TODO: put LLVM into the top-level directory (also, without asserts).
DEPENDENCY_DIRS= \
	$(BOOST_DIR) $(LLVM_DIR)/Release+Asserts $(LLVM_DIR) \
	$(LUAJIT_DIR) $(PROTOBUF_DIR) $(SFML_DIR)
DEPENDENCY_CFLAGS= \
	$(addprefix -isystem ,\
	$(addsuffix /include,$(DEPENDENCY_DIRS)))
DEPENDENCY_LFLAGS= \
	$(addsuffix /lib,\
	$(addprefix -L,$(DEPENDENCY_DIRS)))
BOOST_LIBRARIES_LFLAGS= \
	$(addprefix -lboost_,$(BOOST_LIBRARIES))

# Compiler flags.
C11FLAGS= \
  -std=c++11
CFLAGS= \
	$(C11FLAGS) \
	$(DEPENDENCY_CFLAGS)
LFLAGS= \
	$(DEPENDENCY_LFLAGS) \
	\
	-Wl,-Bstatic \
	$(BOOST_LIBRARIES_LFLAGS) \
	$(shell $(LLVM_DIR)/Release+Asserts/bin/llvm-config --libs) \
	-lluajit-5.1 \
	-lprotobuf \
	-lsfml-graphics-s -lsfml-window-s -lsfml-system-s \
	-lz -lbz2 \
	\
	-Wl,-Bdynamic \
	-lGLEW -lGL -lX11 -lXrandr -ljpeg -lpthread -ldl
PFLAGS= \
	-I=$(SOURCE)/proto \
	--cpp_out=$(GEN)/proto
ifeq ($(DBG), 1)
CFLAGS += -Og -g -ggdb \
	-Werror -Wall -Wextra -Wpedantic \
	-DDEBUG
else
CFLAGS += -O3
endif

# File listings.
PROTOS= \
	$(wildcard $(SOURCE)/proto/*.proto)
PROTO_SOURCES= \
	$(subst $(SOURCE)/,$(GEN)/,$(PROTOS:.proto=.pb.cc))
PROTO_HEADERS= \
	$(subst $(SOURCE)/,$(GEN)/,$(PROTOS:.proto=.pb.h))
L_FILES= \
	$(wildcard $(SOURCE)/*.l) \
  $(wildcard $(SOURCE)/*/*.l)
L_SOURCES= \
  $(subst $(SOURCE)/,$(GEN)/,$(L_FILES:.l=.l.cc))
Y_FILES= \
	$(wildcard $(SOURCE)/*.y) \
	$(wildcard $(SOURCE)/*/*.y)
Y_SOURCES= \
	$(subst $(SOURCE)/,$(GEN)/,$(Y_FILES:.y=.y.cc))
SOURCE_FILES= \
	$(wildcard $(SOURCE)/*.cpp) \
	$(wildcard $(SOURCE)/*/*.cpp)
SOURCES= \
	$(SOURCE_FILES) $(PROTO_SOURCES) $(L_SOURCES) $(Y_SOURCES)
HEADER_FILES= \
	$(wildcard $(SOURCE)/*.h) \
	$(wildcard $(SOURCE)/*/*.h)
HEADERS= \
	$(HEADER_FILES) $(PROTO_HEADERS)
DEPFILES= \
	$(addprefix $(OUTDIR)/,\
	$(addsuffix .deps,$(SOURCES)))
GLSL_FILES= \
	$(wildcard ./data/shaders/*.glsl) \
	$(wildcard ./data/shaders/*/*.glsl)
LUA_FILES= \
	$(wildcard ./data/scripts/*.lua) \
	$(wildcard ./data/scripts/*/*.lua)
DATA_FILES= \
	$(wildcard ./data/tiles/*) \
	$(wildcard ./data/world/*)
SCRIPT_FILES= \
	Makefile Makedeps README ./*.sh

# Master targets.
.PHONY: all
all: \
	$(BINARIES)
.PHONY: yugen
yugen: \
	$(YUGEN_BINARY)
.PHONY: yedit
yedit: \
	$(YEDIT_BINARY)
.PHONY: yang
yang: \
	$(YANG_BINARY)
.PHONY: add
add:
	git add $(SCRIPT_FILES) $(GLSL_FILES) $(LUA_FILES) \
	    $(SOURCE_FILES) $(HEADER_FILES) $(DATA_FILES) \
			$(PROTOS) $(L_FILES) $(Y_FILES)
.PHONY: todo
todo:
	@grep --color -n "T[O]D[O]" \
	    $(SCRIPT_FILES) $(GLSL_FILES) $(LUA_FILES) \
	    $(SOURCE_FILES) $(HEADER_FILES) \
			$(PROTOS) $(L_FILES) $(Y_FILES)
.PHONY: wc
wc:
	@echo Scripts and docs:
	@cat $(SCRIPT_FILES) | wc
	@echo Protos:
	@cat $(PROTOS) | wc
	@echo Flex/YACC files:
	@cat $(L_FILES) $(Y_FILES) | wc
	@echo GLSL:
	@cat $(GLSL_FILES) | wc
	@echo Lua:
	@cat $(LUA_FILES) | wc
	@echo C++:
	@cat $(SOURCE_FILES) $(HEADER_FILES) | wc
	@echo Total:
	@cat $(SCRIPT_FILES) $(GLSL_FILES) $(LUA_FILES) \
	    $(SOURCE_FILES) $(HEADER_FILES) \
			$(PROTOS) $(L_FILES) $(Y_FILES) | wc
.PHONY: clean
clean:
	rm -rf $(OUTDIR)
	rm -rf $(GEN)

# Dependency generation. Each source file generates a corresponding .deps file
# (a Makefile containing a .build target), which is then included. Inclusion
# forces regeneration via the rules provided.
#
# Ridiculous magic: the .deps rule depends on same .build target it generates.
# This means we only need to regenerate dependencies when a dependency changes.
# When the specific .build target doesn't exist, the default causes everything
# to be generated.
.SECONDEXPANSION:
$(DEPFILES): $(OUTDIR)/%.deps: \
	$(OUTDIR)/%.build $(OUTDIR)/%.mkdir ./Makedeps \
	$$(subst \
	$$(OUTDIR)/,,$$($$(subst .,_,$$(subst /,_,$$(subst \
	$$(OUTDIR)/,,./$$(@:.deps=))))_LINK:.o=))
	SOURCE_FILE=$(subst $(OUTDIR)/,,./$(@:.deps=)); \
	    echo Generating dependencies for $$SOURCE_FILE; \
	    echo $(addsuffix .cpp,$(subst $(OUTDIR),$(SOURCE),$(BINARIES))) | \
	        fgrep $$SOURCE_FILE > /dev/null; \
	    ./Makedeps $@ $< $$SOURCE_FILE $(OUTDIR) $$?
.PRECIOUS: $(OUTDIR)/%.build
$(OUTDIR)/%.build: \
	./% $(HEADERS) $(OUTDIR)/%.mkdir
	touch $@

ifneq ('$(MAKECMDGOALS)', 'add')
ifneq ('$(MAKECMDGOALS)', 'todo')
ifneq ('$(MAKECMDGOALS)', 'wc')
ifneq ('$(MAKECMDGOALS)', 'clean')
ifneq ('$(MAKECMDGOALS)', 'clean_all')
-include $(DEPFILES)
endif
endif
endif
endif
endif

# Binary linking. Each binary uses a special variable containing the list of
# object files it depends on, generated in the .deps file.
$(BINARIES): $(OUTDIR)/%: \
	./depend/.build $(OUTDIR)/%.mkdir \
	$$(__source$$(subst /,_,$$(subst $$(OUTDIR),,./$$@))_cpp_LINK)
	@echo Linking ./$@
	$(CXX) -o ./$@ $(filter-out %.build,$(filter-out %.mkdir,$^)) $(LFLAGS)

# Object files. Each object file depends on a .build target contained in the
# autogenerated dependency files. The .build target depends on the source file
# and any header files it needs. The extra level of indirection makes the
# autogeneration much nicer.
# We depend on the projects whose include directories are not created until they
# are built.
$(OUTDIR)/%.o: \
	$(OUTDIR)/%.build $(OUTDIR)/%.mkdir \
	./depend/boost.build ./depend/protobuf.build \
	./depend/luajit.build ./depend/llvm.build
	SOURCE_FILE=$(subst $(OUTDIR)/,,./$(<:.build=)); \
	    echo Compiling $$SOURCE_FILE; \
	    $(CXX) -c $(CFLAGS) -o $@ $$SOURCE_FILE

# Proto files. These are generated in pairs, so we have a little bit of trickery
# to make that work right.
.PRECIOUS: $(PROTO_HEADERS) $(PROTO_SOURCES)
$(GEN)/proto/%.pb.cc: \
	$(GEN)/proto/%.pb.h
	touch $@ $<
$(GEN)/proto/%.pb.h: \
	$(SOURCE)/proto/%.proto $(GEN)/proto/.mkdir \
	./depend/protobuf.build
	@echo Compiling ./$<
	$(PROTOC) $(PFLAGS) ./$<

# Flex/YACC files.
.PRECIOUS: $(L_SOURCES) $(Y_SOURCES)
$(GEN)/%.l.h: \
	$(GEN)/%.l.cc
	touch $@ $<
$(GEN)/%.l.cc: \
	$(SOURCE)/%.l $(GEN)/%.mkdir ./depend/flex.build
	@echo Compiling ./$<
	$(FLEX) -o $@ --header-file=$(@:.cc=.h) $<
$(GEN)/%.y.h: \
	$(GEN)/%.y.cc
	touch $@ $<
$(GEN)/%.y.cc: \
	$(SOURCE)/%.y $(GEN)/%.mkdir ./depend/byacc.build
	@echo Compiling ./$<
	$(YACC) -d -o $@ $<

# Ensure a directory exists.
.PRECIOUS: ./%.mkdir
./%.mkdir:
	mkdir -p $(dir $@)
	touch $@
	
# Makefile for dependencies below here.

# Identifying toolset by name of C compiler is a convenient hack that may not
# work for compilers other than GCC.
BOOST_CONFIGURE_FLAGS= \
	--toolset=$(notdir $(CC)) \
	--prefix=$(CURDIR)/$(BOOST_DIR) \
	$(addprefix --with-,$(BOOST_LIBRARIES))
LUAJIT_MAKE_FLAGS= \
	PREFIX=$(CURDIR)/$(LUAJIT_DIR) \
	INSTALL_INC=$(CURDIR)/$(LUAJIT_DIR)/include/lua
PROTOBUF_CONFIGURE_FLAGS= \
	--prefix=$(CURDIR)/$(PROTOBUF_DIR)

# CMake variables.
CMAKE= \
	cmake
CMAKE_FLAGS= \
	-DCMAKE_CC_COMPILER=$(CC) \
	-DCMAKE_CXX_COMPILER=$(CXX) \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=. \
	-DBUILD_SHARED_LIBS=FALSE
SFML_CMAKE_FLAGS= \
	$(CMAKE_FLAGS) \
	-DSFML_BUILD_DOC=FALSE \
	-DSFML_BUILD_EXAMPLES=FALSE \
	-DSFML_INSTALL_PKGCONFIG_FILES=FALSE

# Dependencies.
./depend/.build: \
	./depend/boost.build \
	./depend/llvm.build \
	./depend/luajit.build \
	./depend/protobuf.build \
	./depend/sfml.build
	touch ./depend/.build

# Build boost.
./depend/boost.build:
	@echo Building boost
	cd $(BOOST_DIR)/tools/build/v2 && ./bootstrap.sh
	cd $(BOOST_DIR)/tools/build/v2 && ./b2 install $(BOOST_CONFIGURE_FLAGS)
	cd $(BOOST_DIR) && ./bin/b2 \
	    --build-dir=. stage $(BOOST_CONFIGURE_FLAGS)
	cd $(BOOST_DIR) && ./bin/b2 install $(BOOST_CONFIGURE_FLAGS)
	touch ./depend/boost.build

# Build Flex.
./depend/flex.build:
	@echo Building Flex
	cd $(FLEX_DIR) && ./configure
	cd $(FLEX_DIR) && $(MAKE)
	touch ./depend/flex.build

# Build BYACC.
./depend/byacc.build:
	@echo Building BYACC
	cd $(BYACC_DIR) && ./configure
	cd $(BYACC_DIR) && $(MAKE)
	touch ./depend/byacc.build

# Build LLVM.
./depend/llvm.build:
	@echo Building LLVM
	cd $(LLVM_DIR) && ./configure
	cd $(LLVM_DIR) && $(MAKE)
	touch ./depend/llvm.build

# Build LuaJIT.
./depend/luajit.build:
	@echo Building LuaJIT
	cd $(LUAJIT_DIR) && $(MAKE) $(LUAJIT_MAKE_FLAGS)
	cd $(LUAJIT_DIR) && $(MAKE) install $(LUAJIT_MAKE_FLAGS)
	touch ./depend/luajit.build

# Build protobuf.
./depend/protobuf.build:
	@echo Building protobuf
	cd $(PROTOBUF_DIR) && ./configure $(PROTOBUF_CONFIGURE_FLAGS)
	cd $(PROTOBUF_DIR) && $(MAKE)
	cd $(PROTOBUF_DIR) && $(MAKE) check
	cd $(PROTOBUF_DIR) && $(MAKE) install
	touch ./depend/protobuf.build

# Build SFML.
./depend/sfml.build:
	@echo Building SFML
	cd $(SFML_DIR) && $(CMAKE) $(SFML_CMAKE_FLAGS)
	cd $(SFML_DIR) && $(MAKE)
	touch ./depend/sfml.build

# Clean dependencies.
.PHONY: clean_all
clean_all: \
	clean
	rm -f ./depend/*.build ./depend/.build
	-cd $(BOOST_DIR) && \
	    [ -f ./bin/b2 ] && ./bin/b2 $(BOOST_CONFIGURE_FLAGS) --clean
	-cd $(BOOST_DIR) && rm -rf ./include ./lib
	-cd $(BYACC_DIR) && [ -f ./Makefile ] && $(MAKE) clean
	-cd $(FLEX_DIR) && [ -f ./Makefile ] && $(MAKE) clean
	-cd $(LLVM_DIR) && $(MAKE) clean
	-cd $(PROTOBUF_DIR) && [ -f ./Makefile ] && $(MAKE) clean
	-cd $(PROTOBUF_DIR) && rm -rf ./bin ./include ./lib
	-cd $(SFML_DIR) && [ -f ./Makefile ] && $(MAKE) clean
	cd $(LUAJIT_DIR) && $(MAKE) $(LUAJIT_MAKE_FLAGS) uninstall
	cd $(LUAJIT_DIR) && $(MAKE) $(LUAJIT_MAKE_FLAGS) clean
