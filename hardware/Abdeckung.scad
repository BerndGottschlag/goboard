include <roundedcube.scad>

$fn = 50;

width = 160.8;
depth = 11.5;
left_offset = 16;
right_offset = 8.5;
height = 2;
height_center = 1;

ledge_height = 3;
ledge_depth = 1.2;

switch_left_offset = 14;
switch_right_offset = 24;
switch_top_offset = 1;
switch_bottom_offset = 5;

usb_left_offset = 29;
usb_right_offset = 40.5;
usb_top_offset = 3;
usb_bottom_offset = 8;

module screw_hole() {
    translate([0, 0, -1])
        cylinder(h = 100, d = 3);
    translate([0, 0, -1])
        cylinder(h = 3, d1 = 9, d2 = 3);
}

difference() {
    // Base form
    union() {
        cube([width, depth, height]);
        translate([left_offset, 0, height])
            cube([width - left_offset - right_offset, ledge_depth, ledge_height]);
    }

    // Cutout to make space for the PCB
    translate([left_offset, ledge_depth, height_center])
        cube([width - left_offset - right_offset, 100, 100]);
    // TODO

    // Cutouts for switch and USB
    translate([width - switch_right_offset, depth - switch_bottom_offset, -1])
        cube([switch_right_offset - switch_left_offset, switch_bottom_offset - switch_top_offset, 100]);
    translate([width - usb_right_offset, depth - usb_bottom_offset, -1])
        roundedcube([usb_right_offset - usb_left_offset, usb_bottom_offset - usb_top_offset, 100], false, 1.0, "z");

    // Screw holes
    translate([width - right_offset / 2, depth / 2, 0])
        screw_hole();
    translate([right_offset / 2, depth / 2, 0])
        screw_hole();
    // TODO
}