eps = 0.1;
X = [1, 0, 0];
Y = [0, 1, 0];
Z = [0, 0, 1];

INTERIOR_HEIGHT = 21;
AST = 0.5;                      // aluminum sheet thickness
TILT = 40;

GRID_W = 21.5;
GRID_H = 44.5 - 1/32 * 25.4;

FP_W = 438;
FP_H = 132.706;
FP_T = 2;

END_T = 15;                     // end cap thickness
END_OH = 2;                     // end cap overhang

touchscreen_w = 121;
touchscreen_h = 76;

modules = [[0, 0, 4, "red"],
           [0, 1, 4, "blue"],
           [0, 2, 4, "blue"],
           [4, 0, 4, "orange"],
           [4, 1, 4, "orange"],
           [4, 2, 2, "orange"],
           [6, 2, 2, "green"],
           [8, 2, 6, "red"],
           [14, 0, 6, "lightblue"], 
           [14, 1, 6, "lightblue"], 
           [16, 2, 4, "lightblue"], 
           [14.5, 2, 1.5, "white"]];

choice_positions = [[0, 0, 6], [0, 1, 6], [0, 2, 6],   // column 1
                    [4, 0, 4], [4, 1, 4], [4, 2, 3],   // column 2
                    [6, 2, 3],                         // mixer
                    [8, 2, 3]];                        // filter

knob_positions = [[1, 0], [2, 0],                      // LFO 1
                  [1, 1], [2, 1],                      // LFO 2
                  [2, 2],                              // Ctlrs
                  [5, 0], [6, 0], [7, 0],              // Osc 1
                  [5, 1], [6, 1], [7, 1],              // Osc 2
                  [5, 2],                              // Noise
                  [9, 2], [10.5, 2], [12, 2], [13, 2], // Filter
                  [14.5, 2],                           // Master Volume
                  [15, 0], [16, 0], [17, 0], [18, 0], [19, 0], // Env 1
                  [15, 1], [16, 1], [17, 1], [18, 1], [19, 1], // Env 2
                  [16, 2], [17, 2], [18, 2], [19, 2]]; // Env 3

assign_positions = [[3, 0], [3, 1], [3, 2], [14, 0]];

module centered_cube(dims, z=0) {
    translate([-dims[0]/2, -dims[1]/2, z])
        cube(dims);
}

module front_panel(z=0) {
    color("black")
        translate([-FP_W / 2, -FP_H / 2, z])
            difference() {
                cube([FP_W, FP_H, FP_T]);
                for (c = choice_positions) {
                    translate([(0.5 + c[0]) * GRID_W - 4,
                               (2.5 - c[1]) * GRID_H - 5,
                               -eps]) {
                        cube([25.4 * 0.4, 2.54 * c[2], FP_T + 2 * eps]);
                        translate([7, -10, 0])
                            cylinder(d=6, h=FP_T + 2 * eps);
                    }
                }
                for (k = knob_positions)
                    translate([(0.5 + k[0]) * GRID_W,
                               (2.5 - k[1]) * GRID_H,
                               -eps])
                        cylinder(r=5.5, h=FP_T + 2 * eps);
                for (a = assign_positions)
                    translate([(0.5 + a[0]) * GRID_W,
                               (2.5 - a[1]) * GRID_H,
                               -eps])
                        cylinder(d=8.5, h=FP_T + 2 * eps);
                translate([8 * GRID_W, GRID_H, -eps])
                    cube([touchscreen_w, touchscreen_h, FP_T + 2 * eps]);
            }
}

module electronics(z) {
    deck_z = 10;
    pcb_t = 2;
    top_z = INTERIOR_HEIGHT;
    comp_z = deck_z + pcb_t;
    comp_t = top_z - comp_z;
    translate([-FP_W / 2, -FP_H / 2, z]) {
        for (m = modules) {
            translate([m[0] * GRID_W + 3, (2 - m[1]) * GRID_H + 1.5, 0])
                color(m[3])
                    cube([m[2] * GRID_W - 6, GRID_H - 3, comp_z]);
            translate([m[0] * GRID_W + 3, (2 - m[1]) * GRID_H + 1.5, comp_z])
            color("purple")
                cube([m[2] * GRID_W - 6, GRID_H - 3, pcb_t]);
        }
        for (c = choice_positions) {
            translate([(0.5 + c[0]) * GRID_W - 4,
                       (2.5 - c[1]) * GRID_H - 5,
                       comp_z]) {
                color("white")
                    cube([25.4 * 0.4, 2.54 * c[2], comp_t + FP_T]);
                translate([7, -10, 0]) {
                    color("silver")
                        centered_cube([12, 12, comp_t]);
                    color("black")
                        cylinder(d=5, h=comp_t + 3);
                }
            }
        }
        for (k = knob_positions) {
            translate([(0.5 + k[0]) * GRID_W,
                       (2.5 - k[1]) * GRID_H,
                       comp_z]) {
                color("green")
                    centered_cube([16, 16, comp_t]);
                color("silver")
                    cylinder(d=10, h=comp_t + 10);
                color("white")
                    cylinder(d=5, h=comp_t + 20);
            }
        }
        for (a = assign_positions)
            translate([(0.5 + a[0]) * GRID_W,
                       (2.5 - a[1]) * GRID_H,
                       comp_z])
                color("black") {
                    cylinder(d=8, h=comp_t + 3);
                    translate((comp_t + 3) * Z)
                        scale([1, 1, 0.3])
                            sphere(d=8);
                }
    }
}

