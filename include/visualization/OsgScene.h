#pragma once

#include "core/GeometryTypes.h"
#include "fitting/CurveFitResult.h"

#include <osg/Group>
#include <osg/Switch>

#include <vector>

namespace curvefit
{

struct SceneLayers
{
    osg::ref_ptr<osg::Group> root;
    osg::ref_ptr<osg::Switch> axes;
    osg::ref_ptr<osg::Switch> pointCloud;
    osg::ref_ptr<osg::Switch> centerPoints;
    osg::ref_ptr<osg::Switch> fittedCurve;
};

SceneLayers createSceneLayers(
    const std::vector<Point3D>& points,
    const std::vector<CenterPoint>& centers);

void updateCurveLayer(
    osg::Switch* curveLayer,
    const CurveFitResult& fitResult);

} // namespace curvefit

