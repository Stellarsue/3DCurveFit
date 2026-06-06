#pragma once

#include "core/PointCloudPipeline.h"
#include "fitting/CurveFitResult.h"
#include "visualization/OsgScene.h"

#include <QMainWindow>
#include <QString>

#include <functional>

class QFrame;
class QLabel;
class QToolButton;
class QWidget;

namespace curvefit
{

class OsgWidget;

class MainWindow : public QMainWindow
{
public:
    MainWindow(
        PointCloudDataset dataset,
        SceneLayers sceneLayers,
        QWidget* parent = nullptr);

private:
    QFrame* createFitInfoPanel(QWidget* parent);
    void updateFitInfoPanel(
        const QString& methodName,
        const QString& details);
    QWidget* createRibbon();
    void executeFit(const std::function<CurveFitResult()>& fitter);
    void connectLayerButton(QToolButton* button, osg::Switch* layer);
    void applyRibbonStyle();

    PointCloudDataset dataset_;
    SceneLayers sceneLayers_;
    OsgWidget* osgWidget_ = nullptr;
    QFrame* fitInfoPanel_ = nullptr;
    QLabel* fitInfoLabel_ = nullptr;
    QToolButton* curveLayerButton_ = nullptr;
};

} // namespace curvefit
