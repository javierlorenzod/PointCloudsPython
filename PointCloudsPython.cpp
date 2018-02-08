#include <cstring>
#include <iostream>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/features/normal_3d.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/features/normal_3d_omp.h>

using namespace std;
using namespace pcl;
using namespace pcl::io;

// HELPERS =========================================================================================

int PclArrayToPointCloud(float* points, int nPoints, PointCloud<PointXYZ>& cloud)
{
  for (int i = 0; i < nPoints; i++)
    cloud.push_back(PointXYZ(points[3*i+0], points[3*i+1], points[3*i+2]));
  
  return 0;
}

int PclArrayToPointCloudPtr(float* points, int nPoints, PointCloud<PointXYZ>::Ptr& cloud)
{
  for (int i = 0; i < nPoints; i++)
    cloud->push_back(PointXYZ(points[3*i+0], points[3*i+1], points[3*i+2]));
  
  return 0;
}

int PclNormalsToNewArray(PointCloud<Normal>& cloud, float** pnormals)
{
  int nNormals = cloud.size();
  float* normals = new float[nNormals * 3];
  *pnormals = normals;
  
  for (int i = 0; i < nNormals; i++)
  {
    normals[3*i+0] = cloud[i].normal_x;
    normals[3*i+1] = cloud[i].normal_y;
    normals[3*i+2] = cloud[i].normal_z;
  }
  
  return 0;
}

int PclPointCloudToNewArray(PointCloud<PointXYZ>& cloud, float** ppoints, int* nPoints)
{
  *nPoints = cloud.size();
  float* points = new float[*nPoints * 3];
  *ppoints = points;
  
  for (int i = 0; i < *nPoints; i++)
  {
    points[3*i+0] = cloud[i].x;
    points[3*i+1] = cloud[i].y;
    points[3*i+2] = cloud[i].z;
  }
  
  return 0;
}

// EXTERN ==========================================================================================

extern "C" int CopyAndFree(float* in, float* out, int nPoints)
{
  memcpy(out, in, sizeof(float)*nPoints*3);
  delete[] in;
  return 0;
}

extern "C" int PclComputeNormals(float* pointsIn, int nPointsIn, int kNeighborhood,
  float radiusNeighborhood, float** normalsOut)
{
  PointCloud<PointXYZ>::Ptr cloudIn(new PointCloud<PointXYZ>);
  PclArrayToPointCloudPtr(pointsIn, nPointsIn, cloudIn);
  
  PointCloud<Normal> normals;
  search::KdTree<PointXYZ>::Ptr tree(new search::KdTree<PointXYZ> ());
  
  //NormalEstimation<PointXYZ, Normal> ne;
  NormalEstimationOMP<PointXYZ, Normal> ne(4);
  ne.setInputCloud(cloudIn);  
  ne.setSearchMethod(tree);
  
  if ((kNeighborhood <= 0) && (radiusNeighborhood > 0))
    ne.setRadiusSearch(radiusNeighborhood);
  else if ((kNeighborhood > 0) && (radiusNeighborhood <= 0))
    ne.setKSearch(kNeighborhood);
  else
    return -1;
  
  ne.compute(normals);
  if (normals.size() != cloudIn->size())
    return -2;
  
  PclNormalsToNewArray(normals, normalsOut);
  return 0;
}

extern "C" int PclLoadPcd(char* fileName, float** ppoints, int* nPoints)
{
  PointCloud<PointXYZ> cloud;
  if (loadPCDFile<PointXYZ>(fileName, cloud) < 0)
    return -1;
  
  PclPointCloudToNewArray(cloud, ppoints, nPoints);
  return 0;
}

extern "C" int PclSavePcd(char* fileName, float* points, int nPoints)
{
  PointCloud<PointXYZ> cloud;
  PclArrayToPointCloud(points, nPoints, cloud);

  if (savePCDFileASCII(fileName, cloud) < 0)
    return -1;
  
  return 0;
}

extern "C" int PclVoxelize(float* pointsIn, int nPointsIn, float voxelSize, float** pointsOut, int* nPointsOut)
{
  PointCloud<PointXYZ>::Ptr cloudIn(new PointCloud<PointXYZ>);
  PointCloud<PointXYZ> cloudOut;
  
  PclArrayToPointCloudPtr(pointsIn, nPointsIn, cloudIn);
  
  VoxelGrid<PointXYZ> grid;
  grid.setInputCloud(cloudIn);
  grid.setLeafSize(voxelSize, voxelSize, voxelSize);
  grid.filter(cloudOut);
  
  PclPointCloudToNewArray(cloudOut, pointsOut, nPointsOut);
  return 0;
}

/*
  template<class T>
  TRAJOPT_API typename pcl::PointCloud<T>::Ptr statisticalOutlierRemoval(typename pcl::PointCloud<T>::ConstPtr in, int kNeighbors, float outlierStd) {
    typename pcl::PointCloud<T>::Ptr out (new typename pcl::PointCloud<T>);
    pcl::StatisticalOutlierRemoval< T > sor;
    sor.setInputCloud (in);
    sor.setMeanK (kNeighbors);
    sor.setStddevMulThresh (outlierStd);
    sor.filter (*out);
    return out;
  }
*/