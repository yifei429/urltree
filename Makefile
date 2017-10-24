CFLAGS= -g -std=gnu99
INCS=
#CC=purify gcc
CC=gcc
SRCS=  
LDFLAGS += -L./
LDFLAGS += -lpthread  
DEPDIR = .deps

all :	$(DEPDIR) urltree_test url_cbtree_test

$(DEPDIR):
	+@[ -d $@ ] || mkdir -p $@

urltree_test: urltree_test.o urltree.o
	$(CC) -o urltree_test urltree_test.o urltree.o $(SRCS) -lm $(LDFLAGS)

url_cbtree_test: url_cbtree_test.o url_cbtree.o
	$(CC) -o url_cbtree_test url_cbtree_test.o url_cbtree.o $(SRCS) -lm $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(DEPDIR)/$(<:.c=.d)

clean:
	rm -rf *.o $(DEPDIR) 

-include $(DEPDIR)/*.d

