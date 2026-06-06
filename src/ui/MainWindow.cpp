#include "ui/MainWindow.h"

#include "fitting/BSplineCurveFitter.h"
#include "fitting/BezierCurveFitter.h"
#include "fitting/PolynomialCurveFitter.h"
#include "ui/OsgWidget.h"

#include <QApplication>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QSizePolicy>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <exception>
#include <utility>
#include <vector>

namespace curvefit
{
namespace
{

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;
constexpr int kCurveSampleCount = 360;

QToolButton* createRibbonButton(
    QWidget* owner,
    const QString& text,
    const QIcon& icon,
    bool checkable = false,
    bool checked = false)
{
    QToolButton* button = new QToolButton(owner);
    button->setText(text);
    button->setIcon(icon);
    button->setIconSize(QSize(26, 26));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setCheckable(checkable);
    button->setChecked(checked);
    button->setMinimumWidth(74);
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    return button;
}

QFrame* createRibbonGroup(
    const QString& title,
    const std::vector<QWidget*>& widgets)
{
    QFrame* frame = new QFrame;
    frame->setObjectName("ribbonGroup");

    QVBoxLayout* outerLayout = new QVBoxLayout(frame);
    outerLayout->setContentsMargins(8, 6, 8, 4);
    outerLayout->setSpacing(3);

    QHBoxLayout* rowLayout = new QHBoxLayout;
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(4);
    for (QWidget* widget : widgets)
    {
        rowLayout->addWidget(widget);
    }
    rowLayout->addStretch(1);

    QLabel* label = new QLabel(title);
    label->setObjectName("ribbonGroupLabel");
    label->setAlignment(Qt::AlignCenter);

    outerLayout->addLayout(rowLayout, 1);
    outerLayout->addWidget(label);
    return frame;
}

QString toQString(const std::string& text)
{
    return QString::fromStdString(text);
}

} // namespace

MainWindow::MainWindow(
    PointCloudDataset dataset,
    SceneLayers sceneLayers,
    QWidget* parent)
    : QMainWindow(parent),
      dataset_(std::move(dataset)),
      sceneLayers_(std::move(sceneLayers))
{
    setWindowTitle("3D Curve Fitting - Qt Ribbon + OSG");
    resize(kWindowWidth, kWindowHeight);

    QWidget* centralContainer = new QWidget(this);
    QGridLayout* centralLayout = new QGridLayout(centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    osgWidget_ = new OsgWidget(sceneLayers_.root.get(), centralContainer);
    fitInfoPanel_ = createFitInfoPanel(centralContainer);

    centralLayout->addWidget(osgWidget_, 0, 0);
    centralLayout->addWidget(
        fitInfoPanel_,
        0,
        0,
        Qt::AlignLeft | Qt::AlignBottom);
    setCentralWidget(centralContainer);

    setMenuWidget(createRibbon());
    updateFitInfoPanel(
        "尚未执行拟合",
        "请在“拟合”页选择多项式、B 样条或贝塞尔拟合。");
    statusBar()->showMessage(
        QString("点云 %1 | 中心代表点 %2 | 文件 %3")
            .arg(dataset_.points.size())
            .arg(dataset_.centers.size())
            .arg(toQString(dataset_.pointCloudFile)));

    applyRibbonStyle();
}

QFrame* MainWindow::createFitInfoPanel(QWidget* parent)
{
    QFrame* panel = new QFrame(parent);
    panel->setObjectName("fitOverlayPanel");
    panel->setMinimumWidth(430);
    panel->setMaximumWidth(620);

    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(4);

    fitInfoLabel_ = new QLabel(panel);
    fitInfoLabel_->setObjectName("fitOverlayLabel");
    fitInfoLabel_->setWordWrap(true);
    fitInfoLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(fitInfoLabel_);
    return panel;
}

void MainWindow::updateFitInfoPanel(
    const QString& methodName,
    const QString& details)
{
    if (!fitInfoLabel_)
    {
        return;
    }

    fitInfoLabel_->setText(
        QString(
            "当前拟合: %1\n"
            "点云数量: %2\n"
            "中心代表点: %3\n"
            "%4")
            .arg(methodName)
            .arg(dataset_.points.size())
            .arg(dataset_.centers.size())
            .arg(details));
}

QWidget* MainWindow::createRibbon()
{
    QTabWidget* tabs = new QTabWidget;
    tabs->setObjectName("ribbon");
    tabs->setDocumentMode(true);
    tabs->setFixedHeight(126);

    QWidget* homePage = new QWidget;
    QHBoxLayout* homeLayout = new QHBoxLayout(homePage);
    homeLayout->setContentsMargins(8, 6, 8, 6);
    homeLayout->setSpacing(8);

    QToolButton* homeButton = createRibbonButton(
        this,
        "复位",
        style()->standardIcon(QStyle::SP_ArrowBack));
    QToolButton* screenshotButton = createRibbonButton(
        this,
        "截图",
        style()->standardIcon(QStyle::SP_DialogSaveButton));
    QToolButton* fullScreenButton = createRibbonButton(
        this,
        "全屏",
        style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    QToolButton* exitButton = createRibbonButton(
        this,
        "退出",
        style()->standardIcon(QStyle::SP_DialogCloseButton));

    connect(homeButton, &QToolButton::clicked, osgWidget_, &OsgWidget::resetView);
    connect(screenshotButton, &QToolButton::clicked, this, [this]() {
        const QString fileName = osgWidget_->saveScreenshot();
        statusBar()->showMessage(
            fileName.isEmpty()
                ? "截图失败"
                : QString("截图已保存: %1").arg(fileName),
            5000);
    });
    connect(fullScreenButton, &QToolButton::clicked, this, [this]() {
        if (isFullScreen())
        {
            showMaximized();
        }
        else
        {
            showFullScreen();
        }
    });
    connect(exitButton, &QToolButton::clicked, qApp, &QApplication::quit);

    homeLayout->addWidget(createRibbonGroup(
        "视图",
        {homeButton, screenshotButton, fullScreenButton, exitButton}));
    homeLayout->addStretch(1);

    QWidget* layersPage = new QWidget;
    QHBoxLayout* layersLayout = new QHBoxLayout(layersPage);
    layersLayout->setContentsMargins(8, 6, 8, 6);
    layersLayout->setSpacing(8);

    QToolButton* pointsButton = createRibbonButton(
        this,
        "点云",
        style()->standardIcon(QStyle::SP_FileDialogDetailedView),
        true,
        true);
    QToolButton* centersButton = createRibbonButton(
        this,
        "中心点",
        style()->standardIcon(QStyle::SP_DialogYesButton),
        true,
        true);
    curveLayerButton_ = createRibbonButton(
        this,
        "拟合曲线",
        style()->standardIcon(QStyle::SP_BrowserReload),
        true,
        true);
    QToolButton* axesButton = createRibbonButton(
        this,
        "坐标轴",
        style()->standardIcon(QStyle::SP_ComputerIcon),
        true,
        true);

    connectLayerButton(pointsButton, sceneLayers_.pointCloud.get());
    connectLayerButton(centersButton, sceneLayers_.centerPoints.get());
    connectLayerButton(curveLayerButton_, sceneLayers_.fittedCurve.get());
    connectLayerButton(axesButton, sceneLayers_.axes.get());

    layersLayout->addWidget(createRibbonGroup(
        "显示图层",
        {pointsButton, centersButton, curveLayerButton_, axesButton}));
    layersLayout->addStretch(1);

    QWidget* fitPage = new QWidget;
    QHBoxLayout* fitLayout = new QHBoxLayout(fitPage);
    fitLayout->setContentsMargins(8, 6, 8, 6);
    fitLayout->setSpacing(8);

    QToolButton* polynomialButton = createRibbonButton(
        this,
        "多项式",
        style()->standardIcon(QStyle::SP_CommandLink));
    QToolButton* bsplineButton = createRibbonButton(
        this,
        "B样条",
        style()->standardIcon(QStyle::SP_BrowserReload));
    QToolButton* bezierButton = createRibbonButton(
        this,
        "贝塞尔",
        style()->standardIcon(QStyle::SP_DriveNetIcon));

    connect(polynomialButton, &QToolButton::clicked, this, [this]() {
        executeFit([this]() {
            return fitPolynomialCurve(
                dataset_.centers,
                dataset_.polynomialDegree,
                kCurveSampleCount);
        });
    });
    connect(bsplineButton, &QToolButton::clicked, this, [this]() {
        executeFit([this]() {
            return fitBSplineCurve(dataset_.centers, kCurveSampleCount);
        });
    });
    connect(bezierButton, &QToolButton::clicked, this, [this]() {
        executeFit([this]() {
            return fitBezierCurve(dataset_.centers, kCurveSampleCount);
        });
    });

    QLabel* hintLabel = new QLabel(
        "选择一种算法后，会更新中心视图中的拟合曲线，"
        "并在左下角显示当前数据和拟合结果。");
    hintLabel->setObjectName("fitInfoLabel");
    hintLabel->setWordWrap(true);
    hintLabel->setMinimumWidth(360);

    fitLayout->addWidget(createRibbonGroup(
        "拟合算法",
        {polynomialButton, bsplineButton, bezierButton}));
    fitLayout->addWidget(createRibbonGroup("结果显示", {hintLabel}));
    fitLayout->addStretch(1);

    tabs->addTab(homePage, "主页");
    tabs->addTab(layersPage, "图层");
    tabs->addTab(fitPage, "拟合");
    return tabs;
}

void MainWindow::executeFit(
    const std::function<CurveFitResult()>& fitter)
{
    try
    {
        const CurveFitResult result = fitter();
        updateCurveLayer(sceneLayers_.fittedCurve.get(), result);
        if (curveLayerButton_)
        {
            curveLayerButton_->setChecked(true);
        }

        updateFitInfoPanel(
            toQString(result.methodName),
            toQString(result.details));
        statusBar()->showMessage(
            QString("%1 已完成，曲线采样点 %2")
                .arg(toQString(result.methodName))
                .arg(result.curvePoints.size()),
            5000);
        osgWidget_->requestRender();
    }
    catch (const std::exception& exception)
    {
        statusBar()->showMessage(
            QString("拟合失败: %1").arg(exception.what()),
            8000);
    }
}

void MainWindow::connectLayerButton(
    QToolButton* button,
    osg::Switch* layer)
{
    connect(button, &QToolButton::toggled, this, [this, layer](bool visible) {
        if (!layer)
        {
            return;
        }

        if (visible)
        {
            layer->setAllChildrenOn();
        }
        else
        {
            layer->setAllChildrenOff();
        }
        osgWidget_->requestRender();
    });
}

void MainWindow::applyRibbonStyle()
{
    setStyleSheet(R"(
        QMainWindow {
            background: #15181d;
        }
        QTabWidget#ribbon::pane {
            border: 0;
            border-bottom: 1px solid #c9d1dc;
            background: #eef2f7;
        }
        QTabBar::tab {
            min-width: 72px;
            height: 28px;
            padding: 0 12px;
            color: #233044;
            background: #dde5ef;
            border: 1px solid #c5ceda;
            border-bottom: 0;
        }
        QTabBar::tab:selected {
            background: #f7f9fc;
            color: #0b1f3a;
        }
        QFrame#ribbonGroup {
            background: #f8fafc;
            border: 1px solid #d5dce6;
            border-radius: 6px;
        }
        QLabel#ribbonGroupLabel {
            color: #607086;
            font-size: 11px;
        }
        QLabel#fitInfoLabel {
            color: #1e293b;
            font-size: 12px;
        }
        QFrame#fitOverlayPanel {
            background: rgba(15, 23, 42, 215);
            border: 1px solid rgba(148, 163, 184, 190);
            border-radius: 6px;
            margin: 0 0 12px 12px;
        }
        QLabel#fitOverlayLabel {
            color: #e5edf7;
            font-family: Menlo;
            font-size: 12px;
        }
        QToolButton {
            color: #1f2937;
            background: transparent;
            border: 1px solid transparent;
            border-radius: 5px;
            padding: 4px 6px;
        }
        QToolButton:hover {
            background: #e9f1fb;
            border-color: #b8c8da;
        }
        QToolButton:checked {
            background: #dbeafe;
            border-color: #7aa7df;
        }
        QStatusBar {
            color: #d9e2ef;
            background: #1f2937;
        }
    )");
}

} // namespace curvefit

