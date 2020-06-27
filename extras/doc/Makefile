###########################################################################
# Makefile for generating documentation.
# Author: J. Ian Lindsay
# Date:   2020.06.26
###########################################################################

###########################################################################
# Environmental awareness...
###########################################################################
SHELL          = /bin/sh

# This is where we will store compiled libs and the final output.
export BUILD_ROOT    = $(shell pwd)
export OUTPUT_PATH   = $(BUILD_ROOT)/doxygen/


###########################################################################
# Rules for building the firmware follow...
###########################################################################

.PHONY: docs

docs: builddir
	cd ../..; doxygen Doxyfile

builddir:
	mkdir -p $(OUTPUT_PATH)

clean:
	rm -rf $(OUTPUT_PATH)
