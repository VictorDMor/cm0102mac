#include "form_layout.hpp"
#include <yoga/Yoga.h>
#include <array>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace cm::aot {
namespace {
// Keep Yoga's public C API private to this translation unit. Nodes are owned
// individually, so partial construction also releases them before the config.
struct Tree {
    using Node = std::unique_ptr<YGNode, decltype(&YGNodeFree)>;
    std::unique_ptr<YGConfig, decltype(&YGConfigFree)> config{YGConfigNew(), YGConfigFree};
    std::vector<Node> nodes;
    Tree() { YGConfigSetPointScaleFactor(config.get(), 1); }
    YGNodeRef node(YGNodeRef parent = nullptr) {
        Node owned{YGNodeNewWithConfig(config.get()), YGNodeFree};
        auto result = owned.get();
        nodes.push_back(std::move(owned));
        if (parent) YGNodeInsertChild(parent, result, YGNodeGetChildCount(parent));
        return result;
    }
};
ScreenRect bounds(YGNodeRef node, ScreenRect parent) {
    const auto x = parent.left + int(std::lround(YGNodeLayoutGetLeft(node)));
    const auto y = parent.top + int(std::lround(YGNodeLayoutGetTop(node)));
    return {x, y, x + int(std::lround(YGNodeLayoutGetWidth(node))),
            y + int(std::lround(YGNodeLayoutGetHeight(node)))};
}
}
std::vector<ScreenRect> form_tracks(ScreenRect available, std::span<const FormTrack> tracks, bool horizontal) {
    Tree tree;
    const auto root = tree.node();
    YGNodeStyleSetWidth(root, available.width());
    YGNodeStyleSetHeight(root, available.height());
    YGNodeStyleSetFlexDirection(root, horizontal ? YGFlexDirectionRow : YGFlexDirectionColumn);
    for (const auto track : tracks) {
        const auto child = tree.node(root);
        YGNodeStyleSetFlexBasis(child, track.minimum);
        YGNodeStyleSetFlexGrow(child, track.grow);
        if (horizontal) {
            YGNodeStyleSetMinWidth(child, track.minimum);
            if (track.maximum) YGNodeStyleSetMaxWidth(child, track.maximum);
        } else {
            YGNodeStyleSetMinHeight(child, track.minimum);
            if (track.maximum) YGNodeStyleSetMaxHeight(child, track.maximum);
        }
    }
    YGNodeCalculateLayout(root, available.width(), available.height(), YGDirectionLTR);
    std::vector<ScreenRect> result;
    result.reserve(tracks.size());
    for (unsigned i = 0; i < tracks.size(); ++i) result.push_back(bounds(YGNodeGetChild(root, i), available));
    return result;
}
ReadingLayout reading_layout(ScreenRect available, ReadingMetrics metrics) {
    const std::array<FormTrack,7> tracks{{
        {metrics.minimum_list_height,1,metrics.maximum_list_height},{metrics.gap},
        {metrics.toolbar_height},{metrics.gap},{metrics.headline_height},{metrics.gap},
        {metrics.minimum_body_height,2}}};
    const auto boxes=form_tracks(available,tracks,false);
    return {boxes[0],boxes[2],boxes[4],boxes[6]};
}
FormLayout form_layout(ScreenRect available, FormMetrics metrics) {
    Tree tree;
    const auto root = tree.node();
    YGNodeStyleSetWidth(root, available.width());
    YGNodeStyleSetHeight(root, available.height());
    YGNodeStyleSetAlignItems(root, YGAlignCenter);
    YGNodeStyleSetJustifyContent(root, YGJustifyCenter);
    const auto panel = tree.node(root);
    YGNodeStyleSetWidthPercent(panel, 100);
    YGNodeStyleSetHeightPercent(panel, 100);
    YGNodeStyleSetMaxWidth(panel, metrics.maximum_width);
    YGNodeStyleSetMaxHeight(panel, metrics.maximum_height);
    YGNodeStyleSetPadding(panel, YGEdgeAll, 10);
    const auto title = tree.node(panel);
    YGNodeStyleSetHeight(title, metrics.title_height);
    YGNodeStyleSetMargin(title, YGEdgeBottom, 10);
    const auto toolbar = tree.node(panel);
    YGNodeStyleSetHeight(toolbar, metrics.toolbar_height);
    YGNodeStyleSetFlexDirection(toolbar, YGFlexDirectionRow);
    YGNodeStyleSetJustifyContent(toolbar, YGJustifySpaceBetween);
    const auto leading = tree.node(toolbar), trailing = tree.node(toolbar);
    YGNodeStyleSetWidth(leading, metrics.toolbar_leading_width);
    YGNodeStyleSetWidth(trailing, metrics.toolbar_trailing_width);
    const auto body = tree.node(panel);
    YGNodeStyleSetFlexGrow(body, 1);
    YGNodeStyleSetMargin(body, YGEdgeTop, 15);
    YGNodeStyleSetMargin(body, YGEdgeBottom, 20);
    YGNodeStyleSetMargin(body, YGEdgeHorizontal, 10);
    YGNodeStyleSetPadding(body, YGEdgeAll, 5);
    const auto fields = tree.node(body);
    YGNodeStyleSetFlexGrow(fields, 1);
    const auto footer = tree.node(panel);
    YGNodeStyleSetHeight(footer, metrics.footer_height);
    YGNodeStyleSetWidthPercent(footer, 100);
    YGNodeStyleSetMaxWidth(footer, metrics.footer_width);
    YGNodeStyleSetAlignSelf(footer, YGAlignFlexEnd);
    YGNodeCalculateLayout(root, available.width(), available.height(), YGDirectionLTR);
    const auto panel_rect = bounds(panel, available), body_rect = bounds(body, panel_rect);
    const auto toolbar_rect = bounds(toolbar, panel_rect);
    return {panel_rect, bounds(title, panel_rect), bounds(leading, toolbar_rect),
            bounds(trailing, toolbar_rect), body_rect, bounds(fields, body_rect), bounds(footer, panel_rect)};
}
}
