# Compiler flags
CC      = gcc
CFLAGS  = -Wall -Werror -Wextra
LDFLAGS = -lSDL2

# Project files
SRCS = main.c util.c
OBJS = $(SRCS:.c=.o)
DEPS = util.h
EXE  = munch

# Debug build settings
DBGDIR = debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS = -g -O0 -DDEBUG

# Test build settings
TSTDIR = test
TSTEXE = $(TSTDIR)/$(EXE)
TSTOBJS = $(addprefix $(TSTDIR)/, $(OBJS))
TSTCFLAGS = -g -O0 -DTEST

# Release build settings
RELDIR = release
RELEXE = $(RELDIR)/$(EXE)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS = -march=native -flto -ffast-math -funroll-all-loops -Ofast -I. -DNTEST -DNDEBUG

.PHONY: all clean debug prep release remake

# Default build
all: release

install: release
	mkdir -p ${out}/bin
	cp ${RELEXE} ${out}/bin/

# Debug rules
debug: $(DBGEXE)

$(DBGEXE): $(DBGOBJS) | prep
	$(CC) $(CFLAGS) $(LDFLAGS) $(DBGCFLAGS) -o $(DBGEXE) $(DBGOBJS)

$(DBGDIR)/%.o: %.c | prep 
	$(CC) -c $(CFLAGS) $(DBGCFLAGS) -o $@ $<

# Test rules
test: $(TSTEXE)

$(TSTEXE): $(TSTOBJS) | prep
	$(CC) $(CFLAGS) $(LDFLAGS) $(TSTCFLAGS) -o $(TSTEXE) $(TSTOBJS)

$(TSTDIR)/%.o: %.c | prep 
	$(CC) -c $(CFLAGS) $(TSTCFLAGS) -o $@ $<

# Release rules
release: $(RELEXE)

$(RELEXE): $(RELOBJS) | prep 
	$(CC) $(CFLAGS) $(LDFLAGS) $(RELCFLAGS) -o $(RELEXE) $(RELOBJS)

$(RELDIR)/%.o: %.c | prep 
	$(CC) -c $(CFLAGS) $(RELCFLAGS) -o $@ $<

# Other rules
prep:
	@mkdir -p $(DBGDIR) $(RELDIR) $(TSTDIR)

clean:
	rm -f $(RELEXE) $(RELOBJS) $(DBGEXE) $(DBGOBJS) $(TSTEXE) $(TSTOBJS)
