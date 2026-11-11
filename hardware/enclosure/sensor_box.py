"""
Parametric enclosure for: HC-SR04 ultrasonic sensor + perfboard + ESP32-C3 "SuperMini"
Run in Blender:   Scripting tab -> Open -> Run Script   (it clears the scene first!)
or headless:      blender -b --python sensor_box.py

Outputs (next to this script, or in $BOX_OUT):
  sensor_box_body.stl / sensor_box_lid.stl   print-ready parts
  electronics_model.stl                      model of sensor + perfboard + ESP (assembled position)
  sensor_box_assembled.blend                 box + electronics + lid, coloured, closed
  sensor_box.blend                           print layout
It also runs a collision check (box+lid  vs  electronics + USB cable plug), including a
tolerance sweep (electronics shifted +-1.5 mm in X, +-1 mm in Y/Z).

All dimensions in millimetres. EDIT THE NUMBERS BELOW if your measurements differ.

v5 (after the second discussion):
  * the STOP is now the brown perfboard: its front edge rests against the inside of the front
    wall, the sensor (blue board + transducers) sticks out of the front from there
  * under the sensor board there are two small L-shaped supports (ledge + stop) that
    compensate the perfboard, so the blue board stays flat and square to the front wall
  * two small posts on the lid press lightly on the back edge of the perfboard
  * verified from the test print and kept: transducer holes, button positions (x, z)
"""
import bpy, bmesh, math, os, struct

# ----------------------------------------------------------------- PARAMETERS
WALL          = 2.0     # wall thickness
LID_T         = 2.0     # lid plate thickness
FIT_CLEAR     = 0.25    # lid lip clearance per side (increase if lid is too tight)

# ---- Front wall / how the sensor is held
FRONT_WALL    = 1.6     # thickness of the front wall (thinner = transducers stick out more)
FRONT_GAP     = 0.25    # air gap between the front edge of the perfboard (the stop) and the front wall
HOLD_GAP      = 0.25    # play between the lid posts and the back edge of the perfboard
                        # (rattles -> make smaller; lid does not close -> make bigger)

# ---- HC-SR04
PCB_L         = 45.0
PCB_H         = 20.0
PCB_T         = 1.6
PCB_Z0        = 1.0     # gap between cavity floor and bottom edge of the HC-SR04 board
PCB_X0        = None    # (derived below) gap between left wall and left end of the HC-SR04 board
XDUCER_D      = 16.0    # transducer can diameter
XDUCER_PITCH  = 26.0    # centre-to-centre distance of the two transducers
XDUCER_CLEAR  = 0.4     # extra diameter for the holes (VERIFIED by test print)
XDUCER_H      = 12.0    # transducer height above the HC-SR04 board (verified: tips stuck out 2.4 mm)
XTAL_W, XTAL_Z0, XTAL_Z1, XTAL_H = 11.0, 1.2, 5.4, 3.5   # crystal on the FRONT of the HC-SR04

# ---- Perfboard (soldered along the top edge of the HC-SR04)
PERF_L        = 50.5    # length along X
PERF_X_OVER_L = 2.5     # how far it sticks out beyond the LEFT end of the HC-SR04 (from the test print)
PERF_FRONT_EXT = 6.5    # *** KEY VALUE ***  how far the perfboard sticks out in FRONT of the HC-SR04 board
                        # (measure it with callipers: front edge of the brown board -> front face of the blue board)
PERF_DEPTH    = 17.9    # total width of the perfboard along Y
PERF_T        = 1.6
PERF_TOP_PARTS = 3.6    # tallest thing on top of the perfboard near its front edge (wire)
SIDE_CLEAR    = 1.6     # air gap between the perfboard ends and the side walls
LID_FROM_PCB  = 17.0    # lid inside surface, measured from the front face of the HC-SR04 board

# ---- ESP32-C3 SuperMini (parallel to the HC-SR04, USB-C pointing to +X)
ESP_L, ESP_W, ESP_T = 22.5, 18.0, 1.0
ESP_BOTTOM_FROM_PCB = 8.3      # underside of the ESP, measured from the front face of the HC-SR04 board
USB_TIP_BEYOND = 4.2    # USB-C connector tip beyond the right end of the HC-SR04 board
USB_PROTRUDE   = 2.3    # how far the connector sticks out of the ESP board edge
USB_CENTER_Z_FROM_TOP = 8.7    # USB-C centre, measured down from the perfboard
BTN_X_FROM_PCB = 38.3   # buttons, measured from the left end of the HC-SR04 (VERIFIED by test print)
BTN_Z_FROM_TOP = (5.7, 11.3)   # (RST, BOOT) measured down from the perfboard (VERIFIED)
BTN_H         = 1.5     # height of the button above the ESP board

