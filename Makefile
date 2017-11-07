CFLAGS= -g -std=gnu99
INCS=
#CC=purify gcc
CC=gcc
URLTEESRCS=  urltree.o ut_policy.o urltree_gnz.o urltree_gnz_conf.o
URLCBTREESRCS = url_cbtree.o
LDFLAGS += -L./
LDFLAGS += -lpthread -lpcre 
DEPDIR = .deps

all :	$(DEPDIR) urltree_test url_cbtree_test

$(DEPDIR):
	+@[ -d $@ ] || mkdir -p $@

urltree_test: urltree_test.o $(URLTEESRCS)
	$(CC) -o urltree_test urltree_test.o $(URLTEESRCS) -lm $(LDFLAGS)

url_cbtree_test: url_cbtree_test.o $(URLCBTREESRCS)
	$(CC) -o url_cbtree_test url_cbtree_test.o $(URLCBTREESRCS) -lm $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(DEPDIR)/$(<:.c=.d)

clean:
	rm -rf *.o $(DEPDIR) urltree_test url_cbtree_test 

-include $(DEPDIR)/*.d

