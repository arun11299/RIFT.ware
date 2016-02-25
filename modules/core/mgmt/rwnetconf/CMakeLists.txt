# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/05/21
# 

cmake_minimum_required(VERSION 2.8)

#rift_allow_compiler_warning(missing-field-initializers)
#rift_allow_compiler_warning(unused-parameter)
#rift_allow_compiler_warning(unused-but-set-variable)
#rift_allow_compiler_warning(sign-compare)

##
# documentation target
##
rift_add_doxygen_target(rwnetconf_doxygen DEST_DIR rwnetconf)


##
# Add the subdirectories.
# ORDER MATTERS!! SOME DIRS HAVE DEPENDENCIES!
##
# ATTN: Not yet
#set(subdirs src vala plugins test)
#rift_add_subdirs(SUBDIR_LIST ${subdirs})

set(subdirs src test)
rift_add_subdirs(SUBDIR_LIST ${subdirs})
