#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "cloudvisualizer.h"

// Min-Cut
#include <pcl/segmentation/min_cut_segmentation.h>

const std::string cloud = "hallo";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    window()->showMaximized();

    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());

    // Feature Database
    //this->databasePath = "D:/Studium/FP/database";
    this->databasePath = "D:/Studium/FP/db_train";
    ui->fdDatabasePath->setText(QString::fromStdString(this->databasePath));
    database = new DatabaseDialog(this);

    ui->console->setFontPointSize(10.5);
    // CONNECTIONS

    // Menu
    connect(ui->actionOpenPointCloud, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(ui->actionSavePointCloud, SIGNAL(triggered()), this, SLOT(saveFile()));
    connect(ui->actionSavePNG, SIGNAL(triggered()), this, SLOT(saveAsPNG()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
    connect(ui->actionUndo, SIGNAL(triggered()), this, SLOT(undo()));
    connect(ui->actionResetColor, SIGNAL(triggered()), this, SLOT(setWhite()));
    connect(ui->actionShowCoordinateSystem, SIGNAL(triggered()), this, SLOT(toggleCoordinateSystem()));
    connect(ui->actionObjectsDock, SIGNAL(triggered()), this, SLOT(toggleObjectsDock()));

    // Icons
    connect(ui->iconOpenPointCloud, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(ui->iconSavePNG, SIGNAL(triggered()), this, SLOT(saveAsPNG()));
    connect(ui->iconSavePointCloud, SIGNAL(triggered()), this, SLOT(saveFile()));
    connect(ui->iconUndo, SIGNAL(triggered()), this, SLOT(undo()));
    connect(ui->iconResetColor, SIGNAL(triggered()), this, SLOT(setWhite()));
    connect(ui->iconAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));


    // Algorithms
    connect(ui->startRegionGrowing, SIGNAL(clicked()), this, SLOT(regionGrowing()));
    connect(ui->startGrouping, SIGNAL(clicked()), this, SLOT(corresGrouping()));
    connect(ui->clSetCloudButton, SIGNAL(clicked()), this, SLOT(clSetCloud()));
    connect(ui->startMinCut, SIGNAL(clicked()), this, SLOT(minCut()));
    connect(ui->startCluster, SIGNAL(clicked()), this, SLOT(cluster()));

    // Feature Database
    connect(ui->openDatabase, SIGNAL(clicked()), this, SLOT(showDatabaseDialog()));
    connect(ui->fdSetDatabase, SIGNAL(clicked()), this, SLOT(setDatabase()));
    connect(ui->calcShotFeatures, SIGNAL(clicked()), this, SLOT(calcShotFeatures()));
    connect(ui->fdAddToDatabase, SIGNAL(clicked()), this, SLOT(addToDatabase()));
    connect(ui->fdStartIdentify, SIGNAL(clicked()), this, SLOT(identifyScene()));

    // Database Manager Connections
    connect(database, SIGNAL(openSelectedModel(QString)), this, SLOT(openFileFromDatabase(QString)));

    // END CONNECTIONS
    mainCloud = pcl::PointCloud<pcl::PointXYZRGB>::Ptr(new pcl::PointCloud<pcl::PointXYZRGB>);
    visu = new CF::CloudVisualizer(ui->vtkwidget, this);
    shotDescriptors = pcl::PointCloud<pcl::SHOT352>::Ptr(new pcl::PointCloud<pcl::SHOT352>);
    shotKeypoints = pcl::PointCloud<pcl::PointXYZRGB>::Ptr(new pcl::PointCloud<pcl::PointXYZRGB>);

    // Bind Point Picking Callback
    boost::function<void(const pcl::visualization::PointPickingEvent&)> f = boost::bind(&MainWindow::mcPickPointCallback, this, _1);
    visu->visualizer.registerPointPickingCallback(f);

    // Init Properties
    this->clCloud = QString();
    this->mcPickPoint = pcl::PointXYZ();
    this->lastFile = QString();
    this->lastClFile = QString();

    this->printInfo("Welcome ...");
    this->setStatusTip("No Point Cloud loaded");

    // Init Windows
    ui->ecDockWidget->hide();
    ui->mcDockWidget->hide();
    ui->rgDockWidget->hide();

//    QtJson::JsonObject j;
//    j["name"] = "Fabi";
//    QtJson::JsonObject result = restAPI.post("recognition", j);
//    this->printInfo((result["status"]).toString());


    // Rest API Status Test
//    this->printInfo("Testing Semantic API ...");
//    QtJson::JsonObject result = restAPI.get("status");
//    QString status = (result["status"]).toString();

//    if(status == "ok") {
//        this->printInfo("Semantic API is running");
//    } else {
//        this->printError("Semantic API is NOT running");
//    }


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::printInfo(QString text)
{
    ui->console->setTextColor(Qt::black);
    this->printWithTime(text);
}

void MainWindow::printError(QString text)
{
    ui->console->setTextColor(Qt::red);
    this->printWithTime(text);
}

void MainWindow::printSuccess(QString text)
{
    ui->console->setTextColor(Qt::darkGreen);
    this->printWithTime(text);
}

void MainWindow::printWithTime(QString text)
{
     ui->console->append("[" + QTime::currentTime().toString() + "]  " + text);
     qApp->processEvents();
}


void MainWindow::exitProgram()
{
    QApplication::quit();
}

/**
 * Opens a Cloud File
 *
 * @brief MainWindow::openFile
 */
void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Point Cloud"), this->lastFile, tr("PCD (*.pcd);;PLY (*.ply)"));

    if(fileName != NULL) {
        this->openMainCloud(fileName);
    }

}

void MainWindow::openFileFromDatabase(QString ident)
{
    QString path = QString::fromStdString(this->databasePath) + "/" + ident + "_model.pcd";
    this->openMainCloud(path);
}

void MainWindow::openMainCloud(QString path)
{
    this->lastFile = QFileInfo(path).path();

    if(path.endsWith(".pcd")) {
        this->printInfo("Loading File: " + path);
        qApp->processEvents();
        pcl::io::loadPCDFile(path.toStdString(), *mainCloud);
        this->printSuccess("Done Loading File: " + path);
    }

    if(path.endsWith(".ply")) {
        this->printInfo("Loading File: " + path);
        qApp->processEvents();
        pcl::io::loadPLYFile(path.toStdString(), *mainCloud);
        this->printSuccess("Done Loading File: " + path);
    }


    visu->visualizer.removeAllPointClouds();
    visu->visualizer.removeAllShapes();
    //this->bleachCloud(mainCloud);
    //pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZRGB> colorHandler (mainCloud, 255, 255, 255);
    visu->visualizer.addPointCloud(mainCloud, cloud);
    visu->visualizer.resetCamera();
    visu->update();

    QString numberPoints = QString::number(mainCloud->points.size());
    this->setStatusTip("Loaded Point Cloud: " + path + " (Points: " + numberPoints + ")");
}


void MainWindow::saveFile()
{
    QString saveFile = QFileDialog::getSaveFileName(this, tr("Save as PCD"), "", ".pcd");

    if(saveFile != NULL) {
        pcl::io::savePCDFile(saveFile.toStdString(), *mainCloud);
    }
    this->printSuccess("Saved as PCD to: " + saveFile);

}

void MainWindow::saveAsPNG()
{
    QString saveFile = QFileDialog::getSaveFileName(this, tr("Save as PNG"), "", ".png");

    if(saveFile != NULL) {
         visu->visualizer.saveScreenshot(saveFile.toStdString());
    }
    this->printSuccess("Exported as PNG to: " + saveFile);
}

void MainWindow::bleachCloud(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud)
{
    for(int i = 0; i < cloud->points.size(); i++) {
        cloud->points[i].r = 255;
        cloud->points[i].g = 255;
        cloud->points[i].b = 255;
    }
}

void MainWindow::setFallBack()
{
    fallbackCloud = pcl::PointCloud<pcl::PointXYZRGB>::Ptr(mainCloud);
}

void MainWindow::displayError(QString message)
{
    QMessageBox msgBox;
    msgBox.critical(0,"Error",message);
    msgBox.setFixedSize(500,200);
}

void MainWindow::updateCloud()
{
    visu->visualizer.updatePointCloud(mainCloud, cloud);
    visu->update();
}

/**
 * @brief MainWindow::regionGrowing
 *
 * Based on PCL-Tutorial
 * http://pointclouds.org/documentation/tutorials/region_growing_segmentation.php
 *
 */

void MainWindow::regionGrowing()
{

    if(mainCloud->size() == 0) {
        this->displayError("Please open a Point Cloud first!");
    } else {

        // Get User Input
        int minClusterSize = 0;
        if(ui->rgMinCluster > 0) {
            minClusterSize = ui->rgMinCluster->value();
        }

        int maxClusterSize = 0;
        if(ui->rgMaxCluster > 0) {
            maxClusterSize = ui->rgMaxCluster->value();
        }

        double smoothnessThreshold = ui->rgSmothnessThres->value();
        double curvatureThreshold = ui->rgCurvature->value();

        this->setFallBack();

        this->printInfo("Performing Region Growing ...");
        qApp->processEvents();

        // Determine Normals
        pcl::search::Search<pcl::PointXYZRGB>::Ptr tree = boost::shared_ptr<pcl::search::Search<pcl::PointXYZRGB>>(new pcl::search::KdTree<pcl::PointXYZRGB>);
        pcl::PointCloud <pcl::Normal>::Ptr normals (new pcl::PointCloud <pcl::Normal>);
        pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal> normal_estimator;
        normal_estimator.setSearchMethod (tree);
        normal_estimator.setInputCloud (mainCloud);
        normal_estimator.setKSearch (50);
        normal_estimator.compute (*normals);

        // Perform Region Growing
        pcl::RegionGrowing<pcl::PointXYZRGB, pcl::Normal> reg;

        if(minClusterSize > 0) {
            reg.setMinClusterSize(minClusterSize);
        }
        if(maxClusterSize > 0) {
            reg.setMaxClusterSize(maxClusterSize);
        }

        reg.setSearchMethod (tree);
        reg.setNumberOfNeighbours (30);
        reg.setInputCloud (mainCloud);
        reg.setInputNormals (normals);
        reg.setSmoothnessThreshold (smoothnessThreshold / 180.0 * M_PI);
        reg.setCurvatureThreshold (curvatureThreshold);

        std::vector <pcl::PointIndices> clusters;
        reg.extract (clusters);

        mainCloud = reg.getColoredCloud ();

        this->printSuccess("Finished Region Growing");
        this->updateCloud();

    }
}


void MainWindow::mcPickPointCallback(const pcl::visualization::PointPickingEvent &event)
{
    event.getPoint(this->mcPickPoint.x, this->mcPickPoint.y, this->mcPickPoint.z);
    visu->visualizer.removeShape("mcPickSphere");
    visu->visualizer.addSphere(this->mcPickPoint, 0.1, 0.5, 0.6, 0.3, "mcPickSphere");
    this->printInfo("Set Point at: x=" + QString::number(this->mcPickPoint.x) + ", y=" + QString::number(this->mcPickPoint.y) + ", z=" + QString::number(this->mcPickPoint.z));
}

void MainWindow::addText(QString text, double x, double y, double z, double size, double r, double g, double b)
{
    //visu->visualizer.addText("hallo",100,100);
    int hash = Util::randInt(1,10000);
    QString id = text + "_" + QString::number(hash);
    pcl::PointXYZRGB p;
    p.x = x;
    p.y = y;
    p.z = z;
    visu->visualizer.addText3D<pcl::PointXYZRGB>(text.toStdString(), p, size, r, g, b, id.toStdString());

}


/**
 * @brief MainWindow::minCut
 *
 * Based on PCL-Tutorial
 * http://pointclouds.org/documentation/tutorials/min_cut_segmentation.php
 *
 */
void MainWindow::minCut()
{
    if(mainCloud->size() == 0) {
        this->displayError("Please open a Point Cloud first!");
        return;
    }

    // Get User Input
    double sigma = ui->mcSigma->value();
    double radius = ui->mcRadius->value();
    double sourceWeight = ui->mcSourceWeight->value();
    int neighbours = ui->mcNeighbours->value();


    this->setFallBack();

    this->printInfo("Performing Min-Cut Segmentation ...");
    qApp->processEvents();

    pcl::PointCloud <pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud <pcl::PointXYZ>);
    pcl::copyPointCloud(*mainCloud, *cloud);

    pcl::IndicesPtr indices (new std::vector <int>);
    pcl::PassThrough<pcl::PointXYZ> pass;
    pass.setInputCloud (cloud);
    pass.setFilterFieldName ("z");
    pass.setFilterLimits (0.0, 1.0);
    pass.filter (*indices);

    pcl::MinCutSegmentation<pcl::PointXYZ> seg;
    seg.setInputCloud (cloud);
    //seg.setIndices (indices);


    pcl::PointCloud<pcl::PointXYZ>::Ptr background_points(new pcl::PointCloud<pcl::PointXYZ> ());
    pcl::PointXYZ bgPoint;

    pcl::PointCloud<pcl::PointXYZ>::Ptr foreground_points(new pcl::PointCloud<pcl::PointXYZ> ());
    pcl::PointXYZ point;
//    point.x = 68.97;
//    point.y = -18.55;
//    point.z = 0.57;

    bgPoint.x = 1.08;
    bgPoint.y = 3.13;
    bgPoint.z = -9.77;

    background_points->points.push_back(bgPoint);
    foreground_points->points.push_back(this->mcPickPoint);
    seg.setForegroundPoints (foreground_points);
    seg.setBackgroundPoints(background_points);

    seg.setSigma(sigma);
    seg.setRadius(radius);
    seg.setNumberOfNeighbours(neighbours);
    seg.setSourceWeight(sourceWeight);


    std::vector <pcl::PointIndices> clusters;
    seg.extract (clusters);

    this->printInfo("Maximim flow is: " + QString::number(seg.getMaxFlow()));

    mainCloud = seg.getColoredCloud();
    this->printSuccess("Finished Min-Cut Segmentation ");
    this->updateCloud();

}

/**
 * @brief MainWindow::cluster
 *
 * Based on PCL-Tutorial
 * http://pointclouds.org/documentation/tutorials/cluster_extraction.php
 */
void MainWindow::cluster()
{
    if(mainCloud->size() == 0) {
        this->displayError("Please open a Point Cloud first!");
        return;
    }

    this->setFallBack();

    this->printInfo("Performing Euclidean Clustering ...");

    // Get User Input
    double clusterTolerance = ui->eclTolerance->value();
    int minCluster = ui->eclMinCluster->value();
    int maxCluster = ui->eclMaxCluster->value();

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_f (new pcl::PointCloud<pcl::PointXYZRGB>);

    // Downsampling

    pcl::VoxelGrid<pcl::PointXYZRGB> vg;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_filtered (new pcl::PointCloud<pcl::PointXYZRGB>);
    vg.setInputCloud (mainCloud);
    vg.setLeafSize (0.01f, 0.01f, 0.01f);
    vg.filter (*cloud_filtered);


     // Create the segmentation object for the planar model and set all the parameters
     pcl::SACSegmentation<pcl::PointXYZRGB> seg;
     pcl::PointIndices::Ptr inliers (new pcl::PointIndices);
     pcl::ModelCoefficients::Ptr coefficients (new pcl::ModelCoefficients);
     pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_plane (new pcl::PointCloud<pcl::PointXYZRGB> ());
     seg.setOptimizeCoefficients (true);
     seg.setModelType (pcl::SACMODEL_PLANE);
     seg.setMethodType (pcl::SAC_RANSAC);
     seg.setMaxIterations (100);
     seg.setDistanceThreshold (0.02);

     int i=0, nr_points = (int) cloud_filtered->points.size ();
     while (cloud_filtered->points.size () > 0.3 * nr_points)
     {
         // Segment the largest planar component from the remaining cloud
         seg.setInputCloud (cloud_filtered);
         seg.segment (*inliers, *coefficients);
         if (inliers->indices.size () == 0)
         {
             std::cout << "Could not estimate a planar model for the given dataset." << std::endl;
             break;
         }

         // Extract the planar inliers from the input cloud
         pcl::ExtractIndices<pcl::PointXYZRGB> extract;
         extract.setInputCloud (cloud_filtered);
         extract.setIndices (inliers);
         extract.setNegative (false);

         // Get the points associated with the planar surface
         extract.filter (*cloud_plane);
         std::cout << "PointCloud representing the planar component: " << cloud_plane->points.size () << " data points." << std::endl;

         // Remove the planar inliers, extract the rest
         extract.setNegative (true);
         extract.filter (*cloud_f);
         *cloud_filtered = *cloud_f;
     }

     // Creating the KdTree object for the search method of the extraction
     pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZRGB>);
     tree->setInputCloud (cloud_filtered);

     std::vector<pcl::PointIndices> cluster_indices;
     pcl::EuclideanClusterExtraction<pcl::PointXYZRGB> ec;
     ec.setClusterTolerance (clusterTolerance);
     ec.setMinClusterSize (minCluster);
     ec.setMaxClusterSize (maxCluster);
     ec.setSearchMethod (tree);
     ec.setInputCloud (cloud_filtered);
     ec.extract (cluster_indices);


     pcl::PointCloud<pcl::PointXYZRGB>::Ptr result (new pcl::PointCloud<pcl::PointXYZRGB>);

     int j = 0;
     for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin (); it != cluster_indices.end (); ++it)
     {
       pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_cluster (new pcl::PointCloud<pcl::PointXYZRGB>);

       for (std::vector<int>::const_iterator pit = it->indices.begin (); pit != it->indices.end (); pit++)
         cloud_cluster->points.push_back (cloud_filtered->points[*pit]);

       cloud_cluster->width = cloud_cluster->points.size ();
       cloud_cluster->height = 1;
       cloud_cluster->is_dense = true;

       int colorR = qrand() % ((255 + 1) - 100);
       int colorG = qrand() % ((255 + 1) - 100);
       int colorB = qrand() % ((255 + 1) - 100);

       for(int i = 0; i < cloud_cluster->points.size(); i++) {
           cloud_cluster->points[i].r = colorR;
           cloud_cluster->points[i].g = colorG;
           cloud_cluster->points[i].b = colorB;
       }

       result->operator +=(*cloud_cluster);

       std::cout << "PointCloud representing the Cluster: " << cloud_cluster->points.size () << " data points." << std::endl;
       std::stringstream ss;
       ss << "cloud_cluster_" << j << ".pcd";
       //writer.write<pcl::PointXYZRGB> (ss.str (), *cloud_cluster, false); //*
       j++;
     }

     mainCloud = result;
     this->printSuccess("Finished Clustering");
     this->updateCloud();
}

