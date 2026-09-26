import Foundation

public enum AssetError: Error, CustomStringConvertible, Equatable {
    case invalid(String)
    public var description: String {
        switch self { case .invalid(let message): return message }
    }
}

struct Bytes {
    let values: [UInt8]
    init(_ data: Data) { values = Array(data) }
    var count: Int { values.count }

    func require(_ offset: Int, _ length: Int) throws {
        guard offset >= 0, length >= 0, offset <= count, length <= count - offset else {
            throw AssetError.invalid("Truncated asset at offset \(offset), length \(length)")
        }
    }

    func u32(_ offset: Int, bigEndian: Bool = true) throws -> UInt32 {
        try require(offset, 4)
        let a = values[offset..<offset + 4]
        return (bigEndian ? Array(a) : a.reversed()).reduce(0) { ($0 << 8) | UInt32($1) }
    }

    func string(_ offset: Int, _ length: Int, encoding: String.Encoding = .macOSRoman) throws -> String {
        try require(offset, length)
        let content = values[offset..<offset + length].prefix { $0 != 0 }
        guard let text = String(bytes: content, encoding: encoding) else {
            throw AssetError.invalid("Invalid string at offset \(offset)")
        }
        return text
    }
}
