# (C) 2018 University of Bristol. See License.txt

import itertools

class chain(object):
    def __init__(self, *args):
        self.args = args
    def __iter__(self):
        return itertools.chain(*self.args)
