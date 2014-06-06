default: release

.PHONY: default release debug dynamic_all clean

OUTPUT=sudoku

CPP_FILES:=$(wildcard src/*.cpp)

DEBUG_D_FILES:=$(CPP_FILES:%.cpp=debug/%.cpp.d)
RELEASE_D_FILES:=$(CPP_FILES:%.cpp=release/%.cpp.d)

DEBUG_O_FILES:=$(CPP_FILES:%.cpp=debug/%.cpp.o)
RELEASE_O_FILES:=$(CPP_FILES:%.cpp=release/%.cpp.o)

CXX=clang++
LD=clang++

PREFIX=/home/wichtounet/install/

WARNING_FLAGS=-Wextra -Wall -Qunused-arguments -Wuninitialized -Wsometimes-uninitialized -Wno-long-long -Winit-self -Wdocumentation

CXX_FLAGS=-Iinclude -Idbn/include -isystem $(PREFIX)/include/ -std=c++1y -stdlib=libc++ $(WARNING_FLAGS)
LD_FLAGS=$(CXX_FLAGS) -L$(PREFIX)/lib/ -lopencv_imgproc -lopencv_core -lopencv_highgui

STATIC_CXX_FLAGS=$(CXX_FLAGS) -DNO_GUI
STATIC_LD_FLAGS=-static -L$(PREFIX)/share/OpenCV/3rdparty/lib/ $(LD_FLAGS) -lpng -ljpeg -ltiff -llibjasper -lIlmImf -lz

DEBUG_FLAGS=-g
RELEASE_FLAGS=-g -Ofast -march=native -fvectorize -fslp-vectorize-aggressive -fomit-frame-pointer

debug/src/%.cpp.o: src/%.cpp
	@ mkdir -p debug/src/
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -o $@ -c $<

release/src/%.cpp.o: src/%.cpp
	@ mkdir -p release/src/
	$(CXX) $(CXX_FLAGS) $(RELEASE_FLAGS) -o $@ -c $<

debug/bin/$(OUTPUT): $(DEBUG_O_FILES)
	@ mkdir -p debug/bin/
	$(LD) $(LD_FLAGS) $(DEBUG_FLAGS) -o $@ $+

release/bin/$(OUTPUT): $(RELEASE_O_FILES)
	@ mkdir -p release/bin/
	$(LD) $(LD_FLAGS) $(RELEASE_FLAGS) -o $@ $+

debug/src/%.cpp.d: $(CPP_FILES)
	@ mkdir -p debug/src/
	@ $(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -MM -MT debug/src/$*.cpp.o src/$*.cpp | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@' > $@

release/src/%.cpp.d: $(CPP_FILES)
	@ mkdir -p release/src/
	@ $(CXX) $(CXX_FLAGS) $(RELEASE_FLAGS) -MM -MT release/src/$*.cpp.o src/$*.cpp | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@' > $@

release: release/bin/$(OUTPUT)
debug: debug/bin/$(OUTPUT)

dynamic_all: release debug

clean:
	rm -rf release/
	rm -rf debug/

-include $(DEBUG_D_FILES)
-include $(RELEASE_D_FILES)