/**
 * @brief MainWindow::corresGrouping
 *
 * Based on PCL-Tutorial
 * http://pointclouds.org/documentation/tutorials/correspondence_grouping.php
 */
void MainWindow::corresGrouping()
{

    // Set Typedefs
    typedef pcl::PointXYZRGB PointType;
    typedef pcl::Normal NormalType;
    typedef pcl::ReferenceFrame RFType;
    typedef pcl::SHOT352 DescriptorType;

    //Algorithm params
    bool show_keypoints_ (true);
    bool show_correspondences_ (true);

    pcl::PointCloud<PointType>::Ptr model (new pcl::PointCloud<PointType> ());
    pcl::PointCloud<PointType>::Ptr model_keypoints (new pcl::PointCloud<PointType> ());
    pcl::PointCloud<PointType>::Ptr scene_keypoints (new pcl::PointCloud<PointType> ());
    pcl::PointCloud<NormalType>::Ptr model_normals (new pcl::PointCloud<NormalType> ());
    pcl::PointCloud<NormalType>::Ptr scene_normals (new pcl::PointCloud<NormalType> ());
    pcl::PointCloud<DescriptorType>::Ptr model_descriptors (new pcl::PointCloud<DescriptorType> ());
    pcl::PointCloud<DescriptorType>::Ptr scene_descriptors (new pcl::PointCloud<DescriptorType> ());

    // Load Model
    if(clCloud.isNull()) {
        this->displayError("Pleas Set a Cloud first.");
        return;
    } else {
        pcl::io::loadPCDFile(this->clCloud.toStdString(), *model);
    }

    // Get User Input
    double sceneRadius = ui->clSceneRadius->value();
    double modelRadius = ui->clModelRadius->value();
    double descriptorRadius = ui->clDescriptorRadius->value();
    double descriptorDistance = ui->clDescriptorDistance->value();
    double referenceRadius = ui->clReferenceRadius->value();
    double binSize = ui->clBinSize->value();
    double threshold = ui->clThreshold->value();
    bool keypoints = false;
    if(ui->clKeypoints->isChecked()) {
        keypoints = true;
    }

    this->setFallBack();
    this->printInfo("Performing Recognition ...");


    // Maybe CLoud Resolution here

    // Compute Normals
    pcl::NormalEstimationOMP<PointType, NormalType> norm_est;
    norm_est.setKSearch (10);
    norm_est.setInputCloud (model);
    norm_est.compute (*model_normals);
    norm_est.setInputCloud (mainCloud);
    norm_est.compute (*scene_normals);


    this->printInfo("Downsampling of the input clouds ...");

    //  Downsample Clouds to Extract keypoints
    pcl::PointCloud<int> sampled_indices;

    pcl::UniformSampling<PointType> uniform_sampling;
    uniform_sampling.setInputCloud (model);
    uniform_sampling.setRadiusSearch (modelRadius);
    uniform_sampling.compute (sampled_indices);
    pcl::copyPointCloud (*model, sampled_indices.points, *model_keypoints);
    this->printInfo("Model Points: " + QString::number(model->size()) + " - Selected Keypoints: " + QString::number(model_keypoints->size()));

    uniform_sampling.setInputCloud (mainCloud);
    uniform_sampling.setRadiusSearch (sceneRadius);
    uniform_sampling.compute (sampled_indices);
    pcl::copyPointCloud (*mainCloud, sampled_indices.points, *scene_keypoints);
    this->printInfo("Scene Points: " + QString::number(mainCloud->size()) + " - Selected Keypoints: " + QString::number(scene_keypoints->size()));


    //  Compute Descriptor for keypoints
    this->printInfo("Determine Descriptors ...");

    pcl::SHOTEstimationOMP<PointType, NormalType, DescriptorType> descr_est;
    descr_est.setRadiusSearch (descriptorRadius);

    descr_est.setInputCloud (model_keypoints);
    descr_est.setInputNormals (model_normals);
    descr_est.setSearchSurface (model);
    descr_est.compute (*model_descriptors);
    this->printInfo("Descriptors for model found: " + QString::number(model_descriptors->size()));

    descr_est.setInputCloud (scene_keypoints);
    descr_est.setInputNormals (scene_normals);
    descr_est.setSearchSurface (mainCloud);
    descr_est.compute (*scene_descriptors);
    this->printInfo("Descriptors for scene found: " + QString::number(scene_descriptors->size()));

    //pcl::io::savePCDFile("C:/Temp/test.pcd", *scene_descriptors);

    //  Find Model-Scene Correspondences with KdTree
    this->printInfo("Find Correspondences ...");
    pcl::CorrespondencesPtr model_scene_corrs (new pcl::Correspondences());

    pcl::KdTreeFLANN<DescriptorType> match_search;
    match_search.setInputCloud (model_descriptors);

    //  For each scene keypoint descriptor, find nearest neighbor into the model keypoints descriptor cloud and add it to the correspondences vector.
    for (size_t i = 0; i < scene_descriptors->size (); ++i)
    {
        std::vector<int> neigh_indices (1);
        std::vector<float> neigh_sqr_dists (1);
        if (!pcl_isfinite (scene_descriptors->at (i).descriptor[0])) //skipping NaNs
        {
            continue;
        }
        int found_neighs = match_search.nearestKSearch (scene_descriptors->at (i), 1, neigh_indices, neigh_sqr_dists);
        if(found_neighs == 1 && neigh_sqr_dists[0] < (float)descriptorDistance) //  add match only if the squared descriptor distance is less than 0.25 (SHOT descriptor distances are between 0 and 1 by design)
        {
            pcl::Correspondence corr (neigh_indices[0], static_cast<int> (i), neigh_sqr_dists[0]);
            model_scene_corrs->push_back (corr);
        }
    }

    this->printInfo("Correspondences found: " + QString::number(model_scene_corrs->size()));

    this->printInfo("Performing Clustering with Hough3D ...");

    //  Actual Clustering
    std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> > rototranslations;
    std::vector<pcl::Correspondences> clustered_corrs;

    //  Using Hough3D
    pcl::PointCloud<RFType>::Ptr model_rf (new pcl::PointCloud<RFType> ());
    pcl::PointCloud<RFType>::Ptr scene_rf (new pcl::PointCloud<RFType> ());

    pcl::BOARDLocalReferenceFrameEstimation<PointType, NormalType, RFType> rf_est;
    rf_est.setFindHoles (true);
    rf_est.setRadiusSearch (referenceRadius);

    rf_est.setInputCloud (model_keypoints);
    rf_est.setInputNormals (model_normals);
    rf_est.setSearchSurface (model);
    rf_est.compute (*model_rf);

    rf_est.setInputCloud (scene_keypoints);
    rf_est.setInputNormals (scene_normals);
    rf_est.setSearchSurface (mainCloud);
    rf_est.compute (*scene_rf);

    //  Clustering
    pcl::Hough3DGrouping<PointType, PointType, RFType, RFType> clusterer;
    clusterer.setHoughBinSize (binSize);
    clusterer.setHoughThreshold (threshold);
    clusterer.setUseInterpolation (true);
    clusterer.setUseDistanceWeight (false);

    clusterer.setInputCloud (model_keypoints);
    clusterer.setInputRf (model_rf);
    clusterer.setSceneCloud (scene_keypoints);
    clusterer.setSceneRf (scene_rf);
    clusterer.setModelSceneCorrespondences (model_scene_corrs);

    //clusterer.cluster (clustered_corrs);
    clusterer.recognize (rototranslations, clustered_corrs);

    // Using GeometricConsistency
//    pcl::GeometricConsistencyGrouping<PointType, PointType> gc_clusterer;
//    gc_clusterer.setGCSize (cg_size_);
//    gc_clusterer.setGCThreshold (cg_thresh_);

//    gc_clusterer.setInputCloud (model_keypoints);
//    gc_clusterer.setSceneCloud (scene_keypoints);
//    gc_clusterer.setModelSceneCorrespondences (model_scene_corrs);

//    //gc_clusterer.cluster (clustered_corrs);
//    gc_clusterer.recognize (rototranslations, clustered_corrs);


    this->printInfo("Model instances found: " + QString::number(rototranslations.size()));

    for (size_t i = 0; i < rototranslations.size (); ++i)
    {

        //this->printInfo("Instance " + QString::number(i + 1) + ": ");
        //this->printInfo("Correspondences belonging to this instance: " + QString::number(clustered_corrs[i].size()));

      // Print the rotation matrix and translation vector
      Eigen::Matrix3f rotation = rototranslations[i].block<3,3>(0, 0);
      Eigen::Vector3f translation = rototranslations[i].block<3,1>(0, 3);

//      printf ("\n");
//      printf ("            | %6.3f %6.3f %6.3f | \n", rotation (0,0), rotation (0,1), rotation (0,2));
//      printf ("        R = | %6.3f %6.3f %6.3f | \n", rotation (1,0), rotation (1,1), rotation (1,2));
//      printf ("            | %6.3f %6.3f %6.3f | \n", rotation (2,0), rotation (2,1), rotation (2,2));
//      printf ("\n");
//      printf ("        t = < %0.3f, %0.3f, %0.3f >\n", translation (0), translation (1), translation (2));
    }


    pcl::PointCloud<PointType>::Ptr off_scene_model (new pcl::PointCloud<PointType> ());
    pcl::PointCloud<PointType>::Ptr off_scene_model_keypoints (new pcl::PointCloud<PointType> ());

    //  We are translating the model so that it doesn't end in the middle of the scene representation
    pcl::transformPointCloud (*model, *off_scene_model, Eigen::Vector3f (-1,0,0), Eigen::Quaternionf (1, 0, 0, 0));
    pcl::transformPointCloud (*model_keypoints, *off_scene_model_keypoints, Eigen::Vector3f (-1,0,0), Eigen::Quaternionf (1, 0, 0, 0));

    pcl::visualization::PointCloudColorHandlerCustom<PointType> off_scene_model_color_handler (off_scene_model, 255, 255, 128);
    visu->visualizer.removePointCloud("off_scene_model");
    visu->visualizer.addPointCloud (off_scene_model, off_scene_model_color_handler, "off_scene_model");

    visu->visualizer.removePointCloud("scene_keypoints");
    visu->visualizer.removePointCloud("off_scene_model_keypoints");

    if(keypoints)
    {
        pcl::visualization::PointCloudColorHandlerCustom<PointType> scene_keypoints_color_handler (scene_keypoints, 0, 0, 255);

        visu->visualizer.addPointCloud (scene_keypoints, scene_keypoints_color_handler, "scene_keypoints");
        visu->visualizer.setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 5, "scene_keypoints");

        pcl::visualization::PointCloudColorHandlerCustom<PointType> off_scene_model_keypoints_color_handler (off_scene_model_keypoints, 0, 0, 255);

        visu->visualizer.addPointCloud (off_scene_model_keypoints, off_scene_model_keypoints_color_handler, "off_scene_model_keypoints");
        visu->visualizer.setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 5, "off_scene_model_keypoints");
    }


    visu->visualizer.removeAllShapes();
    for (size_t i = 0; i < rototranslations.size (); ++i)
    {
        pcl::PointCloud<PointType>::Ptr rotated_model (new pcl::PointCloud<PointType> ());
        pcl::transformPointCloud (*model, *rotated_model, rototranslations[i]);

        std::stringstream ss_cloud;
        ss_cloud << "instance" << i;

        pcl::visualization::PointCloudColorHandlerCustom<PointType> rotated_model_color_handler (rotated_model, 255, 0, 0);
        visu->visualizer.removePointCloud(ss_cloud.str());
        visu->visualizer.addPointCloud (rotated_model, rotated_model_color_handler, ss_cloud.str ());

        if (show_correspondences_)
        {
            for (size_t j = 0; j < clustered_corrs[i].size (); ++j)
            {
                std::stringstream ss_line;
                ss_line << "correspondence_line" << i << "_" << j;
                PointType& model_point = off_scene_model_keypoints->at (clustered_corrs[i][j].index_query);
                PointType& scene_point = scene_keypoints->at (clustered_corrs[i][j].index_match);

                //  We are drawing a line for each pair of clustered correspondences found between the model and the scene
                visu->visualizer.addLine<PointType, PointType> (model_point, scene_point, 0, 255, 0, ss_line.str ());
            }
        }
    }

    this->printInfo("Finished Recognition");
    this->updateCloud();

}

