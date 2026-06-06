#include "visualization/OsgScene.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/ShapeDrawable>
#include <osg/StateSet>

namespace curvefit
{
namespace
{

osg::Vec3 toOsgVec3(const Point3D& point)
{
    return osg::Vec3(
        static_cast<float>(point.x),
        static_cast<float>(point.y),
        static_cast<float>(point.z));
}

osg::Vec4 toOsgColor(const std::array<float, 4>& color)
{
    return osg::Vec4(color[0], color[1], color[2], color[3]);
}

osg::ref_ptr<osg::Geode> createPointCloudGeometry(
    const std::vector<Point3D>& points,
    const osg::Vec4& color,
    float pointSize)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->reserve(points.size());
    for (const Point3D& point : points)
    {
        vertices->push_back(toOsgVec3(point));
    }

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(color);

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(
        new osg::DrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertices->size())));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry.get());

    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setAttribute(new osg::Point(pointSize), osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    return geode;
}

osg::ref_ptr<osg::Geode> createCenterPointNodes(
    const std::vector<CenterPoint>& centers,
    double radius)
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    for (const CenterPoint& center : centers)
    {
        osg::ref_ptr<osg::Sphere> sphere =
            new osg::Sphere(toOsgVec3(center.point), static_cast<float>(radius));
        osg::ref_ptr<osg::ShapeDrawable> drawable =
            new osg::ShapeDrawable(sphere.get());
        drawable->setColor(osg::Vec4(1.0f, 0.35f, 0.05f, 1.0f));
        geode->addDrawable(drawable.get());
    }
    return geode;
}

osg::ref_ptr<osg::Geode> createCurveGeometry(
    const std::vector<Point3D>& curvePoints,
    const osg::Vec4& color,
    float lineWidth)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->reserve(curvePoints.size());
    for (const Point3D& point : curvePoints)
    {
        vertices->push_back(toOsgVec3(point));
    }

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(color);

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(
        new osg::DrawArrays(
            GL_LINE_STRIP,
            0,
            static_cast<GLsizei>(vertices->size())));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry.get());

    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setAttribute(new osg::LineWidth(lineWidth), osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    return geode;
}

osg::ref_ptr<osg::Geode> createAxes(double axisLength)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(-axisLength, 0.0f, 0.0f));
    vertices->push_back(osg::Vec3(axisLength, 0.0f, 0.0f));
    vertices->push_back(osg::Vec3(0.0f, -axisLength, 0.0f));
    vertices->push_back(osg::Vec3(0.0f, axisLength, 0.0f));
    vertices->push_back(osg::Vec3(0.0f, 0.0f, -axisLength));
    vertices->push_back(osg::Vec3(0.0f, 0.0f, axisLength));

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0f, 0.05f, 0.05f, 1.0f));
    colors->push_back(osg::Vec4(1.0f, 0.05f, 0.05f, 1.0f));
    colors->push_back(osg::Vec4(0.05f, 0.85f, 0.1f, 1.0f));
    colors->push_back(osg::Vec4(0.05f, 0.85f, 0.1f, 1.0f));
    colors->push_back(osg::Vec4(0.1f, 0.35f, 1.0f, 1.0f));
    colors->push_back(osg::Vec4(0.1f, 0.35f, 1.0f, 1.0f));

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(
        new osg::DrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices->size())));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry.get());

    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setAttribute(new osg::LineWidth(3.0f), osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    return geode;
}

osg::ref_ptr<osg::Switch> makeLayerSwitch(osg::Node* node)
{
    osg::ref_ptr<osg::Switch> layer = new osg::Switch;
    layer->addChild(node, true);
    return layer;
}

} // namespace

SceneLayers createSceneLayers(
    const std::vector<Point3D>& points,
    const std::vector<CenterPoint>& centers)
{
    SceneLayers layers;
    layers.root = new osg::Group;
    layers.axes = makeLayerSwitch(createAxes(7.0).get());
    layers.pointCloud = makeLayerSwitch(
        createPointCloudGeometry(
            points,
            osg::Vec4(0.82f, 0.82f, 0.86f, 1.0f),
            3.0f).get());
    layers.centerPoints = makeLayerSwitch(
        createCenterPointNodes(centers, 0.085).get());
    layers.fittedCurve = new osg::Switch;

    layers.root->addChild(layers.axes.get());
    layers.root->addChild(layers.pointCloud.get());
    layers.root->addChild(layers.centerPoints.get());
    layers.root->addChild(layers.fittedCurve.get());
    return layers;
}

void updateCurveLayer(
    osg::Switch* curveLayer,
    const CurveFitResult& fitResult)
{
    if (!curveLayer)
    {
        return;
    }

    if (curveLayer->getNumChildren() > 0)
    {
        curveLayer->removeChildren(0, curveLayer->getNumChildren());
    }

    curveLayer->addChild(
        createCurveGeometry(
            fitResult.curvePoints,
            toOsgColor(fitResult.color),
            4.0f).get(),
        true);
}

} // namespace curvefit

