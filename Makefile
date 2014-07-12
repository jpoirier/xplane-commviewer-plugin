# 07-11-2014: Only works for Mac at the moment
CXX=g++
WINDLLMAIN=

HOSTOS=$(shell uname | tr A-Z a-z)
ifeq ($(HOSTOS),darwin)
 # -arch i386 -arch x86_64
 FILE_NAME=mac.xpl
 INCLUDE=-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk/System/Library/Frameworks/OpenGL.framework/Headers
 LIBS=-framework IOKit -framework CoreFoundation -framework OpenGL
 LNFLAGS=-arch x86_64 -dynamiclib -flat_namespace -undefined warning
 # -DTOGGLE_TEST_FEATURE
 CFLAGS=-arch x86_64 -Wall -O3 -D_APPLE_ -DAPL=1 -DIBM=0 -DLIN=0
else
 ifeq ($(HOSTOS),linux)
  FILE_NAME=lin.xpl
  LNFLAGS=-shared -rdynamic -nodefaultlibs
  CFLAGS=-march=i386 -Wall -O3 -DAPL=0 -DIBM=0 -DLIN=1 -fvisibility=hidden
 else # windows
  FILE_NAME=win.xpl
  LIBS=-lXPLM
  LNFLAGS=-m32 -Wl,-O1 -shared -L. -L./SDK/Libraries/Win/
  CFLAGS=-m32 -DAPL=0 -DIBM=1 -DLIN=0 -Wall -fpermissive
  WINDLLMAIN=commviewer_win.o
 endif
endif

# To set user/compiler debug mode (use DPRINTF for stdio): -DDEBUG
# To dynamically check for USB connected saitek panels: -DDO_USBPANEL_CHECK
DEFS=-DXPLM200 -DLOGPRINTF

INCLUDE+=-I.

all:
ifeq ($(HOSTOS),windows)
	$(CXX) -c $(INCLUDE) $(DEFS) $(CFLAGS) commviewer_win.cpp
endif
	$(CXX) -c $(INCLUDE) $(DEFS) $(CFLAGS) commviewer.cpp

	$(CXX) -o $(FILE_NAME) commviewer.o $(WINDLLMAIN) $(LNFLAGS) $(LIBS)

clean:
	$(RM) *.o *.xpl
