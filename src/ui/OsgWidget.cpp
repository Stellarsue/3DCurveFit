#include "ui/OsgWidget.h"

#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osgGA/GUIEventAdapter>

#include <QDateTime>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QSurfaceFormat>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace curvefit
{
namespace
{

constexpr double kCameraFovy = 35.0;
constexpr double kCameraNear = 0.05;
constexpr double kCameraFar = 1000.0;

QPointF mouseEventPosition(const QMouseEvent* event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position();
#else
    return event->localPos();
#endif
}

QPointF wheelEventPosition(const QWheelEvent* event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position();
#else
    return event->posF();
#endif
}

unsigned int osgMouseButton(Qt::MouseButton button)
{
    if (button == Qt::LeftButton)
    {
        return 1;
    }
    if (button == Qt::MiddleButton)
    {
        return 2;
    }
    if (button == Qt::RightButton)
    {
        return 3;
    }
    return 0;
}

int osgKey(const QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        return osgGA::GUIEventAdapter::KEY_Escape;
    case Qt::Key_Left:
        return osgGA::GUIEventAdapter::KEY_Left;
    case Qt::Key_Right:
        return osgGA::GUIEventAdapter::KEY_Right;
    case Qt::Key_Up:
        return osgGA::GUIEventAdapter::KEY_Up;
    case Qt::Key_Down:
        return osgGA::GUIEventAdapter::KEY_Down;
    case Qt::Key_Home:
        return osgGA::GUIEventAdapter::KEY_Home;
    case Qt::Key_End:
        return osgGA::GUIEventAdapter::KEY_End;
    default:
        break;
    }

    if (!event->text().isEmpty())
    {
        return event->text().at(0).toLatin1();
    }
    return event->key();
}

} // namespace

void configureQtOpenGLFormat()
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(2, 1);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);
}

OsgWidget::OsgWidget(osg::Node* sceneRoot, QWidget* parent)
    : QOpenGLWidget(parent),
      sceneRoot_(sceneRoot)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAutoFillBackground(false);
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    renderTimer_ = new QTimer(this);
    connect(renderTimer_, &QTimer::timeout, this, [this]() {
        update();
    });
    renderTimer_->start(16);
}

void OsgWidget::resetView()
{
    if (viewerReady_)
    {
        viewer_.home();
        update();
    }
}

void OsgWidget::requestRender()
{
    update();
}

QString OsgWidget::saveScreenshot()
{
    const QString fileName = QString("osg_qt_screenshot_%1.png")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    return grabFramebuffer().save(fileName) ? fileName : QString();
}

void OsgWidget::initializeGL()
{
    setupViewer();
}

void OsgWidget::paintGL()
{
    if (!viewerReady_)
    {
        setupViewer();
    }

    if (graphicsWindow_.valid())
    {
        graphicsWindow_->setDefaultFboId(defaultFramebufferObject());
    }
    viewer_.frame();
}

void OsgWidget::resizeGL(int, int)
{
    updateOsgViewport();
}

void OsgWidget::mousePressEvent(QMouseEvent* event)
{
    sendMouseButtonEvent(event, true);
}

void OsgWidget::mouseReleaseEvent(QMouseEvent* event)
{
    sendMouseButtonEvent(event, false);
}

void OsgWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (osgGA::EventQueue* queue = eventQueue())
    {
        const QPointF point = toOsgPoint(mouseEventPosition(event));
        queue->mouseMotion(
            static_cast<float>(point.x()),
            static_cast<float>(point.y()));
        update();
    }
}

void OsgWidget::wheelEvent(QWheelEvent* event)
{
    if (osgGA::EventQueue* queue = eventQueue())
    {
        const int deltaY = event->angleDelta().y();
        if (deltaY > 0)
        {
            queue->mouseScroll(osgGA::GUIEventAdapter::SCROLL_UP);
        }
        else if (deltaY < 0)
        {
            queue->mouseScroll(osgGA::GUIEventAdapter::SCROLL_DOWN);
        }

        const QPointF point = toOsgPoint(wheelEventPosition(event));
        queue->mouseMotion(
            static_cast<float>(point.x()),
            static_cast<float>(point.y()));
        update();
    }
    event->accept();
}

void OsgWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
    {
        window()->showNormal();
        event->accept();
        return;
    }

    if (osgGA::EventQueue* queue = eventQueue())
    {
        queue->keyPress(osgKey(event));
        update();
    }
}

void OsgWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (osgGA::EventQueue* queue = eventQueue())
    {
        queue->keyRelease(osgKey(event));
        update();
    }
}

QSize OsgWidget::osgPixelSize() const
{
    const double deviceScale = devicePixelRatioF();
    return QSize(
        std::max(1, static_cast<int>(std::round(width() * deviceScale))),
        std::max(1, static_cast<int>(std::round(height() * deviceScale))));
}

QPointF OsgWidget::toOsgPoint(const QPointF& point) const
{
    const double deviceScale = devicePixelRatioF();
    return QPointF(point.x() * deviceScale, point.y() * deviceScale);
}

osgGA::EventQueue* OsgWidget::eventQueue()
{
    return graphicsWindow_.valid()
        ? graphicsWindow_->getEventQueue()
        : nullptr;
}

void OsgWidget::setupViewer()
{
    if (viewerReady_ || !sceneRoot_.valid())
    {
        return;
    }

    const QSize pixelSize = osgPixelSize();
    viewer_.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer_.setKeyEventSetsDone(osgGA::GUIEventAdapter::KEY_Escape);
    viewer_.setReleaseContextAtEndOfFrameHint(false);

    graphicsWindow_ = viewer_.setUpViewerAsEmbeddedInWindow(
        0,
        0,
        pixelSize.width(),
        pixelSize.height());
    graphicsWindow_->setDefaultFboId(defaultFramebufferObject());

    osg::Camera* camera = viewer_.getCamera();
    camera->setClearColor(osg::Vec4(0.055f, 0.06f, 0.07f, 1.0f));
    camera->setDrawBuffer(GL_COLOR_ATTACHMENT0);
    camera->setReadBuffer(GL_COLOR_ATTACHMENT0);
    camera->setProjectionResizePolicy(osg::Camera::VERTICAL);

    manipulator_ = new osgGA::TrackballManipulator;

    const osg::BoundingSphere bounds = sceneRoot_->getBound();
    const osg::Vec3d center = bounds.center();
    const double radius = std::max(1.0, static_cast<double>(bounds.radius()));
    const double distance =
        radius / std::tan(osg::DegreesToRadians(kCameraFovy * 0.5)) * 0.88;
    const osg::Vec3d viewDirection(0.35, -1.0, 0.45);
    const osg::Vec3d eye =
        center + viewDirection * (distance / viewDirection.length());

    manipulator_->setHomePosition(
        eye,
        center,
        osg::Vec3d(0.0, 0.0, 1.0),
        false);
    manipulator_->setAllowThrow(false);

    viewer_.setSceneData(sceneRoot_.get());
    viewer_.setCameraManipulator(manipulator_.get(), false);
    viewer_.realize();

    viewerReady_ = true;
    updateOsgViewport();
    viewer_.home();
}

void OsgWidget::updateOsgViewport()
{
    if (!graphicsWindow_.valid())
    {
        return;
    }

    const QSize pixelSize = osgPixelSize();
    graphicsWindow_->resized(0, 0, pixelSize.width(), pixelSize.height());
    graphicsWindow_->getEventQueue()->windowResize(
        0,
        0,
        pixelSize.width(),
        pixelSize.height());
    graphicsWindow_->getEventQueue()->setMouseInputRange(
        0.0f,
        0.0f,
        static_cast<float>(pixelSize.width()),
        static_cast<float>(pixelSize.height()));

    osg::Camera* camera = viewer_.getCamera();
    camera->setViewport(0, 0, pixelSize.width(), pixelSize.height());
    camera->setProjectionMatrixAsPerspective(
        kCameraFovy,
        static_cast<double>(pixelSize.width())
            / static_cast<double>(pixelSize.height()),
        kCameraNear,
        kCameraFar);
}

void OsgWidget::sendMouseButtonEvent(QMouseEvent* event, bool pressed)
{
    const unsigned int button = osgMouseButton(event->button());
    if (button == 0)
    {
        return;
    }

    if (osgGA::EventQueue* queue = eventQueue())
    {
        const QPointF point = toOsgPoint(mouseEventPosition(event));
        if (pressed)
        {
            queue->mouseButtonPress(
                static_cast<float>(point.x()),
                static_cast<float>(point.y()),
                button);
        }
        else
        {
            queue->mouseButtonRelease(
                static_cast<float>(point.x()),
                static_cast<float>(point.y()),
                button);
        }
        update();
    }
}

} // namespace curvefit

