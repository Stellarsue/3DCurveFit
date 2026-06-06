#pragma once

#include <osg/Node>
#include <osg/ref_ptr>
#include <osgGA/EventQueue>
#include <osgGA/TrackballManipulator>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>

#include <QOpenGLWidget>

class QKeyEvent;
class QMouseEvent;
class QTimer;
class QWheelEvent;

namespace curvefit
{

void configureQtOpenGLFormat();

class OsgWidget : public QOpenGLWidget
{
public:
    explicit OsgWidget(osg::Node* sceneRoot, QWidget* parent = nullptr);

    void resetView();
    void requestRender();
    QString saveScreenshot();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    QSize osgPixelSize() const;
    QPointF toOsgPoint(const QPointF& point) const;
    osgGA::EventQueue* eventQueue();
    void setupViewer();
    void updateOsgViewport();
    void sendMouseButtonEvent(QMouseEvent* event, bool pressed);

    osg::ref_ptr<osg::Node> sceneRoot_;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> graphicsWindow_;
    osg::ref_ptr<osgGA::TrackballManipulator> manipulator_;
    osgViewer::Viewer viewer_;
    QTimer* renderTimer_ = nullptr;
    bool viewerReady_ = false;
};

} // namespace curvefit
