PROJECT=RemoteServer
GCC=g++
LIBS=
SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,$(OBJDIR)/%.o,$(SRCS))
CFLAG=-g -lpthread -std=c++11
OBJDIR=out
$(shell mkdir -p $(OBJDIR))
$(PROJECT):$(OBJS)
	$(GCC) -o $(OBJDIR)/$@ $^ $(CFLAG) $(LIBS)
$(OBJDIR)/%.o:%.cpp
	$(GCC) $(CFLAG) -c -o $@ $<
clean :
	rm -rf $(OBJDIR)/*