void MainWindow::clSetCloud()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Point Cloud"), this->lastClFile, tr("PCD (*.pcd)"));

    if(fileName != NULL) {

        this->lastClFile = QFileInfo(fileName).path();

        this->clCloud = fileName;
        ui->clFilePath->clear();
        ui->clFilePath->append(fileName);
    }
}

void MainWindow::setWhite()
{
    this->bleachCloud(mainCloud);
    visu->visualizer.updatePointCloud(mainCloud, cloud);
    visu->update();
    this->printSuccess("Set Color of Cloud to White");
}

void MainWindow::undo()
{
    if(fallbackCloud != NULL) {
        mainCloud = pcl::PointCloud<pcl::PointXYZRGB>::Ptr(fallbackCloud);
        visu->visualizer.removeAllPointClouds();
        visu->visualizer.removeAllShapes();
        visu->visualizer.addPointCloud(mainCloud, cloud);
        visu->update();
        this->printSuccess("Undo last Step");
    }

}

void MainWindow::showAboutDialog()
{
    QDialog *about = new QDialog(0,Qt::WindowSystemMenuHint | Qt::WindowTitleHint);

    Ui_aboutDialog aboutUi;
    aboutUi.setupUi(about);

    about->show();
}

void MainWindow::showDatabaseDialog()
{
    database->initData(this->databasePath);
    database->exec();
}

