# Compilers used
CC ?= gcc
CFLAGS += -Wall -Iinclude
CXX ?= g++
CXXFLAGS += -std=c++11 -Wall -Iinclude -I/usr/include -I/usr/include/SDL2 -I/usr/include/SOIL -I/usr/local/include/bullet
LDFLAGS += -fPIC -L/usr/lib -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu
LDLIBS += -lSDL2 -lSDL2main -lGLEW -lSOIL -lassimp -lBulletDynamics -lBulletCollision -lLinearMath -lGL ./libf2.a
PREFIX = /usr/local
# Path to the source directory, relative to the makefile
SRC_PATH = src
SRC_DIR = src

# SRCS
SOURCES := $(filter-out src/main.cpp, $(wildcard $(SRC_PATH)/*.cpp))
CSOURCES=$(wildcard $(SRC_PATH)/*.c)
OBJECTS=$(SOURCES:$(SRC_PATH)/%.cpp=%.o)
COBJECTS=$(CSOURCES:$(SRC_PATH)/%.c=%.o)

TEST_SOURCES := $(SRC_PATH)/main.cpp
TEST_OBJECTS := $(TEST_SOURCES:$(SRC_PATH)/%.cpp=%.o)

$(info $$SOURCES is [${SOURCES}])
$(info $$COBJECTS is [${COBJECTS}])
$(info $$libdir is [${libdir}])

ifneq ($(BUILD), shared)
	BUILD = static
endif

ifeq ($(MSYSTEM), MINGW32)
  EXE_EXT    = .exe
  LIB_EXT    = .dll
  RM=del /F /Q
else
  EXE_EXT    =
  LIB_EXT    = .so
  RM=rm -f
endif

all: libf2.a game

f2: $(BUILD)
static: libf2.a
shared: libf2.so

# Takes all the .o objects and compiles them into a single .a library
# ar -rvs f2.a *.o
libf2.a: $(COBJECTS) $(OBJECTS)
	$(AR) rvs $@ $(COBJECTS) $(OBJECTS)

# TODO
libf2.so: $(COBJECTS) $(OBJECTS)
	$(CXX) -shared $(LDFLAGS) -o $@ $(COBJECTS) $(OBJECTS)

install: install-$(BUILD) game
	$(INSTALL_PROGRAM) game $(DESTDIR)$(bindir)/game
	$(INSTALL_DATA) libf2.a $(DESTDIR)$(libdir)/libf2.a

install-static: libf2.a
	$(MKDIR) $(DESTDIR)$(PREFIX)\/lib/
	$(INSTALL) -pm0755 $< $(DESTDIR)$(PREFIX)/$<

install-shared: libf2.so
	$(MKDIR) $(DESTDIR)$(PREFIX)\/lib/
	$(INSTALL) -pm0755 $< $(DESTDIR)$(PREFIX)/$<

game: $(TEST_OBJECTS) libf2.a
	$(CXX) $(LDFLAGS) -o $@ $< $(LDLIBS)

%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -MD -c $< -o $@
	@cp $*.d $*.P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
			-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
		rm -f $*.d

-include *.P # $(CSOURSES:.c=.P)

%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -MD -c $< -o $@
	@cp $*.d $*.P; \
          sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
              -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
          rm -f $*.d

-include *.P # $(SOURCES:.cpp=.P)

# Removes all build files
clean:
	$(RM) *.o *.a *.so game

.PHONY: all static install shared install-static install-shared clean f2
