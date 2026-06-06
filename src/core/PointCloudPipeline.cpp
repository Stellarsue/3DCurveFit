#include "core/PointCloudPipeline.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>

namespace curvefit
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

Point3D centerLine(double t)
{
    return {
        t,
        2.0 * std::sin(0.8 * t),
        0.3 * t + 1.5 * std::cos(0.5 * t)
    };
}

Point3D centerLineTangent(double t)
{
    return normalize({
        1.0,
        1.6 * std::cos(0.8 * t),
        0.3 - 0.75 * std::sin(0.5 * t)
    });
}

double projectToMainDirection(const Point3D& point, const PCAResult& pca)
{
    return dot(point - pca.mean, pca.direction);
}

} // namespace

void generatePointCloudData(const std::string& fileName)
{
    constexpr int centerSampleCount = 520;
    constexpr int pointsPerCenter = 8;
    constexpr double tMin = -6.0;
    constexpr double tMax = 6.0;
    constexpr double ropeRadius = 0.28;

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> unitDist(0.0, 1.0);
    std::uniform_real_distribution<double> angleDist(0.0, 2.0 * kPi);
    std::normal_distribution<double> measurementNoise(0.0, 0.035);
    std::normal_distribution<double> axialNoise(0.0, 0.025);

    std::ofstream output(fileName);
    if (!output)
    {
        throw std::runtime_error("无法创建点云文件: " + fileName);
    }

    output << std::fixed << std::setprecision(8);

    for (int i = 0; i < centerSampleCount; ++i)
    {
        const double alpha = static_cast<double>(i) / static_cast<double>(centerSampleCount - 1);
        const double t = tMin + (tMax - tMin) * alpha;
        const Point3D center = centerLine(t);
        const Point3D tangent = centerLineTangent(t);

        // 在中心线切向量的法平面内构造两个正交方向。
        const Point3D helper = (std::abs(dot(tangent, {0.0, 0.0, 1.0})) < 0.9)
            ? Point3D{0.0, 0.0, 1.0}
            : Point3D{0.0, 1.0, 0.0};
        const Point3D normal1 = normalize(cross(tangent, helper));
        const Point3D normal2 = normalize(cross(tangent, normal1));

        for (int j = 0; j < pointsPerCenter; ++j)
        {
            const double angle = angleDist(rng);
            const double radius = ropeRadius * std::sqrt(unitDist(rng));
            const double offset1 = radius * std::cos(angle);
            const double offset2 = radius * std::sin(angle);

            const Point3D point = center
                + normal1 * offset1
                + normal2 * offset2
                + tangent * axialNoise(rng)
                + Point3D{measurementNoise(rng), measurementNoise(rng), measurementNoise(rng)};

            output << point.x << ' ' << point.y << ' ' << point.z << '\n';
        }
    }
}

std::vector<Point3D> loadPointCloud(const std::string& fileName)
{
    std::ifstream input(fileName);
    if (!input)
    {
        throw std::runtime_error("无法读取点云文件: " + fileName);
    }

    std::vector<Point3D> points;
    Point3D point;
    while (input >> point.x >> point.y >> point.z)
    {
        points.push_back(point);
    }

    if (points.empty())
    {
        throw std::runtime_error("点云文件为空或格式不正确: " + fileName);
    }

    return points;
}

