LIBS=libpng x11 xft

# -std=c90
CFLAGS=-Wall -Wextra -Wpedantic -std=c90 -pedantic \
	   $(shell for lib in $(LIBS); do pkg-config --cflags --libs $$lib; done) \

SRCS=main.c image.c
OBJS=$(SRCS:.c=.o)
EXE=sbimg

#
# Release variables
#
RELDIR=release
RELEXE=$(RELDIR)/$(EXE)
RELOBJS=$(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS=-Os -s -flto -march=native -mtune=native

#
# Debug variables
#
DBGDIR=debug
DBGEXE=$(DBGDIR)/$(EXE)
DBGOBJS=$(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS=-g -Og -DDEBUG

.PHONY: all
all: prep release debug

.PHONY: prep
prep:
	@mkdir -p $(DBGDIR) $(RELDIR)

#
# Debug rules
#
.PHONY: debug
debug: $(DBGEXE)

$(DBGEXE): $(DBGOBJS)
	$(CC) $(CFLAGS) $(DBGCFLAGS) $^ -o $(DBGEXE)

$(DBGDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $(DBGCFLAGS) $< -o $@

#
# Release rules
#
.PHONY: release
release: $(RELEXE)

$(RELEXE): $(RELOBJS)
	$(CC) $(CFLAGS) $(RELCFLAGS) $^ -o $(RELEXE)

$(RELDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $(RELCFLAGS) $< -o $@

.PHONY: clean
clean:
	rm $(RELEXE) $(RELOBJS) $(DBGEXE) $(DBGOBJS)

.PHONY: remake
remake: clean all
