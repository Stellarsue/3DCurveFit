#include "fitting/BezierCurveFitter.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace curvefit
{
namespace
{

std::vector<Point3D> selectControlPoints(
    const std::vector<CenterPoint>& centers,
    int maxControlCount)
{
    const int controlCount = std::min(maxControlCount, static_cast<int>(centers.size()));
    if (controlCount < 2)
    {
        throw std::runtime_error("贝塞尔曲线至少需要 2 个中心代表点");
    }

    std::vector<Point3D> controlPoints;
    controlPoints.reserve(controlCount);
    for (int i = 0; i < controlCount; ++i)
    {
        const double alpha = static_cast<double>(i) / static_cast<double>(controlCount - 1);
        const int centerIndex = static_cast<int>(
            std::round(alpha * static_cast<double>(centers.size() - 1)));
        controlPoints.push_back(centers[centerIndex].point);
    }
    return controlPoints;
}

Point3D evaluateDeCasteljau(std::vector<Point3D> controlPoints, double parameter)
{
    for (std::size_t level = controlPoints.size() - 1; level > 0; --level)
    {
        for (std::size_t i = 0; i < level; ++i)
        {
            controlPoints[i] =
                controlPoints[i] * (1.0 - parameter)
                + controlPoints[i + 1] * parameter;
        }
    }
    return controlPoints.front();
}

} // namespace

CurveFitResult fitBezierCurve(
    const std::vector<CenterPoint>& centers,
    int sampleCount,
    int maxControlCount)
{
    if (sampleCount < 2 || maxControlCount < 2)
    {
        throw std::runtime_error("贝塞尔曲线拟合参数不合法");
    }

    const std::vector<Point3D> controlPoints =
        selectControlPoints(centers, maxControlCount);

    CurveFitResult result;
    result.methodName = "贝塞尔曲线拟合";
    result.color = {0.7f, 0.55f, 1.0f, 1.0f};
    result.curvePoints.reserve(sampleCount);

    for (int i = 0; i < sampleCount; ++i)
    {
        const double parameter =
            static_cast<double>(i) / static_cast<double>(sampleCount - 1);
        result.curvePoints.push_back(
            evaluateDeCasteljau(controlPoints, parameter));
    }

    std::ostringstream details;
    details << "阶数: " << controlPoints.size() - 1
            << "\n控制点: " << controlPoints.size() << " 个，来自中心代表点等距抽样"
            << "\n采样点: " << result.curvePoints.size()
            << "\n说明: 使用 de Casteljau 算法生成全局贝塞尔趋势曲线。";
    result.details = details.str();
    return result;
}

} // namespace curvefit

