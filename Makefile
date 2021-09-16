PROJECT=remote
GCC=g++
MYLIB=
SYSLIB=
CFLAG=-g -lpthread -std=c++11
OBJDIR=./out
SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,$(OBJDIR)/%.o,$(SRCS))
$(PROJECT):$(OBJS)
	$(GCC) -o $(OBJDIR)/$@ $^ $(CFLAG) $(SYSLIB) $(MYLIB)
$(OBJDIR)/%.o:%.cpp
	$(GCC) $(CFLAG) -c -o $@ $<
clean :
	rm -rf $(OBJDIR)/*