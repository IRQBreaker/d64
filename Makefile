TARGET := d64
BINDIR := bin
SRCDIR := src
OBJDIR := obj

CC := gcc
LINKER := gcc
INCDIRS := -I$(SRCDIR)

DEBUG ?= 0
ifeq ($(DEBUG), 0)
   CFLAGS = -Wall -Wextra -Werror -pedantic --std=c99 -O2 -s
else
   CFLAGS = -Wall -Wextra -Werror -pedantic --std=c99 -O0 -ggdb
endif

SRCFILES := $(wildcard $(SRCDIR)/*.c)
OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCFILES))
DEPFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.d,$(SRCFILES))
NODEPS := clean mrproper cppcheck gtags

$(BINDIR)/$(TARGET): $(OBJFILES)
	@mkdir -p $(@D)
	$(LINKER) $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCDIRS) -c $< -o $@

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(INCDIRS) -MM $< | sed -e 's%^%$@ %' -e 's% % $(OBJDIR)/%'\ > $@

ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
    -include $(DEPFILES)
endif

.PHONY: $(NODEPS)
clean:
	rm -rf $(BINDIR) $(OBJDIR)

mrproper:
	git clean -dxf

cppcheck:
	cppcheck --enable=all --language=c --std=c99 --suppress=missingInclude $(SRCFILES)

gtags:
	git ls-files | gtags -f -
