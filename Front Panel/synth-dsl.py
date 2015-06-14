#!/usr/bin/env python

from abc import ABCMeta
from collections import namedtuple
from contextlib import contextmanager
from enum import Enum, unique
from fractions import Fraction
from math import atan, ceil, cos, floor, pi, sin, sqrt, tan

from reportlab.lib.pagesizes import landscape, letter
from reportlab.lib.units import inch, mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas

# TODO:
# X   dataflow graphics for CV
# v   dataflow graphics for audio
# X   graphic labels
# X   highlight the cutoff knob
# X   test-fit the LCD screen
# X   make the filter envelope arrow swoopy.

# Notes from 6/5
# X   cut lines:       use 0.001 pt. width.
# X   engraving:       try cutting engraving edge.
# X   choice LEDs:     make slot 0.5mm longer at each end.
# X   knobs:           add rectangular cutout.
# X                    add holes for alignment pins.
# X                    make circular hole 1mm bigger diameter.
# X   amount knobs:    change text to white on black.
# X   control text:    increase kerning to 0.5.
# X                    use bigger font size.
# X   choice text:     use much bigger font size.
# X                    use more leading.
# X                    use graphics where possible.
# X   module outlines: make assign arrow integral part.
#     general:         generate all output formats.




# Use fractions here and force all size calculations to rational arithmetic.
inch2mm = Fraction('25.4')
rack_unit = Fraction('1.75') * inch2mm
PANEL_WIDTH = Fraction('438')  # 438mm makes grid_width an even 21.5mm.
PANEL_HEIGHT = 3 * Fraction('44.50') - Fraction(1, 32) * inch2mm
# print("PANEL_HEIGHT %g" % PANEL_HEIGHT)
assert type(PANEL_WIDTH) is Fraction
assert type(PANEL_HEIGHT) is Fraction

Margins = namedtuple('Margins', 'l r t b')

MODULE_LABEL_HEIGHT = 10
CONTROL_LABEL_HEIGHT = 6
CONTROL_LABEL_Y = -13
PANEL_MARGINS = Margins(2, 2, 2, 2)
MODULE_MARGINS = Margins(2, 2, 4, 10)
TITLE_HEIGHT = 10 + Fraction(7, 30)

TITLE_FONT = ('Jupiter', 40)
title_font_file = '/Users/kbob/Library/Fonts/jupiter.ttf'
pdfmetrics.registerFont(TTFont(TITLE_FONT[0], title_font_file))

MODULE_FONT = ('Jupiter', 20)

CONTROL_FONT = ('Lato-Light', 11)
control_font_file = '/Users/kbob/Library/Fonts/Lato-Light.ttf'
pdfmetrics.registerFont(TTFont('Lato-Light', control_font_file))
CONTROL_CHAR_SPACE = 0.5        # pt.

CHOICE_FONT = ('Lato-Light', 9)
CHOICE_CHAR_SPACE = 0.4         # pt.

LED_blue = (0.3, 0.3, 1.0)
LED_amber = (1, 0.6, 0.2)
LED_red = (1, 0.2, 0.2)
LED_green = (0, 1, 0)

ASSIGN_RADIUS = 8.5 / 2

paginate = False
detail = False
flip = False
cutting_guide = False
if cutting_guide:
    FG = (0, 1, 0)
    BG = (1, 1, 1)
    cut_width = 0.001;
    VEC_COLOR = (1, 0, 0)
    CUT_COLOR = (0, 0, 1)

    do_cut = True
    do_vector = True
    do_engrave = True
else:
    # yellow on green
    # BG = (0.12, 0.24, 0.24)
    # FG = (1, 0.8, 0.3)

    # gray on gray
    FG = (0.9, 0.9, 0.9)
    BG = (0.3, 0.3, 0.3)

    VEC_COLOR = FG

    cut_width = 0.3
    CUT_COLOR = (0, 0, 0)
    do_cut = True
    do_vector = True
    do_engrave = True


@unique
class Layout(Enum):
    left = 1,
    right = 2,
    top = 3,
    bottom = 4,
    center = 5,
    stretch = 6,
    horizontal = 7,
    vertical = 8,


class Widget(metaclass=ABCMeta):
    def __init__(self, **kw):
        self.__dict__.update(kw)
        # if kw:
        #     print('Widget %s kw %s' %
        #           (self, ' '.join('%s=%s' % (k, str(v).split('.')[-1])
        #                           for (k, v) in kw.items())))
    def __repr__(self):
        s = type(self).__name__
        if hasattr(self, 'name'):
            s += ' ' + repr(self.name)
        return s
    @property
    def min_size(self):
        return (0, 0)
    @property
    def grid_size(self):
        return (0, 0)

class Source(metaclass=ABCMeta):
    pass

class Destination(metaclass=ABCMeta):
    pass

class Label(Widget, metaclass=ABCMeta):
    def __init__(self, name):
        self.name = name
    def __repr__(self):
        return repr(self.name)
    def __str__(self):
        return str(self.name)

class TextLabel(Label):
    pass

class WaveformLabel(Label):
    def __init__(self, name, waveform, **kwargs):
        super(WaveformLabel, self).__init__(name, **kwargs)
        self.waveform = waveform

def MakeLabel(name, **kwargs):
    if isinstance(name, Label):
        return name
    if 'waveform' in kwargs:
        return WaveformLabel(name, **kwargs)
    else:
        return TextLabel(name, **kwargs)

class Control(Widget, metaclass=ABCMeta):
    def __init__(self, name, align=Layout.left, **kwargs):
        super(Control, self).__init__(**kwargs)
        self.name = MakeLabel(name)
        self.align = align
    @property
    def grid_size(self):
        return (1, 1)

class Blank(Control):
    def __init__(self, **kwargs):
        super(Blank, self).__init__('', **kwargs)

class Button(Control, metaclass=ABCMeta):
    def __init__(self, name, **kwargs):
        super(Button, self).__init__(name, **kwargs)
    pass

class Knob(Control):
    pass

class Rotary(Control):
    pass

class Choice(Button):
    def __init__(self, name, choices, **kwargs):
        super(Choice, self).__init__(name, **kwargs)
        self.choices = [MakeLabel(choice) for choice in choices]
    def __repr__(self):
        return super(Choice, self).__repr__() + ' (%d)' % len(self.choices)

class AssignButton(Button):
    def __init__(self, **kwargs):
        super(AssignButton, self).__init__('Assign', **kwargs)
    # @property
    # def min_size(self):
    #     return (7, 1)
    # @property
    # def grid_size(self):
    #     return (0.5, 1)

class DestKnob(Knob, Destination):
    pass

class AmountKnob(DestKnob):
    def __init__(self, name='Amount', **kwargs):
        super(AmountKnob, self).__init__(name, **kwargs)

class SemitoneRotary(Widget):
    pass

class Module(Widget):
    def __init__(self, name, controls, layout=Layout.horizontal, **kwargs):
        super(Module, self).__init__(**kwargs)
        self.name = MakeLabel(name)
        self.controls = controls
        self.layout = layout
    def control_index(self, predicate):
        for i, con in enumerate(self.controls):
            if predicate(con):
                return i
    @property
    def min_size(self):
        min_sizes = list(zip(*(c.min_size for c in self.controls)))
        sums = [sum(x) for x in min_sizes]
        maxima = [max(x) for x in min_sizes]
        w = MODULE_MARGINS.l + MODULE_MARGINS.r
        h = MODULE_MARGINS.t + MODULE_MARGINS.b
        if hasattr(self, 'x_pad'):
            w += self.x_pad
        if self.layout == Layout.horizontal:
            return (w + sums[0], h + maxima[1])
        else:
            return (w + maxima[0], h + sums[1])
    @property
    def grid_size(self):
        grid_sizes = list(zip(*(c.grid_size for c in self.controls)))
        sums = [sum(x) for x in grid_sizes]
        maxima = [max(x) for x in grid_sizes]
        if self.layout == Layout.horizontal:
            return (sums[0], maxima[1])
        else:
            return (maxima[0], sums[1])
    @property
    def alignment(self):
        if hasattr(self, 'align'):
            return self.align
        if any(c.align == Layout.stretch for c in self.controls):
            return Layout.stretch
        if all(c.align == Layout.right for c in self.controls):
            return Layout.right
        if any(c.align == Layout.right for c in self.controls):
            return Layout.stretch
        return Layout.left

class TouchScreen(Module):
    def __init__(self, dimensions, **kwargs):
        super(TouchScreen, self).__init__('', (), **kwargs)
        self.dimensions = dimensions
    @property
    def min_size(self):
        return self.dimensions
    @property
    def grid_size(self):
        return (0, 0)

