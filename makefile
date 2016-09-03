CC := g++
CFLAGS := -Wall -pedantic -std=c++14 -O2

SRCS := tagged_union_test.cpp tagged_union_error_test.cpp \
	tagged_union_perf_test.cpp

DEPDIR := deps
DEPS := $(patsubst %.cpp,$(DEPDIR)/%.makefile,$(SRCS))
ODIR := build
OBJS := $(patsubst %.cpp,$(ODIR)/%.o,$(SRCS))

tagged_union_test: $(OBJS) 
	$(CC) -o $@ $(OBJS) $(CFLAGS)

$(DEPDIR):
	mkdir $@

$(ODIR):
	mkdir $@

-include $(DEPS)

$(DEPDIR)/%.makefile: | $(DEPDIR)
	$(CC) $(CFLAGS) -MM $*.cpp -MT "$(ODIR)/$*.o $@" > $@
	
$(ODIR)/%.o: | $(ODIR)
	$(CC) -o $@ -c $*.cpp $(CFLAGS)
		
.PHONY: clean
clean:
	rm -f $(DEPS)
	rm -f $(OBJS)
