# Obtained from swig distro with customisations
# File : Makefile.swig
# Makefile for a SWIG module.  Use this file if you are
# producing a Ruby extension for general use or distribution.
#
# 1.  Prepare extconf.rb.
# 2.  Modify this file as appropriate.
# 3.  Type 'make -f Makefile.swig' to generate wrapper code and Makefile.
# 4.  Type 'make' to build your extension.
# 5.  Type 'make install' to install your extension.
# 

MODULE    = datasorter
FEATURE   = $(MODULE)
INTERFACE = $(MODULE).i
RUBY      = ruby
SWIG      = swig

# for C extension
#SWIGOPT   = -ruby
#WRAPPER   = $(MODULE)_wrap.c

## for C++ extension
SWIGOPT   = -ruby -c++
WRAPPER   = $(MODULE)_wrap.cc

LIB = engine/lib/libscalars.so
AUXINTERFACES = std_set.i std_list.i

swigall: $(WRAPPER) Makefile

$(WRAPPER): $(INTERFACE) $(AUXINTERFACES) $(LIB)
	$(SWIG) $(SWIGOPT) -o $@ $(INTERFACE)

Makefile: extconf.rb
	$(RUBY) extconf.rb
	@if [ -f Makefile ] ; then\
		echo "include Makefile.swig" >> Makefile;\
	fi

swigclean: myclean
	@if [ -f Makefile ] ; then\
		make -f Makefile clean;\
	fi
	rm -f Makefile $(WRAPPER)

##Added extra rules

myclean:
	rm -fr engine/lib

$(LIB): engine/CMakeLists.txt
	mkdir -pv engine/lib
	cd engine/lib && cmake ../ && make

engine/CMakeLists.txt:
	(cd engine;ruby ../scripts/CMakeListGen.rb scalars)
