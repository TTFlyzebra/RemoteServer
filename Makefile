PROJECT=remote
GCC=g++
MYLIB=
SYSLIB=
CFLAG=-g -lpthread
OBJDIR=./out
SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,$(OBJDIR)/%.o,$(SRCS))
$(PROJECT):$(OBJS)
	$(GCC) -o $@ $^ $(CFLAG) $(SYSLIB) $(MYLIB)
$(OBJDIR)/%.o:%.cpp
	$(GCC) -c -o $@ $<
clean :
	rm remote $(OBJS)