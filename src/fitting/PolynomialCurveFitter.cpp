#include "fitting/PolynomialCurveFitter.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace curvefit
{
namespace
{

std::vector<double> solveLinearSystem(
    std::vector<std::vector<double>> matrix,
    std::vector<double> rightHandSide)
{
    const int size = static_cast<int>(rightHandSide.size());

    for (int column = 0; column < size; ++column)
    {
        int pivot = column;
        double bestValue = std::abs(matrix[column][column]);
        for (int row = column + 1; row < size; ++row)
        {
            const double candidate = std::abs(matrix[row][column]);
            if (candidate > bestValue)
            {
                bestValue = candidate;
                pivot = row;
            }
        }

        if (bestValue < 1e-12)
        {
            throw std::runtime_error("线性方程组接近奇异，无法求解多项式系数");
        }

        if (pivot != column)
        {
            std::swap(matrix[pivot], matrix[column]);
            std::swap(rightHandSide[pivot], rightHandSide[column]);
        }

        const double pivotValue = matrix[column][column];
        for (int c = column; c < size; ++c)
        {
            matrix[column][c] /= pivotValue;
        }
        rightHandSide[column] /= pivotValue;

        for (int row = 0; row < size; ++row)
        {
            if (row == column)
            {
                continue;
            }

            const double factor = matrix[row][column];
            for (int c = column; c < size; ++c)
            {
                matrix[row][c] -= factor * matrix[column][c];
            }
            rightHandSide[row] -= factor * rightHandSide[column];
        }
    }

    return rightHandSide;
}

std::vector<double> polyfit(
    const std::vector<double>& parameters,
    const std::vector<double>& values,
    int degree)
{
    if (parameters.size() != values.size() || parameters.empty())
    {
        throw std::runtime_error("多项式拟合输入数据不合法");
    }

    const int coefficientCount = degree + 1;
    std::vector<std::vector<double>> normalMatrix(
        coefficientCount,
        std::vector<double>(coefficientCount, 0.0));
    std::vector<double> normalRightHandSide(coefficientCount, 0.0);

    for (std::size_t row = 0; row < parameters.size(); ++row)
    {
        std::vector<double> powers(2 * degree + 1, 1.0);
        for (int power = 1; power <= 2 * degree; ++power)
        {
            powers[power] = powers[power - 1] * parameters[row];
        }

        for (int i = 0; i < coefficientCount; ++i)
        {
            for (int j = 0; j < coefficientCount; ++j)
            {
                normalMatrix[i][j] += powers[i + j];
            }
            normalRightHandSide[i] += values[row] * powers[i];
        }
    }

    return solveLinearSystem(normalMatrix, normalRightHandSide);
}

double evaluatePolynomial(const std::vector<double>& coefficients, double parameter)
{
    double value = 0.0;
    for (auto it = coefficients.rbegin(); it != coefficients.rend(); ++it)
    {
        value = value * parameter + *it;
    }
    return value;
}

std::vector<double> collectParameters(const std::vector<CenterPoint>& centers)
{
    std::vector<double> parameters;
    parameters.reserve(centers.size());
    for (const CenterPoint& center : centers)
    {
        parameters.push_back(center.t);
    }
    return parameters;
}

std::vector<double> collectCoordinate(
    const std::vector<CenterPoint>& centers,
    double Point3D::*coordinate)
{
    std::vector<double> values;
    values.reserve(centers.size());
    for (const CenterPoint& center : centers)
    {
        values.push_back(center.point.*coordinate);
    }
    return values;
}

std::string formatCoefficients(
    const std::string& name,
    const std::vector<double>& coefficients)
{
    std::ostringstream stream;
    stream << name << "(t): " << std::fixed << std::setprecision(6);
    for (std::size_t i = 0; i < coefficients.size(); ++i)
    {
        stream << coefficients[i];
        if (i + 1 < coefficients.size())
        {
            stream << ", ";
        }
    }
    return stream.str();
}

void printCoefficients(
    const std::string& name,
    const std::vector<double>& coefficients)
{
    std::cout << formatCoefficients(name, coefficients) << '\n';
}

} // namespace

CurveFitResult fitPolynomialCurve(
    const std::vector<CenterPoint>& centers,
    int degree,
    int sampleCount)
{
    if (degree < 1 || sampleCount < 2 || static_cast<int>(centers.size()) < degree + 1)
    {
        throw std::runtime_error("中心代表点数量不足或多项式拟合参数不合法");
    }

    const std::vector<double> parameters = collectParameters(centers);
    const std::vector<double> coefficientsX = polyfit(parameters, collectCoordinate(centers, &Point3D::x), degree);
    const std::vector<double> coefficientsY = polyfit(parameters, collectCoordinate(centers, &Point3D::y), degree);
    const std::vector<double> coefficientsZ = polyfit(parameters, collectCoordinate(centers, &Point3D::z), degree);
    const auto [minIt, maxIt] = std::minmax_element(parameters.begin(), parameters.end());

    CurveFitResult result;
    result.methodName = "三阶多项式拟合";
    result.color = {1.0f, 0.9f, 0.05f, 1.0f};
    result.curvePoints.reserve(sampleCount);

    for (int i = 0; i < sampleCount; ++i)
    {
        const double alpha = static_cast<double>(i) / static_cast<double>(sampleCount - 1);
        const double parameter = *minIt + (*maxIt - *minIt) * alpha;
        result.curvePoints.push_back({
            evaluatePolynomial(coefficientsX, parameter),
            evaluatePolynomial(coefficientsY, parameter),
            evaluatePolynomial(coefficientsZ, parameter)
        });
    }

    std::ostringstream details;
    details << "阶数: " << degree
            << "\n采样点: " << result.curvePoints.size()
            << '\n' << formatCoefficients("x", coefficientsX)
            << '\n' << formatCoefficients("y", coefficientsY)
            << '\n' << formatCoefficients("z", coefficientsZ);
    result.details = details.str();

    printCoefficients("x", coefficientsX);
    printCoefficients("y", coefficientsY);
    printCoefficients("z", coefficientsZ);
    return result;
}

} // namespace curvefit

