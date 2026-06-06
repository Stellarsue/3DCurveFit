#pragma once

namespace curvefit
{

struct Point3D
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct PCAResult
{
    Point3D mean;
    Point3D direction;
};

struct CenterPoint
{
    double t = 0.0;
    Point3D point;
    int count = 0;
};

Point3D operator+(const Point3D& a, const Point3D& b);
Point3D operator-(const Point3D& a, const Point3D& b);
Point3D operator*(const Point3D& point, double scalar);
Point3D operator/(const Point3D& point, double scalar);

double dot(const Point3D& a, const Point3D& b);
Point3D cross(const Point3D& a, const Point3D& b);
double length(const Point3D& point);
Point3D normalize(const Point3D& point);

} // namespace curvefit