class SourceModule(Module, Source):
    def __init__(self, name, controls, side=Layout.left, **kwargs):
        if side == Layout.left:
            controls += (AmountKnob(align=Layout.right), 
                         AssignButton(align=Layout.right))
        elif side == Layout.right:
            controls = (AssignButton(), AmountKnob()) + controls
        else:
            raise RuntimeError()
        super(SourceModule, self).__init__(name, controls, **kwargs)
        self.side = side


class Title(Module):
    def __init__(self, name, **kwargs):
        super(Title, self).__init__(name, (), **kwargs)
    @property
    def min_size(self):
        return (0, TITLE_HEIGHT)
    @property
    def grid_size(self):
        return (0, 0)
    @property
    def alignment(self):
        return Layout.stretch


class PanelColumn(Widget):
    def __init__(self, modules, **kwargs):
        super(PanelColumn, self).__init__(**kwargs)
        self.modules = modules
    @property
    def min_size(self):
        min_sizes = [c.min_size for c in self.modules]
        w = max(s[0] for s in min_sizes)
        h = sum(s[1] for s in min_sizes)
        return (w, h)
    @property
    def grid_size(self):
        grid_sizes = [m.grid_size for m in self.modules]
        w = max(s[0] for s in grid_sizes)
        h = sum(s[1] for s in grid_sizes)
        if any(isinstance(m, TouchScreen) for m in self.modules):
            w = 0               # XXX hack hack
        return (w, h)


class PanelRow(Widget):
    def __init__(self, modules, **kwargs):
        super(PanelRow, self).__init__(**kwargs)
        self.modules = modules
    @property
    def min_size(self):
        w = MODULE_MARGINS.l + MODULE_MARGINS.r
        h = MODULE_MARGINS.t + MODULE_MARGINS.b
        return (w, h)
    @property
    def grid_size(self):
        grid_sizes = [m.grid_size for m in self.modules]
        w = sum(s[0] for s in grid_sizes)
        h = max(s[1] for s in grid_sizes)
        return (w, h)
    @property
    def alignment(self):
        return Layout.stretch

class Panel(Widget):
    def __init__(self, columns, **kwargs):
        super(Panel, self).__init__(**kwargs)
        self.columns = columns
    @property
    def min_size(self):
        min_sizes = [c.min_size for c in self.columns]
        # print('panel min_sizes', min_sizes)
        w = PANEL_MARGINS.l + sum(s[0] for s in min_sizes) + PANEL_MARGINS.r
        h = PANEL_MARGINS.t + max(s[1] for s in min_sizes) + PANEL_MARGINS.b
        return (w, h)
    @property
    def grid_size(self):
        grid_sizes = [c.grid_size for c in self.columns]
        w = sum(s[0] for s in grid_sizes)
        h = max(s[1] for s in grid_sizes)
        return (w, h)

##############################################################################

# saw = WaveformLabel('Saw', lambda x: 2*x%2-1)
# square = WaveformLabel('Square', lambda x: +1 if x%1 < 1/2 else -1)
# triangle = WaveformLabel('Triangle',
#                          lambda x: 4*x%4-1 if x%1 < 1/2 else 3-4*x%4)
# sine = WaveformLabel('Sin', lambda x: sin(x * 2 * pi))

# # pulse = ['Pulse', lambda x: +1 if x < 1/4 else -1]
# # skew_tri = ['Skew Triangle',
# #             lambda x: 8*x%8-1 if x%1 < 1/4 else 5/3 - 8/3*(x%1)]

# saw_up = WaveformLabel('Saw Up', lambda x: 2*x%2-1)
# saw_down = WaveformLabel('Saw Down', lambda x: 1-2*x%2)
# random = WaveformLabel('Random',
#                        lambda x: rand_noise(3*x, 9) / -rand_noise(2, 9))
# sample_hold = WaveformLabel('Sample and Hold',
#                             lambda x: rand_noise(int(3 * x), 5))

# def rand_noise(x, b):
#     def bigrand(n): return bigrand(n - 1) * 48271 % 0x7fffffff if n else b
#     def random(n): return bigrand(n) % 23 / 23
#     a = random(int(x))
#     b = random(int(x) + 1)
#     return (a + x%1 * (b - a)) * 2 - 1

class Waveform(Enum):
    saw = 1
    square = 2
    triangle = 3
    sine = 4
    # pulse = 5
    # skew_triangle = 6
    saw_up = saw
    saw_down = 7
    random = 8
    sample_hold = 9

saw = WaveformLabel('Saw', Waveform.saw)
square = WaveformLabel('Square', Waveform.square)
triangle = WaveformLabel('Triangle', Waveform.triangle)
sine = WaveformLabel('Sine', Waveform.sine)

# pulse = WaveformLabel('Pulse', Waveform.pulse)
# skew_tri = WaveformLabel('Skew Triangle', Waveform.skew_triangle)

saw_up = WaveformLabel('Saw Up', Waveform.saw_up)
saw_down = WaveformLabel('Saw Down', Waveform.saw_down)
random = WaveformLabel('Random', Waveform.random)
sample_hold = WaveformLabel('Sample and Hold', Waveform.sample_hold)

af_waveforms = [saw, square, triangle, sine]
# mod_waveforms = [pulse, skew_tri]
lf_waveforms = [triangle,
                saw_up,
                saw_down,
                square,
                random,
                sample_hold]

def LFO(label, *args, **kwargs):
    return SourceModule(label,
                        (Choice('Waveform', lf_waveforms, color=LED_blue),
                         DestKnob('Speed')),
                        **kwargs)

def Envelope(label, *controls, assign=True, **kwargs):
    controls += (Knob('Attack', align=Layout.right),
                 Knob('Decay', align=Layout.right),
                 Knob('Sustain', align=Layout.right),
                 Knob('Release', align=Layout.right))
    if assign:
        return SourceModule(label, controls, side=Layout.right, **kwargs)
    else:
        return Module(label, controls, **kwargs)

## Front Panel Definition

lfo1 = LFO('LFO 1')
lfo2 = LFO('LFO 2')

env1 = Envelope('Envelope 1')
env2 = Envelope('Filter Envelope',
                AmountKnob(align=Layout.right),
                assign=False,
                align=Layout.right,
                destination='Cutoff', early=True)
env3 = Envelope('Amp Envelope',
                assign=False, align=Layout.right,
                destination='Master Volume',
                early=True)

ctls = SourceModule('Controllers',
                    (Choice('Controller', ['Velocity',
                                          'Mod Wheel',
                                          'Aftertouch',
                                           'Other'], color=LED_blue),
                     Blank()),
                    side=Layout.left)

title = Title('Minimum Viable Synth')

screen = TouchScreen(dimensions=(121, 76))

osc1 = Module('Oscillator 1',
              (Choice('Waveform', af_waveforms, color=LED_amber),
               DestKnob('Width'),
               DestKnob('Pitch'),
               # Knob('Fine Pitch'),
               AmountKnob(align=Layout.right)))
osc2 = Module('Oscillator 2',
              (Choice('Waveform', af_waveforms, color=LED_amber),
               DestKnob('Width'),
               DestKnob('Pitch'),
               AmountKnob(align=Layout.left)))
noise = Module('Noise',
               (Choice('Spectrum', ['White', 'Pink', 'Red'], color=LED_amber),
                AmountKnob()))
mixer = Module('Mix',
               (Choice('Operator',
                       ['Mix', 'Ring Mod', 'Hard Sync'],
                       color=LED_green),
                Blank()),
               align=Layout.center)
filter = Module('Filter',
                (Choice('Type',
                        ['Low Pass', 'High Pass', 'Off'],
                        color=LED_red),
                 DestKnob('Cutoff', align=Layout.stretch, highlight=270),
                 DestKnob('Resonance', align=Layout.stretch),
                 DestKnob('Drive', align=Layout.right),
                 DestKnob('Key Track', align=Layout.right)),
                early=True)
# amp = Module('Amp',
#              (AmountKnob('Master Volume', align=Layout.stretch),),
#              align=Layout.center, x_pad=5)
amp = Module('Amp',
             (AmountKnob('Master Volume', align=Layout.stretch),),
             x_pad=5)

col0 = PanelColumn([lfo1, lfo2, ctls])
col1 = PanelColumn([osc1, osc2, PanelRow([noise, mixer])])
col3 = PanelColumn([title, screen, filter])
col4 = PanelColumn([env1, env2, PanelRow([amp, env3])])

if detail:
    PANEL_WIDTH = 4 * Fraction('21.5') + 2 * MODULE_MARGINS[0] + PANEL_MARGINS[0]
    PANEL_HEIGHT = Fraction('44.50') - Fraction(1, 32) * inch2mm
    panel = Panel([PanelColumn([lfo1])], early=True)
else:
    panel = Panel([col0, col1, col3, col4], early=True)

# print('panel min size', panel.min_size)
# print('panel grid size', panel.grid_size)

##############################################################################


