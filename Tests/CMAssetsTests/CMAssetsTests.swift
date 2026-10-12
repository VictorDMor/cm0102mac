import Foundation
import CMAssets

private func check(_ condition: Bool, _ message: String) throws {
    if !condition { throw AssetError.invalid("Test failed: \(message)") }
}

private func rejects(_ body: () throws -> Void) throws {
    do { try body() } catch is AssetError { return }
    throw AssetError.invalid("Test failed: malformed asset was accepted")
}

private func write32(_ value: UInt32, at offset: Int, in data: inout Data, little: Bool = false) {
    let shifts = little ? [0, 8, 16, 24] : [24, 16, 8, 0]
    data.replaceSubrange(offset..<offset + 4, with: shifts.map { UInt8(truncatingIfNeeded: value >> $0) })
}

private func indexFixture(filename: String = "players staff.dam") -> Data {
    var data = Data(repeating: 0, count: 76)
    data.replaceSubrange(8..<8 + filename.utf8.count, with: filename.utf8)
    write32(10, at: 60, in: &data)
    write32(2, at: 64, in: &data)
    write32(2, at: 72, in: &data)
    return data
}

func bigEndianIndexAndRecordBoundary() throws {
    let index = try DatabaseIndex(data: indexFixture())
    try check(index.entries.count == 1, "index entry count")
    let table = index.entries[0]
    try check(table.tableID == 10, "big-endian table ID")
    try check(table.recordCount == 2, "big-endian record count")
    var records = Data(count: 148)
    write32(0, at: 0, in: &records)
    write32(1, at: 72, in: &records)
    write32(0xf1fffff1, at: 144, in: &records)
    try check(try table.recordSize(in: records) == 72, "records begin at zero and have a four-byte end marker")
    var wrongMarker = records
    write32(0, at: 144, in: &wrongMarker)
    try rejects { _ = try table.recordSize(in: wrongMarker) }
    var misplacedMarker = records
    write32(0xf1fffff1, at: 0, in: &misplacedMarker)
    write32(0, at: 144, in: &misplacedMarker)
    try rejects { _ = try table.recordSize(in: misplacedMarker) }
    try rejects { _ = try table.recordSize(in: Data(count: 147)) }
}

func rejectsDamagedAndUnsafeIndex() throws {
    try rejects { _ = try DatabaseIndex(data: indexFixture().dropLast()) }
    try rejects { _ = try DatabaseIndex(data: indexFixture(filename: "../secret")) }
    var duplicate = indexFixture()
    duplicate.append(indexFixture().dropFirst(8))
    try rejects { _ = try DatabaseIndex(data: duplicate) }
}

private func imageFixture(masks: [UInt32], pixels: [UInt16]) -> Data {
    var data = Data(repeating: 0, count: 48 + pixels.count * 2)
    write32(UInt32(pixels.count), at: 0, in: &data)
    write32(1, at: 4, in: &data)
    write32(UInt32(pixels.count * 2), at: 8, in: &data)
    write32(16, at: 28, in: &data, little: true)
    for i in 0..<3 { write32(masks[i], at: 32 + i * 4, in: &data, little: true) }
    for (i, pixel) in pixels.enumerated() {
        data[48 + i * 2] = UInt8(truncatingIfNeeded: pixel >> 8)
        data[49 + i * 2] = UInt8(truncatingIfNeeded: pixel)
    }
    return data
}

func ignoresObsoleteMasksAndDecodesMacPixels() throws {
    // This is intentional: the original Mac disc advertises stale RGB565 masks
    // while its payload is big-endian RGB555. White 0x7fff must stay white.
    let image = try RGMImage(data: imageFixture(masks: [0xf800, 0x07e0, 0x001f], pixels: [0x7c00, 0x03e0, 0x001f]))
    try check(image.width == 3 && image.height == 1, "big-endian RGM dimensions")
    try check(Array(image.rgba) == [255,0,0,255, 0,255,0,255, 0,0,255,255], "big-endian Mac RGB555 pixels")
}

func supports555AndRejectsTruncationOrHugeAllocation() throws {
    var data = imageFixture(masks: [0x7c00, 0x03e0, 0x001f], pixels: [0x7fff])
    try check(Array(try RGMImage(data: data).rgba) == [255,255,255,255], "RGB555 pixels")
    data.replaceSubrange(12..<48, with: repeatElement(UInt8(0xa5), count: 36))
    try check(Array(try RGMImage(data: data).rgba) == [255,255,255,255], "obsolete descriptor is ignored")
    try rejects { _ = try RGMImage(data: data.dropLast()) }
    write32(UInt32.max, at: 0, in: &data)
    try rejects { _ = try RGMImage(data: data) }
}

do {
    // The Command Line Tools include Swift but no XCTest/Testing module. Keeping
    // these deterministic checks executable avoids requiring a global Xcode change.
    try bigEndianIndexAndRecordBoundary()
    try rejectsDamagedAndUnsafeIndex()
    try ignoresObsoleteMasksAndDecodesMacPixels()
    try supports555AndRejectsTruncationOrHugeAllocation()
    print("4 asset validation groups passed.")
} catch {
    FileHandle.standardError.write(Data("\(error)\n".utf8))
    exit(1)
}
