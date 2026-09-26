import Foundation

/// Decodes the uncompressed, 16-bit RGM files observed on the Mac retail disc.
/// Dimensions, payload length, and QuickDraw RGB555 pixels are big-endian.
/// Bytes 12...47 retain an obsolete descriptor from the converted image; some
/// files still advertise Windows RGB565 masks, and feral.rgm contains unrelated
/// values there. Those words do not describe the Mac payload and are not used.
public struct RGMImage: Sendable {
    public let width: Int
    public let height: Int
    public let rgba: Data

    public init(data: Data) throws {
        let bytes = Bytes(data)
        try bytes.require(0, 48)
        let width = Int(try bytes.u32(0))
        let height = Int(try bytes.u32(4))
        guard width > 0, height > 0, width <= 8192, height <= 8192,
              width * height <= 16_777_216 else {
            throw AssetError.invalid("Unsupported RGM dimensions")
        }
        let pixelBytes = width * height * 2
        guard try bytes.u32(8) == pixelBytes, bytes.count == 48 + pixelBytes else {
            throw AssetError.invalid("RGM payload does not match dimensions")
        }
        var output = [UInt8]()
        output.reserveCapacity(width * height * 4)
        for offset in stride(from: 48, to: bytes.count, by: 2) {
            let pixel = UInt32(bytes.values[offset]) << 8 | UInt32(bytes.values[offset + 1])
            for shift in [10, 5, 0] {
                let value = (pixel >> shift) & 31
                output.append(UInt8((value * 255 + 15) / 31))
            }
            output.append(255)
        }
        self.width = width
        self.height = height
        self.rgba = Data(output)
    }
}
