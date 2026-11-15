"""Original canvas constants used by the native adaptive tactics screen.

These are presentation feature fixups, not whole-module reconstruction owners.
Source-image dimensions, video setup and the message box width cap stay intact.
"""
ADAPTIVE_GUI_INSTRUCTIONS = {
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