# ---- USB-C opening in the end wall (a pocket sized for the cable's rubber overmold)
USB_POCKET_W  = 14.4    # along Z
USB_POCKET_H  = 8.6     # along Y
USB_POCKET_R  = 3.0     # corner radius
LIP_NOTCH_W   = 18.0    # width (Z) of the gap in the lid lip around the USB connector

# ---- Button holes in the lid
BTN_HOLE_D    = 3.0     # pin-hole diameter (paperclip / SIM tool / toothpick)
BTN_TUBES     = True    # short guide tubes on the inside of the lid
BTN_TUBE_OD   = 4.6
BTN_TUBE_GAP  = 2.0     # gap left between tube tip and button top (never let it touch!)

# ---- Cavity (inside space) - X is derived from the perfboard, Z from the tallest pin
PCB_X0        = PERF_X_OVER_L + SIDE_CLEAR
IN_X          = PERF_L + 2 * SIDE_CLEAR
IN_Z          = 27.5
LIP_DEPTH     = 3.0     # how far the lid lip goes into the box
LIP_T         = 1.2     # lip ring thickness

# ---- what to do
RUN_FIT_CHECK = True
SWEEP         = True

# ----------------------------------------------------------------- derived (absolute coords)
T_F    = FRONT_WALL                            # front wall thickness
PERF_Y0 = T_F + FRONT_GAP                      # front edge of the perfboard = the stop
Y_PCB  = PERF_Y0 + PERF_FRONT_EXT              # front face of the HC-SR04 board
BODY_Y = Y_PCB + LID_FROM_PCB                  # body is open at Y = BODY_Y
OX = IN_X + 2 * WALL
OZ = IN_Z + 2 * WALL
Z_TOP  = WALL + PCB_Z0 + PCB_H                 # underside of the perfboard
X_PCB0 = WALL + PCB_X0                         # left end of the HC-SR04 board
TIP_X  = X_PCB0 + PCB_L + USB_TIP_BEYOND       # USB-C connector tip
USB_Z  = Z_TOP - USB_CENTER_Z_FROM_TOP
ESP_TOP_Y = Y_PCB + ESP_BOTTOM_FROM_PCB + ESP_T
USB_Y  = ESP_TOP_Y + 1.63                      # USB-C centre
BTN_X  = X_PCB0 + BTN_X_FROM_PCB
BTN_ZS = [Z_TOP - t for t in BTN_Z_FROM_TOP]
BTN_TOP_Y = ESP_TOP_Y + BTN_H
PERF_X0 = X_PCB0 - PERF_X_OVER_L               # perfboard left end
PERF_X1 = PERF_X0 + PERF_L
PERF_Y1 = PERF_Y0 + PERF_DEPTH                 # perfboard back edge
SEG = 96

try:
    OUT_DIR = os.path.dirname(os.path.abspath(__file__))
except NameError:
    OUT_DIR = os.path.expanduser("~")
OUT_DIR = os.environ.get("BOX_OUT", OUT_DIR)

# ----------------------------------------------------------------- helpers
def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()

def _apply_all(o):
    bpy.ops.object.select_all(action='DESELECT')
    o.select_set(True); bpy.context.view_layer.objects.active = o
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

def box(name, x0, y0, z0, x1, y1, z1):
    bpy.ops.mesh.primitive_cube_add(size=1)
    o = bpy.context.active_object
    o.name = name
    o.scale = (x1 - x0, y1 - y0, z1 - z0)
    o.location = ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)
    _apply_all(o)
    return o

def cyl_y(name, x, z, y0, y1, d):
    """Cylinder along the Y axis from y0 to y1."""
    bpy.ops.mesh.primitive_cylinder_add(vertices=SEG, radius=d / 2, depth=(y1 - y0))
    o = bpy.context.active_object
    o.name = name
    o.rotation_euler = (math.pi / 2, 0, 0)
    o.location = (x, (y0 + y1) / 2, z)
    _apply_all(o)
    return o

