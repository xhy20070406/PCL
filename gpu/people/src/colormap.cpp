#include <pcl/gpu/people/colormap.h>
#include <vector>
#include "internal.h"

using namespace std;
using namespace pcl::gpu::people;

pcl::RGB pcl::gpu::people::getLColor (unsigned char l)
{
  const unsigned char* c = LUT_COLOR_LABEL + 3 * l;
  pcl::RGB p;
  p.r = c[0];
  p.g = c[1];
  p.b = c[2];
  return p;
}

pcl::RGB pcl::gpu::people::getLColor (pcl::Label l)
{
  return getLColor(static_cast<unsigned char> (l.label));
}

void pcl::gpu::people::colorLMap ( int W, int H, const trees::Label* l, unsigned char* c )
{
  int numPix = W * H;
  for(int pi = 0; pi < numPix; ++pi) 
  {
    const unsigned char* color = LUT_COLOR_LABEL + 3 * l[pi];
    c[3*pi+0] = color[0];
    c[3*pi+1] = color[1];
    c[3*pi+2] = color[2];
  }
}

void pcl::gpu::people::colorLMap (const pcl::PointCloud<pcl::Label>& cloud_in, pcl::PointCloud<pcl::RGB>& colormap_out)
{
  colormap_out.resize(cloud_in.size());
  for(size_t i = 0; i < cloud_in.size (); i++)
    colormap_out.points[i] = getLColor(cloud_in.points[i]);

  colormap_out.width = cloud_in.width;
  colormap_out.height = cloud_in.height;
}

void pcl::gpu::people::uploadColorMap(DeviceArray<pcl::RGB>& color_map)
{
  // Copy the list of label colors into the devices
  vector<pcl::RGB> rgba(LUT_COLOR_LABEL_LENGTH);
  for(int i = 0; i < LUT_COLOR_LABEL_LENGTH; ++i)
  {
    // !!!! generate in RGB format, not BGR
    rgba[i].r = LUT_COLOR_LABEL[i*3 + 2];
    rgba[i].g = LUT_COLOR_LABEL[i*3 + 1];
    rgba[i].b = LUT_COLOR_LABEL[i*3 + 0];
    rgba[i].a = 255;
  }
  color_map.upload(rgba);
}

void pcl::gpu::people::colorizeLabels(const DeviceArray<pcl::RGB>& color_map, const DeviceArray2D<unsigned char>& labels, DeviceArray2D<pcl::RGB>& color_labels)
{
  color_labels.create(labels.rows(), labels.cols());

  const DeviceArray<uchar4>& map = (const DeviceArray<uchar4>&)color_map;
  device::Image& img = (device::Image&)color_labels;  
  device::colorLMap(labels, map, img);
}

void pcl::gpu::people::colorizeMixedLabels(const DeviceArray<pcl::RGB>& color_map, const DeviceArray2D<unsigned char>& labels, 
                                           const DeviceArray2D<pcl::RGB>& image, DeviceArray2D<pcl::RGB>& color_labels)
{
  color_labels.create(labels.rows(), labels.cols());

  const DeviceArray<uchar4>& map = (const DeviceArray<uchar4>&)color_map;
  device::Image& img = (device::Image&)color_labels;
  device::Image& rgba = (device::Image&)image;
  device::mixedColorMap(labels, map, rgba, img);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const unsigned char pcl::gpu::people::LUT_COLOR_LABEL[] = 
{
     50, 34, 22,
     24,247,196,
     33,207,189,
    254,194,127,
     88,115,175,
    158, 91, 64,
     14, 90,  2,
    100,156,227,
    243,167, 17,
    145,194,184,
    234,171,147,
    220,112, 93,
     93,132,163,
    122,  4, 85,
     75,168, 46,
     15,  5,108,
    180,125,107,
    157, 77,167,
    214, 89, 73,
     52,183, 58,
     54,155, 75,
    249, 61,187,
    143, 57, 11,
    246,198,  0,
    202,177,251,
    229,115, 80,
    159,185,  1,
    186,213,229,
     82, 47,144,
    140, 69,139,
    189,115,117,
     80, 57,150 
};
        
const int pcl::gpu::people::LUT_COLOR_LABEL_LENGTH = sizeof(LUT_COLOR_LABEL)/(sizeof(LUT_COLOR_LABEL[0]) * 3);
