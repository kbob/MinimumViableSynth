#!/usr/bin/env python3

import PIL.Image

with open('foo.bin', 'rb') as infile:
    bb = infile.read()

def dbg(*args):
    pass

def bytes_to_pix(bb):
    i = 0
    while i < len(bb):
        op = bb[i]; i += 1
        if op == 0:
            dbg('single', op)
            pix = bb[i:i+3]; i += 3
            yield pix
        elif op < 128:
            dbg('short ', op)
            pix = bb[i:i+3]; i += 3
            for j in range(op):
                yield pix
        elif op < 255:
            op2 = bb[i]; i += 1
            count = (op & ~128) << 8 | op2
            dbg('long  ', count)
            pix = bb[i:i+3]; i += 3
            for j in range(count):
                yield pix
        else:
            assert op == 255
            op2 = bb[i]; i += 1
            dbg('mixed', op2)
            for j in range(op2):
                pix = bb[i:i+3]; i += 3
                yield pix


def pix_to_ints(pixen):
    for pix in pixen:
        yield tuple(pix)

im = PIL.Image.new('RGBA', (800, 480))

for i, pix in enumerate(pix_to_ints(bytes_to_pix(bb))):
    row = i // 800
    col = i % 800
    assert 0 <= pix[0] < 256
    assert 0 <= pix[1] < 256
    assert 0 <= pix[2] < 256, 'pix = %r' % (pix,)
    color = pix + (0xFF,)
    im.putpixel((col, row), color)


im.show()
