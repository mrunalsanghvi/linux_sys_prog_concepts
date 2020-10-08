BUILDDIR=build
OBJDIR=$(BUILDDIR)/obj
BINDIR=$(BUILDDIR)/bin

vpath %.c src
vpath %.h include


GCC_SANITIZE_FLAGS?=-fsanitize=address -fsanitize=leak
LIB_FLAGS?=-lpthread 
DBG_FLAGS?=-fno-omit-frame-pointer -Wall  -Werror 

#OBJ_TARGET=$(addprefix $(OBJDIR)/,OBJS)
#BIN_TARGET=$(addprefix $(BINDIR)/,BINS)

BIN_TARGETS= client server_socket_epoll 
REL_TARGETS= $(addprefix $(BINDIR)/,$(BIN_TARGETS))

all: build/build $(REL_TARGETS)

build/build:
	mkdir -p  $(BINDIR) $(OBJDIR) $(BUILDDIR) build
	touch build

.PRECIOUS: %.c $(OBJDIR)/%.o

$(BINDIR)/%: $(OBJDIR)/%.o 
	cc -g $(GCC_SANITIZE_FLAGS) $(LIB_FLAGS) $(DBG_FLAGS) -o $@ $^

$(OBJDIR)/%.o: %.c
	cc -g -MMD -c -o $@ $< $(GCC_SANITIZE_FLAGS) $(DBG_FLAGS)
-include $(OBJDIR)/*.d
