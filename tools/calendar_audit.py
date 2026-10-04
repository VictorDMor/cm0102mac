"""Pin the Mac calendar instruction contract used by the semantic adapter."""
import hashlib
import json

CONTRACTS = (
    (0x10715B20, 0x10715BC0, "faab9fb0f48231766f5e9b0595dd109a2be7639b984ec830b14021ad64892c43"),
    (0x107182F0, 0x10718354, "a496c80a33e42a50319536aa245bb4a3a2d2fd64363d1a5e13f277b89ada57a5"),
    (0x10718360, 0x10718438, "aa64b3cf2df3f13dba6c660b2a102a3d98c0492b60a5b63b603df3e12a747798"),
    (0x103317F0, 0x10331858, "e93ed9feee78922e44a69beafbc099f515429b887d7ead794132f8bb59a1f18b"),
    (0x10331860, 0x103318C8, "5a74c4775a352a92e713d648900ade2eae03254c96bb84200d5b2378f24b29d2"),
    (0x10718130, 0x107182F0, "caf9175e8cf1dcf26a7cdd61e644ab9f38576ef2516f3208d1938e36b42fb30b"),
    (0x10717890, 0x10717A0C, "eece70ad391c75b1d55fa58bfbdac11cf92dbeb51fcd619c426b53872eea3021"),
    (0x10717B20, 0x10717CA8, "1d4bea0c9cdbac143a0d55b455d8f3f485a41f1774eb9d1115f328ff626af838"),
    (0x10718440, 0x107184CC, "3cd3bf960d288bf41c7d004205b57d0399a315382c3d7f97822f5b431e1d83e5"),
    (0x107156b0, 0x107156f0, "b5afb42c7cf2f9b2e7700106534f771e3d59aafbbeab5c46df18e96f41433f65"),
    (0x107156f0, 0x1071587c, "08e6abaf2f8492455b469905a1909c7e824ac6f163d2f7d6ac1e83261bfaf1e0"),
    (0x10715880, 0x1071589c, "2c5d4b8b0ff83a4158f0774345d56f0cf44ac6ec826142aa7224c70bf9dccf97"),
    (0x107158a0, 0x10715a28, "aec1204dbb39a1ee4bd0311231a7d2e49680176787fbebf98f068b32f66866e8"),
    (0x10715a30, 0x10715b1c, "bb8d3fd075cea7d4187bfbeca8bcd9a73185ee8f731dd2ebf6b077166f0381c9"),
    (0x10715bc0, 0x10715df4, "55ae2cea35ff6b36d12b9f22f476e189ca749a740fc23451ee54518ed7ea1aaf"),
    (0x10715e00, 0x1071612c, "daa986f528ed2b54a5cd4ced5ff99446bd4dc1523eff1a036171513259374e7b"),
    (0x10716130, 0x107164ec, "f867c1f447d84346b368f2e15f8368e8361ee58b6bde32d234bc2ec8bdbedd03"),
    (0x107164f0, 0x1071689c, "350c0ef220af4a826385f10befd96fb425687e51099c24cb6a947a4ed3e67ab6"),
    (0x107168a0, 0x10716afc, "3acb83e5419540eb4a12342ca212daf79aedcdfbf4744a896c4c05cd80e97a3f"),
    (0x10716b00, 0x10716ea4, "48efde613c9ee9d67341582f635b09fdee63863b34cefa5420bfca563ce2e96d"),
    (0x10716eb0, 0x10717268, "693678498117e0afd7883bbf173245b5e01386361cc11baac4d7dddd8f40f94f"),
    (0x10717270, 0x10717620, "f65b4cef907f4ddff01354eb91edb8628224e74f51df1f726b061dae3ef2a49c"),
    (0x10717620, 0x10717888, "646d1966f1e0062de505dab91c3dc80c4ec39156f081e8fde34ea2f90938df9e"),
    (0x10717a10, 0x10717b1c, "72f98364db6921154752f63dd87dff3143e30cd73021c533ed4b5e251b7b6b4a"),
    (0x10717cb0, 0x10717dc4, "9308b6d394ccc043f08c1c7e29f541bf8ce58968f69c83f70d9dfdd18c7f932b"),
    (0x10717dd0, 0x10718130, "a4d549b50687ae112560fd1fdf290a07c82df06947a212c5739969bca9603684"),
    (0x107184d0, 0x10718570, "05d7437c508319a8a53bbc717d60ab71a850a284be757b2895d23e8d10d8ad24"),
    (0x10718570, 0x1071864c, "1154241d88fdf3b8999908a55ac1af7a98c4c47fb2afbff9dfefcfce52a43eb6"),
    (0x10718650, 0x107186f8, "c981f82653f217d24b5caa7a54f73076b6c3cca490806d5085265a490b3f7908"),
    (0x10718700, 0x107187cc, "9dd95532b99ddd2e004ec7e109d2dcc0b413f4f929fc535a3a29fbd477ea3675"),
    (0x107187d0, 0x1071889c, "a61241b4ef4fce607a4e46b64c00a39464cc636f85eae1ea2b9af1667d00fdb2"),
    (0x107188a0, 0x107189c4, "0f336655908058de27f7ba78b1d5365cfe136f635188e68a56bd394f654d328a"),
    (0x107189d0, 0x10718af0, "c8ac0223a1f8592232ddc9ae68ae2c52e61bd4c5347ebd47e16b39aa97c97a4b"),
    (0x10718af0, 0x10718b38, "e1ccc2c870ee2201ee0c7a0801256828981971e92f1c99459699fdac00817218"),
)


