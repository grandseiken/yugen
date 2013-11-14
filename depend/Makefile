# Makefile for dependencies.
BOOST_DIR= \
	./boost_1_53_0
LUAJIT_DIR= \
	./luajit_2_0_2
PROTOBUF_DIR= \
	./protobuf_2_5_0
SFML_DIR= \
	./sfml_2_1

# Other programs.
CMAKE= \
	cmake

.PHONY: all
all: \
	./sfml.build

./sfml.build:
	@echo Building SFML
	cd $(SFML_DIR) && $(CMAKE) \
	    -DBUILD_SHARED_LIBS=FALSE \
	    -DCMAKE_BUILD_TYPE=Release \
	    -DCMAKE_INSTALL_PREFIX=. \
	    -DSFML_BUILD_DOC=FALSE \
	    -DSFML_BUILD_EXAMPLES=FALSE \
	    -DSFML_INSTALL_PKGCONFIG_FILES=FALSE \
	&& $(MAKE)
	touch ./sfml.build

clean:
	rm -f ./*.build
	-cd $(SFML_DIR) && [ -f ./Makefile ] && $(MAKE) clean
