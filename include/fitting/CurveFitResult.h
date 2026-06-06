#pragma once

#include "core/GeometryTypes.h"

#include <array>
#include <string>
#include <vector>

namespace curvefit
{

struct CurveFitResult
{
    std::string methodName;
    std::string details;
    std::vector<Point3D> curvePoints;
    std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
};

} // namespace curvefit