def cyl_x(name, y, z, x0, x1, d):
    bpy.ops.mesh.primitive_cylinder_add(vertices=48, radius=d / 2, depth=(x1 - x0))
    o = bpy.context.active_object
    o.name = name
    o.rotation_euler = (0, math.pi / 2, 0)
    o.location = ((x0 + x1) / 2, y, z)
    _apply_all(o)
    return o

def prism_x(name, x0, x1, pts):
    """Prism along X from x0 to x1 with the outline pts = [(y, z), ...] (convex)."""
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    a = [bm.verts.new((x0, y, z)) for y, z in pts]
    b = [bm.verts.new((x1, y, z)) for y, z in pts]
    bm.faces.new(a); bm.faces.new(b[::-1])
    n = len(pts)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((a[i], a[j], b[j], b[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(o)
    bpy.context.view_layer.objects.active = o
    return o

def stadium(cy, cz, length_z, height_y, n=24):
    """Outline of a slot with fully rounded ends (long axis along Z)."""
    r = height_y / 2
    d = length_z / 2 - r
    pts = []
    for i in range(n + 1):
        a = math.pi * i / n
        pts.append((cy + r * math.cos(a), cz + d + r * math.sin(a)))
    for i in range(n + 1):
        a = math.pi + math.pi * i / n
        pts.append((cy + r * math.cos(a), cz - d + r * math.sin(a)))
    return pts

def rounded_rect(cy, cz, w_z, h_y, r, n=10):
    """Outline (y, z) of a rounded rectangle, counter-clockwise."""
    pts = []
    for (sy, sz, a0) in ((1, 1, 0), (-1, 1, 90), (-1, -1, 180), (1, -1, 270)):
        ccy = cy + sy * (h_y / 2 - r); ccz = cz + sz * (w_z / 2 - r)
        for i in range(n + 1):
            a = math.radians(a0 + 90 * i / n)
            pts.append((ccy + r * math.cos(a), ccz + r * math.sin(a)))
    return pts

def boolean(target, tool, op):
    m = target.modifiers.new("bool", 'BOOLEAN')
    m.operation = op
    m.solver = 'EXACT'
    m.object = tool
    with bpy.context.temp_override(object=target, active_object=target):
        bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.data.objects.remove(tool, do_unlink=True)

def cut(target, tool):   boolean(target, tool, 'DIFFERENCE')
def fuse(target, tool):  boolean(target, tool, 'UNION')

def paint(obj, rgb, rough=0.5, metal=0.0):
    mat = bpy.data.materials.new(obj.name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*rgb, 1)
    bsdf.inputs["Roughness"].default_value = rough
    bsdf.inputs["Metallic"].default_value = metal
    obj.data.materials.clear(); obj.data.materials.append(mat)

def write_stl(objs, path):
    tris = []
    for obj in objs:
        bm = bmesh.new(); bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        mw = obj.matrix_world
        for f in bm.faces:
            n = (mw.to_3x3() @ f.normal).normalized()
            tris.append((n, [mw @ v.co for v in f.verts]))
        bm.free()
    with open(path, "wb") as f:
        f.write(b"sensor box".ljust(80, b" "))
        f.write(struct.pack("<I", len(tris)))
        for n, vs in tris:
            f.write(struct.pack("<3f", *n))
            for p in vs:
                f.write(struct.pack("<3f", p.x, p.y, p.z))
            f.write(b"\0\0")
    print("exported", path)

# ----------------------------------------------------------------- BOX
clear_scene()

xc = X_PCB0 + PCB_L / 2
zc = WALL + PCB_Z0 + PCB_H / 2

# ---- BODY (tray, open at the back, front wall carries the sensor)
body = box("Body", 0, 0, 0, OX, BODY_Y, OZ)
cut(body, box("cavity", WALL, T_F, WALL, WALL + IN_X, BODY_Y + 1, WALL + IN_Z))
for sx in (-1, 1):                                     # transducer holes
    cut(body, cyl_y("xd", xc + sx * XDUCER_PITCH / 2, zc, -1, T_F + 1, XDUCER_D + XDUCER_CLEAR))
# Supports under the HC-SR04 board (they compensate the perfboard, which only touches the wall at the top):
#   ledge = the bottom edge of the board rests on it,  stop = the front face of the board rests against it
zb = WALL + PCB_Z0
for (xa, xb) in ((X_PCB0 + 0.5, xc - XTAL_W / 2 - 1.5), (xc + XTAL_W / 2 + 1.5, X_PCB0 + PCB_L - 0.5)):
    fuse(body, box("ledge", xa, T_F - 0.5, WALL - 0.1, xb, Y_PCB + PCB_T + 1.5, zb - 0.1))
    fuse(body, box("stop",  xa, T_F - 0.5, WALL - 0.1, xb, Y_PCB - FRONT_GAP, zb + 1.0))
# USB-C opening in the +X end wall: rounded pocket big enough for the plug overmold
cut(body, prism_x("usb", OX - WALL - 1, OX + 1, rounded_rect(USB_Y, USB_Z, USB_POCKET_W, USB_POCKET_H, USB_POCKET_R)))

# ---- LID (plate + lip ring with a gap at the USB connector + button guide tubes)
lid = box("Lid", 0, BODY_Y, 0, OX, BODY_Y + LID_T, OZ)
lx0, lx1 = WALL + FIT_CLEAR, WALL + IN_X - FIT_CLEAR
lz0, lz1 = WALL + FIT_CLEAR, WALL + IN_Z - FIT_CLEAR
ring = box("ring", lx0, BODY_Y - LIP_DEPTH, lz0, lx1, BODY_Y + 0.1, lz1)
cut(ring, box("ring_in", lx0 + LIP_T, BODY_Y - LIP_DEPTH - 1, lz0 + LIP_T,
              lx1 - LIP_T, BODY_Y + 1, lz1 - LIP_T))
cut(ring, box("ring_notch", lx1 - LIP_T - 0.5, BODY_Y - LIP_DEPTH - 1, USB_Z - LIP_NOTCH_W / 2,
              lx1 + 1, BODY_Y + 1, USB_Z + LIP_NOTCH_W / 2))      # gap for connector + plug
fuse(lid, ring)

# two small posts that press (almost) on the back edge of the perfboard -> nothing rattles
hold_y = PERF_Y1 + HOLD_GAP
for px in (PERF_X0 + 5.0, PERF_X1 - 5.0):
    fuse(lid, box("post", px - 1.5, hold_y, Z_TOP + PERF_T / 2 - 1.5, px + 1.5, BODY_Y + 0.5, Z_TOP + PERF_T / 2 + 1.5))

tube_tip_y = BTN_TOP_Y + BTN_TUBE_GAP
if BTN_TUBES:
    for z in BTN_ZS:
        fuse(lid, cyl_y("tube", BTN_X, z, tube_tip_y, BODY_Y + 0.5, BTN_TUBE_OD))
for z in BTN_ZS:
    cut(lid, cyl_y("btnhole", BTN_X, z, tube_tip_y - 1, BODY_Y + LID_T + 1, BTN_HOLE_D))

# ----------------------------------------------------------------- ELECTRONICS MODEL
def make_electronics(dx=0.0, dy=0.0, dz=0.0):
    """Returns (parts, plug). parts: name -> (object, colour). Everything shifted by (dx,dy,dz)."""
    P = {}
    def add(name, o, rgb, rough=0.5, metal=0.0):
        paint(o, rgb, rough, metal); P[name] = o
    def B(name, x0, y0, z0, x1, y1, z1, rgb, rigid=False, **kw):
        # rigid = locked by the transducer holes (X/Z) and the front wall (Y): the HC-SR04 board itself
        sx, sy, sz = (0.0, 0.0, 0.0) if rigid else (dx, dy, dz)
        add(name, box(name, x0 + sx, y0 + sy, z0 + sz, x1 + sx, y1 + sy, z1 + sz), rgb, **kw)

    x0 = X_PCB0; zb = WALL + PCB_Z0
    # --- HC-SR04
    B("hcsr04_pcb", x0, Y_PCB, zb, x0 + PCB_L, Y_PCB + PCB_T, zb + PCB_H, (0.03, 0.15, 0.55), rigid=True)
    back = Y_PCB + PCB_T
    B("hcsr04_header", xc - 5.1, back, Z_TOP - 2.6, xc + 5.1, back + 2.6, Z_TOP, (0.02, 0.02, 0.02), rigid=True)
    B("hcsr04_U1", x0 + 1.5, back, zc - 3.1, x0 + 9.5, back + 2.7, zc + 3.1, (0.02, 0.02, 0.02), rigid=True)
    B("hcsr04_U2", x0 + 18.0, back, zc - 3.1, x0 + 26.0, back + 2.7, zc + 3.1, (0.02, 0.02, 0.02), rigid=True)
    B("hcsr04_crystal", xc - XTAL_W / 2, Y_PCB - XTAL_H, zb + XTAL_Z0, xc + XTAL_W / 2, Y_PCB, zb + XTAL_Z1, (0.75, 0.75, 0.78), metal=1, rough=0.3, rigid=True)
    for k, sx in enumerate((-1, 1)):
        o = cyl_y("transducer%d" % k, xc + sx * XDUCER_PITCH / 2, zc,
                  Y_PCB - XDUCER_H, Y_PCB, XDUCER_D)
        add("transducer%d" % k, o, (0.8, 0.82, 0.85), rough=0.25, metal=1)
    # --- perfboard + things soldered on it
    px0 = PERF_X0
    B("perfboard", px0, PERF_Y0, Z_TOP, PERF_X1, PERF_Y1, Z_TOP + PERF_T, (0.75, 0.33, 0.12))
    ptop = Z_TOP + PERF_T; pback = PERF_Y1
    B("resistor", x0 + 20, pback - 0.2, ptop, x0 + 30, pback + 2.3, ptop + 2.5, (0.8, 0.62, 0.38))
    B("yellow_wire", x0 + 13.5, pback - 2.2, ptop, x0 + 31, pback, ptop + 2.0, (0.9, 0.75, 0.1))
    B("grey_wire", x0 + 8, PERF_Y0 + 0.3, ptop, x0 + 40, PERF_Y0 + 2.5, ptop + 2.0, (0.5, 0.5, 0.52))
    # --- ESP32-C3 SuperMini
    esp_r = TIP_X - USB_PROTRUDE
    ey0 = Y_PCB + ESP_BOTTOM_FROM_PCB
    B("esp_pcb", esp_r - ESP_L, ey0, Z_TOP - ESP_W, esp_r, ey0 + ESP_T, Z_TOP, (0.02, 0.02, 0.03))
    B("esp_pins", esp_r - ESP_L + 1.0, ey0 + 0.1, Z_TOP, esp_r - 1.0, ey0 + 0.7, ptop + 3.5, (0.8, 0.8, 0.8), metal=1)
    B("esp_qfn", esp_r - 16.5, ESP_TOP_Y, Z_TOP - 11.5, esp_r - 11.5, ESP_TOP_Y + 0.9, Z_TOP - 6.5, (0.1, 0.1, 0.12))
    B("usb_connector", TIP_X - 7.4, ESP_TOP_Y, USB_Z - 4.47, TIP_X, ESP_TOP_Y + 3.26, USB_Z + 4.47, (0.8, 0.8, 0.82), metal=1, rough=0.3)
    for k, z in enumerate(BTN_ZS):
        B("button%d" % k, BTN_X - 1.6, ESP_TOP_Y, z - 2.1, BTN_X + 1.6, BTN_TOP_Y, z + 2.1,
          (0.85, 0.12, 0.1) if k == 0 else (0.9, 0.55, 0.1))
    # --- USB-C cable plug, fully inserted (only used for the collision check)
    plug = {}
    def PB(name, x0_, y0_, z0_, x1_, y1_, z1_):
        plug[name] = box(name, x0_ + dx, y0_ + dy, z0_ + dz, x1_ + dx, y1_ + dy, z1_ + dz)
    PB("plug_shell",    TIP_X - 6.2, USB_Y - 1.3,  USB_Z - 4.2, TIP_X + 0.4, USB_Y + 1.3,  USB_Z + 4.2)
    om = prism_x("plug_overmold", TIP_X + 0.4 + dx, TIP_X + 25 + dx,
                 [(y + dy, z + dz) for y, z in rounded_rect(USB_Y, USB_Z, 12.35, 6.5, 2.0)])
    plug["plug_overmold"] = om
    return P, plug

def volume_of_overlap(a, b):
    dup = a.copy(); dup.data = a.data.copy(); bpy.context.collection.objects.link(dup)
    m = dup.modifiers.new("i", 'BOOLEAN'); m.operation = 'INTERSECT'; m.solver = 'EXACT'; m.object = b
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(dup.evaluated_get(dg))
    bm = bmesh.new(); bm.from_mesh(me)
    v = abs(bm.calc_volume()) if len(bm.faces) else 0.0
    bm.free(); bpy.data.objects.remove(dup, do_unlink=True); bpy.data.meshes.remove(me)
    return v

def check(parts, plug, label):
    problems = []
    for name, o in parts.items():
        for part in (body, lid):
            v = volume_of_overlap(o, part)
            if v > 0.05: problems.append("%s x %s = %.1f mm3" % (name, part.name, v))
    for name, o in plug.items():
        targets = [body, lid]
        if name == "plug_overmold":
            targets += [parts[k] for k in ("perfboard", "hcsr04_pcb", "esp_pcb", "resistor", "yellow_wire", "esp_pins")]
        for t in targets:
            v = volume_of_overlap(o, t)
            if v > 0.05: problems.append("%s x %s = %.1f mm3" % (name, t.name, v))
    print("  %-34s %s" % (label, "OK - no collisions" if not problems else "COLLISIONS: " + "; ".join(problems)))
    return not problems

def remove_all(d):
    for o in d.values(): bpy.data.objects.remove(o, do_unlink=True)

parts, plug = make_electronics()
all_ok = True
if RUN_FIT_CHECK:
    print("---- FIT CHECK (box + lid vs electronics + USB cable plug) ----")
    all_ok &= check(parts, plug, "nominal position")
    if SWEEP:
        for (dx, dy, dz) in [(1.0, 0, 0), (-1.0, 0, 0), (0, 0.25, 0), (0, -0.25, 0), (0, 0, 1.0), (0, 0, -1.0)]:
            p2, g2 = make_electronics(dx, dy, dz)
            all_ok &= check(p2, g2, "electronics shifted dx=%+.1f dy=%+.1f dz=%+.1f" % (dx, dy, dz))
            remove_all(p2); remove_all(g2)
    print("RESULT:", "everything fits" if all_ok else "SEE COLLISIONS ABOVE")
print("Transducer tips stick out of the front face by %.1f mm" % (XDUCER_H - Y_PCB))
print("Button top -> lid inside gap: %.1f mm (tube tip %.1f mm above button)" % (BODY_Y - BTN_TOP_Y, BTN_TUBE_GAP))
print("USB-C tip is %.1f mm from the OUTER end face (plug can pass through the pocket)" % (OX - TIP_X))
print("Outer size of the box (with lid): %.1f x %.1f x %.1f mm" % (OX, BODY_Y + LID_T, OZ))
if os.environ.get("LAYOUT_JSON"):
    import json
    json.dump(dict(BTN_X=BTN_X, BTN_ZS=BTN_ZS, USB_Z=USB_Z, USB_Y=USB_Y, Y_PCB=Y_PCB, BODY_Y=BODY_Y, TIP_X=TIP_X,
                   BTN_TOP_Y=BTN_TOP_Y, T_F=T_F, tube_tip_y=tube_tip_y, OX=OX, OZ=OZ), open(os.environ["LAYOUT_JSON"], "w"))

# ----------------------------------------------------------------- assembled export
remove_all(plug)
write_stl(list(parts.values()), os.path.join(OUT_DIR, "electronics_model.stl"))
paint(body, (0.62, 0.65, 0.70), rough=0.6)
paint(lid, (0.80, 0.82, 0.86), rough=0.6)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT_DIR, "sensor_box_assembled.blend"))
remove_all(parts)

# ----------------------------------------------------------------- print layout
def orient(obj, rot_x, x_off=0.0):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True); bpy.context.view_layer.objects.active = obj
    obj.rotation_euler = (rot_x, 0, 0)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    mn = [min(v.co[i] for v in obj.data.vertices) for i in range(3)]
    obj.location = (-mn[0] + x_off, -mn[1], -mn[2])
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)

orient(body, math.pi / 2)             # front face on the bed, open side up
orient(lid, -math.pi / 2, OX + 10)    # plate on the bed, lip + tubes pointing up

write_stl([body], os.path.join(OUT_DIR, "sensor_box_body.stl"))
write_stl([lid],  os.path.join(OUT_DIR, "sensor_box_lid.stl"))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT_DIR, "sensor_box.blend"))
print("Body size:", tuple(round(d, 2) for d in body.dimensions))
print("Lid size: ", tuple(round(d, 2) for d in lid.dimensions))
