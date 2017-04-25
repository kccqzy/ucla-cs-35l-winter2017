#!/usr/bin/env python
from __future__ import print_function
import subprocess
import os
from itertools import chain, combinations

def powerset(iterable):
    s = list(iterable)
    return chain.from_iterable(combinations(s, r) for r in xrange(len(s)+1))

def main():
    i = 0
    while True:
        try:
            os.stat('test%d.1.in' % i)
            os.stat('test%d.2.in' % i)
        except OSError:
            break
        for opts in powerset(["-1", "-2", "-3"]):
            args = list(opts) + ['test%d.1.in' % i, 'test%d.2.in' % i]
            expected = subprocess.check_output(["comm"] + args)
            actual = subprocess.check_output(["../comm.py"] + args)
            if expected != actual:
                print("***** A test failed: test case %d\nArguments: %r\nExpected: %r\nActual: %r\n" % (i, args, expected, actual))
                return
        print("Passed test %d." % i)
        i += 1
    print("OK.")

if __name__ == "__main__":
    main()
