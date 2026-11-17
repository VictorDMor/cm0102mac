"""Original canvas constants used by the adaptive screen layouts.

These are presentation feature fixups, not whole-module reconstruction owners.
Source-image dimensions, video setup and the message box width cap stay intact.
"""
ADAPTIVE_GUI_INSTRUCTIONS = {
    # Club content bottom, before the original tab builder consumes it.
    0x1001f064: "li r0,0x221",
    0x100240d0: "rlwinm r16,r3,0x0,0x18,0x1f",
    0x10024190: "rlwinm r16,r8,0x0,0x18,0x1f",
    0x10029b4c: "rlwinm r22,r0,0x0,0x18,0x1f",
    0x1002e48c: "rlwinm r10,r3,0x0,0x18,0x1f",
    # GUIO restores a bitmap at left/top only; extend the menubar artwork
    # before original controls are painted, then execute the same flag load.
    0x10726050: "lbz r0,0x17a(r31)",
    # Career sidebar background, grids and bottom Continue/status controls.
    0x102a1188: "li r7,0x257",
    0x102a1244: "li r7,0x24e",
    0x102a12ec: "li r7,0x24e",
    0x102a18b0: "li r7,0x24e",
    0x102a47c8: "li r7,0x24e",
    0x102a4b6c: "li r6,0x226",
    0x102a4bec: "li r6,0x226",
    0x102a4ce8: "li r6,0x226",
    0x102a4d68: "li r6,0x226",
    0x102a4e70: "li r6,0x226",
    0x102a4eec: "li r6,0x226",
    0x102a4f8c: "li r6,0x226",
    0x102a5004: "li r6,0x226",
    0x102a4b78: "li r8,0x24e",
    0x102a4bf8: "li r8,0x24e",
    0x102a4cf4: "li r8,0x24e",
    0x102a4d74: "li r8,0x24e",
    0x102a4e7c: "li r8,0x24e",
    0x102a4ef8: "li r8,0x24e",
    0x102a4f98: "li r8,0x24e",
    0x102a5010: "li r8,0x24e",
    # Processing bar and its partial repaint use the committed canvas, even after history is cleared.
    0x1076e6c0: "li r4,0x22b",
    0x1076e798: "li r4,0x22b",
    0x1076e6c4: "li r5,0x316",
    0x1076e79c: "li r5,0x316",
    0x1076e6c8: "li r6,0x24e",
    0x1076e7a0: "li r6,0x24e",
    0x1076e6f8: "li r4,0x235",
    0x1076e71c: "li r4,0x235",
    0x1076e778: "li r4,0x235",
    0x1076e704: "li r6,0x244",
    0x1076e724: "li r6,0x244",
    0x1076e784: "li r6,0x244",
    0x1076e720: "li r5,0x302",
    0x1076e77c: "li r5,0x302",
    0x1076e738: "li r0,0x300",
    0x1076e748: "li r4,0x237",
    0x1076e750: "li r6,0x242",
    # Player/Staff Search bottom and total capacity (heading + body rows).
    0x1031bdcc: "li r3,0x221",
    0x1031d200: "rlwinm r0,r5,0x0,0x18,0x1f",
    # Grid construction, popup anchoring and translation have independent
    # limits from individual controls. Keep both paths on the same canvas.
    0x10713e1c: "cmpwi r0,0x31f", 0x10713e28: "li r0,0x31f",
    0x10713e34: "cmpwi r0,0x257", 0x10713e40: "li r0,0x257",
    0x10714d00: "cmpwi r0,0x258", 0x10714dbc: "cmpwi r0,0x258",
    0x10714e34: "cmpwi r0,0x258", 0x10714df4: "cmpwi r0,0x190",
    0x10714e84: "cmpwi r6,0x190",
    0x10714f88: "cmpwi r0,0x31f", 0x10714fe4: "subfic r28,r0,0x31f",
    0x10715054: "cmpwi r0,0x257", 0x107150b0: "subfic r27,r0,0x257",
    0x10718f8c: "li r5,0x31f", 0x10718f90: "li r6,0x257",
    0x10718fa0: "li r5,0x31f", 0x10718fa4: "li r6,0x257",
    0x10719074: "li r5,0x31f", 0x10719078: "li r6,0x257",
    0x1071929c: "li r5,0x31f", 0x107192a0: "li r6,0x257",
    0x107192c8: "li r5,0x31f", 0x107192cc: "li r6,0x257",
    0x10719318: "li r5,0x31f", 0x1071931c: "li r6,0x257",
    0x1071b074: "cmpwi r0,0x31f", 0x1071b07c: "subfic r27,r4,0x31f",
    0x1071b0a0: "cmpwi r0,0x257", 0x1071b0a8: "subfic r30,r5,0x257",
    0x10725b04: "cmpwi r0,0x31f", 0x10725b10: "li r0,0x31f",
    0x10725b1c: "cmpwi r0,0x257", 0x10725b28: "li r0,0x257",
    0x1076add4: "li r3,0x320", 0x1076ae34: "li r4,0x320",
    0x10769c40: "subfic r3,r4,0x320",
    0x1074b0c4: "li r5,0x22b", 0x1074b0cc: "li r7,0x24e", 0x1074b100: "li r6,0x316",
}
ADAPTIVE_BACKGROUND_BOUNDARY = 0x10719120