# Two formatters span generated pages. Pin each complete page intersection as
# well as the full-body contract, rather than pretending the first page holds
# the whole body. The constants were derived from the verified Mac inventory.
CONTRACT_PAGE_PARTS = {
    (0x10715e00, 0x10715): "62ae6a3aef71e3a1ef12be04b6aa8e7cb2442c45a8ac4f62b11da1b5becf2dd3",
    (0x10715e00, 0x10716): "e5d46d869f67644dd612f360a2502e43c324fb12641095f8db0b9f54287f1c76",
    (0x10716eb0, 0x10716): "171d91b3d058648eba976c1f934a6c370d94c0f5913f6da2cf8e41b73ca49efa",
    (0x10716eb0, 0x10717): "7c0766b652a4f84df7285da9241af8e6bc4658c6912b4a112a464364dca7e382",
    (0x10717dd0, 0x10717): "8c52e2d0847c8465bad216c096765f27ad47021bc7edd907f74e7958d62b8dea",
    (0x10717dd0, 0x10718): "8e13f8eccdbf346140d6de949a0bacbbf69f37e95fef54fdd51cb29a33bc1607",
}


def require_calendar_contract(page, instructions):
    for start, end, fingerprint in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        lower, upper = max(start, page << 12), min(end, (page + 1) << 12)
        records = [{key: item.get(key) for key in ("address", "assembly", "ops", "flows", "fallthrough")}
                   for item in instructions if lower <= item["address"] < upper]
        # Bounded unrelated exports need not include a Date body. An included
        # fragment requires its complete audited page intersection.
        if not records:
            continue
        expected = CONTRACT_PAGE_PARTS.get((start, page), fingerprint)
        actual = hashlib.sha256(json.dumps(records, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
        if actual != expected:
            raise ValueError(f"Mac calendar contract changed at {start:x} page {page:x}")

DATE_SUBSYSTEM_ENTRIES = (275863216, 275863280, 275863680, 275863712, 275864112, 275864512, 275865088, 275865904, 275866864, 275867808, 275868416, 275869360, 275870320, 275871264, 275872272, 275872944, 275873232, 275874880, 275875024, 275875184, 275875408, 275875584, 275875792, 275876000, 275876304, 275876592)
