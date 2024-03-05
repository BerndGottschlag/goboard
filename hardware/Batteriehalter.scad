$fn = 50;

width = 123.4;
height = 16.3 - 5.0;
depth = 14.6;
depth_to_pcb = 16.1;
height_to_pcb = 4.5;

aa_dia = 11;
aa_length = 44.5;
contact_length = 50.0 - aa_length;

module battery_with_insertion_path() {
    union() {
        rotate([0, 90, 0])
            cylinder(h=aa_length + contact_length, d = aa_dia);
        rotate([-45, 0, 0])
            translate([0, -aa_dia / 2, 0])
            cube([aa_length + contact_length, aa_dia, 100]);
    }
}

module contact() {
    /*rotate([180, 0, 0])
        union() {
            translate([-0.5, -4.7, -9.3/2])
                cube([0.8, 10.5, 9.3]);
            translate([-0.5, -4.7, -3.0/2])
                cube([0.6, 17, 3.0]);
        }*/
    translate([-0.5, -5.7, -9.7/2])
        cube([0.6, 30, 9.7]);
    translate([-0.5, -1, -9.7/2])
        cube([1.5, 25, 3]);
    rotate([90, 0, 0])
        cylinder(h=50, d=4, center=true);
}

difference() {
    // Start with a simple block as well as an extensinn towards the PCB.
    union() {
        cube([width, depth, height]);
        translate([0, depth - depth_to_pcb, 0])
            cube([width, depth_to_pcb, height_to_pcb]);
    }

    // Reserve space for two batteries, with a sufficiently large hole to insert the batteries.
    translate([7, depth - aa_dia / 2, height - aa_dia / 2])
        battery_with_insertion_path();
    translate([width - 7 - aa_length - contact_length, depth - aa_dia / 2, height - aa_dia / 2])
        battery_with_insertion_path();

    // Reserve space for the contacts, suitable for heat insertion.
    translate([7, depth - aa_dia / 2, height - aa_dia / 2])
        contact();
    translate([width - 7, depth - aa_dia / 2, height - aa_dia / 2])
        scale([-1, 1, 1])
        contact();
    translate([width - 7 - aa_length - contact_length, depth - aa_dia / 2, height - aa_dia / 2])
        contact();
    translate([7 + aa_length + contact_length, depth - aa_dia / 2, height - aa_dia / 2])
        scale([-1, 1, 1])
        contact();

    // Reserve space to run the cables along the part.
    translate([7, -1, height - aa_dia / 2])
        rotate([0, 90, 0])
        cylinder(d=4, h=width);
}
