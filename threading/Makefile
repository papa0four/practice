CFLAGS 	= -Wall -Wextra -Werror -Wpedantic -g -std=c11
LDLIBS 	= -lpthread

# Automatic project management
SRCDIR 	= src
OBJDIR	= objects
BINDIR  = bin
SRCS	= $(wildcard $(SRCDIR)/*.c)
OBJS	= $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
BIN    	= $(BINDIR)/server

all: $(BIN)

$(BIN): $(OBJS)
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: $(BIN)

clean:
	$(RM) $(BIN) $(OBJDIR)/*.o