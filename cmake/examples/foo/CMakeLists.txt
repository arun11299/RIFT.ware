# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Anil Gunturu
# Creation Date: 11/22/2013
# 

cmake_minimum_required(VERSION 2.8)

##
# To see this example at work follow these steps:
#
# mkdir build
# cd build
# cmake ../
# make
##

##
# The following code adds the subdirectories 
# for the compilation.
##
set(subdirs app lib)
foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach(subdir)

