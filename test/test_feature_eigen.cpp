/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2011, Willow Garage, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: test_feature.cpp 3564 2011-12-16 06:11:13Z rusu $
 *
 */

#include <gtest/gtest.h>

#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/features/feature.h>
#include <pcl/features/normal_3d_omp.h>
#include <pcl/features/moment_invariants.h>
#include <pcl/features/boundary.h>
#include <pcl/features/principal_curvatures.h>
#include <pcl/features/pfh.h>
#include <pcl/features/shot.h>
#include <pcl/features/shot_omp.h>
#include <pcl/features/spin_image.h>
#include <pcl/features/fpfh.h>
#include <pcl/features/fpfh_omp.h>
#include <pcl/features/ppf.h>
#include <pcl/features/vfh.h>
#include <pcl/features/gfpfh.h>
#include <pcl/features/rsd.h>
#include <pcl/features/intensity_gradient.h>
#include <pcl/features/intensity_spin.h>
#include <pcl/features/rift.h>
#include <pcl/features/3dsc.h>
#include <pcl/features/usc.h>
#include <iostream>

using namespace pcl;
using namespace pcl::io;
using namespace std;

typedef search::KdTree<PointXYZ>::Ptr KdTreePtr;

PointCloud<PointXYZ> cloud;
vector<int> indices;
KdTreePtr tree;