# for i, c in enumerate(panel.columns, 1):
#     print('Column', i)
#     for m in c.modules:
#         print(' ', m)
#         for v in m.controls:
#             print('   ', v)

# print('%d columns' % len(panel.columns))
# print('%d controls wide' % panel.control_width())
# print(' + '.join(str(c.control_width()) for c in panel.columns))
# print('%d controls high' % panel.control_height())

class Box(namedtuple('Box', 'x y w h widget')):
    pass

def layout_panel(panel):
    # print('Layout')
    ms = panel.min_size
    gs = panel.grid_size
    grid_width = (PANEL_WIDTH - ms[0]) / gs[0]
    # print("grid_width = (%g - %g) / %g\n" % (PANEL_WIDTH, ms[0], gs[0]))
    global g_grid_width
    g_grid_width = grid_width
    grid_height = (PANEL_HEIGHT - ms[1]) / gs[1]
    grid_size = (grid_width, grid_height)
    # print('panel grid: %g x %g' % (grid_width, grid_height))
    x = PANEL_MARGINS.l
    boxes = [Box(0, 0, PANEL_WIDTH, PANEL_HEIGHT, panel)]
    for column in panel.columns:
        cms = column.min_size
        cgs = column.grid_size
        pos = (x, PANEL_MARGINS.b)
        col_size = (cms[0] + cgs[0] * grid_width,
                    PANEL_HEIGHT - PANEL_MARGINS.t - PANEL_MARGINS.b)
        boxes += layout_column(column, pos, col_size, grid_size)
        x += col_size[0]
    return boxes


def layout_column(column, col_pos, col_size, grid_size):
    # print('  Column at (%g, %g)' % col_pos)
    boxes = []
    ms = column.min_size
    gs = column.grid_size
    grid_width = grid_size[0]
    assert g_grid_width == grid_width
    grid_height = (col_size[1] - ms[1]) / gs[1]
    grid_size = (grid_width, grid_height)
    y = col_pos[1] + col_size[1]
    for module in column.modules:
        mms = module.min_size
        mgs = module.grid_size
        ma = module.alignment
        mod_size = (mms[0] + mgs[0] * grid_width,
                    mms[1] + mgs[1] * grid_height)
        y -= mod_size[1]
        if ma == Layout.left:
            mod_pos = (col_pos[0], y)
        elif ma == Layout.right:
            mod_pos = (col_pos[0] + col_size[0] - mod_size[0], y)
        elif ma == Layout.stretch:
            mod_pos = (col_pos[0], y)
            mod_size = (col_size[0], mod_size[1])
        else:
            assert False, module.name
        if isinstance(module, Module):
            boxes += layout_module(module, mod_pos, mod_size, grid_size)
        elif isinstance(module, PanelRow):
            boxes += layout_row(module, mod_pos, mod_size, grid_size)
    return boxes

def layout_row(row, row_pos, row_size, grid_size):

    def module_width(mod):
        return mod.min_size[0] + mod.grid_size[0] * grid_size[0]

    # print('    Row at (%g, %g)' % row_pos)

    # Check alignments
    alignments = [mod.alignment for mod in row.modules]
    for a0, a1 in zip(alignments[:-1], alignments[1:]):
        assert a0 == a1 or a0 == Layout.left or a1 == Layout.right, modules

    n_center = sum(a == Layout.center for a in alignments)
    n_stretch = sum(a == Layout.stretch for a in alignments)
    stretch_amount = row_size[0] - sum(module_width(m) for m in row.modules)

    boxes = []
    ms = row.min_size
    gs = row.grid_size
    n = len(row.modules)
    x0 = row_pos[0]
    x1 = x0 + row_size[0]
    x = x0
    for (i, module) in enumerate(row.modules):
        # print("loop: x = %g a=%s" % (x, module.alignment))
        # print('      stretch_amount = %g' % stretch_amount)
        mms = module.min_size
        mgs = module.grid_size
        alignment = module.alignment
        if alignment == Layout.left:
            mod_size = (module_width(module), row_size[1])
            mod_pos = (x, row_pos[1])
            x += mod_size[0]
        elif alignment == Layout.center:
            stretch = stretch_amount / (n_center + 1)
            stretch_amount -= stretch
            n_center -= 1
            x += stretch
            mod_size = (module_width(module), row_size[1])
            mod_pos = (x, row_pos[1])
            x += mod_size[0]
        elif alignment == Layout.stretch:
            stretch = stretch_amount / (n_stretch + 1)
            stretch_amount -= stretch
            n_stretch -= 1
            mod_size = (module_width(module) + stretch, row_size[1])
            mod_pos = (x, row_pos[1])
            x += mod_size[0]
        elif alignment == Layout.right:
            x += stretch_amount
            stretch_amount = 0
            mod_size = (module_width(module), row_size[1])
            mod_pos = (x, row_pos[1])
            x += mod_size[0]
        else:
            raise RuntimeError()
        boxes += layout_module(module, mod_pos, mod_size, grid_size)
    return boxes

def layout_module(module, mod_pos, mod_size, grid_size):
    # if str(module.name) == 'Controllers':
    #     print('    Module %s at (%g, %g) size (%g, %g)' %
    #           (module.name, mod_pos[0], mod_pos[1], mod_size[0], mod_size[1]))
    boxes = [Box(mod_pos[0], mod_pos[1], mod_size[0], mod_size[1], module)]
    ms = module.min_size
    gs = module.grid_size
    if module.layout == Layout.vertical:
        grid_width = grid_size[0]
        assert g_grid_width == grid_width
        grid_height = (mod_size[1] - ms[1]) / gs[1] if gs[1] else 0
        x = mod_pos[0] + MODULE_MARGINS.l
        y = mod_pos[1] + mod_size[1] - MODULE_MARGINS.t
        for control in module.controls:
            y -= grid_height
            ctl_pos = (x, y)
            boxes += layout_control(control, ctl_pos, grid_size)
    else:
        control_count = len(module.controls)
        alignments = [c.align for c in module.controls]
        left_count = sum(a == Layout.left for a in alignments)
        stretch_count = sum(a == Layout.stretch for a in alignments)
        x0 = mod_pos[0] + MODULE_MARGINS.l
        x1 = mod_pos[0] + mod_size[0] - MODULE_MARGINS.r
        y = mod_pos[1] + MODULE_MARGINS.b
        for (i, control) in enumerate(module.controls):
            if control.align == Layout.left:
                x = x0 + i * grid_size[0]
            elif control.align == Layout.right:
                x = x1 - (control_count - i) * grid_size[0]
            elif control.align == Layout.stretch:
                stretch_amount = (x1 - x0 - grid_size[0] * control_count) / (stretch_count + 1)
                x = x0 + i * grid_size[0] + stretch_amount * (i - left_count + 1)
            else:
                raise RuntimeError()
            ctl_pos = (x, y)
            boxes += layout_control(control, ctl_pos, grid_size)
        # if str(module.name) == 'Controllers':
        #     print(boxes[0])
        #     print(boxes[1])
    return boxes

def layout_control(control, pos, size):
    # print('      Control %s at (%g %g) size (%g %g)' %
    #       (control, pos[0], pos[1], size[0], size[1]))
    return [Box(pos[0], pos[1], size[0], size[1], control)]


##############################################################################


class CanvasGuard:

    def __init__(self, canvas):
        self.canvas = canvas

    def restoreState(self):
        return self.canvas.restoreState()

    def rotate(self, theta):
        return self.canvas.rotate(theta)

    def save(self):
        return self.canvas.save()

    def saveState(self):
        return self.canvas.saveState()

    def scale(self, x, y):
        assert x in (-1, +1) and y in (-1, +1)
        return self.canvas.scale(x, y)

    # def setFillColorRGB(self, r, g, b):
    #     return self.canvas.setFillColorRGB(r, g, b)

    # def setLineWidth(self, w):
    #     return self.canvas.setLineWidth(w)

    # def setStrokeColorRGB(self, r, g, b):
    #     return self.canvas.setStrokeColorRGB(r, g, b)

    def showPage(self):
        return self.canvas.showPage()

    def stringWidth(self, text, fontName=None, fontSize=None):
        return self.canvas.stringWidth(text, fontName, fontSize)

    def translate(self, dx, dy):
        return self.canvas.translate(dx, dy)


def find_boxp(predicate):
    for box in boxes:
        if predicate(box):
            return box


def find_box(widget):
    return find_boxp(lambda box: box.widget == widget)

@contextmanager
def path(c, stroke=1, fill=0):
    p = c.beginPath()
    yield p
    c.drawPath(p, stroke=stroke, fill=fill)

@contextmanager
def text(c, x=0, y=0):
    t = c.beginText(x, y)
    yield t
    c.drawText(t)

@contextmanager
def state(c):
    c.saveState()
    yield c
    c.restoreState()


