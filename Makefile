CXX= \
	g++-4.8
PROTOC= \
	./depend/protobuf_2_5_0/bin/protoc

ifeq ($(DBG), 1)
OUTDIR=dbg
else
OUTDIR=bin
endif
YUGEN= \
	./$(OUTDIR)/yugen
YEDIT= \
	./$(OUTDIR)/yedit

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

SUBDIRS= \
	$(shell find source/* -type d)
PROTOS= \
	$(wildcard ./source/proto/*.proto)
PROTO_SOURCES= \
	$(PROTOS:.proto=.pb.cc)
SOURCES= \
	$(wildcard ./source/*.cpp) \
	$(wildcard ./source/*/*.cpp) \
	$(wildcard ./source/*.cc) \
	$(wildcard ./source/*/*.cc) \
	$(PROTO_SOURCES)
YUGEN_SOURCES= \
	$(filter-out ./source/yedit.cpp,$(SOURCES))
YEDIT_SOURCES= \
	$(filter-out ./source/yugen.cpp,$(SOURCES))
YUGEN_OBJECTS= \
	$(subst /source/,/$(OUTDIR)/, \
	$(patsubst %.cc,%.cc.o,$(patsubst %.cpp,%.cpp.o,$(YUGEN_SOURCES))))
YEDIT_OBJECTS= \
	$(subst /source/,/$(OUTDIR)/, \
	$(patsubst %.cc,%.cc.o,$(patsubst %.cpp,%.cpp.o,$(YEDIT_SOURCES))))

all: \
	$(YUGEN) $(YEDIT)
yugen: \
	$(YUGEN)
yedit: \
	$(YEDIT)

$(YUGEN): \
	$(YUGEN_OBJECTS)
	@echo Linking yugen
	@$(CXX) -o $@ $^ $(LFLAGS)
$(YEDIT): \
	$(YEDIT_OBJECTS)
	@echo Linking yedit
	@$(CXX) -o $@ $^ $(LFLAGS)
$(OUTDIR)/%.o: \
	source/% $(PROTO_SOURCES) $(OUTDIR)/.out
	@echo Compiling $<
	@$(CXX) -c $(CFLAGS) -o $@ $<
source/proto/%.pb.cc: \
	source/proto/%.pb.h
	@# Noop for intermediate file
source/proto/%.pb.h: \
	source/proto/%.proto
	@echo Compiling $<
	@$(PROTOC) $(PFLAGS) $<
$(OUTDIR)/.out:
	@echo Creating $(OUTDIR)
	@mkdir $(OUTDIR); mkdir $(subst source/,$(OUTDIR)/,$(SUBDIRS)); touch $@

clean:
	@rm -rf ./$(OUTDIR)
