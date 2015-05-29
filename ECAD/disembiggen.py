#!/usr/bin/env python

from decimal import Decimal
import fileinput
import re


# Pattern matches all XML attributes that are X coordinates,
# e.g., x1="12.34"
disembiggen_ = re.compile(r'\b(x\d?)="(.*?)"').sub


# Decimal constants.  We use decimal to prevent loss of precision
# reading and writing numbers.
d0_5 = Decimal('0.5')
d3_5 = Decimal('3.5')


# Replace any X coordinate with the corresponding coordinate on the
# shrunken board.  Undisembiggenated panels are 25 mm.  Shrunken
# panels are 21.5 mm.
def unrighten(m):
    s = m.group(2)
    try:
        n = Decimal(s)
    except ValueError:
        return m.group(0)
    panel = n // 25
    if panel > 0:
        n -= (panel + d0_5) * d3_5
        return '%s="%s"' % (m.group(1), n)
    return m.group(0)


# Find and replace all x coordinates in a single page of XML.
def disembiggen(line):
    return disembiggen_(unrighten, line)


for line in fileinput.input():
    line = line.rstrip('\n')
    print(disembiggen(line))
