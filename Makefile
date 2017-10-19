CFLAGS= -g -std=gnu99
INCS=
#CC=purify gcc
CC=gcc
SRCS= urltree.o 
LDFLAGS += -L./
LDFLAGS += -lpthread  
DEPDIR = .deps

all :	$(DEPDIR) urltree_test

$(DEPDIR):
	+@[ -d $@ ] || mkdir -p $@

urltree_test: urltree_test.o $(SRCS)
	$(CC) -o urltree_test urltree_test.o $(SRCS) -lm $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(DEPDIR)/$(<:.c=.d)

clean:
	rm -rf *.o $(DEPDIR) 

-include $(DEPDIR)/*.d

