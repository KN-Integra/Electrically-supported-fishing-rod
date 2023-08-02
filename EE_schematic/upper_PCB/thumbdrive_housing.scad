//
// Housing for USB Thumbdrive
//

create_whole                = false;

$fn = 50;

tolerance                   =  0.1;

pcb_length                  = 30.0;
pcb_width                   = 18;

pcb_board_height            =  2;
pcb_bottomcomponents_height =  2;
pcb_topcomponents_height    =  2;
pcb_height                  =  pcb_board_height + pcb_topcomponents_height + pcb_bottomcomponents_height;
pcb_support_width           =  1.5;

usb_width                   = 11.9;
usb_height                  =  4.4;
usb_length                  = 15.0;
usb_plug_length             = 12.0;

wall_thickness              =  1.6;
corner_radius               = wall_thickness;
lip_thickness               =  0.8;
lip_height                  =  1.4;

space_dimensions            = [ pcb_length + tolerance,
                                pcb_width  + tolerance,
                                pcb_height + tolerance];

space_dimensions_usb        = [ usb_length + tolerance,
                                usb_width  + tolerance,
                                usb_height + tolerance];

outer_dimensions            = [ space_dimensions[0]+2*corner_radius,
                                space_dimensions[1]+2*corner_radius,
                                space_dimensions[2]+2*corner_radius,];

module button_hole() {
    translate([  10.5,
                 space_dimensions[1]/2,
                 space_dimensions[2]- tolerance
              ]) cylinder(h=wall_thickness+tolerance, r=1.5);
}
module rounded_cube(d,r) {
   minkowski() {
    cube(d);
    sphere(r=r);
   }
}

module usb_hole() {
    translate([  0.01-space_dimensions_usb[0],
                 (space_dimensions[1]-space_dimensions_usb[1])/2,
                 pcb_bottomcomponents_height+pcb_board_height+(tolerance/2)-2
              ]) cube(space_dimensions_usb);
}

module housing() {
    difference() {
        union() {
            difference() {
                rounded_cube(space_dimensions, corner_radius);
                cube(space_dimensions);
            }
            cube([space_dimensions[0], pcb_support_width, pcb_bottomcomponents_height]);
            translate([0, space_dimensions[1]-pcb_support_width, 0])
            cube([space_dimensions[0], pcb_support_width, pcb_bottomcomponents_height]);
        }
        usb_hole();
        button_hole();
    }
}

module lip(oversize) {
    difference() {
        translate([0, 0, pcb_bottomcomponents_height+pcb_board_height]) difference() {
            translate([-lip_thickness-(oversize/2), -lip_thickness-(oversize/2), 0])
            cube( [ space_dimensions[0]+2*lip_thickness+oversize,
                    space_dimensions[1]+2*lip_thickness+oversize, lip_height+oversize] );
            translate([(oversize/2), (oversize/2), -0.01]) cube([space_dimensions[0]-oversize, space_dimensions[1]-oversize, space_dimensions[2]]);
        }
        usb_hole();
    }
}

if(create_whole) housing();

if(!create_whole) translate([0, create_whole?outer_dimensions[1]:corner_radius + 5, 0]) {
    intersection() {
        housing();
        translate([-corner_radius, -corner_radius, -corner_radius])
            cube([outer_dimensions[0], outer_dimensions[1], corner_radius+pcb_bottomcomponents_height+pcb_board_height+tolerance]);
    }
    lip(0.0);
}

if(!create_whole) translate([0, -2*corner_radius-5, space_dimensions[2]]) rotate([180, 0, 0]) {
    difference() {
        intersection() {
            housing();
            translate([-corner_radius, -corner_radius, pcb_bottomcomponents_height+pcb_board_height+tolerance])
                cube([outer_dimensions[0], outer_dimensions[1], pcb_topcomponents_height+corner_radius]);
        }
        translate([0, 0, 0.01]) lip(tolerance);
    }
}

