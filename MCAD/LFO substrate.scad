inch = 25.4;
hq = true;
clr = 0.2;

$fs = hq ? 1 : 4;
$fa = hq ? 1 : 4;
eps = 0.1;

// Constants from the Eagle file

// Board dimensions
board_x = 0;
board_y = 0;
board_w = 81.75;
board_h = 43.5;
board_r = 2;

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
sw_d = 2.5;

// Screw holes
hole1_x = 75.25;
hole1_y = 10;
hole2_x = 76.5;
hole2_y = 38;
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

spine_screw_y = spine_y - hole_d / 2;
teensy_rx = teensy_cx + teensy_w / 2;
spine_cx1 = teensy_paddle_w + hole_d / 2 + 1;

// Z heights
base_z = 1;
deck_z = base_z + 9;
top_z = deck_z + 1.6;
sw_z = deck_z - 1.6;
teensy_z = deck_z - 4.1;
pin_z = deck_z - 9;
bulkhead_z = deck_z - 3;

// The booboo
booboo_x0 = 19.5 - 2;
booboo_x1 = 31.75 + 2;
booboo_y = 39.5;
booboo_h = 2 * (booboo_y - spine_y);
booboo_t = 1.0;

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
    round_rect([board_w, board_h, base_z], board_r);
}

module spine() {
    lx = teensy_rx;
    difference() {
        translate([lx, spine_y, eps])
            round_rect([board_w - lx, board_h - spine_y, top_z - eps], r=board_r);
        // strain relief
        translate([board_w - 12, spine_y - eps, base_z])
            cube([2, board_h, deck_z - base_z - 1]);
    }
}

module bulkheads() {
    bt0 = 3;
    bt1 = 1.5;
    bx = teensy_rx + clr;
    by = sw1_y;
    bw = hole1_x - bx;
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

module spine_cutout() {
    translate([10 - eps, spine_y, deck_z])
         cube([board_w, board_h, top_z]);
     // translate([0, 0, deck_z])
     //     cube([teensy_paddle_w, board_h, top_z]);
}

module screw_support(x, y, z) {
    support_d = hole_d + 2 * 3;
    hull() {
        translate([x, y, base_z - eps])
            centered_cube([support_d, support_d, eps]);
        translate([x, y, z - eps])
            cylinder(d=support_d, h = eps);
    }
}

module assign_screw_supports() {
    screw_support(hole1_x, hole1_y, deck_z);
    screw_support(hole2_x, hole2_y, deck_z);
}

module spine_screw_supports() {
    y = spine_screw_y;
    intersection() {
        screw_support(spine_cx1, spine_screw_y, top_z);
        union() {
            cube([board_w, board_h, deck_z - eps]);
            translate([teensy_paddle_w, 0, eps])
                round_rect([board_w, spine_y, top_z + eps], r=2);
        }
    }
    screw_support(spine_cx2, spine_screw_y, top_z);
    screw_support(spine_cx3, spine_screw_y, top_z);
}

module screw_hole(x, y, z) {
    translate([x, y, z])
        cylinder(d=hole_d + hole_clr, h=deck_z + 3);
}

module assign_screw_holes() {
    screw_hole(hole1_x, hole1_y, base_z);
    screw_hole(hole2_x, hole2_y, base_z);
}

module spine_screw_holes() {
    for (x = [spine_cx1, spine_cx2, spine_cx3])
        screw_hole(x, spine_screw_y, base_z);
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
}

module under_teensy() {
    uw = teensy_paddle_w - teensy_rx;
    translate([teensy_rx + clr, 0, 0])
        hull() {
            cube([2 * uw, board_h, eps]);
            translate([0, 0, deck_z - eps])
                cube([uw, board_h, eps]);
        }
    translate([2, 0, eps])
        cube([2, 4.5 * 2.54, teensy_z - eps]);
}

module header_cutout() {
    hdr_clr = 2;
    translate([-eps, header_cy - header_h / 2 - hdr_clr, -eps])
        round_rect([header_cx + header_w / 2 + hdr_clr, board_h, deck_z + 2 * eps],
                   r=0.5);
}

module booboo_cutout() {
    translate([booboo_x0, booboo_y - booboo_h / 2, deck_z - booboo_t])
        cube([booboo_x1 - booboo_x0, booboo_h, booboo_t + eps]);
}

module substrate() {
    difference() {
        union() {
            base();
            spine();
            bulkheads();
            switch_pillars();
            under_teensy();
            assign_screw_supports();
            spine_screw_supports();
        };
        assign_screw_holes();
        spine_screw_holes();
        spine_cutout();
        header_cutout();
        booboo_cutout();
    };
}

module clips() {
    clip_t = 7;
    od = hole_d + 5;
    x0 = spine_cx1 - od/2;
    x1 = spine_cx3 + od/2;
    y = board_h - spine_screw_y;
    y1 = spine_y - spine_screw_y;
    y2 = board_h - spine_y;

    difference() {
        union() {
            for (x = [spine_cx1, spine_cx2, spine_cx3])
                translate([x, 0, 0])
                    hull() {
                        cylinder(d=od, h=clip_t);
                        translate([-od/2, y - eps, 0])
                            cube([od, eps, clip_t]);
                    }
            translate([x0, y1, 0])
                cube([x1 - x0, y2, clip_t]);
        }
        for (x = [spine_cx1, spine_cx2, spine_cx3])
            translate([x, 0, 0]) {
                cylinder(d=hole_d + hole_clr + 2 * clr, h=3 * clip_t, center=true);
                translate([0, 0, clip_t - 2])
                    cylinder(d=6.5, h=3);
            }
    }
}

//module clips() {
//    for (x = [20, 40, 60])
//        translate([x, -10, 0])
//            clip();
//}

translate([0, -10, 0])
    clips();
substrate();
