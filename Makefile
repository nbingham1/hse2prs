CXXFLAGS =	-O2 -g -Wall -fmessage-length=0

OBJS =		src/chp.o src/common.o src/tokenizer.o src/dot.o src/message.o src/petri.o src/path.o src/path_space.o src/process.o src/minterm.o src/canonical.o src/variable.o src/variable_space.o src/program_counter.o

LIBS =

TARGET =	hse2states

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
