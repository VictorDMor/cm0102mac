// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "CM0102Mac",
    platforms: [.macOS(.v13)],
    products: [
        .library(name: "CMAssets", targets: ["CMAssets"]),
        .executable(name: "cm-assets", targets: ["CMAssetInspector"]),
        .executable(name: "cm-assets-checks", targets: ["CMAssetsChecks"])
    ],
    targets: [
        .target(name: "CMAssets"),
        .executableTarget(name: "CMAssetInspector", dependencies: ["CMAssets"]),
        .executableTarget(name: "CMAssetsChecks", dependencies: ["CMAssets"], path: "Tests/CMAssetsTests")
    ]
)
