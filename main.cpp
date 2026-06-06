#include "core/PointCloudPipeline.h"
#include "ui/MainWindow.h"
#include "ui/OsgWidget.h"
#include "visualization/OsgScene.h"

#include <QApplication>

#include <exception>
#include <iostream>
#include <utility>

int main(int argc, char** argv)
{
    try
    {
        curvefit::configureQtOpenGLFormat();
        QApplication application(argc, argv);

        curvefit::PointCloudDataset dataset =
            curvefit::preparePointCloudDataset("point_cloud.txt");
        curvefit::printPointCloudDataset(dataset);

        curvefit::SceneLayers sceneLayers =
            curvefit::createSceneLayers(dataset.points, dataset.centers);

        curvefit::MainWindow window(
            std::move(dataset),
            std::move(sceneLayers));
        window.showMaximized();

        std::cout
            << "Qt Ribbon 主窗口已启动，OSG 场景已嵌入中心视图。"
            << std::endl;
        return application.exec();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "程序错误: " << exception.what() << '\n';
        return 1;
    }
}

