/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2010, Willow Garage, Inc.
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
 * $Id$
 *
 */
// PCL
#include <Eigen/Geometry>
#include <pcl/common/common.h>
#include <pcl/io/pcd_io.h>
#include <cfloat>
#include <pcl/visualization/point_cloud_handlers.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/visualization/histogram_visualizer.h>
#include <pcl/console/print.h>
#include <pcl/console/parse.h>
#include <pcl/console/time.h>

using pcl::console::print_color;
using pcl::console::print_error;
using pcl::console::print_error;
using pcl::console::print_warn;
using pcl::console::print_info;
using pcl::console::print_debug;
using pcl::console::print_value;
using pcl::console::print_highlight;
using pcl::console::TT_BRIGHT;
using pcl::console::TT_RED;
using pcl::console::TT_GREEN;
using pcl::console::TT_BLUE;

typedef pcl_visualization::PointCloudColorHandler<sensor_msgs::PointCloud2> ColorHandler;
typedef ColorHandler::Ptr ColorHandlerPtr;
typedef ColorHandler::ConstPtr ColorHandlerConstPtr;

typedef pcl_visualization::PointCloudGeometryHandler<sensor_msgs::PointCloud2> GeometryHandler;
typedef GeometryHandler::Ptr GeometryHandlerPtr;
typedef GeometryHandler::ConstPtr GeometryHandlerConstPtr;

#define NORMALS_SCALE 0.01
#define PC_SCALE 0.001

bool
isValidFieldName (const std::string &field)
{
  if (field == "_")
    return (false);

  if ((field == "vp_x") || (field == "vx") || (field == "vp_y") || (field == "vy") || (field == "vp_z") || (field == "vz"))
    return (false);
  return (true);
}

bool
  isMultiDimensionalFeatureField (const sensor_msgs::PointField &field)
{
  if (field.count > 1) 
    return (true);
  return (false);
}

void
  printHelp (int argc, char **argv)
{
  print_error ("Syntax is: %s <file_name 1..N>.pcd <options>\n", argv[0]);
  print_info ("  where options are:\n");
  print_info ("                     -bc r,g,b                = background color\n");
  print_info ("                     -fc r,g,b                = foreground color\n");
  print_info ("                     -ps X                    = point size ("); print_value ("1..64"); print_info (") \n");
  print_info ("                     -opaque X                = rendered point cloud opacity ("); print_value ("0..1"); print_info (")\n");

  print_info ("                     -ax "); print_value ("n"); print_info ("                    = enable on-screen display of "); 
  print_color (stdout, TT_BRIGHT, TT_RED, "X"); print_color (stdout, TT_BRIGHT, TT_GREEN, "Y"); print_color (stdout, TT_BRIGHT, TT_BLUE, "Z");
  print_info (" axes and scale them to "); print_value ("n\n");
  print_info ("                     -ax_pos X,Y,Z            = if axes are enabled, set their X,Y,Z position in space (default "); print_value ("0,0,0"); print_info (")\n");

  print_info ("\n");
  print_info ("                     -cam (*)                 = use given camera settings as initial view\n");
  print_info (stderr, " (*) [Clipping Range / Focal Point / Position / ViewUp / Distance / Window Size / Window Pos] or use a <filename.cam> that contains the same information.\n");

  print_info ("\n");
  print_info ("                     -multiview 0/1           = enable/disable auto-multi viewport rendering (default "); print_value ("disabled"); print_info (")\n");
  print_info ("\n");

  print_info ("\n");
  print_info ("                     -normals 0/X             = disable/enable the display of every Xth point's surface normal as lines (default "); print_value ("disabled"); print_info (")\n");
  print_info ("                     -normals_scale X         = resize the normal unit vector size to X (default "); print_value ("0.02"); print_info (")\n");
  print_info ("\n");
  print_info ("                     -pc 0/X                  = disable/enable the display of every Xth point's principal curvatures as lines (default "); print_value ("disabled"); print_info (")\n");
  print_info ("                     -pc_scale X              = resize the principal curvatures vectors size to X (default "); print_value ("0.02"); print_info (")\n");
  print_info ("\n");

  print_info ("\n(Note: for multiple .pcd files, provide multiple -{fc,ps,opaque} parameters; they will be automatically assigned to the right file)\n");
}

