# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Max Beckett
# Creation Date: 9/5/2015
# 

def iterate_with_lookahead(iterable):
    '''Provides a way of knowing if you are the first or last iteration.'''
    iterator = iter(iterable)
    last = next(iterator)
    for val in iterator:
        yield last, False
        last = val
    yield last, True
