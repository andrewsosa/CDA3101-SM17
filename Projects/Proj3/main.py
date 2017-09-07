# main.py

from write_thru import WriteThru
from sys import stdin, argv

def main():

    block = int(argv[argv.index('-b')+1])
    sets  = int(argv[argv.index('-s')+1])
    assoc = int(argv[argv.index('-n')+1])

    cache = WriteThru(block, sets, assoc)
    trace = [line.strip() for line in stdin.readlines()]

    for t in trace:
        op, addr = tuple(t.split())
        cache.access(op, int(addr))

    cache.report()


if __name__ == '__main__':
    main()