def graphics(c):
    if not cutting_guide:
        with state(c):
            yield c.canvas


@contextmanager
def cutter(c):
    cut = c.canvas.beginPath()
    yield cut
    if do_cut:
        with state(c):
            c.canvas.setFillColorRGB(*BG)
            c.canvas.setLineWidth(cut_width)
            c.canvas.setStrokeColorRGB(*CUT_COLOR)
            c.canvas.drawPath(cut, stroke=1, fill=0)


@contextmanager
def engraver(c):
    egv = c.canvas.beginPath()
    yield egv
    if do_vector or do_engrave:
        with state(c):
            c.canvas.setFillColorRGB(*FG)
            c.canvas.setLineWidth(cut_width)
            c.canvas.setStrokeColorRGB(*VEC_COLOR)
            c.canvas.drawPath(egv, stroke=do_vector, fill=do_engrave)

@contextmanager
def stroke_engraver(c):
    with state(c):
        egv = c.canvas.beginPath()
        yield egv
        if do_engrave:
            c.canvas.setFillColorRGB(*FG)
            c.canvas.setStrokeColorRGB(*FG)
            c.canvas.drawPath(egv, stroke=1, fill=0)

@contextmanager
def vector_engraver(c):
    egv = c.canvas.beginPath()
    yield egv
    if do_vector:
        with state(c):
            c.canvas.setFillColorRGB(*FG)
            c.canvas.setLineWidth(cut_width)
            c.canvas.setStrokeColorRGB(*VEC_COLOR)
            c.canvas.drawPath(egv, stroke=1, fill=0)
            

@contextmanager
def text_engraver(c, x=0, y=0, fill=0):
    if fill and not do_vector:
        mode = 0
    elif do_vector and not fill:
        mode = 1
    elif fill and do_vector:
        mode = 2
    else:
        mode = 3
    
    t = c.canvas.beginText(x, y)
    t.setTextRenderMode(mode)
    yield t
    if do_engrave:
        with state(c):
            c.canvas.setFillColorRGB(*FG)
            c.canvas.setStrokeColorRGB(*VEC_COLOR)
            c.canvas.setLineWidth(cut_width)
            c.canvas.drawText(t)
            # t.setTextRenderMode(0)
            # c.canvas.setLineWidth(4)
            # c.canvas.setStrokeColorRGB(0, 1, 0)
            # c.canvas.drawText(t)

@contextmanager
def inverse_text_engraver(c, x=0, y=0, fill=0):
    if fill and not do_vector:
        mode = 0
    elif do_vector and not fill:
        mode = 1
    elif fill and do_vector:
        mode = 2
    else:
        mode = 3

    t = c.canvas.beginText(x, y)
    t.setTextRenderMode(mode)
    yield t
    if do_engrave:
        with state(c):
            c.canvas.setFillColorRGB(*BG)
            c.canvas.setLineWidth(cut_width)
            c.canvas.setStrokeColorRGB(*VEC_COLOR)
            c.canvas.drawText(t)
            # t.setTextRenderMode(0)
            # c.setStrokeColorRGB(*VEC_COLOR)
            # c.canvas.drawText(t)
            

def render(boxes):
    if paginate:
        pagesize = landscape(letter)
        c = canvas.Canvas('Synth Pages.pdf', pagesize=pagesize)
        c = CanvasGuard(c)
        for x in (1, 10 - PANEL_WIDTH / inch2mm):
            c.translate(x * inch,
                        (pagesize[1] - PANEL_HEIGHT * mm) / 2)
            render_boxes(c, boxes, early=True)
            render_boxes(c, boxes)
            c.showPage()
        c.save()
    else:
        pagesize = (PANEL_WIDTH*mm + 1*inch, PANEL_HEIGHT*mm + 1*inch)
        c = canvas.Canvas('Synth Panel.pdf', pagesize=pagesize)
        c = CanvasGuard(c)
        if flip:
            c.translate((pagesize[0] + PANEL_WIDTH * mm) / 2,
                        (pagesize[1] - PANEL_HEIGHT * mm) / 2)
            c.scale(-1, +1)
        else:
            c.translate((pagesize[0] - PANEL_WIDTH * mm) / 2,
                        (pagesize[1] - PANEL_HEIGHT * mm) / 2)
        render_boxes(c, boxes, early=True)
        render_boxes(c, boxes)
        c.showPage()
        c.save()


def render_boxes(c, boxes, early=False):

    for box in boxes:
        w = box.widget
        if getattr(w, 'early', False) != early:
            continue
        if isinstance(w, Panel):
            render_panel(c, box)
        elif isinstance(w, Title):
            render_title(c, box)
        elif isinstance(w, TouchScreen):
            render_touchscreen(c, box)
        # elif isinstance(w, SourceModule):
        #     render_source_module(c, box)
        elif isinstance(w, Module):
            if getattr(w, 'destination', None):
                render_module_dest(c, box)
            render_module(c, box)
        elif isinstance(w, Blank):
            render_blank(c, box)
        elif isinstance(w, Choice):
            render_choice(c, box)
        elif isinstance(w, AssignButton):
            render_assign_button(c, box)
        elif isinstance(w, AmountKnob):
             render_amount_knob(c, box)
        elif isinstance(w, Knob):
             render_knob(c, box)
        else:
            assert False, w


def render_panel(c, box):
    with cutter(c) as cut:
        cut.rect(box.x * mm, box.y * mm, box.w * mm, box.h * mm)
    for g in graphics(c):
        g.setFillColorRGB(*BG)
        g.rect(box.x * mm, box.y * mm, box.w * mm, box.h * mm, fill=1, stroke=0)
    

def render_title(c, box):
    w = box.widget
    s = str(w.name)
    width = c.stringWidth(s, *TITLE_FONT)
    with text_engraver(c, box.x * mm, (box.y + 3) * mm, fill=1) as t:
        t.setFont(*TITLE_FONT)
        t.setHorizScale(100 * box.w * mm / width)
        t.textOut(s)
    # w = box.widget
    # s = str(w.name)
    # with state(c):
    #     c.setFont(*TITLE_FONT)
    #     c.setFillColorRGB(*FG)
    #     width = c.stringWidth(s, None, None)
    #     c.translate(box.x * mm, (box.y + 3) * mm)
    #     if do_engrave:
    #         t = c.beginText()
    #         t.setHorizScale(box.w * mm / width * 100)
    #         t.textOut(s)
    #         c.drawText(t)


def render_touchscreen(c, box):
    with cutter(c) as cut:
        cut.rect(box.x * mm, box.y * mm, box.w * mm, box.h * mm)
    for g in graphics(c):
        g.setFillGray(0.9)
        g.rect(box.x * mm, box.y * mm, box.w * mm, box.h * mm, stroke=0, fill=1)
    # with state(c):
    #     if cutting_guide:
    #         fill = 0
    #         stroke = 1
    #     else:
    #         fill = 1
    #         stroke = 0
    #         c.setFillGray(0.9)
    #     c.setLineWidth(cut_width)
    #     c.setStrokeColorRGB(*CUT_COLOR)
    #     if do_cut:
    #         c.rect(box.x*mm, box.y*mm,
    #                box.w*mm, box.h*mm,
    #                stroke=stroke, fill=fill)

# def render_source_module(c, box):

#     render_module(c, box)

#     # mod = box.widget
#     # amount_index = mod.control_index(lambda c: isinstance(c, AmountKnob))
#     # assign_index = mod.control_index(lambda c: isinstance(c, AssignButton))
#     # amount_box = find_box(mod.controls[amount_index])
#     # assign_box = find_box(mod.controls[assign_index])
#     # amount_cx = amount_box.x + amount_box.w / 2
#     # amount_cy = amount_box.y + amount_box.h / 2
#     # assign_cx = assign_box.x + assign_box.w / 2
#     # assign_cy = assign_box.y + assign_box.h / 2
#     # with state(c):
#     #     c.setStrokeColorRGB(*FG)
#     #     c.setFillColorRGB(*FG)
#     #     if do_engrave:
#     #         c.circle(assign_cx*mm, assign_cy*mm, ASSIGN_RADIUS*mm, stroke=0, fill=1)
#     #     if assign_index:
#     #         c.translate((assign_cx - ASSIGN_RADIUS)*mm, assign_cy*mm)
#     #         c.rotate(135)
#     #         if do_engrave:
#     #             c.rect(0, 0, 10*mm, 10*mm, stroke=0, fill=1)
#     #     else:
#     #         c.translate((assign_cx + ASSIGN_RADIUS)*mm, assign_cy*mm)
#     #         c.rotate(-45)
#     #         if do_engrave:
#     #             c.rect(0, 0, 10*mm, 10*mm, stroke=0, fill=1)