/* ---[ */
int
  main (int argc, char** argv)
{
  srand (time (0));

  if (argc < 2)
  {
    printHelp (argc, argv);
    return (-1);
  }

  // Command line parsing
  double bcolor[3] = {0, 0, 0};
  pcl::console::parse_3x_arguments (argc, argv, "-bc", bcolor[0], bcolor[1], bcolor[2]);

  std::vector<double> fcolor_r, fcolor_b, fcolor_g;
  bool fcolorparam = pcl::console::parse_multiple_3x_arguments (argc, argv, "-fc", fcolor_r, fcolor_g, fcolor_b);

  std::vector<int> psize;
  pcl::console::parse_multiple_arguments (argc, argv, "-ps", psize);

  std::vector<double> opaque;
  pcl::console::parse_multiple_arguments (argc, argv, "-opaque", opaque);

  int mview = 0;
  pcl::console::parse_argument (argc, argv, "-multiview", mview);

  int normals = 0;
  pcl::console::parse_argument (argc, argv, "-normals", normals);
  double normals_scale = NORMALS_SCALE;
  pcl::console::parse_argument (argc, argv, "-normals_scale", normals_scale);

  int pc = 0;
  pcl::console::parse_argument (argc, argv, "-pc", pc);
  double pc_scale = PC_SCALE;
  pcl::console::parse_argument (argc, argv, "-pc_scale", pc_scale);

  // Parse the command line arguments for .pcd files
  std::vector<int> p_file_indices;
  p_file_indices = pcl::console::parse_file_extension_argument (argc, argv, ".pcd");
  if (p_file_indices.size () == 0)
  {
    print_error ("No .PCD file given. Nothing to visualize.\n");
    return (-1);
  }

  // Multiview enabled?
  int y_s = 0, x_s = 0;
  double x_step = 0, y_step = 0;
  if (mview)
  {
    print_highlight ("Multi-viewport rendering enabled.\n");

    y_s = static_cast<int>(floor (sqrt (static_cast<float>(p_file_indices.size ()))));
    x_s = y_s + static_cast<int>(ceil ((p_file_indices.size () / static_cast<double>(y_s)) - y_s));
    x_step = static_cast<double>(1.0 / static_cast<double>(x_s));
    y_step = static_cast<double>(1.0 / static_cast<double>(y_s));
    print_highlight ("Preparing to load "); print_value ("%d", p_file_indices.size ());
    print_info (" files ("); print_value ("%d", x_s);    print_info ("x"); print_value ("%d", y_s);
    print_info (" / ");      print_value ("%f", x_step); print_info ("x"); print_value ("%f", y_step);
    print_info (")\n");
  }

  // Fix invalid multiple arguments
  if (psize.size () != p_file_indices.size () && psize.size () > 0)
    for (size_t i = psize.size (); i < p_file_indices.size (); ++i)
      psize.push_back (1);
  if (opaque.size () != p_file_indices.size () && opaque.size () > 0)
    for (size_t i = opaque.size (); i < p_file_indices.size (); ++i)
      opaque.push_back (1.0);

  // Create the PCLVisualizer object
  boost::shared_ptr<pcl_visualization::PCLHistogramVisualizer> ph;
  boost::shared_ptr<pcl_visualization::PCLVisualizer> p;

  // Using min_p, max_p to set the global Y min/max range for the histogram
  float min_p = FLT_MAX; float max_p = -FLT_MAX;

  int k = 0, l = 0, viewport = 0;
  // Load the data files
  pcl::PCDReader pcd;
  pcl::console::TicToc tt;
  ColorHandlerPtr color_handler;
  GeometryHandlerPtr geometry_handler;

  for (size_t i = 0; i < p_file_indices.size (); ++i)
  {
    sensor_msgs::PointCloud2::Ptr cloud (new sensor_msgs::PointCloud2);
    Eigen::Vector4f origin;
    Eigen::Quaternionf orientation;
    int version;

    print_highlight (stderr, "Loading "); print_value (stderr, "%s ", argv[p_file_indices.at (i)]);

    tt.tic ();
    if (pcd.read (argv[p_file_indices.at (i)], *cloud, origin, orientation, version) < 0)
      return (-1);
   
    std::stringstream cloud_name;

    // ---[ Special check for 1-point multi-dimension histograms
    if (cloud->fields.size () == 1 && isMultiDimensionalFeatureField (cloud->fields[0]))
    {
      cloud_name << argv[p_file_indices.at (i)];

      if (!ph)
        ph.reset (new pcl_visualization::PCLHistogramVisualizer);
      print_info ("[done, "); print_value ("%g", tt.toc ()); print_info (" seconds : "); print_value ("%d", cloud->fields[0].count); print_info (" points]\n");

      pcl::getMinMax (*cloud, 0, cloud->fields[0].name, min_p, max_p);
      ph->addFeatureHistogram (*cloud, cloud->fields[0].name, cloud_name.str ());
      continue;
    }

    cloud_name << argv[p_file_indices.at (i)] << "-" << i;
    // Create the PCLVisualizer object here on the first encountered XYZ file
    if (!p)
      p.reset (new pcl_visualization::PCLVisualizer (argc, argv, "PCD viewer"));

    // Multiview enabled?
    if (mview)
    {
      p->createViewPort (k * x_step, l * y_step, (k + 1) * x_step, (l + 1) * y_step, viewport);
      k++;
      if (k >= x_s)
      {
        k = 0;
        l++;
      }
    }

    // Convert from blob to pcl::PointCloud
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_xyz (new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromROSMsg (*cloud, *cloud_xyz);

    if (cloud_xyz->points.size () == 0)
    {
      print_error ("[error: no points found!]\n");
      return (-1);
    }
    print_info ("[done, "); print_value ("%g", tt.toc ()); print_info (" seconds : "); print_value ("%d", (int)cloud_xyz->points.size ()); print_info (" points]\n");
    print_info ("Available dimensions: "); print_value ("%s\n", pcl::getFieldsList (*cloud).c_str ());
   
    // If no color was given, get random colors
    if (fcolorparam)
    {
      if (fcolor_r.size () > i && fcolor_g.size () > i && fcolor_b.size () > i)
        color_handler.reset (new pcl_visualization::PointCloudColorHandlerCustom<sensor_msgs::PointCloud2> (cloud, fcolor_r[i], fcolor_g[i], fcolor_b[i]));
      else
        color_handler.reset (new pcl_visualization::PointCloudColorHandlerRandom<sensor_msgs::PointCloud2> (cloud));
    }
    else
      color_handler.reset (new pcl_visualization::PointCloudColorHandlerRandom<sensor_msgs::PointCloud2> (cloud));

    // Add the dataset with a XYZ and a random handler 
    geometry_handler.reset (new pcl_visualization::PointCloudGeometryHandlerXYZ<sensor_msgs::PointCloud2> (cloud));
    // Add the cloud to the renderer
    p->addPointCloud<pcl::PointXYZ> (cloud_xyz, geometry_handler, color_handler, cloud_name.str (), viewport);

    // If normal lines are enabled
    if (normals != 0)
    {
      int normal_idx = pcl::getFieldIndex (*cloud, "normal_x");
      if (normal_idx == -1)
      {
        print_error ("Normal information requested but not available.\n");
        continue;
        //return (-1);
      }      
      //
      // Convert from blob to pcl::PointCloud
      pcl::PointCloud<pcl::Normal>::Ptr cloud_normals (new pcl::PointCloud<pcl::Normal>);
      pcl::fromROSMsg (*cloud, *cloud_normals);
      std::stringstream cloud_name_normals;
      cloud_name_normals << argv[p_file_indices.at (i)] << "-" << i << "-normals";
      p->addPointCloudNormals<pcl::PointXYZ, pcl::Normal> (cloud_xyz, cloud_normals, normals, normals_scale, cloud_name_normals.str (), viewport);
    }

    // If principal curvature lines are enabled
    if (pc != 0)
    {
      if (normals == 0)
        normals = pc;

      int normal_idx = pcl::getFieldIndex (*cloud, "normal_x");
      if (normal_idx == -1)
      {
        print_error ("Normal information requested but not available.\n");
        continue;
        //return (-1);
      }      
      int pc_idx = pcl::getFieldIndex (*cloud, "principal_curvature_x");
      if (pc_idx == -1)
      {
        print_error ("Principal Curvature information requested but not available.\n");
        continue;
        //return (-1);
      }      
      //
      // Convert from blob to pcl::PointCloud
      pcl::PointCloud<pcl::Normal>::Ptr cloud_normals (new pcl::PointCloud<pcl::Normal>);
      pcl::fromROSMsg (*cloud, *cloud_normals);
      pcl::PointCloud<pcl::PrincipalCurvatures>::Ptr cloud_pc (new pcl::PointCloud<pcl::PrincipalCurvatures>);
      pcl::fromROSMsg (*cloud, *cloud_pc);
      std::stringstream cloud_name_normals_pc;
      cloud_name_normals_pc << argv[p_file_indices.at (i)] << "-" << i << "-normals";
      int factor = (std::min)(normals, pc);
      p->addPointCloudNormals<pcl::PointXYZ, pcl::Normal> (cloud_xyz, cloud_normals, factor, normals_scale, cloud_name_normals_pc.str (), viewport);
      p->setPointCloudRenderingProperties (pcl_visualization::PCL_VISUALIZER_COLOR, 1.0, 0.0, 0.0, cloud_name_normals_pc.str ());
      p->setPointCloudRenderingProperties (pcl_visualization::PCL_VISUALIZER_LINE_WIDTH, 3, cloud_name_normals_pc.str ());
      cloud_name_normals_pc << "-pc";
      p->addPointCloudPrincipalCurvatures (cloud_xyz, cloud_normals, cloud_pc, factor, pc_scale, cloud_name_normals_pc.str (), viewport);
      p->setPointCloudRenderingProperties (pcl_visualization::PCL_VISUALIZER_LINE_WIDTH, 3, cloud_name_normals_pc.str ());
    }

    // Add every dimension as a possible color
    if (!fcolorparam)
    {
      for (size_t f = 0; f < cloud->fields.size (); ++f)
      {
        if (cloud->fields[f].name == "rgb" || cloud->fields[f].name == "rgba")
          color_handler.reset (new pcl_visualization::PointCloudColorHandlerRGBField<sensor_msgs::PointCloud2> (cloud));
        else
        {
          if (!isValidFieldName (cloud->fields[f].name))
            continue;
          color_handler.reset (new pcl_visualization::PointCloudColorHandlerGenericField<sensor_msgs::PointCloud2> (cloud, cloud->fields[f].name));
        }
        // Add the cloud to the renderer
        p->addPointCloud<pcl::PointXYZ> (cloud_xyz, color_handler, cloud_name.str (), viewport);
      }
    }
    // Additionally, add normals as a handler
    geometry_handler.reset (new pcl_visualization::PointCloudGeometryHandlerSurfaceNormal<sensor_msgs::PointCloud2> (cloud));
    if (geometry_handler->isCapable ())
      p->addPointCloud<pcl::PointXYZ> (cloud_xyz, geometry_handler, cloud_name.str (), viewport);

    // Change the cloud rendered point size
    if (psize.size () > 0)
      p->setPointCloudRenderingProperties (pcl_visualization::PCL_VISUALIZER_POINT_SIZE, psize.at (i), cloud_name.str ());

    // Change the cloud rendered opacity
    if (opaque.size () > 0)
      p->setPointCloudRenderingProperties (pcl_visualization::PCL_VISUALIZER_OPACITY, opaque.at (i), cloud_name.str ());
  }

  if (p)
    p->setBackgroundColor (bcolor[0], bcolor[1], bcolor[2]);
  // Read axes settings
  double axes  = 0.0;
  pcl::console::parse_argument (argc, argv, "-ax", axes);
  if (axes != 0.0 && p)
  {
    double ax_x = 0.0, ax_y = 0.0, ax_z = 0.0;
    pcl::console::parse_3x_arguments (argc, argv, "-ax_pos", ax_x, ax_y, ax_z, false);
    // Draw XYZ axes if command-line enabled
    p->addCoordinateSystem (axes, ax_x, ax_y, ax_z);
  }

  if (p)
    p->spin ();
  if (ph)
  {
    print_highlight ("Setting the global Y range for all histograms to: "); print_value ("%f -> %f\n", min_p, max_p);
    ph->setGlobalYRange (min_p, max_p);
    ph->updateWindowPositions ();
    ph->spin ();
  }
}
/* ]--- */
