import Foundation

public struct DatabaseEntry: Codable, Equatable, Sendable {
    public let filename: String
    public let tableID: UInt32
    public let recordCount: UInt32
    public let storedOffset: UInt32
    public let storedType: UInt32

    // The supplied Mac media has a four-byte prefix followed by fixed-size records.
    // A divisible size establishes structural consistency, not field semantics.
    public func recordSize(in data: Data) throws -> Int? {
        guard recordCount > 0 else { return nil }
        guard data.count >= 4, (data.count - 4) % Int(recordCount) == 0 else {
            throw AssetError.invalid("Record count does not match \(filename)")
        }
        let size = (data.count - 4) / Int(recordCount)
        guard size > 0 else { throw AssetError.invalid("Empty records in \(filename)") }
        return size
    }
}

public struct DatabaseIndex: Sendable {
    public let entries: [DatabaseEntry]

    public init(data: Data) throws {
        let bytes = Bytes(data)
        guard bytes.count >= 8, (bytes.count - 8) % 68 == 0 else {
            throw AssetError.invalid("Expected an eight-byte prefix and 68-byte index records")
        }
        guard try bytes.u32(0) == 0, try bytes.u32(4) == 0 else {
            throw AssetError.invalid("Unsupported database index prefix")
        }
        var result: [DatabaseEntry] = []
        var seenIDs = Set<UInt32>()
        var seenNames = Set<String>()
        for offset in stride(from: 8, to: bytes.count, by: 68) {
            let name = try bytes.string(offset, 52)
            guard !name.isEmpty, name != ".", name != "..",
                  !name.contains("/"), !name.contains("\\"), !name.contains(":"),
                  seenNames.insert(name).inserted else {
                throw AssetError.invalid("Invalid or duplicate database filename")
            }
            let entry = try DatabaseEntry(
                filename: name, tableID: bytes.u32(offset + 52),
                recordCount: bytes.u32(offset + 56), storedOffset: bytes.u32(offset + 60),
                storedType: bytes.u32(offset + 64)
            )
            guard seenIDs.insert(entry.tableID).inserted else {
                throw AssetError.invalid("Duplicate database table ID")
            }
            result.append(entry)
        }
        entries = result
    }
}
