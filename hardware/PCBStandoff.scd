
$fn=100;

lower_width = 6.8;
upper_width = 5.5 + 0.5;

height = 1.8;

nut_height = 1.2;
nut_width = 4.0;

nut_offset = (upper_width - nut_width) / 2;

difference() {
    union() {
        cube([upper_width, upper_width, height]);
        translate([-(lower_width - upper_width) / 2, -(lower_width - upper_width) / 2, height - 0.4])
            cube([lower_width, lower_width, 0.4]);
    }
    
    // screw hole
    translate([upper_width / 2, upper_width / 2, 0])
        cylinder(h = 10, d = 2.4, center = true);
    
    translate([nut_offset, nut_offset, height - nut_height])
        cube([nut_width, nut_width, nut_height + 0.1]);
    
}