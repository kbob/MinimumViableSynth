#!/usr/bin/env python3.4

from contextlib import contextmanager
from math import cos, pi, sin

from reportlab.lib.pagesizes import letter
from reportlab.lib.units import inch, mm
from reportlab.pdfgen import canvas

ANGLE = 35
CHASSIS_WIDTH = 135
CHASSIS_HEIGHT = 24.5
OVERHANG = 2

WIDTH = CHASSIS_WIDTH + 2 * OVERHANG
HEIGHT = CHASSIS_HEIGHT + 2 * OVERHANG

RADIUS1 = 6.35 / 2
RADIUS2 = 11 / 2
SCREW_POS = [34, 100]

@contextmanager
def state(c):
    c.saveState()
    yield c
    c.restoreState()

@contextmanager
def path(c, stroke=1, fill=0):
    p = c.beginPath()
    yield p
    c.drawPath(p, stroke=stroke, fill=fill)

def draw_outline(c):
    a = ANGLE * pi / 180
    y1 = HEIGHT * cos(a)
    p2 = (WIDTH * cos(a), y1 + WIDTH * sin(a))
    p3 = (p2[0] + HEIGHT * sin(a), p2[1] - y1)
    p4 = (p3[0], 0)
    c.setLineWidth(0.1)
    with path(c) as p:
        p.moveTo(0, 0)
        p.lineTo(0, y1)
        p.lineTo(*p2)
        p.lineTo(*p3)
        p.lineTo(*p4)
        p.lineTo(0, 0)

@contextmanager
def chassis_transform(c):
    a = ANGLE * pi / 180
    with state(c):
        c.translate(HEIGHT * sin(a), 0)
        c.rotate(ANGLE)
        c.translate(OVERHANG, OVERHANG)
        yield c

def draw_chassis(c):
    with chassis_transform(c):
        c.setDash(2, 1)
        c.rect(0, 0, CHASSIS_WIDTH, CHASSIS_HEIGHT)

def draw_holes(c):
    with chassis_transform(c):
        for x in SCREW_POS:
            with state(c):
                c.translate(x, CHASSIS_HEIGHT / 2)
                c.circle(0, 0, RADIUS1)
                c.circle(0, 0, RADIUS2)
                c.line(-15, 0, +15, 0)
                c.line(0, -15, 0, +15)

def draw_endcap(c):
    with state(c):
        c.scale(mm, mm)
        c.translate(20, 20)
        draw_outline(c)
        draw_chassis(c)
        draw_holes(c)

def draw_it_all(c):
    with state(c):
        draw_endcap(c)
        c.translate(8.5*inch, 11/2*inch)
        c.scale(-1, +1)
        draw_endcap(c)

def render_document():
    output_file = 'endcap.pdf'
    pagesize = letter
    c = canvas.Canvas(output_file, pagesize=pagesize)
    draw_it_all(c)
    c.showPage()
    c.save()

if __name__ == '__main__':
    render_document()
