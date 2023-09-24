.PHONY: doxygen clean

DOXYGEN_OUTPUT_DIRECTORY ?= .
export DOXYGEN_OUTPUT_DIRECTORY
export DOXYGEN_SOURCE_DIRECTORY := ..

# THISDIR is absolute location of this directory
THISDIR = $(PWD)
INSTALLPATH = /u/group/hallc/www/hallcweb/html/hcana
INSTALLPATHNPS = $(INSTALLPATH)/npsdocs/

all: doxygen

doxygen html/index.html: Doxyfile ROOT.tag Doxygen_hcana.tag Doxygen_podd.tag
	doxygen

ROOT.tag:
	wget https://root.cern.ch/doc/master/ROOT.tag.gz
	gunzip ROOT.tag

Doxygen_hcana.tag:
	wget https://hallcweb.jlab.org/hcana/docs/Doxygen_hcana.tag

Doxygen_podd.tag:
	wget https://hallcweb.jlab.org/hcana/docs/Doxygen_podd.tag

clean:
	rm -r $(DOXYGEN_OUTPUT_DIRECTORY)/html
	rm Doxygen_podd.tag
	rm Doxygen_hcana.tag

# Must have permission to write to $(INSTALLPATH)
install: doxygen
	rsync -av --delete html/ $(INSTALLPATHNPS)
