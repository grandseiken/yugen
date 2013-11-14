# Makefile for dependencies.
BOOST_DIR= \
	./boost_1_53_0
LUAJIT_DIR= \
	./luajit_2_0_2
PROTOBUF_DIR= \
	./protobuf_2_5_0
SFML_DIR= \
	./sfml_2_1

# CMake variables.
CMAKE= \
	cmake
CMAKE_FLAGS= \
  -DCMAKE_CXX_COMPILER=$(CXX) \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=. \
	-DBUILD_SHARED_LIBS=FALSE
SFML_CMAKE_FLAGS= \
	$(CMAKE_FLAGS) \
	-DSFML_BUILD_DOC=FALSE \
	-DSFML_BUILD_EXAMPLES=FALSE \
	-DSFML_INSTALL_PKGCONFIG_FILES=FALSE

.PHONY: all
all: \
	./sfml.build

# Build SFML. CMake seems to fail the first time and work the second time.
# I can't work out why.
./sfml.build:
	@echo Building SFML
	-cd $(SFML_DIR) && $(CMAKE) $(SFML_CMAKE_FLAGS)
	cd $(SFML_DIR) && $(CMAKE) $(SFML_CMAKE_FLAGS)
	cd $(SFML_DIR) && $(MAKE)
	touch ./sfml.build

clean:
	rm -f ./*.build
	-cd $(SFML_DIR) && [ -f ./Makefile ] && $(MAKE) clean
