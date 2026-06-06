#pragma once

#include "core/GeometryTypes.h"

#include <string>
#include <vector>

namespace curvefit
{

struct PointCloudDataset
{
    std::string pointCloudFile;
    std::vector<Point3D> points;
    std::vector<CenterPoint> centers;
    PCAResult pca;
    int polynomialDegree = 3;
};

void generatePointCloudData(const std::string& fileName);
std::vector<Point3D> loadPointCloud(const std::string& fileName);
PCAResult estimateMainDirectionPCA(const std::vector<Point3D>& points);
std::vector<CenterPoint> extractCenterPointsByBins(
    const std::vector<Point3D>& points,
    const PCAResult& pca,
    int binCount);

PointCloudDataset preparePointCloudDataset(
    const std::string& pointCloudFile,
    int binCount = 90,
    int polynomialDegree = 3);

void printPointCloudDataset(const PointCloudDataset& dataset);

} // namespace curvefit

