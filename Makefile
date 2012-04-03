#
#
#
#
#

TARGET = Curly

IDIR=include
SDIR=src
ODIR=obj

_OBJS= Curly.o SelectCurler.o LibEventCurler.o StringUtil.o ConnectionData.o LibEventCurlerCallbacks.o Trace.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

_DEPS= Curly.h SelectCurler.h LibEventCurler.h AbortIf.h ConnectionData.h LibEventCurlerCallbacks.h Trace.h
DEPS= $(patsubst %,$(IDIR)/%,$(_DEPS))

LIBS = -lstdc++ -levent-1.4.2 -lcurl
CXXFLAGS = -DTRACE_DEBUG -g -Wall -fmessage-length=0 -I./ -I$(IDIR)/ -L/Users/stephane/Downloads/libevent-1.4.14b-stable//.libs -L/usr/local/lib -L/opt/local/lib

$(ODIR)/%.o : $(SDIR)/%.cpp 
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(ODIR)/%.o : $(SDIR)/%.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS) $($(CXX_SYSLIBS))

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean

