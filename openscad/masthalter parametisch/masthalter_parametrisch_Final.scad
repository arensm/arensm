/*
  Parametrischer Masthalter

  Koordinaten:
    X = Breite quer zum Mast
    Y = Laenge entlang des Masts
    Z = Hoehe

  Alle Masse in Millimetern.
  Die Metallschellen werden nicht modelliert. Fuer jede Schelle ist ein
  U-foermiger Hohlkanal vorgesehen: von oben an einer Seite hinein,
  unter der Mastmulde hindurch und an der anderen Seite wieder nach oben.
*/

/* [Hauptabmessungen] */

// Radius des Masts (roter Radius im Referenzbild)
mast_radius = 25;

// Zusaetzliches radiales Spiel zwischen Mast und Halter
mast_clearance = 0.5;

// Gesamthoehe des Halters
holder_height = 38;

// Gesamtlaenge entlang des Masts
holder_length = 80;

// Seitliche Wandstaerke neben der Mastauflage
side_wall = 9;

// Materialstaerke unter dem tiefsten Punkt der Mastauflage
bottom_wall = 7;

/* [Montageplatte] */

// Abmessungen der unten liegenden Montageplatte
plate_width = 100;
plate_length = 100;

// Dicke der Montageplatte
plate_thickness = 6;

// Durchmesser der vier Durchgangsbohrungen (5.5 mm = Spiel fuer M5)
mounting_hole_diameter = 5.5;

// Abstand der Bohrungsmittelpunkte von den Plattenaussenkanten
mounting_hole_edge_distance = 10;

// Rundungsradius der vier Plattenecken
plate_corner_radius = 8;

/* [Banddurchfuehrungen] */

// Anzahl der Spannbaender
band_count = 2;

// Breite des Metallbands entlang des Masts
band_width = 12;

// Freie Dicke des Hohlkanals fuer das Metallband
band_slot_thickness = 2.0;

// Abstand der Bandmitten; bei zwei Baendern deren Mittenabstand
band_spacing = 45;

// Material zwischen Mastmulde und U-foermigem Hohlkanal
band_tunnel_inner_wall = 2.0;

/* [Form und Darstellung] */

// Rundungsradius der Grundflaeche
footprint_corner_radius = 8;

// Aufloesung der Rundungen
curve_resolution = 96;

// Nur zur Kontrolle: transparenten Beispielmast anzeigen
show_preview_mast = false;

// Nur zur Kontrolle: Verlauf der U-Kanaele rot hervorheben
show_band_tunnel_preview = false;

// Diagnoseansicht: nur die U-Kanaele anzeigen
show_band_tunnels_only = false;

$fn = curve_resolution;

trough_radius = mast_radius + mast_clearance;
trough_center_z = bottom_wall + trough_radius;
holder_width = 2 * (trough_radius + side_wall);
cut_extra = 1;

assert(mast_radius > 0, "mast_radius muss groesser als 0 sein.");
assert(mast_clearance >= 0, "mast_clearance darf nicht negativ sein.");
assert(holder_length > 0, "holder_length muss groesser als 0 sein.");
assert(holder_height > trough_center_z,
       str("holder_height muss groesser als ", trough_center_z,
           " mm sein, damit die U-Seiten oberhalb des Radiusmittelpunkts stehen."));
assert(bottom_wall > 0, "bottom_wall muss groesser als 0 sein.");
assert(side_wall > band_tunnel_inner_wall + band_slot_thickness,
       "side_wall ist fuer den U-foermigen Bandkanal zu duenn.");
assert(bottom_wall > band_tunnel_inner_wall + band_slot_thickness,
       "bottom_wall ist fuer den U-foermigen Bandkanal zu duenn.");
assert(plate_width >= holder_width && plate_length >= holder_length,
       "Die Montageplatte muss mindestens so gross wie der Halter sein.");
assert(plate_thickness > 0, "plate_thickness muss groesser als 0 sein.");
assert(mounting_hole_diameter > 0,
       "mounting_hole_diameter muss groesser als 0 sein.");
assert(mounting_hole_edge_distance > mounting_hole_diameter / 2 &&
       mounting_hole_edge_distance < min(plate_width, plate_length) / 2,
       "mounting_hole_edge_distance liegt ausserhalb des gueltigen Bereichs.");
assert(plate_corner_radius >= 0 &&
       plate_corner_radius < min(plate_width, plate_length) / 2,
       "plate_corner_radius ist fuer die Montageplatte zu gross.");
assert(band_count >= 1 && band_count == floor(band_count),
       "band_count muss eine positive ganze Zahl sein.");
assert(band_width > 0 && band_slot_thickness > 0,
       "Die Abmessungen der Bandkanaele muessen groesser als 0 sein.");
assert(band_count == 1 || band_spacing > 0,
       "band_spacing muss bei mehreren Baendern groesser als 0 sein.");
assert((band_count - 1) * band_spacing / 2 + band_width / 2 < holder_length / 2,
       "Banddurchfuehrungen liegen ausserhalb des Halters: Laenge oder Abstand anpassen.");
assert(footprint_corner_radius >= 0 &&
       footprint_corner_radius < min(holder_width, holder_length) / 2,
       "footprint_corner_radius ist fuer die Grundflaeche zu gross.");

module rounded_footprint(width, length, radius) {
    if (radius > 0) {
        offset(r = radius)
            square([width - 2 * radius, length - 2 * radius], center = true);
    } else {
        square([width, length], center = true);
    }
}

module outer_body() {
    translate([0, 0, plate_thickness])
        linear_extrude(height = holder_height)
            rounded_footprint(holder_width, holder_length, footprint_corner_radius);
}

module mounting_plate() {
    linear_extrude(height = plate_thickness)
        rounded_footprint(plate_width, plate_length, plate_corner_radius);
}

// U-foermige Mastmulde: untere Halbkreisauflage plus senkrechte Oeffnung.
module mast_trough_cutout() {
    union() {
        translate([0, 0, plate_thickness + trough_center_z])
            rotate([90, 0, 0])
                cylinder(h = holder_length + 2 * cut_extra,
                         r = trough_radius,
                         center = true);

        translate([-trough_radius,
                   -holder_length / 2 - cut_extra,
                   plate_thickness + trough_center_z])
            cube([2 * trough_radius,
                  holder_length + 2 * cut_extra,
                  holder_height - trough_center_z + 2 * cut_extra]);
    }
}

module one_band_tunnel(y_position) {
    tunnel_inner_radius = trough_radius + band_tunnel_inner_wall;
    tunnel_outer_radius = tunnel_inner_radius + band_slot_thickness;
    global_center_z = plate_thickness + trough_center_z;

    union() {
        // Unterer Ringbogen folgt dem Radius der Mastmulde.
        intersection() {
            difference() {
                translate([0, y_position, global_center_z])
                    rotate([90, 0, 0])
                        cylinder(h = band_width,
                                 r = tunnel_outer_radius,
                                 center = true);

                translate([0, y_position, global_center_z])
                    rotate([90, 0, 0])
                        cylinder(h = band_width + 2 * cut_extra,
                                 r = tunnel_inner_radius,
                                 center = true);
            }

            translate([-tunnel_outer_radius - cut_extra,
                       y_position - band_width / 2 - cut_extra,
                       global_center_z - tunnel_outer_radius - cut_extra])
                cube([2 * (tunnel_outer_radius + cut_extra),
                      band_width + 2 * cut_extra,
                      tunnel_outer_radius + 2 * cut_extra]);
        }

        // Beide Kanalenden laufen senkrecht bis durch die Oberseite.
        for (side = [-1, 1]) {
            translate([side * (tunnel_inner_radius + band_slot_thickness / 2),
                       y_position,
                       global_center_z
                       + (holder_height - trough_center_z + cut_extra) / 2])
                cube([band_slot_thickness,
                      band_width,
                      holder_height - trough_center_z + cut_extra],
                     center = true);
        }
    }
}

module all_band_cutouts() {
    for (index = [0 : band_count - 1]) {
        y_position = (index - (band_count - 1) / 2) * band_spacing;
        one_band_tunnel(y_position);
    }
}

module mounting_holes() {
    for (x = [-1, 1], y = [-1, 1]) {
        translate([x * (plate_width / 2 - mounting_hole_edge_distance),
                   y * (plate_length / 2 - mounting_hole_edge_distance),
                   plate_thickness / 2])
            cylinder(h = plate_thickness + 2 * cut_extra,
                     d = mounting_hole_diameter,
                     center = true);
    }
}

module mast_holder() {
    difference() {
        union() {
            mounting_plate();
            outer_body();
        }
        mast_trough_cutout();
        all_band_cutouts();
        mounting_holes();
    }
}

if (show_band_tunnels_only) {
    color([1, 0, 0]) all_band_cutouts();
} else {
    mast_holder();
}

// Transparente Vorschau, wird nicht Teil eines STL-Exports.
if (show_preview_mast) {
    %color([0.75, 0.75, 0.78, 0.45])
        translate([0, 0, plate_thickness + trough_center_z])
            rotate([90, 0, 0])
                cylinder(h = holder_length + 20,
                         r = mast_radius,
                         center = true);
}

// Transparente rote Kontrollansicht der innenliegenden U-Kanaele.
if (show_band_tunnel_preview) {
    %color([1, 0, 0, 0.75]) all_band_cutouts();
}