def render_module_dest(c, box):

    render_module(c, box)
    mod = box.widget
    amount_index = mod.control_index(lambda c: isinstance(c, AmountKnob)) or 0
    dest_box = find_boxp(lambda box: str(getattr(box.widget,
                                                 'name',
                                                 None)) == mod.destination)
    amount_box = find_box(mod.controls[amount_index])
    amount_cx = amount_box.x + amount_box.w / 2
    amount_cy = amount_box.y + amount_box.h / 2
    tip_x = amount_box.x - amount_box.w / 2 + ASSIGN_RADIUS
    dest_cx = dest_box.x + dest_box.w / 2
    dest_cy = dest_box.y + dest_box.h / 2
    # dx = dest_cx - amount_cx
    # dy = dest_cy - amount_cy
    # tip_y = amount_cy + (tip_x - amount_cx) * dy / dx
    tip_y = amount_cy
    arrow_width = (amount_box.x - tip_x) * 2
    with state(c), stroke_engraver(c) as g:
        c.canvas.setLineWidth(0.5 * mm)
        c.canvas.setDash(1 * mm, 1 * mm)
        if abs(tip_y - dest_cy) < 0.00001:
            g.moveTo(tip_x*mm, tip_y*mm)
            g.lineTo(dest_cx*mm, dest_cy*mm)
        else:
            mid_x = tip_x - 15.3
            mid_y = (tip_y + dest_cy) / 2 - 4
            g.moveTo(tip_x*mm, tip_y*mm)
            g.curveTo((tip_x - 12)*mm, tip_y*mm,
                      (mid_x + 10)*mm, (mid_y + 10)*mm,
                      mid_x*mm, mid_y*mm)
            g.curveTo((mid_x - 15)*mm, (mid_y - 15)*mm,
                      (dest_cx + 20)*mm, (dest_cy + 25)*mm,
                      dest_cx*mm, dest_cy*mm)
        
            # # Encircle the control point.
            # for g in graphics(c):
            #     g.setStrokeColorRGB(0, 1, 0)
            #     g.setLineWidth(0.1)
            #     g.setDash(100)
            #     g.circle(mid_x*mm, mid_y*mm, 1.5*mm)

    # with state(c):
    #     c.setStrokeColorRGB(*FG)
    #     c.setFillColorRGB(*FG)
    #     with state(c):
    #         c.translate(tip_x*mm, tip_y*mm)
    #         # c.rotate(atan(dy / dx) * 180/pi)
    #         c.setLineWidth(arrow_width*mm)
    #         if do_engrave:
    #             c.line((arrow_width / 2)*mm, 0, (amount_cx - tip_x)*mm, 0)
    #         with state(c):
    #             c.rotate(-45)
    #             if do_engrave:
    #                 wr2 = arrow_width / sqrt(2)
    #                 c.rect(0, 0, wr2*mm, wr2*mm,
    #                        # (arrow_width / sqrt(2))*mm, (arrow_width / sqrt(2))*mm,
    #                        stroke=0, fill=1)
    #     c.setLineWidth(0.5*mm)
    #     c.setDash(1*mm, 1*mm)
    #     if do_engrave:
    #         with path(c) as p:
    #             if abs(tip_y - dest_cy) < 0.00001:
    #                 p.moveTo(tip_x*mm, tip_y*mm)
    #                 p.lineTo(dest_cx*mm, dest_cy*mm)
    #             else:
    #                 mid_x = tip_x - 16.5
    #                 mid_y = (tip_y + dest_cy) / 2 - 4
    #                 p.moveTo(tip_x*mm, tip_y*mm)
    #                 p.curveTo((tip_x - 12)*mm, tip_y*mm,
    #                           (mid_x + 10)*mm, (mid_y + 10)*mm,
    #                           mid_x*mm, mid_y*mm)
    #                 p.curveTo((mid_x - 15)*mm, (mid_y - 15)*mm,
    #                           (dest_cx + 20)*mm, (dest_cy + 25)*mm,
    #                           dest_cx*mm, dest_cy*mm)
    #                 # with state(c):
    #                 #     c.setStrokeColorRGB(0, 1, 0)
    #                 #     c.setLineWidth(0.1)
    #                 #     c.setDash(100)
    #                 #     c.circle(mid_x*mm, mid_y*mm, 1.5*mm)


