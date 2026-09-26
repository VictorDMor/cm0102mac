import AppKit
import CMAssets
import Foundation

struct EntryReport: Encodable {
    let entry: DatabaseEntry
    let exists: Bool
    let recordSize: Int?
}

struct ImageReport: Encodable {
    let filename: String
    let width: Int
    let height: Int
}

struct UnsupportedImage: Encodable {
    let filename: String
    let reason: String
}

struct Report: Encodable {
    let purpose = "Native asset validation only; the game engine is not implemented."
    let hostArchitecture: String
    let tables: [EntryReport]
    let images: [ImageReport]
    let unsupportedImages: [UnsupportedImage]
}

func savePNG(_ image: RGMImage, to url: URL) throws {
    guard let bitmap = NSBitmapImageRep(
        bitmapDataPlanes: nil, pixelsWide: image.width, pixelsHigh: image.height,
        bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
        colorSpaceName: .deviceRGB, bytesPerRow: image.width * 4, bitsPerPixel: 32
    ), let destination = bitmap.bitmapData else {
        throw AssetError.invalid("Cannot allocate image")
    }
    image.rgba.copyBytes(to: destination, count: image.rgba.count)
    guard let png = bitmap.representation(using: .png, properties: [:]) else {
        throw AssetError.invalid("Cannot encode PNG")
    }
    try png.write(to: url, options: .atomic)
}

do {
    let args = Array(CommandLine.arguments.dropFirst())
    guard args.count == 2 else {
        throw AssetError.invalid("Usage: cm-assets <original Data directory> <output directory>")
    }
    let source = URL(fileURLWithPath: args[0], isDirectory: true).standardizedFileURL.resolvingSymlinksInPath()
    let output = URL(fileURLWithPath: args[1], isDirectory: true).standardizedFileURL.resolvingSymlinksInPath()
    guard output != source, !output.path.hasPrefix(source.path + "/") else {
        throw AssetError.invalid("Output must be outside the original data directory")
    }
    let index = try DatabaseIndex(data: Data(contentsOf: source.appendingPathComponent("index.dam")))
    var entries: [EntryReport] = []
    for entry in index.entries {
        let url = source.appendingPathComponent(entry.filename)
        let exists = FileManager.default.fileExists(atPath: url.path)
        if !exists && entry.recordCount > 0 {
            throw AssetError.invalid("Missing table: \(entry.filename)")
        }
        let size = exists ? try entry.recordSize(in: Data(contentsOf: url)) : nil
        entries.append(EntryReport(entry: entry, exists: exists, recordSize: size))
    }
    try FileManager.default.createDirectory(at: output, withIntermediateDirectories: true)
    let imageURLs = try FileManager.default.contentsOfDirectory(at: source, includingPropertiesForKeys: nil)
        .filter { $0.pathExtension.lowercased() == "rgm" }.sorted { $0.path < $1.path }
    var images: [ImageReport] = []
    var unsupportedImages: [UnsupportedImage] = []
    for url in imageURLs {
        let image: RGMImage
        do {
            image = try RGMImage(data: Data(contentsOf: url))
        } catch let error as AssetError {
            unsupportedImages.append(UnsupportedImage(filename: url.lastPathComponent, reason: error.description))
            continue
        }
        try savePNG(image, to: output.appendingPathComponent(url.deletingPathExtension().lastPathComponent + ".png"))
        images.append(ImageReport(filename: url.lastPathComponent, width: image.width, height: image.height))
    }
    #if arch(arm64)
    let architecture = "arm64"
    #else
    let architecture = "non-arm64"
    #endif
    let report = Report(hostArchitecture: architecture, tables: entries, images: images, unsupportedImages: unsupportedImages)
    let encoder = JSONEncoder()
    encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
    try encoder.encode(report).write(to: output.appendingPathComponent("assets.json"), options: .atomic)
    print("Validated \(entries.count) index entries and decoded \(images.count) images on \(architecture); \(unsupportedImages.count) unsupported images.")
    print("This validates asset loading, not game execution.")
} catch {
    FileHandle.standardError.write(Data("\(error)\n".utf8))
    exit(1)
}
