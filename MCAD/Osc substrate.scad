hq = true;
clr = 0.2;

$fs = hq ? 1 : 4;
$fa = hq ? 1 : 4;
eps = 0.1;

// Constants from the Eagle file

// Board dimensions
board_x = 0;
board_y = 0;
board_w = 86;
board_h = 43.5;
board_r = 2;
board_ne_r = 3.3;
board_min_y = 1;                // screwup: make board 1mm shorter.

// Teensy location
teensy_cx =  9.08;
teensy_cy = 19.05;
teensy_w =  7 * 2.54;
teensy_h = 14 * 2.54;
teensy_paddle_w = 20.75;

// Tactile switches
sw1_x = 32.25;
sw1_y = 7;
sw2_x = 53.75;
sw2_y = 7;
sw3_x = 75.25;
sw3_y = 7;
swlast_x = sw3_x;
sw_d = 2.5;

// Screw holes
hole1_x = 21.25;
hole1_y = 36.75;

hole2_x = 43;
hole2_y = 36.75;

hole3_x = 64.5;
hole3_y = 36.75;

hole4_x = 82.75;
hole4_y = 40.25;

hole_d = 3;
hole_clr = 0;

// Paddles
paddle_y = 3.5;

// Spine
spine_y = 38.25;
spine_cx2 = (37.25 + 48.5) / 2;
spine_cx3 = (58.75 + 66.72) / 2;

// Header
header_cx = 7.79;
header_cy = 39.73;
header_w = 3 * 2.54;
header_h = 2 * 2.54;

// End Eagle constants

teensy_rx = teensy_cx + teensy_w / 2 + clr; // Teensy right side x coord.

// Z heights
base_z = 1;
deck_z = base_z + 9;
sw_z = deck_z - 1.6;
teensy_z = deck_z - 4.1;
bulkhead_z = deck_z - 3;

module centered_cube(size, zt=0) {
    translate([-size[0] / 2, -size[1] / 2, zt])
        cube(size);
}

module round_rect(size, r) {
    hull()
        for (x = [r, size[0] - r]) for ( y = [r, size[1] - r]) {
            translate([x, y, 0])
                cylinder(r=r, h=size[2]);
        }
}


module base() {
    translate([0, board_min_y, 0])
        round_rect([board_w, board_h - board_min_y, base_z], board_r);
}

module spine() {
    lx = teensy_rx + clr;
    difference() {
        translate([lx + eps, spine_y, eps])
            cube([board_w - lx - board_ne_r - eps,
                  board_h - spine_y,
                  deck_z - eps]);
        // strain relief
        translate([board_w - 16, spine_y - eps, base_z])
            cube([2, board_h, deck_z - base_z - 1]);
    }
}

module bulkheads() {
    bt0 = 3;
    bt1 = 1.5;
    bx = teensy_rx + clr;
    by = sw1_y;
    bw = swlast_x - bx;
    bh = board_h - by;
    translate([bx + eps, by, 0])
        hull() {
            translate([0, -bt0/2, base_z - eps])
                cube([bw + bt0/2, bt0, eps]);
            translate([0, -bt1/2, bulkhead_z - eps])
                cube([bw + bt1/2, bt1, eps]);
        }
    translate([bx + bw, board_h, 0])
        rotate(-90)
            hull() {
                translate([eps, -bt0/2, base_z - eps])
                    cube([bh + bt0/2 - eps, bt0, eps]);
                translate([eps, -bt1/2, bulkhead_z - eps])
                    cube([bh + bt1/2 - eps, bt1, eps]);
            }
}

module screw_support(x, y, z) {
    support_d = hole_d + 2.5;
    hull() {
        translate([x, y, base_z - eps])
            centered_cube([support_d, support_d, eps]);
        translate([x, y, z - eps])
            cylinder(d=support_d, h = eps);
    }
}

module end_screw_support(x, y ,z) {
    support_r = board_ne_r;
    translate([x, y, base_z - eps])
        cylinder(r = support_r, h = z - base_z + eps);
}

module screw_supports() {
    screw_support(hole1_x, hole1_y, deck_z);
    screw_support(hole2_x, hole2_y, deck_z);
    screw_support(hole3_x, hole3_y, deck_z);
    end_screw_support(hole4_x, hole4_y, deck_z);
}

module screw_hole(x, y, z) {
    translate([x, y, z])
        cylinder(d=hole_d + hole_clr, h=deck_z + 3);
}

module screw_holes() {
    screw_hole(hole1_x, hole1_y, base_z);
    screw_hole(hole2_x, hole2_y, base_z);
    screw_hole(hole3_x, hole3_y, base_z);
    screw_hole(hole4_x, hole4_y, base_z);
}

module switch_pillars() {
    module switch_pillar(x, y) {
        mbh = 2 * (y - paddle_y);
        mbw = mbh;
        translate([x, y, 0])
            hull() {
                centered_cube([mbw, mbh, eps], zt=base_z - eps);
                translate([0, 0, sw_z - eps])
                    cylinder(d=sw_d + 1, h=eps);
            }
    }
    switch_pillar(sw1_x, sw1_y);
    switch_pillar(sw2_x, sw2_y);
    switch_pillar(sw3_x, sw3_y);
}

module under_teensy() {
    uw = teensy_paddle_w - teensy_rx;
    translate([teensy_rx + clr, board_min_y, 0])
        hull() {
            cube([2 * uw, board_h - board_min_y, eps]);
            translate([0, 0, deck_z - eps])
                cube([uw, board_h - board_min_y, eps]);
        }
    translate([2.5, board_min_y, base_z - eps])
        cube([2, 4.5 * 2.54 - board_min_y, teensy_z - base_z - eps]);
}

module header_cutout() {
    hdr_clr = 2;
    translate([-eps, header_cy - header_h / 2 - hdr_clr, -eps])
        round_rect([header_cx + header_w / 2 + hdr_clr,
                    board_h,
                    deck_z + 2 * eps],
                   r=0.5);
}

module substrate() {
    difference() {
        union() {
            base();
            spine();
            bulkheads();
            switch_pillars();
            under_teensy();
            screw_supports();
        };
        screw_holes();
        header_cutout();
    };
}

substrate();
