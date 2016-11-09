#!/usr/bin/env python

from decimal import Decimal
import fileinput
import re


# Pattern matches all XML attributes that are X coordinates,
# e.g., x1="12.34"
embiggen_ = re.compile(r'\b(x\d?)="(.*?)"').sub

type_x = {'22.3',
          '22.8333375',
          '23.000003125',
          '23.33338125',
          '23.5',
          '24.5',
          '24.50005',
          '26'}

cr_x = {'37.29', '38.75', '45.25', '46.71'}
rd_x = {'57.79', '59.25', '65.75', '67.21'}
dk_x = {'78.29', '79.75', '86.25', '36.79'}

current_x = [0,
             31.75 - 10.75,
             52.25 - 10.75,
             72.75 - 10.75,
             93.25 - 10.75]

desired_x = [21.5 * i for i in range(5)]

delta = [b - a for (a, b) in zip(current_x, desired_x)]

# cutoff_offset = 24+2/3 - 21
# resonance_offset = 49+1/3 - 41.5
# drive_offset = 74 - 62
# keytrack_offset = 97.5 - 82.6

# print('C', cutoff_offset)
# print('R', resonance_offset)
# print('D', drive_offset)
# print('K', keytrack_offset)
# print()

cutoff_offset = delta[1]
resonance_offset = delta[2]
drive_offset = delta[3]
keytrack_offset = delta[4]

# print('C', cutoff_offset)
# print('R', resonance_offset)
# print('D', drive_offset)
# print('K', keytrack_offset)
# print()

cr_offset = (cutoff_offset + resonance_offset) / 2
rd_offset = (resonance_offset + drive_offset) / 2
dk_offset = (drive_offset + keytrack_offset) / 2

# Replace any X coordinate with the corresponding coordinate on the
# enlarged board.  Disembiggenated panels are 25 mm.  Shrunken
# panels are 21.5 mm.
def unleften(m):
    s = m.group(2)
    try:
       x = float(s)
    except ValueError:
        return m.group(0)

    if s in type_x:
        return m.group(0)
    elif s in cr_x:
        x += cr_offset
    elif s in rd_x:
        x += rd_offset
    elif s in dk_x:
        x += dk_offset
    elif x < 22:                # Type
        return m.group(0)
    elif x < 41:                # Cutoff
        x += cutoff_offset
    elif x < 43:
        x += cr_offset
    elif x < 62:                # Resonance
        x += resonance_offset
    elif x < 63:
        x += rd_offset
    elif x < 82:                # Drive
        x += drive_offset
    elif x < 84:
        x += dk_offset
    elif x < 112:
        x += keytrack_offset
    else:
        return m.group(0)
    return '%s="%s"' % (m.group(1), x)


# Find and replace all x coordinates in a single page of XML.
def embiggen(line):
    return embiggen_(unleften, line)


for line in fileinput.input(openhook=fileinput.hook_encoded('utf-8')):
    line = line.rstrip('\n')
    print(embiggen(line))