def render_module(c, box):

    if not do_engrave:
        return

    # Dims: dimension that change between outer and inner outline
    # x0: left edge
    # y0: bottom edge
    # x1: right edge
    # y1: top edge
    # xm: x margin
    # ym: y margin
    # hr; horizontal radius
    # vr: vertical radius
    # by: baseline y
    # tw: tab width
    # ax0: assign button x0 (0 for no assign button on left)
    # ax1: assign button x1 (0 for no assign button on right)
    # ay: assign button y
    dims = namedtuple('dims', 'x0 y0 x1 y1 hr vr by tw ax0 ax1 ay')

    def outline(egv, d, color):
        x0 = d.x0
        y0 = d.y0
        x1 = d.x1
        y1 = d.y1
        hr = d.hr
        vr = d.vr
        hd = 2 * hr
        vd = 2 * vr

        ta_r = tab_angle * pi/180
        a_r = atan(tan(ta_r) * tab_hr / tab_vr)
        a = a_r * 180/pi
        tx0 = x0 + d.tw
        tx1 = tx0 + (d.by - y0) / tan(ta_r)
        c0 = (tx0 - tab_hr * tan(a_r / 2), y0 + tab_vr)
        c1 = (tx1 + tab_hr * tan(a_r / 2), d.by - tab_vr)

        with state(c):
            egv.moveTo((x0 + hr)*mm, y1*mm)
            egv.arcTo((x1 - hd)*mm, y1*mm, x1*mm, (y1 - vd)*mm, 90, -90)
            if d.ax1:
                # print('ay %.5g' % d.ay)
                # print('ax1', d.ax1)
                # print('x1 %.5g' % x1)
                # print('y', (d.ay+d.ax1-x1))
                egv.lineTo(x1*mm, (d.ay + d.ax1 - x1)*mm)
                egv.lineTo(d.ax1*mm, d.ay*mm)
                egv.lineTo(x1*mm, (d.ay - d.ax1 + x1)*mm)
            egv.arcTo(x1*mm, (d.by + vd)*mm, (x1 - hd)*mm, d.by*mm, 0, -90)
            if d.by != y0:
                egv.arcTo((c1[0] - tab_hr)*mm, (c1[1] - tab_vr)*mm,
                          (c1[0] + tab_hr)*mm, (c1[1] + tab_vr)*mm,
                          90, a)
                egv.arcTo((c0[0] - tab_hr)*mm, (c0[1] - tab_vr)*mm,
                          (c0[0] + tab_hr)*mm, (c0[1] + tab_vr)*mm,
                          270 + a, -a)
                if c1[0] > x1 - hr:
                    print('%s: c1.x=%g > x1=%g - hr=%g' %
                          (mod.name, c1[0], x1, hr))
            egv.arcTo((x0 + hd)*mm, y0*mm, x0*mm, (y0 + vd)*mm, 270, -90)
            if d.ax0:
                egv.lineTo(x0*mm, (d.ay - x0 + d.ax0) * mm)
                egv.lineTo(d.ax0*mm, d.ay*mm)
                egv.lineTo(x0*mm, (d.ay + x0 - d.ax0) * mm)
            egv.arcTo(x0*mm, (y1 - vd)*mm, (x0 + hd)*mm, y1*mm, 180, -90)

    # Independent variables
    margin = (MODULE_MARGINS[0], 1)
    thickness = (2, 1)
    radius = (5, 4)
    tab_angle = 55
    tab_x0 = 13
    label_pad = (2.5, 1)
    label_pos = (tab_x0 + label_pad[0], 0)
    label_height = 6
    al_height = 3
    al_pad = (0, 2)

    # Dependent variables
    mod = box.widget
    base_y = margin[1] + label_height / mm + 2 * label_pad[1]
    tab_hr = radius[0] - thickness[0]
    tab_vr = radius[1] - thickness[1]
    label_w = c.stringWidth(str(mod.name), *MODULE_FONT) / mm
    outer_x0 = margin[0]
    outer_x1 = box.w - margin[0]
    inner_x0 = outer_x0 + thickness[0]
    inner_y0 = margin[1] + thickness[1]
    inner_x1 = outer_x1 - thickness[0]
    label_x0 = inner_x0 + 1

    # Variants
    controls = mod.controls
    n_controls = len(controls)
    choice_index = mod.control_index(lambda c: isinstance(c, Choice))
    has_choice = choice_index is not None
    assign_index = mod.control_index(lambda c: isinstance(c, AssignButton))
    has_assign = assign_index is not None
    amount_index = mod.control_index(lambda c: isinstance(c, AmountKnob))
    has_amount = amount_index is not None
    assign_cx0 = assign_cx1 = 0
    assign_cy = 0

    if has_assign:
        n_controls -= 1
        assign_box = find_box(controls[assign_index])
        if assign_index:
            # Assign button is on right.
            outer_x1 = assign_box.x - box.x
            inner_x1 = outer_x1 - thickness[0]
            assign_cx1 = assign_box.x - box.x + assign_box.w / 2 - ASSIGN_RADIUS
            # print('assign_box.x = %.5g' % assign_box.x)
            # print('assign_box.w = %.5g' % assign_box.w)
            # print('ASSIGN_RADIUS = %g' % ASSIGN_RADIUS)
            # print('box.x = %.5g' % box.x)
            # print('assign_box.y = %.5g' % assign_box.y)
            # print('box.y = %.5g' % box.y)
            assign_cy = assign_box.y - box.y + assign_box.h / 2
        else:
            # Assign button is on left.
            outer_x0 = assign_box.x - box.x + assign_box.w
            inner_x0 = outer_x0 + thickness[0]
            label_x0 += assign_box.w
            assign_cx0 = assign_box.x - box.x + assign_box.w / 2 + ASSIGN_RADIUS
            assign_cy = assign_box.y - box.y + assign_box.h / 2

    if has_amount:
        n_controls -= 1
        amount_box = find_box(controls[amount_index])
        if amount_index == has_assign:
            # amount control on left
            inner_x0 = amount_box.x + amount_box.w - box.x
            alx0 = outer_x0 + thickness[0]
            if n_controls:
                alx1 = inner_x0 - thickness[0]
            else:
                alx1 = outer_x1 - thickness[0]
        else:
            # amount control on right
            inner_x1 = amount_box.x - box.x
            alx0 = inner_x1 + thickness[0]
            alx1 = outer_x1 - thickness[0]
    if hasattr(mod, 'destination'):
        left_box = find_box(mod.controls[0])
        assign_cx0 = left_box.x - box.x - left_box.w / 2 + ASSIGN_RADIUS
        assign_cy = left_box.y - box.y + left_box.h / 2

    if has_choice:
        choice_box = find_box(controls[choice_index])
        label_x0 = label_pos[0]
    else:
        inner_y0 = base_y + thickness[1]

    with state(c), engraver(c) as egv:
        c.translate(box.x*mm, box.y*mm)
        outer_dims = dims(x0=outer_x0,
                          y0=margin[1],
                          x1=outer_x1,
                          y1=box.h - margin[1],
                          hr=radius[0],
                          vr=radius[1],
                          by=base_y,
                          tw=label_x0 + label_w + 2 * label_pad[0] - outer_x0,
                          ax0=assign_cx0,
                          ax1=assign_cx1,
                          ay=assign_cy)
        outline(egv, outer_dims, color=FG)

        if n_controls:
            inner_dims = dims(x0=inner_x0,
                              y0=inner_y0,
                              x1=inner_x1,
                              y1=box.h - margin[1] - thickness[1],
                              hr=radius[0] - thickness[0],
                              vr=radius[1] - thickness[1],
                              by=base_y + thickness[1],
                              tw=tab_x0 - margin[0] - thickness[0],
                              ax0=0,
                              ax1=0,
                              ay=0)
            outline(egv, inner_dims, color=BG)
        if has_amount:
            aly0 = amount_box.y - box.y + amount_box.h / 2 + CONTROL_LABEL_Y - 3
            aly0 = base_y + thickness[1] * 1.5
            aly1 = amount_box.y - box.y + amount_box.h / 2 + CONTROL_LABEL_Y + al_height + 3
            alhr = radius[0] - thickness[0]
            alvr = radius[1] - thickness[1]
            alhd = 2 * alhr
            alvd = 2 * alvr
            # egv.moveTo(alx1*mm, (aly1+alvr)*mm)
            egv.moveTo(alx1*mm, (aly1-alvr)*mm)
            egv.arcTo((alx1 - alhd)*mm, aly0*mm,
                      alx1*mm, (aly0 + alvd)*mm,
                      0, -90)
            egv.arcTo(alx0*mm, aly0*mm,
                      (alx0+alhd)*mm, (aly0+alvd)*mm,
                      -90, -90)
            # egv.arcTo(alx0*mm, aly1*mm,
            #           (alx0+alhd)*mm, (aly1+alhd)*mm,
            #           180, 90)
            # egv.arcTo((alx1-alhd)*mm, aly1*mm,
            #           alx1*mm, (aly1+alhd)*mm,
            #           270, 90)
            egv.arcTo(alx0*mm, (aly1-alhd)*mm,
                      (alx0+alhd)*mm, aly1*mm,
                      180, -90)
            egv.arcTo((alx1-alhd)*mm, (aly1-alhd)*mm,
                      alx1*mm, aly1*mm,
                      90, -90)
            # if amount_index == has_assign:
            #     # amount on left
            #     print("amount on left", mod)
            #     egv.moveTo(alx1*mm, aly0*mm)
            #     egv.arcTo(alx0*mm, aly0*mm,
            #               (alx0+alhd)*mm, (aly0+alvd)*mm,
            #               270, -90)
            #     egv.lineTo(alx0*mm, aly1*mm)
            #     egv.arcTo((alx1-alhd)*mm, (aly1-alvd)*mm,
            #               alx1*mm, aly1*mm,
            #               90, -90)
            #     egv.close()
            # else:
            #     # amount on right
            #     print("amount on right", mod)
            #     egv.moveTo(alx0*mm, aly0*mm)
            #     egv.arcTo(alx0*mm, (aly1 - alvd)*mm,
            #               (alx0 + alhd)*mm, aly1*mm,
            #               180, -90)
            #     # c.canvas.rect(alx0*mm, (aly1 - alvd)*mm,
            #     #        (alx0 + alhd - alx0)*mm, (aly1- (aly1 - alvd))*mm)
            #     egv.lineTo(alx1*mm, aly1*mm)
            #     egv.arcTo((alx1 - alhd)*mm, aly0*mm,
            #               alx1*mm, (aly0 + alvd)*mm,
            #               0, -90)
            #     egv.close()

    # module label
    with state(c):
        # c.setFillColorRGB(*BG)
        render_module_label(c,
                            (box.x + label_x0 + label_pad[0])*mm,
                            (box.y +
                             margin[1] + label_pos[1] +
                             label_pad[1])*mm,
                            mod)
        # if mod == filter:
        #     print('Filter')
        #     print('  box origin = (%s, %s)' % (box.x, box.y))
        #     print('  box size   = (%g, %g)' % (box.w, box.h))
        #     print()
        #     print('  Label')
        #     lxo = box.x + label_x0 + label_pad[0]
        #     lyo = box.y + margin[1] + label_pos[1] + label_pad[1]
        #     lxmid = lxo + label_w / 2
        #     lymid = box.y + (margin[1] + base_y + thickness[1]) / 2
        #     print('    origin = (%g, %g)' % (lxo, lyo))
        #     print('    mid    = (%g, %g)' % (lxmid, lymid))
        #     print()

    # # outline module label
    # with state(c):
    #     c.scale(mm, mm)
    #     c.translate(box.x, box.y)
    #     c.setStrokeColorRGB(1, 0, 1)
    #     c.setLineWidth(1/mm)
    #     c.rect(label_x0, label_pos[1], label_w + 2 * label_pad[0], 7)


def render_blank(c, box):
    """What?  It's blank."""


