#include "core/GeometryTypes.h"

#include <cmath>

namespace curvefit
{

Point3D operator+(const Point3D& a, const Point3D& b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Point3D operator-(const Point3D& a, const Point3D& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Point3D operator*(const Point3D& point, double scalar)
{
    return {point.x * scalar, point.y * scalar, point.z * scalar};
}

Point3D operator/(const Point3D& point, double scalar)
{
    return {point.x / scalar, point.y / scalar, point.z / scalar};
}

double dot(const Point3D& a, const Point3D& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Point3D cross(const Point3D& a, const Point3D& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

double length(const Point3D& point)
{
    return std::sqrt(dot(point, point));
}

Point3D normalize(const Point3D& point)
{
    const double pointLength = length(point);
    if (pointLength < 1e-12)
    {
        return {1.0, 0.0, 0.0};
    }
    return point / pointLength;
}

} // namespace curvefit

