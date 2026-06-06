#pragma once

#include "core/GeometryTypes.h"
#include "fitting/CurveFitResult.h"

#include <vector>

namespace curvefit
{

CurveFitResult fitPolynomialCurve(
    const std::vector<CenterPoint>& centers,
    int degree,
    int sampleCount);

} // namespace curvefit