void MainWindow::setDatabase()
{
    QString directory = QFileDialog::getExistingDirectory(this, tr("Select a Directory"),QDir::currentPath());

    if(directory != NULL) {
        this->databasePath = directory.toStdString();
        ui->fdDatabasePath->clear();
        ui->fdDatabasePath->append(directory);
    }

}

void MainWindow::calcShotFeatures()
{
    if(mainCloud->size() == 0) {
        this->displayError("Please open a Point Cloud first!");
        return;
    }

    // Set Typedefs
    typedef pcl::PointXYZRGB PointType;
    typedef pcl::Normal NormalType;
    typedef pcl::ReferenceFrame RFType;
    typedef pcl::SHOT352 DescriptorType;

    pcl::PointCloud<NormalType>::Ptr normals (new pcl::PointCloud<NormalType> ());
    //pcl::PointCloud<DescriptorType>::Ptr descriptors (new pcl::PointCloud<DescriptorType> ());

    // Get User Input
    double radius = ui->fdShotSceneRadius->value();
    double descriptorRadius = ui->fdShotDescriptorRadius->value();

    this->printInfo("Calculating Features ...");

    // Compute Normals
    pcl::NormalEstimationOMP<PointType, NormalType> norm_est;
    norm_est.setKSearch(10);
    norm_est.setInputCloud(mainCloud);
    norm_est.compute(*normals);

    this->printInfo("Downsampling of the cloud ...");

    //Downsample Clouds to Extract keypoints
    pcl::PointCloud<int> sampled_indices;
    pcl::UniformSampling<PointType> uniform_sampling;
    uniform_sampling.setInputCloud(mainCloud);
    uniform_sampling.setRadiusSearch(radius);
    uniform_sampling.compute(sampled_indices);
    pcl::copyPointCloud(*mainCloud, sampled_indices.points, *shotKeypoints);
    this->printInfo("Points: " + QString::number(mainCloud->size()) + " - Selected Keypoints: " + QString::number(shotKeypoints->size()));

    //Compute Descriptor for keypoints
    this->printInfo("Determine Descriptors ...");

    pcl::SHOTEstimationOMP<PointType, NormalType, DescriptorType> descr_est;
    descr_est.setRadiusSearch(descriptorRadius);

    descr_est.setInputCloud(shotKeypoints);
    descr_est.setInputNormals(normals);
    descr_est.setSearchSurface(mainCloud);
    descr_est.compute(*shotDescriptors);

    size_t size = shotDescriptors->size();
    this->printInfo("Descriptors found: " + QString::number(size));
    this->printInfo("Caution! Descriptors are not saved yet!");

    if(size > 0){
        ui->fdAddToDatabase->setEnabled(true);
    }

    ui->fdMessage->setText(" Descriptors found: " + QString::number(size) + " ");
    ui->fdMessage->setStyleSheet("QLabel { background-color : green; color : white; }");
    //Util::writeFile(this->databasePath, "hallo.txt", "Hallo Welt!");
}

