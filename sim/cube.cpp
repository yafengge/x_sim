#include "cube.h"

Cube::Cube(const SystolicConfig& cfg, std::shared_ptr<Clock> external_clock)
        : config_(cfg),
            clock_(external_clock ? external_clock : std::make_shared<Clock>()),
            systolic_(config_, clock_) {
        // Cube 顶层当前仅封装脉动阵列的初始化，后续可在此扩展更多子模块。
}
