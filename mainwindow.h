#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QSettings>
#include <QMap>
#include <QList>
#include <QTime>

#include "cloudvisualizer.h"
#include "cloudio.h"

// PCL //////////////////////////
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/io/io.h>
#include <pcl/common/distances.h>

// Region Growing
#include <vector>
#include <pcl/point_types.h>
#include <pcl/search/search.h>
#include <pcl/search/kdtree.h>
#include <pcl/features/normal_3d.h>
#include <pcl/filters/passthrough.h>
#include <pcl/segmentation/region_growing.h>


// Correspondence Grouping
#include <pcl/point_cloud.h>
#include <pcl/correspondence.h>
#include <pcl/features/normal_3d_omp.h>
#include <pcl/features/shot_omp.h>
#include <pcl/features/board.h>
#include <pcl/keypoints/uniform_sampling.h>
#include <pcl/recognition/cg/hough_3d.h>
#include <pcl/recognition/cg/geometric_consistency.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/kdtree/impl/kdtree_flann.hpp>
#include <pcl/common/transforms.h>
#include <pcl/console/parse.h>

// Euclidean Clustering
#include <pcl/ModelCoefficients.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/kdtree/kdtree.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>


// END PCL ///////////////////////

#include "ui_about.h"
#include "databaseDialog.h"
#include "json.h"
#include "rest.h"

#include <QVTKWidget.h>

using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void printInfo(QString text);
    void printError(QString text);
    void printSuccess(QString text);
    void printWithTime(QString text);

    // The Main Cloud!
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr mainCloud;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr fallbackCloud;
    
private slots:
    void exitProgram();
    void openFile();
    void openFileFromDatabase(QString ident);
    void saveFile();
    void saveAsPNG();

    void regionGrowing();

    void minCut();

    void cluster();

    void corresGrouping();
    void clSetCloud();

    void setWhite();
    void undo();

    void showAboutDialog();

    void showDatabaseDialog();
    void setDatabase();
    void calcShotFeatures();
    void addToDatabase();
    void identifyScene();

    void toggleCoordinateSystem();

    void toggleObjectsDock();

private:
    Ui::MainWindow *ui;
    CF::CloudVisualizer *visu;
    DatabaseDialog *database;

    QString clCloud;
    void bleachCloud(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud);
    void setFallBack();
    void displayError(QString message);
    void updateCloud();
    void openMainCloud(QString path);

    void mcPickPointCallback(const pcl::visualization::PointPickingEvent &event);
    pcl::PointXYZ mcPickPoint;

    QString lastFile;
    QString lastClFile;

    std::string databasePath;
    pcl::PointCloud<pcl::SHOT352>::Ptr shotDescriptors;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr shotKeypoints;

    void addText(QString text, double x, double y, double z, double size, double r, double g, double b);
    pcl::PointXYZ calcOffset(pcl::PointXYZ& center, Eigen::Vector4f& mainCloudCenter, double offset = 1.5);

    Rest restAPI;

};

#endif // MAINWINDOW_H
