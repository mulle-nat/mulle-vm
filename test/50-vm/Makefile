# Makefile to convert all .dot files to .svg

DOTFILES := $(wildcard *.dot)
SVGFILES := $(DOTFILES:.dot=.svg)

all: $(SVGFILES)

%.svg: %.dot
	dot -Tsvg $< -o $@

clean:
	rm -f $(SVGFILES)