///////////////////////////////////////////////////////////////////////////////////
template <typename FeatureEstimation, typename PointT, typename NormalT> void
testIndicesAndSearchSurfaceEigen (const typename PointCloud<PointT>::Ptr & points,
                                  const typename PointCloud<NormalT>::Ptr & normals,
                                  const boost::shared_ptr<vector<int> > & indices, int ndims)
{
  //
  // Test setIndices and setSearchSurface
  //
  PointCloud<Eigen::MatrixXf> full_output, output0, output1, output2;

  // Compute for all points and then subsample the results
  FeatureEstimation est0;
  est0.setSearchMethod (typename search::KdTree<PointT>::Ptr (new search::KdTree<PointT>));
  est0.setKSearch (10);
  est0.setInputCloud (points);
  est0.setInputNormals (normals);
  est0.compute (full_output);
  output0 = PointCloud<Eigen::MatrixXf> (full_output, *indices);
  //copyPointCloud (full_output, *indices, output0);

  // Compute with all points as "search surface" and the specified sub-cloud as "input"
  typename PointCloud<PointT>::Ptr subpoints (new PointCloud<PointT>);
  copyPointCloud (*points, *indices, *subpoints);
  FeatureEstimation est1;
  est1.setSearchMethod (typename search::KdTree<PointT>::Ptr (new search::KdTree<PointT>));
  est1.setKSearch (10);
  est1.setInputCloud (subpoints);
  est1.setSearchSurface (points);
  est1.setInputNormals (normals);
  est1.compute (output1);

  // Compute with all points as "input" and the specified indices
  FeatureEstimation est2;
  est2.setSearchMethod (typename search::KdTree<PointT>::Ptr (new search::KdTree<PointT>));
  est2.setKSearch (10);
  est2.setInputCloud (points);
  est2.setInputNormals (normals);
  est2.setIndices (indices);
  est2.compute (output2);

  // All three of the above cases should produce equivalent results
  ASSERT_EQ (output0.points.rows (), output1.points.rows ());
  ASSERT_EQ (output1.points.rows (), output2.points.rows ());
  for (int i = 0; i < output1.points.rows (); ++i)
  {
    for (int j = 0; j < ndims; ++j)
    {
      ASSERT_EQ (output0.points (i, j), output1.points (i, j));
      ASSERT_EQ (output1.points (i, j), output2.points (i, j));
    }
  }

  //
  // Test the combination of setIndices and setSearchSurface
  //
  PointCloud<Eigen::MatrixXf> output3, output4;

  boost::shared_ptr<vector<int> > indices2 (new vector<int> (0));
  for (size_t i = 0; i < (indices->size ()/2); ++i)
    indices2->push_back (i);

  // Compute with all points as search surface + the specified sub-cloud as "input" but for only a subset of indices
  FeatureEstimation est3;
  est3.setSearchMethod (typename search::KdTree<PointT>::Ptr (new search::KdTree<PointT>));
  est3.setKSearch (10);
  est3.setSearchSurface (points);
  est3.setInputNormals (normals);
  est3.setInputCloud (subpoints);
  est3.setIndices (indices2);
  est3.compute (output3);

  // Start with features for each point in "subpoints" and then subsample the results
  output4 = PointCloud<Eigen::MatrixXf> (output0, *indices2); // (Re-using "output0" from above)
  //copyPointCloud (output0, *indices2, output4); 

  // The two cases above should produce equivalent results
  ASSERT_EQ (output3.points.rows (), output4.points.rows ());
  for (int i = 0; i < output3.points.rows (); ++i)
  {
    for (int j = 0; j < ndims; ++j)
    {
      ASSERT_EQ (output3.points (i, j), output4.points (i, j));
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////
template <typename FeatureEstimation, typename PointT, typename NormalT, typename OutputT> FeatureEstimation
createSHOTDesc (const typename PointCloud<NormalT>::Ptr & normals,
                const int nr_shape_bins = 10,
                const int nr_color_bins = 30,
                const bool describe_shape = true,
                const bool describe_color = false)
{
  FeatureEstimation f (nr_shape_bins);
  f.setInputNormals (normals);
  return (f);
}

///////////////////////////////////////////////////////////////////////////////////
template <typename FeatureEstimation, typename NormalT, typename OutputT> FeatureEstimation
createSHOTDesc (const typename PointCloud<NormalT>::Ptr & normals,
                const int nr_shape_bins = 10,
                const int nr_color_bins = 30,
                const bool describe_shape = true,
                const bool describe_color = false)
{
  FeatureEstimation f (describe_shape, describe_color, nr_shape_bins,nr_color_bins);
  f.setInputNormals (normals);
  return (f);
}

///////////////////////////////////////////////////////////////////////////////////
template <> UniqueShapeContext<PointXYZ, Eigen::MatrixXf>
createSHOTDesc<UniqueShapeContext<PointXYZ, Eigen::MatrixXf>, PointXYZ, Normal, Eigen::MatrixXf> (
    const PointCloud<Normal>::Ptr & normals,
    const int nr_shape_bins,
    const int nr_color_bins,
    const bool describe_shape,
    const bool describe_color)
    {
  UniqueShapeContext<PointXYZ, Eigen::MatrixXf> usc;
  usc.setAzimuthBins (4);
  usc.setElevationBins (4);
  usc.setRadiusBins (4);
  usc.setMinimalRadius (0.004);
  usc.setPointDensityRadius (0.008);
  usc.setLocalRadius (0.04);
  return (usc);
    }

///////////////////////////////////////////////////////////////////////////////////
template <> ShapeContext3DEstimation<PointXYZ, Normal, Eigen::MatrixXf>
createSHOTDesc<ShapeContext3DEstimation<PointXYZ, Normal, Eigen::MatrixXf>, PointXYZ, Normal, Eigen::MatrixXf> (
    const PointCloud<Normal>::Ptr & normals,
    const int nr_shape_bins,
    const int nr_color_bins,
    const bool describe_shape,
    const bool describe_color)
    {
  ShapeContext3DEstimation<PointXYZ, Normal, Eigen::MatrixXf> sc3d;
  sc3d.setAzimuthBins (4);
  sc3d.setElevationBins (4);
  sc3d.setRadiusBins (4);
  sc3d.setMinimalRadius (0.004);
  sc3d.setPointDensityRadius (0.008);
  sc3d.setInputNormals (normals);
  return (sc3d);
    }

///////////////////////////////////////////////////////////////////////////////////
template <typename FeatureEstimation, typename PointT, typename NormalT> void
testSHOTIndicesAndSearchSurfaceEigen (const typename PointCloud<PointT>::Ptr & points,
                                      const typename PointCloud<NormalT>::Ptr & normals,
                                      const boost::shared_ptr<vector<int> > & indices,
                                      const int nr_shape_bins = 10,
                                      const int nr_color_bins = 30,
                                      const bool describe_shape = true,
                                      const bool describe_color = false)
{
  double radius = 0.04;
  //
  // Test setIndices and setSearchSurface
  //
  PointCloud<Eigen::MatrixXf> full_output, output0, output1, output2;

  // Compute for all points and then subsample the results
  FeatureEstimation est0 = createSHOTDesc<FeatureEstimation, PointT, NormalT, Eigen::MatrixXf>(normals, nr_shape_bins,nr_color_bins,describe_shape,describe_color);
  est0.setSearchMethod (typename search::KdTree<PointT>::Ptr (new search::KdTree<PointT>));
  est0.setRadiusSearch (radius);
  est0.setInputCloud (points);
  est0.compute (full_output);

  output0 = PointCloud<Eigen::MatrixXf> (full_output, *indices);

  // Compute with all points as "search surface" and the specified sub-cloud as "input"
  typename PointCloud<PointT>::Ptr subpoints (new PointCloud<PointT>);
  copyPointCloud (*points, *indices, *subpoints);
  FeatureEstimation est1 = createSHOTDesc<FeatureEstimation, PointT, NormalT, Eigen::MatrixXf>(normals, nr_shape_bins,nr_color_bins,describe_shape,describe_color);
  est1.setSearchMethod (typename search::KdTree<PointT>::Ptr (new search::KdTree<PointT>));
  est1.setRadiusSearch (radius);
  est1.setInputCloud (subpoints);
  est1.setSearchSurface (points);
  est1.compute (output1);

  //// Compute with all points as "input" and the specified indices
  FeatureEstimation est2 = createSHOTDesc<FeatureEstimation, PointT, NormalT, Eigen::MatrixXf>(normals, nr_shape_bins,nr_color_bins,describe_shape,describe_color);
  est2.setSearchMethod (typename search::KdTree<PointT>::Ptr (new search::KdTree<PointT>));
  est2.setRadiusSearch (radius);
  est2.setInputCloud (points);
  est2.setIndices (indices);
  est2.compute (output2);

  // All three of the above cases should produce equivalent results
  ASSERT_EQ (output0.points.rows (), output1.points.rows ());
  ASSERT_EQ (output1.points.rows (), output2.points.rows ());
  for (int i = 0; i < output1.points.rows (); ++i)
  {
    for (int j = 0; j < output0.points.cols (); ++j)
    {
      ASSERT_EQ (output0.points (i, j), output1.points (i, j));
      ASSERT_EQ (output1.points (i, j), output2.points (i, j));
    }
  }

  //
  // Test the combination of setIndices and setSearchSurface
  //
  PointCloud<Eigen::MatrixXf> output3, output4;

  boost::shared_ptr<vector<int> > indices2 (new vector<int> (0));
  for (size_t i = 0; i < (indices->size ()/2); ++i)
    indices2->push_back (i);

  // Compute with all points as search surface + the specified sub-cloud as "input" but for only a subset of indices
  FeatureEstimation est3 = createSHOTDesc<FeatureEstimation, PointT, NormalT, Eigen::MatrixXf>(normals, nr_shape_bins,nr_color_bins,describe_shape,describe_color);
  est3.setSearchMethod (typename search::KdTree<PointT>::Ptr (new search::KdTree<PointT>));
  est3.setRadiusSearch (0.04);
  est3.setSearchSurface (points);
  est3.setInputCloud (subpoints);
  est3.setIndices (indices2);
  est3.compute (output3);

  // Start with features for each point in "subpoints" and then subsample the results
  output4 = PointCloud<Eigen::MatrixXf> (output0, *indices2);

  // The two cases above should produce equivalent results
  ASSERT_EQ (output3.points.rows (), output4.points.rows ());
  for (int i = 0; i < output3.points.rows (); ++i)
    for (int j = 0; j < output3.points.cols (); ++j)
      ASSERT_EQ (output3.points (i, j), output4.points (i, j));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, NormalEstimationEigen)
{
  Eigen::Vector4f plane_parameters;
  float curvature;

  NormalEstimation<PointXYZ, Eigen::MatrixXf> n;

  // computePointNormal (indices, Vector)
  computePointNormal (cloud, indices, plane_parameters, curvature);
  EXPECT_NEAR (fabs (plane_parameters[0]), 0.035592, 1e-4);
  EXPECT_NEAR (fabs (plane_parameters[1]), 0.369596, 1e-4);
  EXPECT_NEAR (fabs (plane_parameters[2]), 0.928511, 1e-4);
  EXPECT_NEAR (fabs (plane_parameters[3]), 0.0622552, 1e-4);
  EXPECT_NEAR (curvature, 0.0693136, 1e-4);

  float nx, ny, nz;
  // computePointNormal (indices)
  n.computePointNormal (cloud, indices, nx, ny, nz, curvature);
  EXPECT_NEAR (fabs (nx), 0.035592, 1e-4);
  EXPECT_NEAR (fabs (ny), 0.369596, 1e-4);
  EXPECT_NEAR (fabs (nz), 0.928511, 1e-4);
  EXPECT_NEAR (curvature, 0.0693136, 1e-4);

  // computePointNormal (Vector)
  computePointNormal (cloud, plane_parameters, curvature);
  EXPECT_NEAR (plane_parameters[0],  0.035592,  1e-4);
  EXPECT_NEAR (plane_parameters[1],  0.369596,  1e-4);
  EXPECT_NEAR (plane_parameters[2],  0.928511,  1e-4);
  EXPECT_NEAR (plane_parameters[3], -0.0622552, 1e-4);
  EXPECT_NEAR (curvature,            0.0693136, 1e-4);

  // flipNormalTowardsViewpoint (Vector)
  flipNormalTowardsViewpoint (cloud.points[0], 0, 0, 0, plane_parameters);
  EXPECT_NEAR (plane_parameters[0], -0.035592,  1e-4);
  EXPECT_NEAR (plane_parameters[1], -0.369596,  1e-4);
  EXPECT_NEAR (plane_parameters[2], -0.928511,  1e-4);
  EXPECT_NEAR (plane_parameters[3],  0.0799743, 1e-4);

  // flipNormalTowardsViewpoint
  flipNormalTowardsViewpoint (cloud.points[0], 0, 0, 0, nx, ny, nz);
  EXPECT_NEAR (nx, -0.035592, 1e-4);
  EXPECT_NEAR (ny, -0.369596, 1e-4);
  EXPECT_NEAR (nz, -0.928511, 1e-4);

  // Object
  PointCloud<Eigen::MatrixXf>::Ptr normals (new PointCloud<Eigen::MatrixXf> ());

  // set parameters
  PointCloud<PointXYZ>::Ptr cloudptr = cloud.makeShared ();
  n.setInputCloud (cloudptr);
  EXPECT_EQ (n.getInputCloud (), cloudptr);
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  EXPECT_EQ (n.getIndices (), indicesptr);
  n.setSearchMethod (tree);
  EXPECT_EQ (n.getSearchMethod (), tree);
  n.setKSearch (indices.size ());

  // estimate
  n.compute (*normals);
  EXPECT_EQ (normals->points.rows (), indices.size ());

  for (int i = 0; i < normals->points.rows (); ++i)
  {
    EXPECT_NEAR (normals->points.row (i)[0], -0.035592, 1e-4);
    EXPECT_NEAR (normals->points.row (i)[1], -0.369596, 1e-4);
    EXPECT_NEAR (normals->points.row (i)[2], -0.928511, 1e-4);
    EXPECT_NEAR (normals->points.row (i)[3], 0.0693136, 1e-4);
  }

  PointCloud<PointXYZ>::Ptr surfaceptr = cloudptr;
  n.setSearchSurface (surfaceptr);
  EXPECT_EQ (n.getSearchSurface (), surfaceptr);

  // Additional test for searchForNeigbhors
  surfaceptr.reset (new PointCloud<PointXYZ>);
  *surfaceptr = *cloudptr;
  surfaceptr->points.resize (640 * 480);
  surfaceptr->width = 640;
  surfaceptr->height = 480;
  EXPECT_EQ (surfaceptr->points.size (), surfaceptr->width * surfaceptr->height);
  n.setSearchSurface (surfaceptr);
  tree.reset ();
  n.setSearchMethod (tree);

  // estimate
  n.compute (*normals);
  EXPECT_EQ (normals->points.rows (), indices.size ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, NormalEstimationOpenMPEigen)
{
  NormalEstimationOMP<PointXYZ, Eigen::MatrixXf> n (4); // instantiate 4 threads

  // Object
  PointCloud<Eigen::MatrixXf>::Ptr normals (new PointCloud<Eigen::MatrixXf>);

  // set parameters
  PointCloud<PointXYZ>::Ptr cloudptr = cloud.makeShared ();
  n.setInputCloud (cloudptr);
  EXPECT_EQ (n.getInputCloud (), cloudptr);
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  EXPECT_EQ (n.getIndices (), indicesptr);
  n.setSearchMethod (tree);
  EXPECT_EQ (n.getSearchMethod (), tree);
  n.setKSearch (indices.size ());

  // estimate
  n.compute (*normals);
  EXPECT_EQ (normals->points.rows (), indices.size ());

  for (int i = 0; i < normals->points.rows (); ++i)
  {
    EXPECT_NEAR (normals->points.row (i)[0], -0.035592, 1e-4);
    EXPECT_NEAR (normals->points.row (i)[1], -0.369596, 1e-4);
    EXPECT_NEAR (normals->points.row (i)[2], -0.928511, 1e-4);
    EXPECT_NEAR (normals->points.row (i)[3], 0.0693136, 1e-4);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, MomentInvariantsEstimationEigen)
{
  float j1, j2, j3;

  MomentInvariantsEstimation<PointXYZ, Eigen::MatrixXf> mi;

  // computePointMomentInvariants (indices))
  mi.computePointMomentInvariants (cloud, indices, j1, j2, j3);
  EXPECT_NEAR (j1, 1.59244, 1e-4);
  EXPECT_NEAR (j2, 0.652063, 1e-4);
  EXPECT_NEAR (j3, 0.053917, 1e-4);

  // computePointMomentInvariants
  mi.computePointMomentInvariants (cloud, indices, j1, j2, j3);
  EXPECT_NEAR (j1, 1.59244, 1e-4);
  EXPECT_NEAR (j2, 0.652063, 1e-4);
  EXPECT_NEAR (j3, 0.053917, 1e-4);

  // Object
  PointCloud<Eigen::MatrixXf>::Ptr moments (new PointCloud<Eigen::MatrixXf>);

  // set parameters
  mi.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  mi.setIndices (indicesptr);
  mi.setSearchMethod (tree);
  mi.setKSearch (indices.size ());

  // estimate
  mi.compute (*moments);
  EXPECT_EQ (moments->points.rows (), indices.size ());

  for (int i = 0; i < moments->points.rows (); ++i)
  {
    EXPECT_NEAR (moments->points (i, 0), 1.59244, 1e-4);
    EXPECT_NEAR (moments->points (i, 1), 0.652063, 1e-4);
    EXPECT_NEAR (moments->points (i, 2), 0.053917, 1e-4);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, BoundaryEstimationEigen)
{
  Eigen::Vector4f u = Eigen::Vector4f::Zero ();
  Eigen::Vector4f v = Eigen::Vector4f::Zero ();

  // Estimate normals first
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal>);
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setKSearch (indices.size ());
  // estimate
  n.compute (*normals);

  BoundaryEstimation<PointXYZ, Normal, Eigen::MatrixXf> b;
  b.setInputNormals (normals);
  EXPECT_EQ (b.getInputNormals (), normals);

  // getCoordinateSystemOnPlane
  for (size_t i = 0; i < normals->points.size (); ++i)
  {
    b.getCoordinateSystemOnPlane (normals->points[i], u, v);
    Vector4fMap n4uv = normals->points[i].getNormalVector4fMap ();
    EXPECT_NEAR (n4uv.dot(u), 0, 1e-4);
    EXPECT_NEAR (n4uv.dot(v), 0, 1e-4);
    EXPECT_NEAR (u.dot(v), 0, 1e-4);
  }

  // isBoundaryPoint (indices)
  bool pt = false;
  pt = b.isBoundaryPoint (cloud, 0, indices, u, v, M_PI / 2.0);
  EXPECT_EQ (pt, false);
  pt = b.isBoundaryPoint (cloud, indices.size () / 3, indices, u, v, M_PI / 2.0);
  EXPECT_EQ (pt, false);
  pt = b.isBoundaryPoint (cloud, indices.size () / 2, indices, u, v, M_PI / 2.0);
  EXPECT_EQ (pt, false);
  pt = b.isBoundaryPoint (cloud, indices.size () - 1, indices, u, v, M_PI / 2.0);
  EXPECT_EQ (pt, true);

  // isBoundaryPoint (points)
  pt = false;
  pt = b.isBoundaryPoint (cloud, cloud.points[0], indices, u, v, M_PI / 2.0);
  EXPECT_EQ (pt, false);
  pt = b.isBoundaryPoint (cloud, cloud.points[indices.size () / 3], indices, u, v, M_PI / 2.0);
  EXPECT_EQ (pt, false);
  pt = b.isBoundaryPoint (cloud, cloud.points[indices.size () / 2], indices, u, v, M_PI / 2.0);
  EXPECT_EQ (pt, false);
  pt = b.isBoundaryPoint (cloud, cloud.points[indices.size () - 1], indices, u, v, M_PI / 2.0);
  EXPECT_EQ (pt, true);

  // Object
  PointCloud<Eigen::MatrixXf>::Ptr bps (new PointCloud<Eigen::MatrixXf> ());

  // set parameters
  b.setInputCloud (cloud.makeShared ());
  b.setIndices (indicesptr);
  b.setSearchMethod (tree);
  b.setKSearch (indices.size ());

  // estimate
  b.compute (*bps);
  EXPECT_EQ (bps->points.rows (), indices.size ());

  pt = bps->points (0, 0);
  EXPECT_EQ (pt, false);
  pt = bps->points (indices.size () / 3, 0);
  EXPECT_EQ (pt, false);
  pt = bps->points (indices.size () / 2, 0);
  EXPECT_EQ (pt, false);
  pt = bps->points (indices.size () - 1, 0);
  EXPECT_EQ (pt, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, PrincipalCurvaturesEstimationEigen)
{
  float pcx, pcy, pcz, pc1, pc2;

  // Estimate normals first
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setKSearch (10); // Use 10 nearest neighbors to estimate the normals
  // estimate
  n.compute (*normals);

  PrincipalCurvaturesEstimation<PointXYZ, Normal, Eigen::MatrixXf> pc;
  pc.setInputNormals (normals);
  EXPECT_EQ (pc.getInputNormals (), normals);

  // computePointPrincipalCurvatures (indices)
  pc.computePointPrincipalCurvatures (*normals, 0, indices, pcx, pcy, pcz, pc1, pc2);
  EXPECT_NEAR (fabs (pcx), 0.98509, 1e-4);
  EXPECT_NEAR (fabs (pcy), 0.10714, 1e-4);
  EXPECT_NEAR (fabs (pcz), 0.13462, 1e-4);
  EXPECT_NEAR (pc1, 0.23997423052787781, 1e-4);
  EXPECT_NEAR (pc2, 0.19400238990783691, 1e-4);

  pc.computePointPrincipalCurvatures (*normals, 2, indices, pcx, pcy, pcz, pc1, pc2);
  EXPECT_NEAR (pcx, 0.98079, 1e-4);
  EXPECT_NEAR (pcy, -0.04019, 1e-4);
  EXPECT_NEAR (pcz, 0.19086, 1e-4);
  EXPECT_NEAR (pc1, 0.27207490801811218, 1e-4);
  EXPECT_NEAR (pc2, 0.19464978575706482, 1e-4);

  pc.computePointPrincipalCurvatures (*normals, indices.size () - 3, indices, pcx, pcy, pcz, pc1, pc2);
  EXPECT_NEAR (pcx, 0.86725, 1e-4);
  EXPECT_NEAR (pcy, -0.37599, 1e-4);
  EXPECT_NEAR (pcz, 0.32635, 1e-4);
  EXPECT_NEAR (pc1, 0.25900053977966309, 1e-4);
  EXPECT_NEAR (pc2, 0.17906945943832397, 1e-4);

  pc.computePointPrincipalCurvatures (*normals, indices.size () - 1, indices, pcx, pcy, pcz, pc1, pc2);
  EXPECT_NEAR (pcx, 0.86725, 1e-4);
  EXPECT_NEAR (pcy, -0.37599, 1e-4);
  EXPECT_NEAR (pcz, 0.32636, 1e-4);
  EXPECT_NEAR (pc1, 0.2590005099773407,  1e-4);
  EXPECT_NEAR (pc2, 0.17906956374645233, 1e-4);

  // Object
  PointCloud<Eigen::MatrixXf>::Ptr pcs (new PointCloud<Eigen::MatrixXf>);

  // set parameters
  pc.setInputCloud (cloud.makeShared ());
  pc.setIndices (indicesptr);
  pc.setSearchMethod (tree);
  pc.setKSearch (indices.size ());

  // estimate
  pc.compute (*pcs);
  EXPECT_EQ (pcs->points.rows (), indices.size ());

  // Adjust for small numerical inconsitencies (due to nn_indices not being sorted)
  EXPECT_NEAR (fabs (pcs->points (0, 0)), 0.98509, 1e-4);
  EXPECT_NEAR (fabs (pcs->points (0, 1)), 0.10713, 1e-4);
  EXPECT_NEAR (fabs (pcs->points (0, 2)), 0.13462, 1e-4);
  EXPECT_NEAR (fabs (pcs->points (0, 3)), 0.23997458815574646, 1e-4);
  EXPECT_NEAR (fabs (pcs->points (0, 4)), 0.19400238990783691, 1e-4);

  EXPECT_NEAR (pcs->points (2, 0), 0.98079, 1e-4);
  EXPECT_NEAR (pcs->points (2, 1), -0.04019, 1e-4);
  EXPECT_NEAR (pcs->points (2, 2), 0.19086, 1e-4);
  EXPECT_NEAR (pcs->points (2, 3), 0.27207502722740173, 1e-4);
  EXPECT_NEAR (pcs->points (2, 4), 0.1946497857570648,  1e-4);

  EXPECT_NEAR (pcs->points (indices.size () - 3, 0), 0.86725, 1e-4);
  EXPECT_NEAR (pcs->points (indices.size () - 3, 1), -0.37599, 1e-4);
  EXPECT_NEAR (pcs->points (indices.size () - 3, 2), 0.32636, 1e-4);
  EXPECT_NEAR (pcs->points (indices.size () - 3, 3), 0.2590007483959198,  1e-4);
  EXPECT_NEAR (pcs->points (indices.size () - 3, 4), 0.17906941473484039, 1e-4);

  EXPECT_NEAR (pcs->points (indices.size () - 1, 0), 0.86725, 1e-4);
  EXPECT_NEAR (pcs->points (indices.size () - 1, 1), -0.37599, 1e-4);
  EXPECT_NEAR (pcs->points (indices.size () - 1, 2), 0.32636, 1e-4);
  EXPECT_NEAR (pcs->points (indices.size () - 1, 3), 0.25900065898895264, 1e-4);
  EXPECT_NEAR (pcs->points (indices.size () - 1, 4), 0.17906941473484039, 1e-4);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, SHOTShapeEstimationEigen)
{
  // Estimate normals first
  double mr = 0.002;
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setRadiusSearch (20 * mr);
  n.compute (*normals);

  EXPECT_NEAR (normals->points[103].normal_x, 0.36683175, 1e-4);
  EXPECT_NEAR (normals->points[103].normal_y, -0.44696972, 1e-4);
  EXPECT_NEAR (normals->points[103].normal_z, -0.81587529, 1e-4);
  EXPECT_NEAR (normals->points[200].normal_x, -0.71414840, 1e-4);
  EXPECT_NEAR (normals->points[200].normal_y, -0.06002361, 1e-4);
  EXPECT_NEAR (normals->points[200].normal_z, -0.69741613, 1e-4);

  EXPECT_NEAR (normals->points[140].normal_x, -0.45109111, 1e-4);
  EXPECT_NEAR (normals->points[140].normal_y, -0.19499126, 1e-4);
  EXPECT_NEAR (normals->points[140].normal_z, -0.87091631, 1e-4);

  SHOTEstimation<PointXYZ, Normal, Eigen::MatrixXf> shot;
  shot.setInputNormals (normals);
  EXPECT_EQ (shot.getInputNormals (), normals);
  shot.setRadiusSearch (20 * mr);

  // Object
  PointCloud<Eigen::MatrixXf>::Ptr shots (new PointCloud<Eigen::MatrixXf>);

  // set parameters
  shot.setInputCloud (cloud.makeShared ());
  shot.setIndices (indicesptr);
  shot.setSearchMethod (tree);

  // estimate
  shot.compute (*shots);
  EXPECT_EQ (shots->points.rows (), indices.size ());

  EXPECT_NEAR (shots->points (103, 9 ), 0.0072018504, 1e-4);
  EXPECT_NEAR (shots->points (103, 10), 0.0023103887, 1e-4);
  EXPECT_NEAR (shots->points (103, 11), 0.0024724449, 1e-4);
  EXPECT_NEAR (shots->points (103, 19), 0.0031367359, 1e-4);
  EXPECT_NEAR (shots->points (103, 20), 0.17439659, 1e-4);
  EXPECT_NEAR (shots->points (103, 21), 0.070665278, 1e-4);
  EXPECT_NEAR (shots->points (103, 42), 0.013304681, 1e-4);
  EXPECT_NEAR (shots->points (103, 53), 0.0073520984, 1e-4);
  EXPECT_NEAR (shots->points (103, 54), 0.013584172, 1e-4);
  EXPECT_NEAR (shots->points (103, 55), 0.0050609680, 1e-4);


  // Test results when setIndices and/or setSearchSurface are used

  boost::shared_ptr<vector<int> > test_indices (new vector<int> (0));
  for (size_t i = 0; i < cloud.points.size (); i+=3)
    test_indices->push_back (i);

  testSHOTIndicesAndSearchSurfaceEigen<SHOTEstimation<PointXYZ, Normal, Eigen::MatrixXf>, PointXYZ, Normal>
  (cloud.makeShared (), normals, test_indices);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, GenericSHOTShapeEstimation)
{
  // SHOT length
  const int shapeStep_ = 20;
  //const int dim = 32*(shapeStep_+1);

  // Estimate normals first
  double mr = 0.002;
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setRadiusSearch (20 * mr);
  n.compute (*normals);

  SHOTEstimation<PointXYZ, Normal, Eigen::MatrixXf> shot (shapeStep_);
  shot.setInputNormals (normals);
  EXPECT_EQ (shot.getInputNormals (), normals);

  shot.setRadiusSearch (20 * mr);

  PointCloud<Eigen::MatrixXf>::Ptr shots (new PointCloud<Eigen::MatrixXf>);

  // set parameters
  shot.setInputCloud (cloud.makeShared ());
  shot.setIndices (indicesptr);
  shot.setSearchMethod (tree);

  // estimate
  shot.compute (*shots);
  EXPECT_EQ (shots->points.rows (), indices.size ());

  EXPECT_NEAR (shots->points (103, 18), 0.0077019366, 1e-5);
  EXPECT_NEAR (shots->points (103, 19), 0.0024708188, 1e-5);
  EXPECT_NEAR (shots->points (103, 21), 0.0079652183, 1e-5);
  EXPECT_NEAR (shots->points (103, 38), 0.0067090928, 1e-5);
  EXPECT_NEAR (shots->points (103, 39), 0.17498907, 1e-5);
  EXPECT_NEAR (shots->points (103, 40), 0.078413926, 1e-5);
  EXPECT_NEAR (shots->points (103, 81), 0.014228539, 1e-5);
  EXPECT_NEAR (shots->points (103, 103), 0.022390056, 1e-5);
  EXPECT_NEAR (shots->points (103, 105), 0.0058866320, 1e-5);
  EXPECT_NEAR (shots->points (103, 123), 0.019105887, 1e-5);

  // Test results when setIndices and/or setSearchSurface are used
  boost::shared_ptr<vector<int> > test_indices (new vector<int> (0));
  for (size_t i = 0; i < cloud.size (); i+=3)
    test_indices->push_back (i);

  testSHOTIndicesAndSearchSurfaceEigen<SHOTEstimation<PointXYZ, Normal, Eigen::MatrixXf>, PointXYZ, Normal>
  (cloud.makeShared (), normals, test_indices, shapeStep_);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, SHOTShapeAndColorEstimation)
{
  double mr = 0.002;
  // Estimate normals first
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setRadiusSearch (20 * mr);
  n.compute (*normals);

  search::KdTree<PointXYZRGBA>::Ptr rgbaTree;
  rgbaTree.reset (new search::KdTree<PointXYZRGBA> (false));

  // Object
  SHOTEstimation<PointXYZRGBA, Normal, Eigen::MatrixXf> shot (true, true);
  shot.setInputNormals (normals);
  EXPECT_EQ (shot.getInputNormals (), normals);

  shot.setRadiusSearch (20 * mr);

  // Create fake point cloud with colors
  PointCloud<PointXYZRGBA> cloudWithColors;
  for (size_t i = 0; i < cloud.points.size (); ++i)
  {
    PointXYZRGBA p;
    p.x = cloud.points[i].x;
    p.y = cloud.points[i].y;
    p.z = cloud.points[i].z;

    p.rgba = ( (i%255) << 16 ) + ( ( (255 - i ) %255) << 8) + ( ( i*37 ) %255);
    cloudWithColors.push_back (p);
  }

  rgbaTree->setInputCloud (cloudWithColors.makeShared ());
  PointCloud<Eigen::MatrixXf>::Ptr shots (new PointCloud<Eigen::MatrixXf>);

  shot.setInputCloud (cloudWithColors.makeShared ());
  shot.setIndices (indicesptr);
  shot.setSearchMethod (rgbaTree);

  // estimate
  shot.compute (*shots);
  EXPECT_EQ (shots->points.rows (), indices.size ());

  EXPECT_NEAR (shots->points (103, 10), 0.0020453099, 1e-5);
  EXPECT_NEAR (shots->points (103, 11), 0.0021887729, 1e-5);
  EXPECT_NEAR (shots->points (103, 21), 0.062557608, 1e-5);
  EXPECT_NEAR (shots->points (103, 42), 0.011778189, 1e-5);
  EXPECT_NEAR (shots->points (103, 53), 0.0065085669, 1e-5);
  EXPECT_NEAR (shots->points (103, 54), 0.012025614, 1e-5);
  EXPECT_NEAR (shots->points (103, 55), 0.0044803056, 1e-5);
  EXPECT_NEAR (shots->points (103, 64), 0.064429596, 1e-5);
  EXPECT_NEAR (shots->points (103, 65), 0.046486385, 1e-5);
  EXPECT_NEAR (shots->points (103, 86), 0.011518310, 1e-5);

  EXPECT_NEAR (shots->points (103, 357), 0.0020453099, 1e-5);
  EXPECT_NEAR (shots->points (103, 360), 0.0027993850, 1e-5);
  EXPECT_NEAR (shots->points (103, 386), 0.045115642, 1e-5);
  EXPECT_NEAR (shots->points (103, 387), 0.059068538, 1e-5);
  EXPECT_NEAR (shots->points (103, 389), 0.0047547864, 1e-5);
  EXPECT_NEAR (shots->points (103, 453), 0.0051176427, 1e-5);
  EXPECT_NEAR (shots->points (103, 481), 0.0053625242, 1e-5);
  EXPECT_NEAR (shots->points (103, 482), 0.012025614, 1e-5);
  EXPECT_NEAR (shots->points (103, 511), 0.0057367259, 1e-5);
  EXPECT_NEAR (shots->points (103, 512), 0.048357654, 1e-5);

  // Test results when setIndices and/or setSearchSurface are used
  boost::shared_ptr<vector<int> > test_indices (new vector<int> (0));
  for (size_t i = 0; i < cloud.size (); i+=3)
    test_indices->push_back (i);

  testSHOTIndicesAndSearchSurfaceEigen<SHOTEstimation<PointXYZRGBA, Normal, Eigen::MatrixXf>, PointXYZRGBA, Normal>
  (cloudWithColors.makeShared (), normals, test_indices);
}

/*
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, SHOTShapeEstimationOpenMP)
{
  // Estimate normals first
  double mr = 0.002;
  NormalEstimationOMP<PointXYZ, Normal> n (omp_get_max_threads ());
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setRadiusSearch (20 * mr);
  n.compute (*normals);

  SHOTEstimationOMP<PointXYZ, Normal, SHOT> shot;
  shot.setInputNormals (normals);
  EXPECT_EQ (shot.getInputNormals (), normals);

  shot.setRadiusSearch ( 20 * mr);

  // Object
  PointCloud<SHOT>::Ptr shots (new PointCloud<SHOT>);

  // set parameters
  shot.setInputCloud (cloud.makeShared ());
  shot.setIndices (indicesptr);
  shot.setSearchMethod (tree);

  // estimate
  shot.compute (*shots);
  EXPECT_EQ (shots->points.size (), indices.size ());

  EXPECT_NEAR (shots->points[103].descriptor[9 ], 0.0072018504, 1e-4);
  EXPECT_NEAR (shots->points[103].descriptor[10], 0.0023103887, 1e-4);
  EXPECT_NEAR (shots->points[103].descriptor[11], 0.0024724449, 1e-4);
  EXPECT_NEAR (shots->points[103].descriptor[19], 0.0031367359, 1e-4);
  EXPECT_NEAR (shots->points[103].descriptor[20], 0.17439659, 1e-4);
  EXPECT_NEAR (shots->points[103].descriptor[21], 0.070665278, 1e-4);
  EXPECT_NEAR (shots->points[103].descriptor[42], 0.013304681, 1e-4);
  EXPECT_NEAR (shots->points[103].descriptor[53], 0.0073520984, 1e-4);
  EXPECT_NEAR (shots->points[103].descriptor[54], 0.013584172, 1e-4);
  EXPECT_NEAR (shots->points[103].descriptor[55], 0.0050609680, 1e-4);

   // Test results when setIndices and/or setSearchSurface are used
  boost::shared_ptr<vector<int> > test_indices (new vector<int> (0));
  for (size_t i = 0; i < cloud.size (); i+=3)
    test_indices->push_back (i);

  testSHOTIndicesAndSearchSurfaceEigen<SHOTEstimationOMP<PointXYZ, Normal, Eigen::MatrixXf>, PointXYZ, Normal>
    (cloud.makeShared (), normals, test_indices);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL,SHOTShapeAndColorEstimationOpenMP)
{
  double mr = 0.002;
  // Estimate normals first
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setRadiusSearch (20 * mr);
  n.compute (*normals);

  search::KdTree<PointXYZRGBA>::Ptr rgbaTree;

  rgbaTree.reset (new search::KdTree<PointXYZRGBA> (false));

  // Object
  SHOTEstimationOMP<PointXYZRGBA, Normal, SHOT> shot (true, true, -1);
  shot.setInputNormals (normals);

  EXPECT_EQ (shot.getInputNormals (), normals);

  shot.setRadiusSearch ( 20 * mr);

  // Create fake point cloud with colors
  PointCloud<PointXYZRGBA> cloudWithColors;
  for (size_t i = 0; i < cloud.points.size (); ++i)
  {
    PointXYZRGBA p;
    p.x = cloud.points[i].x;
    p.y = cloud.points[i].y;
    p.z = cloud.points[i].z;

    p.rgba = ( (i%255) << 16 ) + ( ( (255 - i ) %255) << 8) + ( ( i*37 ) %255);
    cloudWithColors.push_back(p);
  }

  rgbaTree->setInputCloud (cloudWithColors.makeShared ());

  PointCloud<SHOT>::Ptr shots (new PointCloud<SHOT> ());

  shot.setInputCloud (cloudWithColors.makeShared ());
  shot.setIndices (indicesptr);
  shot.setSearchMethod (rgbaTree);

  // estimate
  shot.compute (*shots);
  EXPECT_EQ (shots->points.size (), indices.size ());

  EXPECT_NEAR (shots->points[103].descriptor[10], 0.0020453099, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[11], 0.0021887729, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[21], 0.062557608, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[42], 0.011778189, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[53], 0.0065085669, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[54], 0.012025614, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[55], 0.0044803056, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[64], 0.064429596, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[65], 0.046486385, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[86], 0.011518310, 1e-5);

  EXPECT_NEAR (shots->points[103].descriptor[357], 0.0020453099, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[360], 0.0027993850, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[386], 0.045115642, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[387], 0.059068538, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[389], 0.0047547864, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[453], 0.0051176427, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[481], 0.0053625242, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[482], 0.012025614, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[511], 0.0057367259, 1e-5);
  EXPECT_NEAR (shots->points[103].descriptor[512], 0.048357654, 1e-5);

  // Test results when setIndices and/or setSearchSurface are used
  boost::shared_ptr<vector<int> > test_indices (new vector<int> (0));
  for (size_t i = 0; i < cloud.size (); i+=3)
    test_indices->push_back (i);

  testSHOTIndicesAndSearchSurfaceEigen<SHOTEstimationOMP<PointXYZRGBA, Normal, Eigen::MatrixXf>, PointXYZRGBA, Normal>
    (cloudWithColors.makeShared (), normals, test_indices);
}
 */

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, 3DSCEstimationEigen)
{
  float meshRes = 0.002;
  size_t nBinsL = 4;
  size_t nBinsK = 4;
  size_t nBinsJ = 4;
  float radius = 20.0 * meshRes;
  float rmin = radius / 10.0;
  float ptDensityRad = radius / 5.0;

  PointCloud<PointXYZ>::Ptr cloudptr = cloud.makeShared ();

  // Estimate normals first
  NormalEstimation<PointXYZ, Normal> ne;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  ne.setInputCloud (cloudptr);
  ne.setSearchMethod (tree);
  ne.setRadiusSearch (radius);
  // estimate
  ne.compute (*normals);
  ShapeContext3DEstimation<PointXYZ, Normal, Eigen::MatrixXf> sc3d;
  sc3d.setInputCloud (cloudptr);
  sc3d.setInputNormals (normals);
  sc3d.setSearchMethod (tree);
  sc3d.setRadiusSearch (radius);
  sc3d.setAzimuthBins (nBinsL);
  sc3d.setElevationBins (nBinsK);
  sc3d.setRadiusBins (nBinsJ);
  sc3d.setMinimalRadius (rmin);
  sc3d.setPointDensityRadius (ptDensityRad);
  // Compute the features
  PointCloud<Eigen::MatrixXf>::Ptr sc3ds (new PointCloud<Eigen::MatrixXf>);
  sc3d.compute (*sc3ds);
  EXPECT_EQ (sc3ds->points.rows (), cloud.size ());

  // 3DSC does not define a repeatable local RF, we set it to zero to signal it to the user
  //EXPECT_NEAR ((*sc3ds)[0].rf[0], 0.2902f, 1e-4f);
  //EXPECT_NEAR ((*sc3ds)[0].rf[1], 0.7334f, 1e-4f);
  //EXPECT_NEAR ((*sc3ds)[0].rf[2], -0.6146f, 1e-4f);
  //EXPECT_NEAR ((*sc3ds)[0].rf[3], 0.9486f, 1e-4f);
  //EXPECT_NEAR ((*sc3ds)[0].rf[4], -0.3051f, 1e-4f);
  //EXPECT_NEAR ((*sc3ds)[0].rf[5], 0.0838f, 1e-4f);
  //EXPECT_NEAR ((*sc3ds)[0].rf[6], -0.1261f, 1e-4f);
  //EXPECT_NEAR ((*sc3ds)[0].rf[7], -0.6074f, 1e-4f);
  //EXPECT_NEAR ((*sc3ds)[0].rf[8], -0.7843f, 1e-4f);

  EXPECT_NEAR (sc3ds->points (0, 0), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 1), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 2), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 3), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 4), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 5), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 6), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 7), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 8), 0.0f, 1e-4f);

  EXPECT_EQ   (sc3ds->points.row (0).size (), 64 + 9);
  EXPECT_NEAR (sc3ds->points (0, 9 + 4), 52.2474f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 9 + 6), 150.901611328125, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 9 + 7), 169.09703063964844, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 9 + 8), 0, 1e-4f);
  EXPECT_NEAR (sc3ds->points (0, 9 + 21), 39.1745f, 1e-4f);

  EXPECT_NEAR (sc3ds->points (2, 9 + 4), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (2, 9 + 6), 73.7986f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (2, 9 + 7), 209.97763061523438, 1e-4f);

  EXPECT_NEAR (sc3ds->points (2, 9 + 9), 68.5553f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (2, 9 + 16), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (2, 9 + 17), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (2, 9 + 18), 0.0f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (2, 9 + 20), 0.0f, 1e-4f);

  EXPECT_NEAR (sc3ds->points (2, 9 + 21), 39.1745f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (2, 9 + 22), 154.2060f, 1e-4f);
  EXPECT_NEAR (sc3ds->points (2, 9 + 23), 275.63433837890625, 1e-4f);

  // Test results when setIndices and/or setSearchSurface are used
  boost::shared_ptr<vector<int> > test_indices (new vector<int> (0));
  for (size_t i = 0; i < cloud.size (); i++)
    test_indices->push_back (i);

  testSHOTIndicesAndSearchSurfaceEigen<ShapeContext3DEstimation<PointXYZ, Normal, Eigen::MatrixXf>, PointXYZ, Normal>
  (cloudptr, normals, test_indices);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, USCEstimation)
{
  float meshRes = 0.002;
  size_t nBinsL = 4;
  size_t nBinsK = 4;
  size_t nBinsJ = 4;
  float radius = 20.0 * meshRes;
  float rmin = radius / 10.0;
  float ptDensityRad = radius / 5.0;

  // estimate
  UniqueShapeContext<PointXYZ, Eigen::MatrixXf> uscd;
  uscd.setInputCloud (cloud.makeShared ());
  uscd.setSearchMethod (tree);
  uscd.setRadiusSearch (radius);
  uscd.setAzimuthBins (nBinsL);
  uscd.setElevationBins (nBinsK);
  uscd.setRadiusBins (nBinsJ);
  uscd.setMinimalRadius (rmin);
  uscd.setPointDensityRadius (ptDensityRad);
  uscd.setLocalRadius (radius);
  // Compute the features
  PointCloud<Eigen::MatrixXf>::Ptr uscds (new PointCloud<Eigen::MatrixXf>);
  uscd.compute (*uscds);
  EXPECT_EQ (uscds->points.rows (), cloud.size ());

  EXPECT_NEAR (uscds->points (0, 0), 0.9876f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 1), -0.1408f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 2), -0.06949f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 3), -0.06984f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 4), -0.7904f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 5), 0.6086f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 6), -0.1406f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 7), -0.5962f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 8), -0.7904f, 1e-4f);

  EXPECT_EQ   (uscds->points.row (0).size (), 9+64);
  EXPECT_NEAR (uscds->points (0, 9 + 4), 52.2474f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 9 + 5), 39.1745f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 9 + 6), 176.2354f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 9 + 7), 199.4478f, 1e-4f);
  EXPECT_NEAR (uscds->points (0, 9 + 8), 0.0f, 1e-4f);

  EXPECT_NEAR (uscds->points (2, 9 + 6), 110.1472f, 1e-4f);
  EXPECT_NEAR (uscds->points (2, 9 + 7), 145.5597f, 1e-4f);
  EXPECT_NEAR (uscds->points (2, 9 + 8), 69.6632f, 1e-4f);
  EXPECT_NEAR (uscds->points (2, 9 + 22), 57.2765f, 1e-4f);
  EXPECT_NEAR (uscds->points (2, 9 + 23), 172.8134f, 1e-4f);
  EXPECT_NEAR (uscds->points (2, 9 + 25), 68.5554f, 1e-4f);
  EXPECT_NEAR (uscds->points (2, 9 + 26), 0.0f, 1e-4f);
  EXPECT_NEAR (uscds->points (2, 9 + 27), 0.0f, 1e-4f);
  EXPECT_NEAR (uscds->points (2, 9 + 37), 39.1745f, 1e-4f);
  EXPECT_NEAR (uscds->points (2, 9 + 38), 71.5957f, 1e-4f);

  // Test results when setIndices and/or setSearchSurface are used
  boost::shared_ptr<vector<int> > test_indices (new vector<int> (0));
  for (size_t i = 0; i < cloud.size (); i+=3)
    test_indices->push_back (i);

  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  testSHOTIndicesAndSearchSurfaceEigen<UniqueShapeContext<PointXYZ, Eigen::MatrixXf>, PointXYZ, Normal>
  (cloud.makeShared (), normals, test_indices);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, PFHEstimationEigen)
{
  float f1, f2, f3, f4;

  // Estimate normals first
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setKSearch (10); // Use 10 nearest neighbors to estimate the normals
  // estimate
  n.compute (*normals);

  PFHEstimation<PointXYZ, Normal, Eigen::MatrixXf> pfh;
  pfh.setInputNormals (normals);
  EXPECT_EQ (pfh.getInputNormals (), normals);

  // computePairFeatures
  pfh.computePairFeatures (cloud, *normals, 0, 12, f1, f2, f3, f4);
  EXPECT_NEAR (f1, -0.072575, 1e-4);
  EXPECT_NEAR (f2, -0.040221, 1e-4);
  EXPECT_NEAR (f3, 0.068133, 1e-4);
  EXPECT_NEAR (f4, 0.006130, 1e-4);

  // computePointPFHSignature
  int nr_subdiv = 3;
  Eigen::VectorXf pfh_histogram (nr_subdiv * nr_subdiv * nr_subdiv);
  pfh.computePointPFHSignature (cloud, *normals, indices, nr_subdiv, pfh_histogram);
  EXPECT_NEAR (pfh_histogram[0], 0.534875, 1e-4);
  EXPECT_NEAR (pfh_histogram[1], 1.481491, 1e-4);
  EXPECT_NEAR (pfh_histogram[2], 0.211284, 1e-4);
  EXPECT_NEAR (pfh_histogram[3], 0.751871, 1e-4);
  EXPECT_NEAR (pfh_histogram[4], 3.254728, 1e-4);
  EXPECT_NEAR (pfh_histogram[5], 0.190981, 1e-4);
  EXPECT_NEAR (pfh_histogram[6], 1.070379, 1e-4);
  EXPECT_NEAR (pfh_histogram[7], 2.534648, 1e-4);
  EXPECT_NEAR (pfh_histogram[8], 0.190346, 1e-4);
  EXPECT_NEAR (pfh_histogram[9], 1.780308, 1e-4);
  EXPECT_NEAR (pfh_histogram[10], 4.376545, 1e-4);
  EXPECT_NEAR (pfh_histogram[11], 0.707457, 1e-4);
  EXPECT_NEAR (pfh_histogram[12], 2.162236, 1e-4);
  EXPECT_NEAR (pfh_histogram[13], 18.234676, 1e-4);
  EXPECT_NEAR (pfh_histogram[14], 0.737912, 1e-4);
  EXPECT_NEAR (pfh_histogram[15], 3.294063, 1e-4);
  EXPECT_NEAR (pfh_histogram[16], 8.590946, 1e-4);
  EXPECT_NEAR (pfh_histogram[17], 0.831817, 1e-4);
  EXPECT_NEAR (pfh_histogram[18], 5.927044, 1e-4);
  EXPECT_NEAR (pfh_histogram[19], 3.893601, 1e-4);
  EXPECT_NEAR (pfh_histogram[20], 1.003129, 1e-4);
  EXPECT_NEAR (pfh_histogram[21], 11.114402, 1e-4);
  EXPECT_NEAR (pfh_histogram[22], 11.265974, 1e-4);
  EXPECT_NEAR (pfh_histogram[23], 1.323517, 1e-4);
  EXPECT_NEAR (pfh_histogram[24], 6.231051, 1e-4);
  EXPECT_NEAR (pfh_histogram[25], 6.215185, 1e-4);
  EXPECT_NEAR (pfh_histogram[26], 1.837407, 1e-4);


  // Object
  PointCloud<Eigen::MatrixXf>::Ptr pfhs (new PointCloud<Eigen::MatrixXf>);

  // set parameters
  pfh.setInputCloud (cloud.makeShared ());
  pfh.setIndices (indicesptr);
  pfh.setSearchMethod (tree);
  pfh.setKSearch (indices.size ());

  // estimate
  pfh.compute (*pfhs);
  EXPECT_EQ (pfhs->points.rows (), indices.size ());

  for (int i = 0; i < pfhs->points.rows (); ++i)
  {
    EXPECT_NEAR (pfhs->points (i, 0), 0.078042, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 1), 0.273464, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 2), 0.206843, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 3), 0.092000, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 4), 0.057738, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 5), 0.028552, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 6), 0.135780, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 7), 0.184636, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 8), 0.133242, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 9), 0.001903, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 10), 0.051393, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 11), 0.165601, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 12), 0.255064, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 13), 0.150373, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 14), 0.000634, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 15), 0.058373, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 16), 0.258237, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 17), 0.237298, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 18), 0.132607, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 19), 0.000634, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 20), 0.074235, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 21), 0.389576, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 22), 0.232222, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 23), 0.111035, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 24), 0.010786, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 25), 0.142759, 1e-4);
    EXPECT_NEAR (pfhs->points (i, 26), 0.182098, 1e-4);
  }


  // Test results when setIndices and/or setSearchSurface are used

  boost::shared_ptr<vector<int> > test_indices (new vector<int> (0));
  for (size_t i = 0; i < cloud.size (); i+=3)
    test_indices->push_back (i);

  testIndicesAndSearchSurfaceEigen<PFHEstimation<PointXYZ, Normal, Eigen::MatrixXf>, PointXYZ, Normal>
  (cloud.makeShared (), normals, test_indices, 125);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, FPFHEstimationEigen)
{
  // Estimate normals first
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setKSearch (10); // Use 10 nearest neighbors to estimate the normals
  // estimate
  n.compute (*normals);

  FPFHEstimation<PointXYZ, Normal, Eigen::MatrixXf> fpfh;
  fpfh.setInputNormals (normals);
  EXPECT_EQ (fpfh.getInputNormals (), normals);

  // computePointSPFHSignature
  int nr_subdiv = 11; // use the same number of bins for all three angular features
  Eigen::MatrixXf hist_f1 (indices.size (), nr_subdiv), hist_f2 (indices.size (), nr_subdiv), hist_f3 (indices.size (), nr_subdiv);
  hist_f1.setZero (); hist_f2.setZero (); hist_f3.setZero ();
  for (size_t i = 0; i < indices.size (); ++i)
    fpfh.computePointSPFHSignature (cloud, *normals, i, i, indices, hist_f1, hist_f2, hist_f3);

  EXPECT_NEAR (hist_f1 (0, 0), 3.030303, 1e-4);
  EXPECT_NEAR (hist_f1 (0, 1), 1.262626, 1e-4);
  EXPECT_NEAR (hist_f1 (0, 2), 5.050506, 1e-4);
  EXPECT_NEAR (hist_f1 (0, 3), 20.202024, 1e-4);
  EXPECT_NEAR (hist_f1 (0, 4), 40.151524, 1e-4);
  EXPECT_NEAR (hist_f1 (0, 5), 21.464651, 1e-4);
  EXPECT_NEAR (hist_f1 (0, 6), 7.575759, 1e-4);
  EXPECT_NEAR (hist_f1 (0, 7), 0.000000, 1e-4);
  EXPECT_NEAR (hist_f1 (0, 8), 0.000000, 1e-4);
  EXPECT_NEAR (hist_f1 (0, 9), 0.000000, 1e-4);
  EXPECT_NEAR (hist_f1 (0, 10), 1.262626, 1e-4);

  EXPECT_NEAR (hist_f2 (0, 0), 0.000000, 1e-4);
  EXPECT_NEAR (hist_f2 (0, 1), 0.505050, 1e-4);
  EXPECT_NEAR (hist_f2 (0, 2), 2.272727, 1e-4);
  EXPECT_NEAR (hist_f2 (0, 3), 10.606063, 1e-4);
  EXPECT_NEAR (hist_f2 (0, 4), 24.747480, 1e-4);
  EXPECT_NEAR (hist_f2 (0, 5), 19.949499, 1e-4);
  EXPECT_NEAR (hist_f2 (0, 6), 17.424246, 1e-4);
  EXPECT_NEAR (hist_f2 (0, 7), 11.868690, 1e-4);
  EXPECT_NEAR (hist_f2 (0, 8), 6.313132, 1e-4);
  EXPECT_NEAR (hist_f2 (0, 9), 3.787879, 1e-4);
  EXPECT_NEAR (hist_f2 (0, 10), 2.525253, 1e-4);

  EXPECT_NEAR (hist_f3 (0, 0), 0.000000, 1e-4);
  EXPECT_NEAR (hist_f3 (0, 1), 5.050506, 1e-4);
  EXPECT_NEAR (hist_f3 (0, 2), 4.545455, 1e-4);
  EXPECT_NEAR (hist_f3 (0, 3), 5.050506, 1e-4);
  EXPECT_NEAR (hist_f3 (0, 4), 1.515151, 1e-4);
  EXPECT_NEAR (hist_f3 (0, 5), 11.868690, 1e-4);
  EXPECT_NEAR (hist_f3 (0, 6), 19.444448, 1e-4);
  EXPECT_NEAR (hist_f3 (0, 7), 37.373745, 1e-4);
  EXPECT_NEAR (hist_f3 (0, 8), 12.121215, 1e-4);
  EXPECT_NEAR (hist_f3 (0, 9), 3.030303, 1e-4);
  EXPECT_NEAR (hist_f3 (0, 10), 0.000000, 1e-4);

  // weightPointSPFHSignature
  Eigen::VectorXf fpfh_histogram (nr_subdiv + nr_subdiv + nr_subdiv);
  fpfh_histogram.setZero ();
  vector<float> dists (indices.size ());
  for (size_t i = 0; i < dists.size (); ++i) dists[i] = i;
  fpfh.weightPointSPFHSignature (hist_f1, hist_f2, hist_f3, indices, dists, fpfh_histogram);

  EXPECT_NEAR (fpfh_histogram[0], 2.235183, 1e-4);
  EXPECT_NEAR (fpfh_histogram[1], 4.277350, 1e-4);
  EXPECT_NEAR (fpfh_histogram[2], 9.270952, 1e-4);
  EXPECT_NEAR (fpfh_histogram[3], 23.578320, 1e-4);
  EXPECT_NEAR (fpfh_histogram[4], 29.768152, 1e-4);
  EXPECT_NEAR (fpfh_histogram[5], 18.492353, 1e-4);
  EXPECT_NEAR (fpfh_histogram[6], 7.528943, 1e-4);
  EXPECT_NEAR (fpfh_histogram[7], 1.635015, 1e-4);
  EXPECT_NEAR (fpfh_histogram[8], 0.809479, 1e-4);
  EXPECT_NEAR (fpfh_histogram[9], 0.955730, 1e-4);
  EXPECT_NEAR (fpfh_histogram[10], 1.448486, 1e-4);
  EXPECT_NEAR (fpfh_histogram[11], 1.324533, 1e-4);
  EXPECT_NEAR (fpfh_histogram[12], 3.484986, 1e-4);
  EXPECT_NEAR (fpfh_histogram[13], 7.022393, 1e-4);
  EXPECT_NEAR (fpfh_histogram[14], 10.956238, 1e-4);
  EXPECT_NEAR (fpfh_histogram[15], 18.838923, 1e-4);
  EXPECT_NEAR (fpfh_histogram[16], 17.499048, 1e-4);
  EXPECT_NEAR (fpfh_histogram[17], 12.990987, 1e-4);
  EXPECT_NEAR (fpfh_histogram[18], 10.547534, 1e-4);
  EXPECT_NEAR (fpfh_histogram[19], 6.818617, 1e-4);
  EXPECT_NEAR (fpfh_histogram[20], 5.871921, 1e-4);
  EXPECT_NEAR (fpfh_histogram[21], 4.644787, 1e-4);
  EXPECT_NEAR (fpfh_histogram[22], 0.732736, 1e-4);
  EXPECT_NEAR (fpfh_histogram[23], 2.810466, 1e-4);
  EXPECT_NEAR (fpfh_histogram[24], 4.833643, 1e-4);
  EXPECT_NEAR (fpfh_histogram[25], 5.986883, 1e-4);
  EXPECT_NEAR (fpfh_histogram[26], 5.575591, 1e-4);
  EXPECT_NEAR (fpfh_histogram[27], 8.190613, 1e-4);
  EXPECT_NEAR (fpfh_histogram[28], 14.659019, 1e-4);
  EXPECT_NEAR (fpfh_histogram[29], 20.581768, 1e-4);
  EXPECT_NEAR (fpfh_histogram[30], 18.648823, 1e-4);
  EXPECT_NEAR (fpfh_histogram[31], 12.813086, 1e-4);
  EXPECT_NEAR (fpfh_histogram[32], 5.167324, 1e-4);

  // Object
  PointCloud<Eigen::MatrixXf>::Ptr fpfhs (new PointCloud<Eigen::MatrixXf>);

  // set parameters
  fpfh.setInputCloud (cloud.makeShared ());
  fpfh.setNrSubdivisions (11, 11, 11);
  fpfh.setIndices (indicesptr);
  fpfh.setSearchMethod (tree);
  fpfh.setKSearch (indices.size ());

  // estimate
  fpfh.compute (*fpfhs);
  EXPECT_EQ (fpfhs->points.rows (), indices.size ());

  EXPECT_NEAR (fpfhs->points (0, 0), 2.121176, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 1), 3.166825, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 2), 7.646459, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 3), 23.723145, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 4), 33.103897, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 5), 19.550802, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 6), 7.403052, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 7), 1.148289, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 8), 0.484390, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 9), 0.587204, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 10), 1.064761, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 11), 0.628057, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 12), 2.187382, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 13), 5.761130, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 14), 10.818363, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 15), 20.232224, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 16), 19.494528, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 17), 15.078060, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 18), 10.543551, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 19), 6.200090, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 20), 5.051622, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 21), 4.004990, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 22), 0.366050, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 23), 3.003855, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 24), 4.798720, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 25), 5.552973, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 26), 5.131496, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 27), 10.366766, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 28), 17.552223, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 29), 23.837053, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 30), 18.135691, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 31), 9.081526, 1e-4);
  EXPECT_NEAR (fpfhs->points (0, 32), 2.173666, 1e-4);


  // Test results when setIndices and/or setSearchSurface are used
  boost::shared_ptr<vector<int> > test_indices (new vector<int> (0));
  for (size_t i = 0; i < cloud.size (); i+=3)
    test_indices->push_back (i);

  testIndicesAndSearchSurfaceEigen <FPFHEstimation<PointXYZ, Normal, Eigen::MatrixXf>, PointXYZ, Normal>
  (cloud.makeShared (), normals, test_indices, 33);
}
/*
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, FPFHEstimationOpenMP)
{
  // Estimate normals first
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setKSearch (10); // Use 10 nearest neighbors to estimate the normals
  // estimate
  n.compute (*normals);
  FPFHEstimationOMP<PointXYZ, Normal, FPFHSignature33> fpfh (4); // instantiate 4 threads
  fpfh.setInputNormals (normals);

  // Object
  PointCloud<FPFHSignature33>::Ptr fpfhs (new PointCloud<FPFHSignature33> ());

  // set parameters
  fpfh.setInputCloud (cloud.makeShared ());
  fpfh.setNrSubdivisions (11, 11, 11);
  fpfh.setIndices (indicesptr);
  fpfh.setSearchMethod (tree);
  fpfh.setKSearch (indices.size ());

  // estimate
  fpfh.compute (*fpfhs);
  EXPECT_EQ (fpfhs->points.size (), indices.size ());

  EXPECT_NEAR (fpfhs->points[0].histogram[0], 2.11328, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[1], 3.13866, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[2], 7.07176, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[3], 23.0986, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[4], 32.988, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[5], 18.74372, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[6], 8.118416, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[7], 1.9162, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[8], 1.19554, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[9], 0.577558, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[10], 1.03827, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[11], 0.631236, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[12], 2.13356, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[13], 5.67842, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[14], 10.8759, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[15], 20.2439, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[16], 19.674, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[17], 15.3302, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[18], 10.773, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[19], 6.80136, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[20], 4.03065, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[21], 3.82776, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[22], 0.208905, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[23], 0.392544, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[24], 1.27637, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[25], 2.61976, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[26], 5.12960, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[27], 12.35568, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[28], 21.89877, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[29], 25.55738, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[30], 19.1552, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[31], 9.22763, 1e-4);
  EXPECT_NEAR (fpfhs->points[0].histogram[32], 2.17815, 1e-4);



  // Test results when setIndices and/or setSearchSurface are used

  boost::shared_ptr<vector<int> > test_indices (new vector<int> (0));
  for (size_t i = 0; i < cloud.size (); i+=3)
    test_indices->push_back (i);

  testIndicesAndSearchSurface<FPFHEstimationOMP<PointXYZ, Normal, FPFHSignature33>, PointXYZ, Normal, FPFHSignature33>
    (cloud.makeShared (), normals, test_indices, 33);
}
 */
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, PPFEstimation)
{
  // Estimate normals
  NormalEstimation<PointXYZ, Normal> normal_estimation;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  normal_estimation.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  normal_estimation.setIndices (indicesptr);
  normal_estimation.setSearchMethod (tree);
  normal_estimation.setKSearch (10); // Use 10 nearest neighbors to estimate the normals
  normal_estimation.compute (*normals);

  PPFEstimation <PointXYZ, Normal, PPFSignature> ppf_estimation;
  ppf_estimation.setInputCloud (cloud.makeShared ());
  ppf_estimation.setInputNormals (normals);
  PointCloud<PPFSignature>::Ptr feature_cloud (new PointCloud<PPFSignature> ());
  ppf_estimation.compute (*feature_cloud);

  // Check for size of output
  EXPECT_EQ (feature_cloud->points.size (), indices.size () * cloud.points.size ());

  // Now check for a few values in the feature cloud
  EXPECT_EQ (pcl_isnan (feature_cloud->points[0].f1), true);
  EXPECT_EQ (pcl_isnan (feature_cloud->points[0].f2), true);
  EXPECT_EQ (pcl_isnan (feature_cloud->points[0].f3), true);
  EXPECT_EQ (pcl_isnan (feature_cloud->points[0].f4), true);
  EXPECT_TRUE (pcl_isnan (feature_cloud->points[0].alpha_m));
  EXPECT_NEAR (feature_cloud->points[15127].f1, 2.516365, 1e-4);
  EXPECT_NEAR (feature_cloud->points[15127].f2, -0.003132, 1e-4);
  EXPECT_NEAR (feature_cloud->points[15127].f3, 0.076962, 1e-4);
  EXPECT_NEAR (feature_cloud->points[15127].f4, 0.010681, 1e-4);
  EXPECT_NEAR (feature_cloud->points[15127].alpha_m, -0.255664, 1e-4);
  EXPECT_NEAR (feature_cloud->points[30254].f1, 0.185142, 1e-4);
  EXPECT_NEAR (feature_cloud->points[30254].f2, 0.042500, 1e-4);
  EXPECT_NEAR (feature_cloud->points[30254].f3, -0.191276, 1e-4);
  EXPECT_NEAR (feature_cloud->points[30254].f4, 0.013851, 1e-4);
  EXPECT_NEAR (feature_cloud->points[30254].alpha_m, 2.429551, 1e-4);
  EXPECT_NEAR (feature_cloud->points[45381].f1, -1.962630, 1e-4);
  EXPECT_NEAR (feature_cloud->points[45381].f2, -0.431919, 1e-4);
  EXPECT_NEAR (feature_cloud->points[45381].f3, 0.868716, 1e-4);
  EXPECT_NEAR (feature_cloud->points[45381].f4, 0.140129, 1e-4);
  EXPECT_NEAR (feature_cloud->points[45381].alpha_m, -1.972757, 1e-4);
}
/*
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, VFHEstimation)
{
  // Estimate normals first
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setKSearch (10); // Use 10 nearest neighbors to estimate the normals
  // estimate
  n.compute (*normals);
  VFHEstimation<PointXYZ, Normal, VFHSignature308> vfh;
  vfh.setInputNormals (normals);

  //  PointCloud<PointNormal> cloud_normals;
  //  concatenateFields (cloud, normals, cloud_normals);
  //  savePCDFile ("bun0_n.pcd", cloud_normals);

  // Object
  PointCloud<VFHSignature308>::Ptr vfhs (new PointCloud<VFHSignature308> ());

  // set parameters
  vfh.setInputCloud (cloud.makeShared ());
  vfh.setIndices (indicesptr);
  vfh.setSearchMethod (tree);

  // estimate
  vfh.compute (*vfhs);
  EXPECT_EQ ((int)vfhs->points.size (), 1);

  //for (size_t d = 0; d < 308; ++d)
  //  std::cerr << vfhs.points[0].histogram[d] << std::endl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, GFPFH)
{
  PointCloud<PointXYZL>::Ptr cloud (new PointCloud<PointXYZL>());

  const unsigned num_classes = 3;

  // Build a cubic shape with a hole and changing labels.
  for (int z = -10; z < 10; ++z)
  for (int y = -10; y < 10; ++y)
  for (int x = -10; x < 10; ++x)
  {
    if (x >= -9 && x < 9 && y >= -9 && y < 9 && z >= -9 && z < 9)
      continue;
    unsigned label = 1 + (std::abs (x+y+z) % num_classes);
    PointXYZL p;
    p.label = label;
    p.x = x;
    p.y = y;
    p.z = z;
    cloud->points.push_back (p);
  }
  cloud->width = cloud->points.size ();
  cloud->height = 1;

  GFPFHEstimation<PointXYZL, PointXYZL, GFPFHSignature16> gfpfh;
  gfpfh.setNumberOfClasses (num_classes);
  gfpfh.setOctreeLeafSize (2);
  gfpfh.setInputCloud (cloud);
  gfpfh.setInputLabels (cloud);
  PointCloud<GFPFHSignature16> descriptor;
  gfpfh.compute (descriptor);

  const float ref_values[] = { 3216, 7760, 8740, 26584, 4645, 2995, 3029, 4349, 6192, 5440, 9514, 47563, 21814, 22073, 5734, 1253 };

  EXPECT_EQ (descriptor.points.size (), 1);
  for (size_t i = 0; i < (size_t) descriptor.points[0].descriptorSize (); ++i)
  {
    EXPECT_EQ (descriptor.points[0].histogram[i], ref_values[i]);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, RSDEstimation)
{
  // Estimate normals first
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setKSearch (10); // Use 10 nearest neighbors to estimate the normals
  // estimate
  n.compute (*normals);
  RSDEstimation<PointXYZ, Normal, PrincipalRadiiRSD> rsd;
  rsd.setInputNormals (normals);

  // Object
  PointCloud<PrincipalRadiiRSD>::Ptr rsds (new PointCloud<PrincipalRadiiRSD> ());

  // set parameters
  rsd.setInputCloud (cloud.makeShared ());
  rsd.setIndices (indicesptr);
  rsd.setSearchMethod (tree);
  rsd.setRadiusSearch (0.015);

  // estimate
  rsd.compute (*rsds);
  //  EXPECT_NEAR (rsds->points[0].r_min, 0.04599, 0.005);
  //  EXPECT_NEAR (rsds->points[0].r_max, 0.07053, 0.005);

  // Save output
  //PointCloud<PointNormal> normal_cloud;
  //concatenateFields (cloud, *normals, normal_cloud);
  //savePCDFile ("./test/bun0-normal.pcd", normal_cloud);
  //savePCDFile ("./test/bun0-rsd.pcd", *rsds);
}
 */
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, IntensityGradientEstimation)
{
  // Create a test cloud
  PointCloud<PointXYZI> cloud_xyzi;
  cloud_xyzi.height = 1;
  cloud_xyzi.is_dense = true;
  for (float x = -5.0; x <= 5.0; x += 0.1)
  {
    for (float y = -5.0; y <= 5.0; y += 0.1)
    {
      PointXYZI p;
      p.x = x;
      p.y = y;
      p.z = 0.1 * pow (x, 2) + 0.5 * y + 1.0;
      p.intensity = 0.1 * pow (x, 3) + 0.2 * pow (y, 2) + 1.0 * p.z + 20000.0;

      cloud_xyzi.points.push_back (p);
    }
  }
  cloud_xyzi.width = cloud_xyzi.points.size ();
  PointCloud<PointXYZI>::ConstPtr cloud_ptr = cloud_xyzi.makeShared ();

  // Estimate surface normals
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  NormalEstimation<PointXYZI, Normal> norm_est;
  norm_est.setInputCloud (cloud_ptr);
  search::KdTree<PointXYZI>::Ptr treept1 (new search::KdTree<PointXYZI> (false));
  norm_est.setSearchMethod (treept1);
  norm_est.setRadiusSearch (0.25);
  norm_est.compute (*normals);

  // Estimate intensity gradient
  PointCloud<Eigen::MatrixXf> gradient;
  IntensityGradientEstimation<PointXYZI, Normal, Eigen::MatrixXf> grad_est;
  grad_est.setInputCloud (cloud_ptr);
  grad_est.setInputNormals (normals);
  search::KdTree<PointXYZI>::Ptr treept2 (new search::KdTree<PointXYZI> (false));
  grad_est.setSearchMethod (treept2);
  grad_est.setRadiusSearch (0.25);
  grad_est.compute (gradient);

  // Compare to gradient estimates to actual values
  for (size_t i = 0; i < cloud_ptr->points.size (); ++i)
  {
    const PointXYZI &p = cloud_ptr->points[i];

    // A pointer to the estimated gradient values
    const float * g_est = gradient.points.row (i).data ();

    // Compute the surface normal analytically.
    float nx = -0.2 * p.x;
    float ny = -0.5;
    float nz = 1.0;
    float magnitude = sqrt (nx * nx + ny * ny + nz * nz);
    nx /= magnitude;
    ny /= magnitude;
    nz /= magnitude;

    // Compute the intensity gradient analytically...
    float tmpx = (0.3 * pow (p.x, 2));
    float tmpy = (0.4 * p.y);
    float tmpz = (1.0);
    // ...and project the 3-D gradient vector onto the surface's tangent plane.
    float gx = (1 - nx * nx) * tmpx + (-nx * ny) * tmpy + (-nx * nz) * tmpz;
    float gy = (-ny * nx) * tmpx + (1 - ny * ny) * tmpy + (-ny * nz) * tmpz;
    float gz = (-nz * nx) * tmpx + (-nz * ny) * tmpy + (1 - nz * nz) * tmpz;

    // Compare the estimates to the derived values.
    const float tolerance = 0.1;
    EXPECT_NEAR (g_est[0], gx, tolerance);
    EXPECT_NEAR (g_est[1], gy, tolerance);
    EXPECT_NEAR (g_est[2], gz, tolerance);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, SpinImageEstimationEigen)
{
  // Estimate normals first
  double mr = 0.002;
  NormalEstimation<PointXYZ, Normal> n;
  PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
  // set parameters
  n.setInputCloud (cloud.makeShared ());
  boost::shared_ptr<vector<int> > indicesptr (new vector<int> (indices));
  n.setIndices (indicesptr);
  n.setSearchMethod (tree);
  n.setRadiusSearch (20 * mr);
  n.compute (*normals);

  EXPECT_NEAR (normals->points[103].normal_x, 0.36683175, 1e-4);
  EXPECT_NEAR (normals->points[103].normal_y, -0.44696972, 1e-4);
  EXPECT_NEAR (normals->points[103].normal_z, -0.81587529, 1e-4);
  EXPECT_NEAR (normals->points[200].normal_x, -0.71414840, 1e-4);
  EXPECT_NEAR (normals->points[200].normal_y, -0.06002361, 1e-4);
  EXPECT_NEAR (normals->points[200].normal_z, -0.69741613, 1e-4);

  EXPECT_NEAR (normals->points[140].normal_x, -0.45109111, 1e-4);
  EXPECT_NEAR (normals->points[140].normal_y, -0.19499126, 1e-4);
  EXPECT_NEAR (normals->points[140].normal_z, -0.87091631, 1e-4);

  SpinImageEstimation<PointXYZ, Normal, Eigen::MatrixXf> spin_est (8, 0.5, 16);
  // set parameters
  spin_est.setInputWithNormals (cloud.makeShared (), normals);
  spin_est.setIndices (indicesptr);
  spin_est.setSearchMethod (tree);
  spin_est.setRadiusSearch (40*mr);

  // Object
  PointCloud<Eigen::MatrixXf>::Ptr spin_images (new PointCloud<Eigen::MatrixXf>);

  // radial SI
  spin_est.setRadialStructure ();

  // estimate
  spin_est.compute (*spin_images);
  EXPECT_EQ (spin_images->points.rows (), indices.size ());

  EXPECT_NEAR (spin_images->points (100, 0), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 12), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 24), 0.00233226, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 36), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 48), 8.48662e-005, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 60), 0.0266387, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 72), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 84), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 96), 0.0414662, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 108), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 120), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 132), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 144), 0.0128513, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 0), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 12), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 24), 0.00932424, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 36), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 48), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 60), 0.0145733, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 72), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 84), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 96), 0.00034457, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 108), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 120), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 132), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 144), 0.0121195, 1e-5);


  // radial SI, angular spin-images
  spin_est.setAngularDomain ();

  // estimate
  spin_est.compute (*spin_images);
  EXPECT_EQ (spin_images->points.rows (), indices.size ());

  EXPECT_NEAR (spin_images->points (100, 0), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 12), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 24), 0.132141, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 36), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 48), 0.908802, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 60), 0.63875, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 72), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 84), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 96), 0.550392, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 108), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 120), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 132), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 144), 0.257136, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 0), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 12), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 24), 0.230605, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 36), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 48), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 60), 0.764872, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 72), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 84), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 96), 1.02824, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 108), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 120), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 132), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 144), 0.293567, 1e-5);


  // rectangular SI
  spin_est.setRadialStructure (false);
  spin_est.setAngularDomain (false);

  // estimate
  spin_est.compute (*spin_images);
  EXPECT_EQ (spin_images->points.rows (), indices.size ());

  EXPECT_NEAR (spin_images->points (100, 0), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 12), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 24), 0.000889345, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 36), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 48), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 60), 0.0489534, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 72), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 84), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 96), 0.0747141, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 108), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 120), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 132), 0.0173423, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 144), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 0), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 12), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 24), 0.0267132, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 36), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 48), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 60), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 72), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 84), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 96), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 108), 0.0209709, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 120), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 132), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 144), 0.029372, 1e-5);

  // rectangular SI, angular spin-images
  spin_est.setAngularDomain ();

  // estimate
  spin_est.compute (*spin_images);
  EXPECT_EQ (spin_images->points.rows (), indices.size ());

  EXPECT_NEAR (spin_images->points (100, 0), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 12), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 24), 0.132141, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 36), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 48), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 60), 0.388027, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 72), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 84), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 96), 0.468881, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 108), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 120), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 132), 0.678995, 1e-5);
  EXPECT_NEAR (spin_images->points (100, 144), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 0), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 12), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 24), 0.143845, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 36), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 48), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 60), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 72), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 84), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 96), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 108), 0.706084, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 120), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 132), 0, 1e-5);
  EXPECT_NEAR (spin_images->points (300, 144), 0.272542, 1e-5);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, IntensitySpinEstimation)
{
  // Generate a sample point cloud
  PointCloud<PointXYZI> cloud_xyzi;
  cloud_xyzi.height = 1;
  cloud_xyzi.is_dense = true;

  for (float x = -10.0; x <= 10.0; x += 1.0)
  {
    for (float y = -10.0; y <= 10.0; y += 1.0)
    {
      PointXYZI p;
      p.x = x;
      p.y = y;
      p.z = sqrt (400 - x * x - y * y);
      p.intensity = exp (-(pow (x - 3, 2) + pow (y + 2, 2)) / (2 * 25.0)) + exp (-(pow (x + 5, 2) + pow (y - 5, 2))
                                                                                 / (2 * 4.0));

      cloud_xyzi.points.push_back (p);
    }
  }
  cloud_xyzi.width = cloud_xyzi.points.size ();

  // Compute the intensity-domain spin features
  typedef Histogram<20> IntensitySpin;
  IntensitySpinEstimation<PointXYZI, Eigen::MatrixXf> ispin_est;
  search::KdTree<PointXYZI>::Ptr treept3 (new search::KdTree<PointXYZI> (false));
  ispin_est.setSearchMethod (treept3);
  ispin_est.setRadiusSearch (10.0);
  ispin_est.setNrDistanceBins (4);
  ispin_est.setNrIntensityBins (5);

  ispin_est.setInputCloud (cloud_xyzi.makeShared ());
  PointCloud<Eigen::MatrixXf> ispin_output;
  ispin_est.compute (ispin_output);

  // Compare to independently verified values
  Eigen::VectorXf ispin = ispin_output.points.row (220);
  const float correct_ispin_feature_values[20] = {2.4387, 9.4737, 21.3232, 28.3025, 22.5639, 13.2426, 35.7026, 60.0755,
                                                  66.9240, 50.4225, 42.7086, 83.5818, 105.4513, 97.8454, 67.3801,
                                                  75.7127, 119.4726, 120.9649, 93.4829, 55.4045};
  for (int i = 0; i < 20; ++i)
  {
    EXPECT_NEAR (ispin[i], correct_ispin_feature_values[i], 1e-4);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, RIFTEstimation)
{
  // Generate a sample point cloud
  PointCloud<PointXYZI> cloud_xyzi;
  cloud_xyzi.height = 1;
  cloud_xyzi.is_dense = true;
  for (float x = -10.0; x <= 10.0; x += 1.0)
  {
    for (float y = -10.0; y <= 10.0; y += 1.0)
    {
      PointXYZI p;
      p.x = x;
      p.y = y;
      p.z = sqrt (400 - x * x - y * y);
      p.intensity = exp ((-pow (x - 3, 2) + pow (y + 2, 2)) / (2 * 25.0)) + exp ((-pow (x + 5, 2) + pow (y - 5, 2))
                                                                                 / (2 * 4.0));

      cloud_xyzi.points.push_back (p);
    }
  }
  cloud_xyzi.width = cloud_xyzi.points.size ();

  // Generate the intensity gradient data
  PointCloud<IntensityGradient> gradient;
  gradient.height = 1;
  gradient.width = cloud_xyzi.points.size ();
  gradient.is_dense = true;
  gradient.points.resize (gradient.width);
  for (size_t i = 0; i < cloud_xyzi.points.size (); ++i)
  {
    const PointXYZI &p = cloud_xyzi.points[i];

    // Compute the surface normal analytically.
    float nx = p.x;
    float ny = p.y;
    float nz = p.z;
    float magnitude = sqrt (nx * nx + ny * ny + nz * nz);
    nx /= magnitude;
    ny /= magnitude;
    nz /= magnitude;

    // Compute the intensity gradient analytically...
    float tmpx = -(p.x + 5) / 4.0 / exp ((pow (p.x + 5, 2) + pow (p.y - 5, 2)) / 8.0) - (p.x - 3) / 25.0
        / exp ((pow (p.x - 3, 2) + pow (p.y + 2, 2)) / 50.0);
    float tmpy = -(p.y - 5) / 4.0 / exp ((pow (p.x + 5, 2) + pow (p.y - 5, 2)) / 8.0) - (p.y + 2) / 25.0
        / exp ((pow (p.x - 3, 2) + pow (p.y + 2, 2)) / 50.0);
    float tmpz = 0;
    // ...and project the 3-D gradient vector onto the surface's tangent plane.
    float gx = (1 - nx * nx) * tmpx + (-nx * ny) * tmpy + (-nx * nz) * tmpz;
    float gy = (-ny * nx) * tmpx + (1 - ny * ny) * tmpy + (-ny * nz) * tmpz;
    float gz = (-nz * nx) * tmpx + (-nz * ny) * tmpy + (1 - nz * nz) * tmpz;

    gradient.points[i].gradient[0] = gx;
    gradient.points[i].gradient[1] = gy;
    gradient.points[i].gradient[2] = gz;
  }

  // Compute the RIFT features
  RIFTEstimation<PointXYZI, IntensityGradient, Eigen::MatrixXf> rift_est;
  search::KdTree<PointXYZI>::Ptr treept4 (new search::KdTree<PointXYZI> (false));
  rift_est.setSearchMethod (treept4);
  rift_est.setRadiusSearch (10.0);
  rift_est.setNrDistanceBins (4);
  rift_est.setNrGradientBins (8);

  rift_est.setInputCloud (cloud_xyzi.makeShared ());
  rift_est.setInputGradient (gradient.makeShared ());
  PointCloud<Eigen::MatrixXf> rift_output;
  rift_est.compute (rift_output);

  // Compare to independently verified values
  Eigen::VectorXf rift = rift_output.points.row (220);
  const float correct_rift_feature_values[32] = {0.0187, 0.0349, 0.0647, 0.0881, 0.0042, 0.0131, 0.0346, 0.0030,
                                                 0.0076, 0.0218, 0.0463, 0.0030, 0.0087, 0.0288, 0.0920, 0.0472,
                                                 0.0076, 0.0420, 0.0726, 0.0669, 0.0090, 0.0901, 0.1274, 0.2185,
                                                 0.0147, 0.1222, 0.3568, 0.4348, 0.0149, 0.0806, 0.2787, 0.6864};
  for (int i = 0; i < 32; ++i)
  {
    EXPECT_NEAR (rift[i], correct_rift_feature_values[i], 1e-4);
  }
}

/* ---[ */
int
main (int argc, char** argv)
{
  if (argc < 2)
  {
    std::cerr << "No test file given. Please download `bun0.pcd` and pass its path to the test." << std::endl;
    return (-1);
  }

  sensor_msgs::PointCloud2 cloud_blob;
  if (loadPCDFile (argv[1], cloud_blob) < 0)
  {
    std::cerr << "Failed to read test file. Please download `bun0.pcd` and pass its path to the test." << std::endl;
    return (-1);
  }

  fromROSMsg (cloud_blob, cloud);

  indices.resize (cloud.points.size ());
  for (size_t i = 0; i < indices.size (); ++i)
  {
    indices[i] = i;
  }

  tree.reset (new search::KdTree<PointXYZ> (false));
  tree->setInputCloud (cloud.makeShared ());

  testing::InitGoogleTest (&argc, argv);
  return (RUN_ALL_TESTS ());
}
/* ]--- */