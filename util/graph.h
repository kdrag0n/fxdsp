#pragma once

#include <vector>
#include <span>

#include "../util/math_ext.h"

namespace fxdsp::graph {

void interpolate(std::vector<float>& in_x, std::vector<float>& in_y,
                 std::vector<float>& out_x, std::vector<float>& out_y,
                 float start_x = nan);

void impulse_response_curves(std::vector<float>& ir,
                             std::vector<float>& ir_out_x,
                             std::vector<float>& ir_out_y,
                             std::vector<float>& fr_out_x,
                             std::vector<float>& fr_out_y,
                             std::vector<float>& pr_out_x,
                             std::vector<float>& pr_out_y,
                             float max_freq);

}