def render_choice(c, box):

    # LEDs 5mm by 2mm, 2.54mm spacing
    # LED carrier 0.4 inch by 1 inch
    # button 7mm at y = -.55inch

    LED_box_pos = (0*mm, -7.0*mm)
    LED_pos = ((LED_box_pos[0] - 2.5)*mm + 0.2*inch, -6.6*mm)
    LED_size = (5*mm, 2*mm)
    LED_spacing = 2.54*mm
    button_diam = 6.25*mm
    button_pos = (0.4*inch - button_diam / 2, -17*mm)
    label_pos = (LED_pos[0] + 9*mm, -4*mm)
    label_width = 10*mm

    widget = box.widget
    n = len(widget.choices)
    # if widget == ctls.controls[0]:
    #     assert str(widget.name) == 'Controller'
    #     cbox = find_box(ctls)
    #     tx = box.x - cbox.x
    #     ty = box.y - cbox.y + box.h / 2
    #     print('Controller')
    #     print('  cbox origin = (%g, %g)' % (cbox.x, cbox.y))
    #     print('  cbox size = (%g, %g)' % (cbox.w, cbox.h))
    #     print()
    #     print('  Choice box')
    #     print('    x, y   = (%g, %g)' % (box.x, box.y))
    #     print('    w, h   = (%g, %g)' % (box.w, box.h))
    #     print('    txlate = (%g, %g)' % (tx, ty))
    #     print()

    #     print('  LED box')
    #     lbo = tuple(c / mm for c in LED_box_pos)
    #     lbc = (lbo[0] + 0.4 * 25.4 / 2, lbo[1] + 3 * LED_spacing / mm)
    #     lbcg = (lbc[0] + tx, lbc[1] + ty)
    #     print('    origin = (%g, %g)' % lbo)
    #     print('    center = (%g, %g)' % lbc)
    #     print('    global:  (%g, %g)' % lbcg)
    #     print()

    #     print('  Button')
    #     bc = tuple(c / mm for c in button_pos)
    #     gbc = (bc[0] + tx, bc[1] + ty)
    #     print('    center = (%g, %g)' % bc)
    #     print('    global:  (%g, %g)' % gbc)
    #     print()

    with state(c):
        c.translate(box.x * mm, (box.y + box.h/2) * mm)

        # LED outline
        with cutter(c) as cut:
            cut.rect(LED_box_pos[0],
                     LED_box_pos[1],
                     0.4*inch,
                     n * LED_spacing + 0.8*mm)
        for g in graphics(c):
            g.setFillGray(0.6)
            g.rect(LED_box_pos[0],
                   LED_box_pos[1],
                   0.4*inch,
                   n * LED_spacing + 0.8*mm,
                   fill=True)
        # if cutting_guide:
        #     fill_outline=0
        # else:
        #     c.setFillGray(0.6)
        #     fill_outline=1
        # c.setLineWidth(cut_width)
        # c.setStrokeColorRGB(*CUT_COLOR)
        # if do_cut:
        #     c.rect(LED_box_pos[0],
        #            LED_box_pos[1],
        #            0.4*inch,
        #            n * LED_spacing + 0.8*mm,
        #            fill=fill_outline)

        # LEDs and labels
        # if not cutting_guide:
        #     c.setFillColorRGB(*(getattr(widget, 'color', (0.3, 0.3, 1))))
        # c.setFont(*CHOICE_FONT)
        label_extra_leading = 1.6*mm
        for i, label in enumerate(widget.choices):
            LED_y = LED_pos[1] + (n - i - 1) * LED_spacing
            label_y = LED_y
            # if True or isinstance(label, TextLabel):
            label_y += ((n-1)/2 - i) * (label_extra_leading)
            # print(str(label), label_y)

            # LED
            # if not cutting_guide:
            #     c.rect(LED_pos[0], LED_y, LED_size[0], LED_size[1], fill=1)
            for g in graphics(c):
                if i == 0:
                    g.setFillColorRGB(*(getattr(widget, 'color', (0.3, 0.3, 1))))
                else:
                    g.setFillColorRGB(0.4, 0.4, 0.4)

                g.rect(LED_pos[0], LED_y, LED_size[0], LED_size[1], fill=1)
                
            # LED label
            # c.setFillColorRGB(*FG)
            render_choice_label(c,
                                (label_pos[0], label_y),
                                label_width,
                                widget, 
                                label)
            # if not cutting_guide:
            #     c.setFillColorRGB(1, 1, 1)
            #     c.setFillColorRGB(0.4, 0.4, 0.4)

        # button
        with cutter(c) as cut:
            cut.circle(button_pos[0], button_pos[1],
                       button_diam / 2)
        for g in graphics(c):
            g.setFillGray(0.2)
            g.circle(button_pos[0], button_pos[1],
                     button_diam / 2,
                     fill=1)

        # if cutting_guide:
        #     fill_button = False
        # else:
        #     fill_button = True
        #     c.setFillGray(0.2)
        # if do_cut:
        #     c.circle(button_pos[0], button_pos[1],
        #              button_diam / 2,
        #              fill=fill_button)


def render_assign_button(c, box):

    # button diameter is 8.5mm
    # lighted ring is 5mm
    base_diam = 8.5*mm
    ring_diam = 5*mm

    with state(c):
        c.translate((box.x + box.w/2) * mm, (box.y + box.h/2) * mm)

        # Hole
        with cutter(c) as cut:
            cut.circle(0, 0, base_diam / 2)

        for g in graphics(c):

            # Base
            g.setFillGray(0.2)
            g.circle(0, 0, base_diam / 2, fill=1)

            # Red Ring
            g.setStrokeColorRGB(1, 0.2, 0.2)
            g.setLineWidth(0.6*mm)
            g.circle(0, 0, ring_diam / 2, stroke=1, fill=0)

        # Label
        render_control_label(c, box)

        # # Base
        # if cutting_guide:
        #     fill_base = 0
        # else:
        #     c.setFillGray(0.2)
        #     fill_base = 1
        # c.setLineWidth(cut_width);
        # c.setStrokeColorRGB(*CUT_COLOR)
        # if do_cut:
        #     c.circle(0, 0, base_diam / 2, fill=fill_base)

        # # Red Ring
        # if not cutting_guide:
        #     c.setStrokeColorRGB(1, 0.2, 0.2)
        #     c.setLineWidth(0.6*mm)
        #     c.circle(0, 0, ring_diam / 2)

        # # Label
        # c.setStrokeColorRGB(*FG)
        # c.setFillColorRGB(*FG)
        # render_control_label(c, box)


def render_amount_knob(c, box):
    render_knob(c, box, engrave_alignments=False)

def render_knob(c, box, engrave_alignments=True):

    # knob base is 13.2mm diameter
    # knob cap is 10mm diameter
    # indicator is 3.8mm by 1mm
    knob_pos = (0, 0)
    base_diam = 13.2*mm
    cap_diam = 10*mm
    indicator_size = (1*mm, 3.8*mm)
    indicator_pos = (-indicator_size[0]/2, 1.7*mm)
    label_pos = (box.x, box.y)

    with state(c):
        c.translate((box.x + box.w/2) * mm, (box.y + box.h/2) * mm)

        # alignment pinholes and key
        if engrave_alignments:
            with engraver(c) as egv:
                egv.rect(-3.5*mm, -5.5*mm, 7*mm, 11*mm)
                if not hasattr(box.widget, 'highlight'):
                    egv.circle(-4.8*mm, -5.4*mm, r=0.5*mm)
                    egv.circle(+4.8*mm, +5.4*mm, r=0.5*mm)

        # hole
        with cutter(c) as cut:
            cut.circle(0, 0, 5.5*mm)

        for g in graphics(c):
            # Base
            g.setFillGray(1)
            g.circle(0, 0, base_diam / 2, fill=1)

            # Cap
            g.setLineWidth(0.1)
            g.setFillGray(0.8)
            g.circle(0, 0, cap_diam / 2, stroke=1, fill=1)

            # Indicator
            g.setFillGray(1.0)
            g.setLineWidth(0.1)
            g.rotate(150)
            g.rect(indicator_pos[0], indicator_pos[1],
                   indicator_size[0], indicator_size[1],
                   fill=1)

        # if cutting_guide:
        #     c.setLineWidth(cut_width)
        #     c.setStrokeColorRGB(*CUT_COLOR)
        #     if do_cut:
        #         c.circle(0, 0, 5.5*mm)
        #         c.rect(-3.5*mm, -5.5*mm, +7*mm, +11*mm)

        #     # Alignment pinholes
        #     if do_engrave:
        #         c.setFillColorRGB(*FG)
        #         c.circle(-4.8*mm, -5.4*mm, r=0.5*mm, stroke=0, fill=1)
        #         c.circle(+4.8*mm, +5.4*mm, r=0.5*mm, stroke=0, fill=1)
        # else:
        #     # Base
        #     c.setFillGray(1)
        #     c.circle(0, 0, base_diam / 2, fill=1)

        #     # Cap
        #     c.setLineWidth(0.1)
        #     c.setFillGray(0.8)
        #     c.circle(0, 0, cap_diam / 2, stroke=1, fill=1)

        #     # Indicator
        #     c.setFillGray(1.0)
        #     c.setLineWidth(0.1)
        #     with state(c):
        #         c.rotate(150)
        #         c.rect(indicator_pos[0], indicator_pos[1],
        #                indicator_size[0], indicator_size[1],
        #                fill=1)

        # Highlight
        if getattr(box.widget, 'highlight', None) == 270:
            # arc from -60 to +240 degrees
            # inner radius is 6.5mm.
            # outer radius is 8.5mm.
            a0 = -60
            a1 = +240
            r0 = 6.5 * mm
            r1 = 8.5 * mm
            with engraver(c) as egv:
                egv.moveTo(cos(a0 * pi / 180) * r0, sin(a0 * pi / 180) * r0)
                egv.arcTo(-r1, -r1, +r1, +r1, a0, a1 - a0)
                egv.arcTo(-r0, -r0, +r0, +r0, a1, a0 - a1)

        # if getattr(box.widget, 'highlight', None) == 270:
        #     c.setStrokeColorRGB(*FG)
        #     c.setLineWidth(3*mm)
        #     c.arc(-8*mm, -8*mm, +8*mm, +8*mm, -60, 300)

        # Label
        # c.setStrokeColorRGB(*label_color)
        # c.setFillColorRGB(*label_color)
        render_control_label(c, box)


