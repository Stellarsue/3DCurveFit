#pragma once

#include "core/GeometryTypes.h"
#include "fitting/CurveFitResult.h"

#include <vector>

namespace curvefit
{

CurveFitResult fitBSplineCurve(
    const std::vector<CenterPoint>& centers,
    int sampleCount);

} // namespace curvefit

