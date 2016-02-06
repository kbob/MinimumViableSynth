eps = 0.1;
X = [1, 0, 0];
Y = [0, 1, 0];
Z = [0, 0, 1];

// left endplug
L = 133;
H = 20 - 3;
T = 4 - 1.8;
BZ = 12.5;

// right endplug
L = 133;
T = 20;
H = 20 - 3;
BZ = 11;

module centered_cube(dims, z=0) {
    translate([-dims[0]/2, -dims[1]/2, z])
        cube(dims);
}

module endplug() {
    difference() {
        centered_cube([L, 20-3, T]);
        for (x = [-33, +33])
            translate([x, BZ - H/2, T - 1.4])
                cylinder(d=4.9, h=3.4 + eps, $fs = 0.1);
    }
}

module washer() {
    $fs = 0.2;
    $fa = 6;
    difference() {
        cylinder(d=10, h=0.6);
        cylinder(d=5.4, h=5, center=true);
    }
}

endplug();

// for (x = [-10, +10])
//     translate([x, -20, 0])
//         washer();