def saw_path(curve, bbox):
    xmin = 0.5
    xmax = 2.5

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    # curve = c.beginPath()
    x = xmin
    curve.moveTo(sx(x), sy(x % 1 * 2 - 1))
    while x < xmax:
        if x == int(x):
            curve.lineTo(sx(x), sy(+1))
            curve.lineTo(sx(x), sy(-1))
            x = x + 1
        else:
            x = ceil(x)
        if x > xmax:
            x = xmax
            curve.lineTo(sx(x), sy(x % 1 * 2 - 1))
    return curve


def saw_down_path(curve, bbox):
    xmin = 0.5
    xmax = 2.5

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    # curve = c.beginPath()
    x = xmin
    curve.moveTo(sx(x), sy(-(x % 1 * 2 - 1)))
    while x < xmax:
        if x == int(x):
            curve.lineTo(sx(x), sy(-1))
            curve.lineTo(sx(x), sy(+1))
            x = x + 1
        else:
            x = ceil(x)
        if x > xmax:
            x = xmax
            curve.lineTo(sx(x), sy(-(x % 1 * 2 - 1)))
    return curve


def square_path(curve, bbox):
    xmin = -0.25
    xmax = 1.75

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    # curve = c.beginPath()
    x = xmin
    curve.moveTo(sx(x), sy(+1 if 2 * x % 2 < 1 else -1))
    while x < xmax:
        if 2 * x % 2 == 0:
            curve.lineTo(sx(x), sy(-1))
            curve.lineTo(sx(x), sy(+1))
            x += 1/2
        elif 2 * x % 2 == 1:
            curve.lineTo(sx(x), sy(+1))
            curve.lineTo(sx(x), sy(-1))
            x += 1/2
        else:
            x = ceil(2 * x) / 2
        if x > xmax:
            x = xmax
            curve.lineTo(sx(x), sy(+1 if 2 * x % 2 < 1 else -1))
            
    return curve


def tri_path(curve, bbox):
    xmin = 0.25
    xmax = 2.25

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    # curve = c.beginPath()
    x = xmin
    curve.moveTo(sx(x), sy((2*x % 1 if 2 * x % 2 < 1 else 1 - 2*x % 1) * 2 - 1))
    while x < xmax:
        if 2 * x % 2 == 0:
            curve.lineTo(sx(x), sy(-1))
            x += 1/2
        elif 2 * x % 2 == 1:
            curve.lineTo(sx(x), sy(+1))
            x += 1/2
        else:
            x = ceil(2 * x) / 2
        if x > xmax:
            x = xmax
            xx = 2 * x
            curve.lineTo(sx(x),
                         sy((xx % 1 if xx % 2 < 1 else 1 - xx % 1) * 2 - 1))
    return curve


def sin_path(curve, bbox):
    xmin = 0
    xmax = 2

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    basis = ((0, 0), (0.5251 / pi / 2, 0.5251), (1.005 / pi / 2, 1), (1 / 4, 1))
    segments = range(floor(xmin * 4), ceil(xmax * 4))

    # curve = c.beginPath()
    curve.moveTo(sx(xmin), sy(sin(xmin*2*pi)))
    for s in segments:
        q = s % 4
        x0 = s / 4
        if q == 0:
            p1, p2, p3 = basis[1:]
        elif q == 1:
            p1 = (1/4 - basis[2][0], basis[2][1])
            p2 = (1/4 - basis[1][0], basis[1][1])
            p3 = (1/4 - basis[0][0], basis[0][1])
        elif q == 2:
            p1 = (basis[1][0], -basis[1][1])
            p2 = (basis[2][0], -basis[2][1])
            p3 = (basis[3][0], -basis[3][1])
            pass
        else:
            assert q == 3
            p1 = (1/4 - basis[2][0], -basis[2][1])
            p2 = (1/4 - basis[1][0], -basis[1][1])
            p3 = (1/4 - basis[0][0], -basis[0][1])
        curve.curveTo(sx(x0 + p1[0]), sy(p1[1]),
                      sx(x0 + p2[0]), sy(p2[1]),
                      sx(x0 + p3[0]), sy(p3[1]))
    return curve


def rand_noise(x, b):
    def bigrand(n): return bigrand(n - 1) * 48271 % 0x7fffffff if n else b
    def random(n): return bigrand(n) % 23 / 23
    a = random(int(x))
    b = random(int(x) + 1)
    return (a + x%1 * (b - a)) * 2 - 1


def rand_path(curve, bbox):
    xmin = 0
    xmax = 6

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    # curve = c.beginPath()
    curve.moveTo(sx(xmin), sy(rand_noise(0, 9)))
    for x in range(xmin + 1, xmax + 1):
        curve.lineTo(sx(x), sy(rand_noise(x, 9)))
    return curve


def snh_path(curve, bbox):
    xmin = 0
    xmax = 6

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    # curve = c.beginPath()
    curve.moveTo(sx(xmin), sy(rand_noise(0, 5)))
    for x in range(xmin, xmax):
        curve.lineTo(sx(x), sy(rand_noise(x, 5)))
        curve.lineTo(sx(x + 1), sy(rand_noise(x, 5)))
    return curve

path_funcs = {
    Waveform.saw: saw_path,
    Waveform.square: square_path,
    Waveform.triangle: tri_path,
    Waveform.sine: sin_path,
    Waveform.saw_down: saw_down_path,
    Waveform.random: rand_path,
    Waveform.sample_hold: snh_path,
}


def render_module_label(c, x, y, module):
    # with state(c):
    #     c.setFont(*MODULE_FONT)
    #     c.drawString(x, y, str(module.name))
    with inverse_text_engraver(c, x, y, fill=1) as t:
        t.setFont(*MODULE_FONT)
        t.textOut(str(module.name))

def render_control_label(c, box):
    label_pos = (0, CONTROL_LABEL_Y*mm)
    control = box.widget
    txt = str(control.name)
    with state(c), text_engraver(c, *label_pos) as t:
        t.setFont(*CONTROL_FONT)
        if control == filter.controls[-1]:
            # Nudge Key Track to the left.
            c.translate(-2*mm, 0)
        t.setCharSpace(CONTROL_CHAR_SPACE)
        t.textOut(txt)
        lw = t.getX()
        lw += (len(txt) - 1) * CONTROL_CHAR_SPACE
        c.translate(-lw / 2, 0)
    # with state(c):
    #     c.setFont(*CONTROL_FONT)
    #     c.setLineWidth(cut_width)
    #     if isinstance(box.widget, AmountKnob):
    #         do_render = do_engrave
    #         mode = 0
    #     else:
    #         do_render = do_text
    #         mode = 1
    #     if do_render:
    #         if control == filter.controls[-1]:
    #             c.translate(-2*mm, 0)
    #         with text(c, label_pos[0], label_pos[1]) as t:
    #             txt = str(control.name)
    #             t.setTextRenderMode(mode)
    #             t.setCharSpace(CONTROL_CHAR_SPACE)
    #             t.textOut(txt)
    #             lw = t.getX()
    #             lw += (len(txt) - 1) * CONTROL_CHAR_SPACE
    #             c.translate(-lw / 2, 0)


def render_choice_label(c, pos, w, widget, label):
    with state(c):
        if isinstance(label, TextLabel):
            with text_engraver(c, pos[0], pos[1]) as t:
                t.setFont(*CHOICE_FONT)
                t.setCharSpace(CHOICE_CHAR_SPACE)
                t.textOut(str(label))
        elif isinstance(label, WaveformLabel):
            bbox = pos + (21.5*mm, pos[1] + 0.1*inch)            
            with vector_engraver(c) as egv:
                path_funcs[label.waveform](egv, bbox)
        else:
            assert False, 'Unknown label type %r' % type(label)

    # with state(c):
    #     c.setFillGray(1)
    #     c.setStrokeColorRGB(*FG)
    #     c.setLineWidth(cut_width)
    #     if isinstance(label, TextLabel):
    #         if do_text:
    #             c.setFont(*CHOICE_FONT)
    #             c.drawString(pos[0], pos[1], str(label), 1)
    #     elif isinstance(label, WaveformLabel):
    #         bbox = pos + (21.5*mm, pos[1] + 0.1*inch)            
    #         try:
    #             curve = path_funcs[label.waveform](c, bbox)
    #             c.setStrokeColorRGB(*FG)
    #             c.drawPath(curve)
    #         except KeyError:
    #             pass
    #         pass
    


boxes = layout_panel(panel)
print('grid width %g' % g_grid_width)
# render(boxes[:47] + boxes[48:55] + boxes[56:])
render(boxes)
