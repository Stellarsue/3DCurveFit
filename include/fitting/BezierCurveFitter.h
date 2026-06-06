#pragma once

#include "core/GeometryTypes.h"
#include "fitting/CurveFitResult.h"

#include <vector>

namespace curvefit
{

CurveFitResult fitBezierCurve(
    const std::vector<CenterPoint>& centers,
    int sampleCount,
    int maxControlCount = 8);

} // namespace curvefit

