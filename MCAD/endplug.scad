eps = 0.1;
X = [1, 0, 0];
Y = [0, 1, 0];
Z = [0, 0, 1];

H = 4;

module centered_cube(dims, z=0) {
    translate([-dims[0]/2, -dims[1]/2, z])
        cube(dims);
}

module endplug() {
    difference() {
        centered_cube([130, 20, H]);
        for (x = [-33, +33])
            translate([x, 0, H - 3.4])
                cylinder(d=4.9, h=3.4 + eps, $fs = 0.1);
    }
}

endplug();
