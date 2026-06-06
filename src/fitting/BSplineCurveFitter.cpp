#include "fitting/BSplineCurveFitter.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace curvefit
{
namespace
{

Point3D evaluateUniformCubicSegment(
    const Point3D& p0,
    const Point3D& p1,
    const Point3D& p2,
    const Point3D& p3,
    double parameter)
{
    const double parameter2 = parameter * parameter;
    const double parameter3 = parameter2 * parameter;
    const double basis0 = (1.0 - 3.0 * parameter + 3.0 * parameter2 - parameter3) / 6.0;
    const double basis1 = (4.0 - 6.0 * parameter2 + 3.0 * parameter3) / 6.0;
    const double basis2 = (1.0 + 3.0 * parameter + 3.0 * parameter2 - 3.0 * parameter3) / 6.0;
    const double basis3 = parameter3 / 6.0;
    return p0 * basis0 + p1 * basis1 + p2 * basis2 + p3 * basis3;
}

} // namespace

CurveFitResult fitBSplineCurve(
    const std::vector<CenterPoint>& centers,
    int sampleCount)
{
    if (centers.size() < 4 || sampleCount < 2)
    {
        throw std::runtime_error("三次 B 样条至少需要 4 个中心代表点");
    }

    std::vector<Point3D> controlPoints;
    controlPoints.reserve(centers.size());
    for (const CenterPoint& center : centers)
    {
        controlPoints.push_back(center.point);
    }

    const int segmentCount = static_cast<int>(controlPoints.size()) - 3;

    CurveFitResult result;
    result.methodName = "三次均匀 B 样条拟合";
    result.color = {0.1f, 0.85f, 1.0f, 1.0f};
    result.curvePoints.reserve(sampleCount);

    for (int i = 0; i < sampleCount; ++i)
    {
        const double alpha = static_cast<double>(i) / static_cast<double>(sampleCount - 1);
        const double scaledParameter = alpha * static_cast<double>(segmentCount);
        const int segment = std::min(
            segmentCount - 1,
            static_cast<int>(std::floor(scaledParameter)));
        const double localParameter =
            (segment == segmentCount - 1 && i == sampleCount - 1)
            ? 1.0
            : scaledParameter - static_cast<double>(segment);

        result.curvePoints.push_back(evaluateUniformCubicSegment(
            controlPoints[segment],
            controlPoints[segment + 1],
            controlPoints[segment + 2],
            controlPoints[segment + 3],
            localParameter));
    }

    std::ostringstream details;
    details << "阶数: 3"
            << "\n控制点: " << controlPoints.size() << " 个中心代表点"
            << "\n曲线段: " << segmentCount
            << "\n采样点: " << result.curvePoints.size()
            << "\n说明: 使用三次均匀 B 样条平滑中心趋势。";
    result.details = details.str();
    return result;
}

} // namespace curvefit

