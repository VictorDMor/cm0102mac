"""Pin read-only presentation observation points to the audited Mac program.

Digests contain no original executable payload. Original instructions always
execute after each observation; this is not a semantic replacement manifest.
"""
import hashlib
import json

DRAWING_BOUNDARY_DIGESTS = {
    0x1074f800: "e0d95beb2bcc3d6ab9fcea6d234239d106606c146d6d5ba0d675bce4a89edd57",
    0x1074fb58: "3e30a579799a8e17be61b546d29c12e520ad682b3866827952600a98b3220b26",
    0x107500b4: "2a0562b11548d7c929f1f65f9b0815f9eeb3cfbd6fa549ceb4c14c09d630ba16",
    0x107500e8: "4596ccc0324234e0920847a99f10021a2e9a5f1dc8629782388908ddc6cc2f8c",
    0x10750580: "417113e0a0f40eb185c7f15684fccba96e8821810b01612ae7a80bf8ece628ea",
    0x10750740: "65a8391a27fc79338c4ab85f7c842222328f0406d214f3dbd41d4e9c7322cf76",
    0x107509f0: "bb7be8f11b29192e75c3db1366cf895b96f46e1c2bf56edf9f0a1724f0fac03e",
    0x107620d0: "9af30be435a87cceb463c043748f43ba2f2210bb54aa18f14286f46f79c2f195",
    0x10769460: "51f0aeeadd0747c25622e72f522de973d0436031157f5b0f4aab540f24736b20",
    0x107696cc: "96bb9523a7e688af3b3f0b996a157a4046c91fc96ab16fa2010fdb0f458d6876",
    0x107697b0: "8f2663dbf055b5bec0e88e633d7cf02c2528e86dc58d545b9a66ba713b6b6089",
    0x107697c0: "3f3d45dc39d6fd5856d18bcb360bac9c8a19da3371409194df806cbfce3b24bb",
    0x107698c4: "f67c56ad2d7cd495f366e85c490ca1bd2a23d8fe69b626706561acb2948b82fe",
    0x10769980: "028990d443a75e71a5b5ecb0c714b84fe3723ea7da84261a3035c31dd37b2b9d",
    0x10769b48: "3d57ce7f394c275e1c50928746bc8990ed468f365ea9d467b05bff43c8f31b83",
    0x1076a140: "99670cbebf65e0461cfd9c79112f0d60fac03d8033cf1b384e0da9badd773e72",
    0x1076a570: "b89c3a11d3f0185deb8b81707722de67e49a79156163ad53bd0382f93e437b61",
    0x1076a580: "26c14e5963cbe665c5c2a9347e976655303a783163b9feea76ed1147b21fe12a",
    0x1076a784: "db46df38f0ef3f203145d43aff145debf05b62c0a65beea147ef3999af7dedd7",
    0x1076ad50: "0e1c8ab11d37acfab564ed5187377c419cdcd649d6e5dd03fb7814c27a8d9817",
    0x1076b014: "70ba5670c1d92fc0dd41db7961bdb94316cd170c7cf772923090b280c661787c",
    0x1076b020: "bf3d9b071280046363d6f024b86a6a41f2923559424d92390c4929d112176684",
    0x1076b138: "a20b4be2d0cd0b78c1dbe22894ddd5faf25d816dbc8c09d97b56507190e5d637",
    0x1076b140: "273286c266571f462f3b0c5c11380a8b89989a6249691ec833b631d0612d5dcf",
    0x1076b234: "c4995a25e714a6863d55567bbc73b6d436ecc6d7a2d318594731fb1106a05d6f",
    0x1076b7e0: "64420552d6dd77e2c4c1e0e9a73d0d7d1759baf5f4c68e1e28a1765779a5f1a6",
    0x1076c0b0: "4396007295f95fcd45194ab18d3524ab8ad6ce15c44fddda5cc439dd388ce8ad",
    0x1076c980: "a4feb611be74efbb41b3cbc2b6e1c111c5088f05ecc29294e17e400e2fbd9494",
    0x1076ca50: "595d22fd8c944e79d5fc4eb91ad728805adebc65c914d23f85aaa77a259e812b",
    0x1076d194: "9936627e4417f4a36ac916b57b013b19e0a1c4d6aa88c6a436751362158f7aa1",
}
NATIVE_DRAWING_BOUNDARIES = tuple(DRAWING_BOUNDARY_DIGESTS)


def require_native_drawing_contract(records):
    for record in records:
        address = record['address']
        if address not in DRAWING_BOUNDARY_DIGESTS:
            continue
        digest = hashlib.sha256(json.dumps(record, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        if digest != DRAWING_BOUNDARY_DIGESTS[address]:
            raise ValueError(f"Native drawing observation contract changed at {address:#x}")
