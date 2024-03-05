$fn = 100;

length_total = 34.5; // +1.0mm below the PCB
width_total = 12;
height_total = 6.9;

pcb_top = 5.7;
pcb_bottom = pcb_top - 1.6;
pcb_width = 10.2;
pcb_length = 7.2;

led_center_offset = 26;
led_to_pcb_edge = 1.5;

screw_hole_dia = 1.4;
screw_post_dia = 4.5;

module mounting_screw_hole() {
    translate([0, 0, -1])
        cylinder(d = screw_hole_dia, h = pcb_bottom + 2);
}

module mounting_screw() {
    difference() {
        cylinder(d = screw_post_dia, h = pcb_bottom);
        
        //mounting_screw_hole();
    }
}

difference(){
    union() {
        // Base
        cube([width_total, length_total, 1.5]);
        // Front ledge towards the PCB
        cube([width_total, 3.0, 3.0]);
        translate([0, -1, 0])
            cube([width_total, 2, 1]);
        
        // Back PCB holder
        difference() {
            translate([0.25, led_center_offset + led_to_pcb_edge - 3.0, 0])
                cube([width_total - 0.5, 5.0, height_total]);
            
            translate([(width_total - pcb_width) / 2, led_center_offset + led_to_pcb_edge  - 5.0, pcb_bottom])
                cube([pcb_width, 5.0, 100]);
        }
        
        translate([screw_post_dia / 2, led_center_offset + led_to_pcb_edge - pcb_length - 1.0, 0])
            mounting_screw();
        
        translate([width_total - screw_post_dia / 2, led_center_offset + led_to_pcb_edge - pcb_length - 1.0, 0])
            mounting_screw();
    }
    
    translate([screw_post_dia / 2, led_center_offset + led_to_pcb_edge - pcb_length - 1.0, 0])
        mounting_screw_hole();
    
    translate([width_total - screw_post_dia / 2, led_center_offset + led_to_pcb_edge - pcb_length - 1.0, 0])
        mounting_screw_hole();
}