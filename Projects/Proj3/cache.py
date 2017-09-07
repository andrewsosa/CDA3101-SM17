# cache.py

from math import log
from report import Report

class Block:
    """ Simple struct-like class representing a block.
    """

    def __init__(self, age, tag, valid=True):
        self.age = age
        self.tag = tag
        self.valid = valid

    def __str__(self):
        print '<%s>' % self.tag


class Cache:
    """ Super-class for abstracting non-policy-specfic behavior.
    """

    def __init__(self, block_size, sets, assoc):
        # validate sizing
        if sets % assoc != 0:
            print "Invalid cache sizing"
            exit()

        # set up cache dimens
        self.length = sets
        self.width = assoc

        # calculate field widths
        self.index_width = int(log(sets,2))
        self.block_width = int(log(block_size, 2))
        self.tag_width   = 32 - self.block_width - self.index_width

        # LRU tracking
        self.counter = 0

        # reporting storage
        self.__report__ = Report()

        # initialize cache data
        self.data = [[Block(-1,-1,False) for y in range(self.width)] for x in range(self.length)]


    """ Functions supporting cache interaction operations.
    """

    def access(self, op, addr):
        self.counter += 1
        self.__report__.refs += 1
        return self.read(addr) if (op == 'R') else self.write(addr)

    def hit(self, addr):
        set_ = self.data[self.index(addr)]
        return any(block.tag == self.tag(addr) for block in set_), set_

    def read(self, addr):
        print 'R %d' % addr

    def write(self, addr):
        print 'W %d' % addr


    """ These functions just get integer values of the address' field.
    """

    def index(self, addr):
        binary = '{0:032b}'.format(addr)
        index = binary[self.tag_width:-self.block_width]
        return int(index, 2)

    def tag(self, addr):
        binary = '{0:032b}'.format(addr)
        tag = binary[:self.tag_width]
        return int(tag, 2)


    """ This just prints the report.
    """

    def report(self):
        self.__report__.print_report("Write Through with No Write Allocate")
