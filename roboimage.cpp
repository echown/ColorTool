/* This class deals with RoboCup images.  It is far from complete.  For one,
  it only deals with the actual image and not the joint values.
  */
#include "roboimage.h"

/* Constructor.  Takes a width and height.
  @param wd         width of our images
  @param ht         height
  */
RoboImage::RoboImage(int wd, int ht)
{
    width = wd;
    height = ht;
    yImg = new int*[wd];
    uImg = new int*[wd];
    vImg = new int*[wd];
    for (int i = 0; i < wd; i++) {
        yImg[i] = new int[ht];
        vImg[i] = new int[ht];
        uImg[i] = new int[ht];
    }
}

/* Read an image.  Only mostly (see above).
  Reading bytes in QT is kind of weird (or at least the way I figured out is weird).
  We read in one byte chunks - this is done by reading into a byte array of size 1.
  Then we need to make sure that QT interprets our numbers the way we want them,
  meaning between 0 and 255.  This actually takes a fair amount of care.  See the
  code for more.
  @param filename      the image filename
  */
void RoboImage::read(QString filename)
{
    QFile file(filename);
    QTextStream out(stdout);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        out << "The file would not open properly" << "\n";
            return;
    }
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; x += 2)
        {
            QByteArray temp;
            // read a single byte
            temp = file.read(1);
            // now put that where we want it
            yImg[x][y] = temp[0];
            // sometimes it gets interpreted as a negative number, so make it positive
            while (yImg[x][y] < 0)
            {
                yImg[x][y] = 256 + yImg[x][y];
            }
            // sometimes it gets interpreted as a number larger than 255, so reduce it
            yImg[x][y] = yImg[x][y] % 256;
            temp = file.read(1);
            uImg[x][y] = temp[0];
            while (uImg[x][y] < 0)
            {
                uImg[x][y] = 256 + uImg[x][y];
            }
            uImg[x][y] = uImg[x][y] % 256;
            uImg[x+1][y] = uImg[x][y];
            temp = file.read(1);
            yImg[x+1][y] = temp[0];
            temp = file.read(1);
            while (yImg[x+1][y] < 0)
            {
                yImg[x+1][y] = 256 + yImg[x+1][y];
            }
            yImg[x+1][y] = yImg[x+1][y] % 256;
            vImg[x][y] = temp[0];
            while (vImg[x][y] < 0)
            {
                vImg[x][y] = 256 + vImg[x][y];
            }
            vImg[x][y] = vImg[x][y] % 256;
            vImg[x+1][y] = vImg[x][y];
        }
    }
    file.close();
}

/* Used for returning bitmapped images.  Never really got this to work properly.
  To Do: fix
  */
QImage RoboImage::fast()
{
    ColorZone cz();
    return bmp();
}

/* Get R information for the specified pixel
  @param x          x value
  @param y          y value
  @return           amount of red in that pixel
  */
int RoboImage::getRed(int x, int y)
{
    ColorSpace c;
    c.setYuv(yImg[x][y], uImg[x][y], vImg[x][y]);
    return c.getRb();
}

/* Get G information for the specified pixel
  @param x          x value
  @param y          y value
  @return           amount of green in that pixel
  */
int RoboImage::getGreen(int x, int y)
{
    ColorSpace c;
    c.setYuv(yImg[x][y], uImg[x][y], vImg[x][y]);
    return c.getGb();
}

/* Get B information for the specified pixel
  @param x          x value
  @param y          y value
  @return           amount of blue in that pixel
  */
int RoboImage::getBlue(int x, int y)
{
    ColorSpace c;
    c.setYuv(yImg[x][y], uImg[x][y], vImg[x][y]);
    return c.getBb();
}

/* Get H information for the specified pixel
  @param x          x value
  @param y          y value
  @return           amount of Hue in that pixel
  */
int RoboImage::getH(int x, int y)
{
    ColorSpace c;
    c.setYuv(yImg[x][y], uImg[x][y], vImg[x][y]);
    return c.getHb();
}

/* Get S information for the specified pixel
  @param x          x value
  @param y          y value
  @return           amount of saturation in that pixel
  */
int RoboImage::getS(int x, int y)
{
    ColorSpace c;
    c.setYuv(yImg[x][y], uImg[x][y], vImg[x][y]);
    return c.getSb();
}

/* Get Z(v) information for the specified pixel
  @param x          x value
  @param y          y value
  @return           amount of z in that pixel
  */
int RoboImage::getZ(int x, int y)
{
    ColorSpace c;
    c.setYuv(yImg[x][y], uImg[x][y], vImg[x][y]);
    return c.getZb();
}

/* Return a bitmapped image of the image.  Doesn't really work.
  To Do: fix
  */
QImage RoboImage::bmp()
{
    QImage img(width, height, QImage::Format_RGB32);
    img.fill(0);
    ColorSpace c;
    DisplayModes dm = Color;
    ColorZone cz;
    QTextStream out(stdout);

    for (int j = 0; j < height; ++j)
      for (int i = 0; i < width; ++i)
      {
        c.setYuv(yImg[i][j], uImg[i][j], vImg[i][j]);
        if (cz.within(c))
        {
          int r, g, b;
          switch (dm)
          {
            case Color:
              r = c.getRb();
              g = c.getGb();
              b = c.getBb();
              //out << i << " " << j << " " << r << " " << g << " " << b << "\n";
              break;

            case Y:
              r = g = b = yImg[i][j];
              break;

            case U:
              r = g = b = uImg[i][j];
              break;

            case V:
              r = g = b = vImg[i][j];
              break;

            case Red:
              r = g = b = c.getRb();
              break;

            case Green:
              r = g = b = c.getGb();
              break;

            case Blue:
              r = g = b = c.getBb();
              break;

            case Hue:
              if (c.getS() >= 0.25f && c.getY() >= 0.2f)
              {
                ColorSpace h = ColorSpace();
                h.setHsz(c.getH(), c.getS(), 0.875f);
                r = h.getRb();
                g = h.getGb();
                b = h.getBb();
              }
              else
                r = g = b = 0;
              break;

            case Saturation:
              r = g = b = c.getSb();
              break;

            case Value:
              r = g = b = c.getZb();
              break;

            default:
              r = g = b = 0;
              break;
          }
          QRgb value = qRgb(r, g, b);
          //value = Qt::cyan;
          //out << " " << value.red() << " " << value.green() << " " << value.blue() << "\n";
          img.setPixel(i, j, value);
        }
      }

    return img;
}