module inner_cover(z=0) {
    w = FP_W;
    h = FP_H;
    t = INTERIOR_HEIGHT + AST;
    n_l = 15;                   // nut: length, width, thickness
    n_w = 10;
    n_t = 3;
    color("silver")
        translate([0, 0, z])
            difference() {
                union() {
                    difference() {
                        centered_cube([w, h, t]);
                        centered_cube([w - 2 * AST, h + 2 * eps, t - 2 * AST],
                                      z=AST);
                        centered_cube([w - 6, h + 2 * eps, t], z=AST);
                    }
                    for (x = [-1, +1], y = [-1, +1])
                        scale([x, y, 1])
                                translate([-w/2 + AST, h/4, t / 2])
                                    rotate(90, Y)
                                        centered_cube([n_w, n_l, n_t]);
                }
                for (y = [-h/4, +h/4])
                    translate([0, y, t / 2])
                        rotate(90, Y)
                            cylinder(d=5, h=w + 2 * eps, center=true);
            }
}

module outer_cover(x=0, z=0) {
    w = FP_W;
    h = FP_H + 2 * AST;
    t = INTERIOR_HEIGHT + 3 * AST + FP_T;
    color("lightsalmon")
        translate([x, 0, z])
            difference() {
                centered_cube([w, h, t]);
                translate([-w/2 - eps, -h/2 + AST, AST])
                    cube([w + 2 * eps, 2 + eps, t - 2 * AST]);
                translate([-w/2 - eps, h/2 - AST - 2 - eps, AST])
                    cube([w + 2 * eps, 2 + eps, t - 2 * AST]);
//                centered_cube([w + 2 * eps, h - 2 * AST, t - 2 * AST], z=AST);
                centered_cube([w + 2 * eps, h - 4, t], z=AST);
            }
}

module left_end(x=0) {
    lx = x - FP_W / 2 - END_T;
    l = FP_H + 2 * END_OH;
    t = INTERIOR_HEIGHT + FP_T + 3 * AST + END_OH;
    color("indigo")
        translate(lx * X)
            difference() {
                hull() {
                    tilt(TILT)
                        translate([0, -l/2, 0])
                            cube([END_T, l, t]);
                    linear_extrude(height=eps)
                            projection()
                                tilt(TILT)
                                    translate([0, -l/2, 0])
                                        cube([END_T, l, t]);
                }
                tilt(TILT)
                    for (y = [-FP_H / 4, +FP_H / 4])
                        translate([-eps, y, (INTERIOR_HEIGHT + 2 * AST) / 2])
                            rotate(90, Y) {
                                cylinder(d=5, h=100);
                                cylinder(d=15, h=8);
                            }
            }
}

module left_screws(x=0) {
    lx = x - FP_W / 2 - END_T + 1;
    l = FP_H + 2 * END_OH;
    t = INTERIOR_HEIGHT + FP_T + 3 * AST + END_OH;
    color("black")
        translate(lx * X)
            tilt(TILT)
                for (y = [-FP_H/4, +FP_H/4])
                    translate([0, y, (INTERIOR_HEIGHT + 2 * AST) / 2])
                        rotate(90, Y) {
                            cylinder(d=5, h=17);
                            cylinder(d=8.5, h=5);
                        }
}

module right_end(x=0) {
    scale([-1, +1, +1])
        left_end(-x);
}

module right_screws(x=0) {
    scale([-1, +1, +1])
        left_screws(-x);
}

function ramp(t0, t1, x0, x1) =
    ($t < t0)
        ? x0
        : ($t > t1)
              ? x1
              : x0 + ($t - t0) / (t1 - t0) * (x1 - x0);

module tilt(a) {
    translate(-FP_H / 2 * Y)
        rotate(a, X)
            translate(FP_H / 2 * Y)
                children();
}

module animate() {

    // slide electronics down
    // slide front panel down
    // slide outer cover left
    // rotate assembly
    // slide endpieces in

    n   = 7;
    ez  = ramp(0/n, 1/n, 200, AST);
    fpz = ramp(1/n, 2/n, 400, INTERIOR_HEIGHT + 2 * AST);
    ocx = ramp(2/n, 3/n, 600, 0);
    tlt = ramp(3/n, 4/n, 0, TILT);
    lex = ramp(4/n, 5/n, -200, 0);
    rex = ramp(4/n, 5/n, +750, 0);
    lsx = ramp(5/n, 5.3/n, -100, 0);
    rsx = ramp(5/n, 5.3/n, +100, 0);

    tilt(tlt) {
        inner_cover(z=AST);
        electronics(z=ez);
        front_panel(z=fpz);
        outer_cover(z=0, x=ocx);
    }
    left_end(x=lex);
    right_end(x=rex);
    left_screws(lsx + lex);
    right_screws(rsx + rex);

}

animate();

// inner_cover(z=0);
// electronics(z=AST);
// front_panel(z=INTERIOR_HEIGHT + 2 * AST + 3);
// outer_cover(x=0);
// left_end();
// right_end();
// left_screws();
// right_screws();
