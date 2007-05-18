#! /usr/bin/env python
from distutils.core import setup, Extension

ext_modules = [ 
	Extension('mol-img', 
		sources=['py-mol-img.c', 'mol-img-lib.c'],
		### FIXME - hack for now...
		include_dirs=[ "../../src/include", "../../src/shared", "../../obj-ppc/include" ],
		),
		
	]

setup(	name="mol-img",
	version="1.0",
	ext_modules=ext_modules )