PCAResult estimateMainDirectionPCA(const std::vector<Point3D>& points)
{
    if (points.empty())
    {
        throw std::runtime_error("点云为空，无法估计 PCA 主方向");
    }

    Point3D mean;
    for (const Point3D& point : points)
    {
        mean = mean + point;
    }
    mean = mean / static_cast<double>(points.size());

    double covariance[3][3] = {};
    for (const Point3D& point : points)
    {
        const Point3D delta = point - mean;
        covariance[0][0] += delta.x * delta.x;
        covariance[0][1] += delta.x * delta.y;
        covariance[0][2] += delta.x * delta.z;
        covariance[1][0] += delta.y * delta.x;
        covariance[1][1] += delta.y * delta.y;
        covariance[1][2] += delta.y * delta.z;
        covariance[2][0] += delta.z * delta.x;
        covariance[2][1] += delta.z * delta.y;
        covariance[2][2] += delta.z * delta.z;
    }

    const double inverseCount = 1.0 / static_cast<double>(points.size());
    for (auto& row : covariance)
    {
        for (double& value : row)
        {
            value *= inverseCount;
        }
    }

    Point3D direction{1.0, 0.0, 0.0};
    for (int iteration = 0; iteration < 80; ++iteration)
    {
        direction = normalize({
            covariance[0][0] * direction.x + covariance[0][1] * direction.y + covariance[0][2] * direction.z,
            covariance[1][0] * direction.x + covariance[1][1] * direction.y + covariance[1][2] * direction.z,
            covariance[2][0] * direction.x + covariance[2][1] * direction.y + covariance[2][2] * direction.z
        });
    }

    if (direction.x < 0.0)
    {
        direction = direction * -1.0;
    }

    return {mean, direction};
}

std::vector<CenterPoint> extractCenterPointsByBins(
    const std::vector<Point3D>& points,
    const PCAResult& pca,
    int binCount)
{
    if (points.empty() || binCount <= 0)
    {
        throw std::runtime_error("分段中心点提取参数不合法");
    }

    struct ProjectedPoint
    {
        double t = 0.0;
        Point3D point;
    };

    std::vector<ProjectedPoint> projected;
    projected.reserve(points.size());
    for (const Point3D& point : points)
    {
        projected.push_back({projectToMainDirection(point, pca), point});
    }

    std::sort(projected.begin(), projected.end(), [](const ProjectedPoint& a, const ProjectedPoint& b) {
        return a.t < b.t;
    });

    const double minT = projected.front().t;
    const double maxT = projected.back().t;
    const double binWidth = (maxT - minT) / static_cast<double>(binCount);
    if (binWidth <= 0.0)
    {
        throw std::runtime_error("投影参数范围过小，无法分段");
    }

    std::vector<Point3D> pointSums(binCount);
    std::vector<double> tSums(binCount, 0.0);
    std::vector<int> counts(binCount, 0);

    for (const ProjectedPoint& item : projected)
    {
        int index = static_cast<int>((item.t - minT) / binWidth);
        index = std::clamp(index, 0, binCount - 1);
        pointSums[index] = pointSums[index] + item.point;
        tSums[index] += item.t;
        ++counts[index];
    }

    std::vector<CenterPoint> centers;
    centers.reserve(binCount);
    for (int i = 0; i < binCount; ++i)
    {
        if (counts[i] == 0)
        {
            continue;
        }

        centers.push_back({
            tSums[i] / static_cast<double>(counts[i]),
            pointSums[i] / static_cast<double>(counts[i]),
            counts[i]
        });
    }

    return centers;
}

PointCloudDataset preparePointCloudDataset(
    const std::string& pointCloudFile,
    int binCount,
    int polynomialDegree)
{
    generatePointCloudData(pointCloudFile);

    PointCloudDataset dataset;
    dataset.pointCloudFile = pointCloudFile;
    dataset.polynomialDegree = polynomialDegree;
    dataset.points = loadPointCloud(pointCloudFile);
    dataset.pca = estimateMainDirectionPCA(dataset.points);
    dataset.centers = extractCenterPointsByBins(dataset.points, dataset.pca, binCount);
    return dataset;
}

void printPointCloudDataset(const PointCloudDataset& dataset)
{
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "点云文件: " << dataset.pointCloudFile << '\n';
    std::cout << "点云数量: " << dataset.points.size() << '\n';
    std::cout << "中心代表点数量: " << dataset.centers.size() << '\n';
    std::cout << "多项式阶数: " << dataset.polynomialDegree << '\n';
    std::cout << "PCA 主方向: ("
              << dataset.pca.direction.x << ", "
              << dataset.pca.direction.y << ", "
              << dataset.pca.direction.z << ")\n";
}

} // namespace curvefit