void MainWindow::addToDatabase()
{
    QString ident = ui->fdIdent->text();
    if(ident.isEmpty() || ident.length() < 3 ) {
        this->displayError("Please enter a valid Identifier! (At least 3 Characters)");
        return;
    }

    QString databasePath = QString::fromStdString(this->databasePath);

    QString metaFileName = databasePath + "/" + ident + ".meta";
    QString modelFileName = databasePath + "/" + ident + "_model.pcd";
    QString keypointsFileName = databasePath + "/" + ident + "_points.pcd";
    QString featuresFileName = databasePath + "/" + ident + "_features.pcd";

    this->printInfo("Saving to: " + metaFileName);

    pcl::io::savePCDFile(featuresFileName.toStdString(), *shotDescriptors);
    pcl::io::savePCDFile(keypointsFileName.toStdString(), *shotKeypoints);
    pcl::io::savePCDFile(modelFileName.toStdString(), *mainCloud);

    QSettings settings(metaFileName, QSettings::IniFormat);
    settings.setValue("identifier",ident);
    settings.setValue("countFeatures",QString::number(shotDescriptors->size()));
    settings.setValue("url",ui->fdURL->text());
    settings.sync();
}

void MainWindow::identifyScene()
{
    this->calcShotFeatures();

    // Get User Input
    double descriptorDist = ui->fdDescriptorDistance->value();

    // Set Typedefs
    typedef pcl::PointXYZRGB PointType;
    typedef pcl::Normal NormalType;
    typedef pcl::ReferenceFrame RFType;
    typedef pcl::SHOT352 DescriptorType;

    // Build Data Structures
    list<string> files = Util::readFileNames(databasePath,"meta");
    QString databasePath = QString::fromStdString(this->databasePath);

    QList<string> databaseList = QList<string>::fromStdList(files);
    pcl::PointCloud<pcl::SHOT352>::Ptr loadedFeatures = pcl::PointCloud<pcl::SHOT352>::Ptr(new pcl::PointCloud<pcl::SHOT352>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr loadedKeypoints = pcl::PointCloud<pcl::PointXYZRGB>::Ptr(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr loadedModel = pcl::PointCloud<pcl::PointXYZRGB>::Ptr(new pcl::PointCloud<pcl::PointXYZRGB>);

    //QMap<QString,pcl::PointCloud<pcl::SHOT352>::Ptr> featuresMap;
    //QMap<QString,pcl::PointCloud<pcl::PointXYZRGB>::Ptr> keypointsMap;

    QList<QString> recognizedObjects;
    QMap<QString,pcl::PointCloud<pcl::PointXYZRGB>::Ptr> objectMap;
    QMap<QString,pcl::PointXYZ> centerMap;

    // Calculate the center of the main cloud to determine direction of the text
    Eigen::Vector4f mainCloudCenter;
    pcl::compute3DCentroid(*mainCloud, mainCloudCenter);

    // Iterate through every element in the database and init the Maps
    for (int i = 0; i < databaseList.size(); ++i) {
        // Get the identifier
        QString ident = QString::fromStdString(databaseList.at(i)).split(".",QString::SkipEmptyParts).at(0);

        // Set the filenames
        QString metaFileName = databasePath + "/" + ident + ".meta";
        QString modelFileName = databasePath + "/" + ident + "_model.pcd";
        QString keypointsFileName = databasePath + "/" + ident + "_points.pcd";
        QString featuresFileName = databasePath + "/" + ident + "_features.pcd";

        pcl::PointCloud<NormalType>::Ptr model_normals (new pcl::PointCloud<NormalType> ());
        pcl::PointCloud<NormalType>::Ptr scene_normals (new pcl::PointCloud<NormalType> ());

        this->printInfo("Current Database Element: " + ident);

        //keypointsMap[ident] = pcl::PointCloud<pcl::PointXYZRGB>::Ptr(new pcl::PointCloud<pcl::PointXYZRGB>);
        pcl::io::loadPCDFile(featuresFileName.toStdString(), *loadedFeatures);
        pcl::io::loadPCDFile(keypointsFileName.toStdString(), *loadedKeypoints);
        pcl::io::loadPCDFile(modelFileName.toStdString(), *loadedModel);

        pcl::KdTreeFLANN<pcl::SHOT352> match_search;
        match_search.setInputCloud (loadedFeatures);
        pcl::CorrespondencesPtr model_scene_corrs (new pcl::Correspondences());

        //  For each scene keypoint descriptor, find nearest neighbor into the model keypoints descriptor cloud and add it to the correspondences vector.
        for (size_t i = 0; i < shotDescriptors->size (); ++i)
        {
            std::vector<int> neigh_indices (1);
            std::vector<float> neigh_sqr_dists (1);
            if (!pcl_isfinite (shotDescriptors->at (i).descriptor[0])) //skipping NaNs
            {
                continue;
            }
            int found_neighs = match_search.nearestKSearch (shotDescriptors->at (i), 1, neigh_indices, neigh_sqr_dists);
            if(found_neighs == 1 && neigh_sqr_dists[0] < (float)descriptorDist) //  add match only if the squared descriptor distance is less than 0.25 (SHOT descriptor distances are between 0 and 1 by design)
            {
                pcl::Correspondence corr (neigh_indices[0], static_cast<int> (i), neigh_sqr_dists[0]);
                model_scene_corrs->push_back (corr);
            }
        }

        this->printInfo("Correspondences found: " + QString::number(model_scene_corrs->size()));
        this->printInfo("Performing Identifying with Hough3D ...");

        // Compute Normals
        pcl::NormalEstimationOMP<PointType, NormalType> norm_est;
        norm_est.setKSearch (10);
        norm_est.setInputCloud (loadedModel);
        norm_est.compute (*model_normals);
        norm_est.setInputCloud (mainCloud);
        norm_est.compute (*scene_normals);


        //  Actual Clustering
        std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> > rototranslations;
        std::vector<pcl::Correspondences> clustered_corrs;

        //  Using Hough3D
        pcl::PointCloud<RFType>::Ptr model_rf (new pcl::PointCloud<RFType> ());
        pcl::PointCloud<RFType>::Ptr scene_rf (new pcl::PointCloud<RFType> ());

        pcl::BOARDLocalReferenceFrameEstimation<PointType, NormalType, RFType> rf_est;
        rf_est.setFindHoles (true);

        rf_est.setRadiusSearch (0.10);
        //rf_est.setRadiusSearch (0.60);

        rf_est.setInputCloud (loadedKeypoints);
        rf_est.setInputNormals (model_normals);
        rf_est.setSearchSurface (loadedModel);
        rf_est.compute (*model_rf);

        rf_est.setInputCloud (shotKeypoints);
        rf_est.setInputNormals (scene_normals);
        rf_est.setSearchSurface (mainCloud);
        rf_est.compute (*scene_rf);

        //  Clustering
        pcl::Hough3DGrouping<PointType, PointType, RFType, RFType> clusterer;
        //clusterer.setHoughBinSize (0.10);
        clusterer.setHoughBinSize (0.21);
        //clusterer.setHoughThreshold (-0.4);
        clusterer.setHoughThreshold (-1.0);
        //clusterer.setHoughThreshold (-0.5);
        clusterer.setUseInterpolation (true);
        clusterer.setUseDistanceWeight (false);

        clusterer.setInputCloud (loadedKeypoints);
        clusterer.setInputRf (model_rf);
        clusterer.setSceneCloud (shotKeypoints);
        clusterer.setSceneRf (scene_rf);
        clusterer.setModelSceneCorrespondences (model_scene_corrs);

        //clusterer.cluster (clustered_corrs);
        clusterer.recognize (rototranslations, clustered_corrs);

        this->printInfo("Model instances found: " + QString::number(rototranslations.size()));

        for (size_t i = 0; i < rototranslations.size (); ++i)
        {
            pcl::PointCloud<PointType>::Ptr rotated_model (new pcl::PointCloud<PointType> ());
            pcl::transformPointCloud (*loadedModel, *rotated_model, rototranslations[i]);
            std::stringstream ss_cloud;
            ss_cloud << ident.toStdString() << i;

            pcl::visualization::PointCloudColorHandlerCustom<PointType> rotated_model_color_handler (rotated_model, Util::randInt(50,255), Util::randInt(50,255), Util::randInt(192,255));
            visu->visualizer.removePointCloud(ss_cloud.str());

            // Draw the labels of the objects
            Eigen::Vector4f center;
            pcl::compute3DCentroid(*rotated_model, center);
            pcl::PointXYZ startLine(center[0], center[1], center[2]);
            centerMap[ident] = startLine;

            //this->printInfo("Center: " + QString::number(center[0]));
            visu->visualizer.addPointCloud (rotated_model, rotated_model_color_handler, ss_cloud.str ());

            // Add for semenatic detection later
            objectMap[ident] = rotated_model;
            recognizedObjects.append(ident);

        }

        this->updateCloud();

    }

    // Do the semantic
    this->printInfo("Sending Request to Semantic Service");
    QtJson::JsonObject request;
    QtJson::JsonArray objects;

    QMap<QString, QString> urlMap;

    for (int i = 0; i < recognizedObjects.size(); ++i) {
        QString ident = recognizedObjects.at(i);
        QSettings settings(QString::fromStdString(this->databasePath) + "/" + ident + ".meta", QSettings::IniFormat);
        QString url = settings.value("url", "unknown").toString();
        objects.append(url);
        urlMap[ident] = url;
    }

    request["objects"] = objects;
    this->printInfo("Request Payload: " + QtJson::serialize(request));
    QtJson::JsonObject result = restAPI.post("recognition", request);
    for (int i = 0; i < recognizedObjects.size(); ++i) {
        QString identOrigin = recognizedObjects.at(i);
        QString url = urlMap[recognizedObjects.at(i)];

       pcl::PointXYZ endLine = this->calcOffset(centerMap[identOrigin],mainCloudCenter);
       visu->visualizer.addLine<pcl::PointXYZ, pcl::PointXYZ> (centerMap[identOrigin], endLine, 0, 255, 0, identOrigin.toStdString() + "_line");
       visu->visualizer.setShapeRenderingProperties(pcl::visualization::PCL_VISUALIZER_LINE_WIDTH, 3.0, identOrigin.toStdString() + "_line");
       QString label = identOrigin;

       if(result.contains(url)) {
            QtJson::JsonObject sub = result[url].toMap();
            if(sub.contains("connections")) {
                QtJson::JsonArray connections = (sub["connections"]).toList();

                foreach(QVariant c, connections) {
                    QString connection = c.toString();
                    this->printSuccess("Found Connection: " + url + " >>> " + connection);
                    QString identConnection = urlMap.key(connection);
                    if(recognizedObjects.contains(identConnection)) {
                        string lineId = identOrigin.toStdString() + identConnection.toStdString() + "_line";

                        float distance = pcl::euclideanDistance<pcl::PointXYZ,pcl::PointXYZ>(centerMap[identConnection],centerMap[identOrigin]);

                        this->printInfo("Distance: " + QString::number(distance));
                        if(distance < 2.0) {
                            this->printInfo("Visualize Connection");
                            visu->visualizer.addLine<pcl::PointXYZ, pcl::PointXYZ> (this->calcOffset(centerMap[identConnection], mainCloudCenter),
                                                                                    endLine,
                                                                                    255, 255, 0,
                                                                                    lineId);
                            visu->visualizer.setShapeRenderingProperties(pcl::visualization::PCL_VISUALIZER_LINE_WIDTH, 3.0, lineId);
                            double x = (centerMap[identConnection].x + centerMap[identOrigin].x) / 2;
                            double y = (centerMap[identConnection].y + centerMap[identOrigin].y) / 2;
                            double z = (centerMap[identConnection].z + centerMap[identOrigin].z) / 2;
                        }

                        //this->addText("hallo welt", x, y, z, 0.1, 1.0, 0.0, 0.0);

                    }
                }

            }

            if(sub.contains("name")) {
                label = sub["name"].toString();
            }

            if(sub.contains("properties")) {
                QtJson::JsonObject properties = sub["properties"].toMap();
                QtJson::JsonObject::const_iterator i = properties.constBegin();
                double lineWidth = 0.4;
                while(i != properties.constEnd()) {
                    label += "\n" + i.key() + ": " + i.value().toString();
                    //this->addText(i.key() + ": " + i.value().toString(), endLine.x+lineWidth, endLine.y, endLine.z, 0.10, 1.0, 1.0, 1.0);
                    //this->printSuccess(i.key() + " --- "  + i.value().toString());
                    ++i;
                    lineWidth += lineWidth;
                }

            }
        }

       pcl::PointXYZ endLabel = this->calcOffset(centerMap[identOrigin],mainCloudCenter,1.7);
       this->addText(label, endLabel.x, endLabel.y, endLabel.z, 0.10, 1.0, 1.0, 1.0);

    }



    this->printInfo("Finished Semantic");
    this->updateCloud();


}

pcl::PointXYZ MainWindow::calcOffset(pcl::PointXYZ& center, Eigen::Vector4f& mainCloudCenter, double offset) {
    Eigen::Vector4f endLineVector;
    endLineVector[0] = center.x;
    endLineVector[1] = center.y;
    endLineVector[2] = center.z;
    for(int i = 0; i < 3; i++){
        if(endLineVector[i] >= mainCloudCenter[i]) {
            endLineVector[i] += offset;
        } else {
            endLineVector[i] -= offset;
        }
    }
   pcl::PointXYZ endLine(endLineVector[0], endLineVector[1], endLineVector[2]);
   return endLine;
}




void MainWindow::toggleCoordinateSystem()
{
    if(ui->actionShowCoordinateSystem->isChecked()) {
        visu->visualizer.addCoordinateSystem();
    }
    else {
        visu->visualizer.removeCoordinateSystem();
    }
    visu->update();
}

void MainWindow::toggleObjectsDock()
{
    if(ui->rgDockWidget->isVisible()) {
        ui->rgDockWidget->hide();
        ui->actionObjectsDock->setChecked(false);
    } else {
        ui->rgDockWidget->show();
        ui->actionObjectsDock->setChecked(true);
    }
}

