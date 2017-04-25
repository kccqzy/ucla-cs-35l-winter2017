#!/usr/bin/python

"""
Select or reject lines common to two files

Copyright 2005, 2007 Paul Eggert.
Copyright 2010 Darrell Benjamin Carbajal.
Copyright 2017 Joe Qian.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Please see <http://www.gnu.org/licenses/> for a copy of the license.
"""

from __future__ import print_function
import sys
import locale
from optparse import OptionParser

def to_sort_key(s):
    """Convert a string to a sort key. Converts a word to a locale-specific
representation so that its comparison is locale-aware. In case two strings have
the same sort order according to the locale, sort according to their byte
representations in UTF-8.
    """
    return (locale.strxfrm(s), s.encode('utf8'))

def line_from_file(filename, err_cont):
    """Obtain each line from a file and yield as a generator, or reports an
error."""
    try:
        with open(filename, 'r') if filename != "-" else sys.stdin as fd:
            for line in fd:
                yield line[:len(line) - 1]
        yield None
        # Like indices, always allow one past the final element for nice
        # abstraction. StopIteration is too brutal and makes things
        # unnecessarily difficult.
    except IOError as ioe:
        errno, strerror = ioe.args
        err_cont("{0}: {2} ({1})". format(filename, errno, strerror))

def sorted_comm(gen1, gen2):
    """Yields lines from the two files according to POSIX comm. Runs in constant
memory and works for infinite files."""
    l1 = next(gen1)
    l2 = next(gen2)
    while l1 is not None and l2 is not None:
        l1k = to_sort_key(l1)
        l2k = to_sort_key(l2)
        if l1k < l2k:
            yield (l1, None)
            l1 = next(gen1)
        elif l1k > l2k:
            yield (None, l2)
            l2 = next(gen2)
        else:
            yield (l1, l2)
            l1 = next(gen1)
            l2 = next(gen2)
    if l1 is not None:
        yield (l1, None)
    elif l2 is not None:
        yield (None, l2)
    for r1 in gen1:
        if r1 is not None:
            yield (r1, None)
    for r2 in gen2:
        if r2 is not None:
            yield (None, r2)

def unsorted_comm(gen1, gen2):
    """An idiotic version of comm that works on unsorted files. According to the
spec, output lines should appear in the same order as the input lines. If a
line appears in both input files, it should be output according to the first
input file's order. Lines that appear only in the second input file should be
output after all other lines.

A note on efficiency: the author has not attempted to write any efficient code
for this version. It is just not worth it given the idiotic requirements and
the inability to use other standard library modules like collections or
itertools. As such, the code unnecessarily has O(N^2) time complexity. """
    all_l2 = list(gen2)[:-1]
    all_l2_indices = range(len(all_l2))
    all_l2_keys = map(to_sort_key, all_l2)
    all_l2_paired = list(zip(all_l2, all_l2_indices, all_l2_keys))
    for l1 in gen1:
        if l1 is not None:
            l1k = to_sort_key(l1)
            try:
                v = next((l2, i, l2k) for (l2, i, l2k) in all_l2_paired if l2k == l1k)
                l2, _, _ = v
                yield(l1, l2)
                all_l2_paired.remove(v)
            except StopIteration:
                yield(l1, None)
    for l2, _, _ in all_l2_paired:
        yield (None, l2)

def main():
    locale.setlocale(locale.LC_ALL, '')
    version_msg = "%prog 2.0"
    usage_msg = """%prog [OPTION]... FILE1 FILE2

Select or reject lines common to two files."""

    parser = OptionParser(version=version_msg,
                          usage=usage_msg)
    parser.add_option("-1", action="store_true", dest="suppress_first",
                      help="suppress lines unique to the first file")
    parser.add_option("-2", action="store_true", dest="suppress_second",
                      help="suppress lines unique to the second file")
    parser.add_option("-3", action="store_true", dest="suppress_dups",
                      help="suppress lines duplicated in the files")
    parser.add_option("-u", action="store_true", dest="unsorted",
                      help="assume input is unsorted")
    opts, args = parser.parse_args(sys.argv[1:])

    if len(args) != 2:
        parser.error("wrong number of operands; "
                     "expected 2, given %d" % len(args))

    (fn1, fn2) = args

    file1 = line_from_file(fn1, parser.error)
    file2 = line_from_file(fn2, parser.error)
    for (one, two) in (unsorted_comm if opts.unsorted else sorted_comm)(file1, file2):
        if not two:
            if not opts.suppress_first:
                print(one)
        elif not one:
            if not opts.suppress_second:
                print(("" if opts.suppress_first else "\t") + two)
        elif not opts.suppress_dups:
            tab_count = (not opts.suppress_first) + (not opts.suppress_second)
            print("\t" * tab_count + one)


if __name__ == "__main__":
    main()
