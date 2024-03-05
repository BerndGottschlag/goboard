include <roundedcube.scad>

$fn = 50;

width = 162.2;
depth = 11.1;
left_offset = 13.5;
right_offset = 10;
height = 1.6;
height_center = 1;

switch_left_offset = 16.5;
switch_right_offset = 26.5;
switch_top_offset = 10;
switch_bottom_offset = 5;

usb_left_offset = 31.5;
usb_right_offset = 43;
usb_top_offset = 8.5;
usb_bottom_offset = 3;

module screw_hole() {
    translate([0, 0, -1])
        cylinder(h = 100, d = 2);
    //translate([0, 0, -1])
    //    cylinder(h = 2, d = 3.5);
}

difference() {
    // Base form
    union() {
        cube([width, depth, height]);
    }

    // Cutout to make space for the PCB
    translate([left_offset, -1, height_center])
        cube([width - left_offset - right_offset, 100, 100]);
    // TODO

    // Cutouts for switch and USB
    translate([width - switch_right_offset, switch_bottom_offset, -1])
        cube([switch_right_offset - switch_left_offset, switch_top_offset - switch_bottom_offset, 100]);
    translate([width - usb_right_offset, usb_bottom_offset, -1])
        roundedcube([usb_right_offset - usb_left_offset, usb_top_offset - usb_bottom_offset, 100], false, 1.0, "z");

    // Screw holes
    translate([width - right_offset / 2, depth / 2, 0])
        screw_hole();
    translate([right_offset / 2, depth / 2, 0])
        screw_hole();
    // TODO
}