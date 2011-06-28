/* Main file.  This is a color calibration tool (for now).  Pops up a window
  that can load image frames and display them in three ways:
  1) The raw image
  2) The image calibrated according to a color table, or this tool's controls
  3) Lots of other things (e.g. just the Y channel of the image)

  We make color tables with this tool by using its sliders.  The sliders control
  ranges (e.g. set a max and minimum value for Y) for each color.  Those define
  a region of color space that the color is defined for.  What this does is
  to quickly build a highly filled in color region.  For now we then go back
  to the old tool to refine the image.  In the future it would be nice to
  be able to refine directly in this tool.

  Some things we don't have yet:
  1) The ability to grab key images and save them into a "key image" directory
  2) The ability to run our vision code and display object results
  3) The ability to specify one color's dominance over another (e.g. if pink
       && orange then choose pink)
  4) Lots of other stuff I haven't thought of yet

  Things that aren't really done that well (this was intended as a prototype)
  1) Data management
  2) More or less everything - this tool is oddly slow, I don't know QT well enough
  to know why.
  */

#include "colorcreator.h"
#include "ui_colorcreator.h"
#include "math.h"

#include <QTextStream>
#include <QFileDialog>
#include <QString>
#include <QStringRef>
#include <QRgb>
#include <QImage>
#include <QPixmap>

ColorCreator::ColorCreator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ColorCreator),
    roboimage(WIDTH, HEIGHT)

{
    // Display images
    img = new QImage(640, 480, QImage::Format_RGB32);
    img2 = new QImage(320, 240, QImage::Format_RGB32);
    img3 = new QImage(320, 240, QImage::Format_RGB32);
    img4 = new QImage(320, 240, QImage::Format_RGB32);
    wheel = new QImage(200, 200, QImage::Format_RGB32);

    // Each color gets its own value for everything specified by sliders
    hMin = new float[COLORS];
    hMax = new float[COLORS];
    sMin = new float[COLORS];
    sMax = new float[COLORS];
    zMin = new float[COLORS];
    zMax = new float[COLORS];
    yMin = new int[COLORS];
    yMax = new int[COLORS];
    vMin = new int[COLORS];
    vMax = new int[COLORS];
    cols = new QColor[COLORS+SOFT];
    bitColor = new unsigned[COLORS];

    table = new ColorTable();

    // initialize colors for drawing thresholds
    cols[Orange] = QColor(255, 128, 0);
    cols[Blue] = QColor(100, 149, 237);
    cols[Green] = QColor(0, 139, 0);
    cols[Yellow] = QColor(255, 255, 0);
    cols[White] = QColor(255, 255, 255);
    cols[Pink] = QColor(255, 181, 197);
    cols[Navy] = QColor(0, 0, 205);
    cols[Black] = QColor(100, 100, 100);
    cols[OrangeRed] = QColor(238, 64, 0);
    cols[BlueGreen] = QColor(72, 209, 234);
    cols[BlueNavy] =  QColor(105, 89, 205);

    // initialize bitColors for generating color tables
    bitColor[Orange] = ORANGE_COL;
    bitColor[Blue] = BLUE_COL;
    bitColor[Yellow] = YELLOW_COL;
    bitColor[Green] = GREEN_COL;
    bitColor[White] = WHITE_COL;
    bitColor[Pink] = RED_COL;
    bitColor[Navy] = NAVY_COL;
    bitColor[Black] = GREY_COL;

    ui->setupUi(this);
    // Default file directory - obviously this should not be user specific
    baseDirectory = "/Users/ericchown/nbites/data/frames";
    baseColorTable = "/Users/ericchown/nbites/data/tables";
    haveFile = false;
    viewerEnabled = false;
    tableMode = false;
    defineMode = false;
    cornerStatus = true;

    // color selection UI item
    ui->colorSelect->addItem(tr("Orange"), Orange);
    ui->colorSelect->addItem(tr("Blue"), Blue);
    ui->colorSelect->addItem(tr("Yellow"), Yellow);
    ui->colorSelect->addItem(tr("Green"), Green);
    ui->colorSelect->addItem(tr("White"), White);
    ui->colorSelect->addItem(tr("Pink"), Pink);
    ui->colorSelect->addItem(tr("Navy"), Navy);
    ui->colorSelect->addItem(tr("Black"), Black);

    ui->viewChoice->addItem(tr("Single Color"), Single);
    ui->viewChoice->addItem(tr("All Colors"), Multiple);


    ui->channel->addItem(tr("Y"), Y);
    ui->channel->addItem(tr("U"), U);
    ui->channel->addItem(tr("V"), V);
    ui->channel->addItem(tr("Blue"), Bluec);
    ui->channel->addItem(tr("Red"), Redc);
    ui->channel->addItem(tr("Green"), Greenc);
    ui->channel->addItem(tr("H"), H);
    ui->channel->addItem(tr("S"), S);
    ui->channel->addItem(tr("Z"), Z);
    ui->channel->addItem(tr("Edge"), EDGE);
    ui->channel->addItem(tr("Table"), Table);
    mode = Single;
    shape = Y;

    currentColor = Orange;
    currentDirectory = baseDirectory;
    currentColorDirectory = baseColorTable;
    zSlice = 0.75f;

    edgediff = 12;

    // initialize all of our values.  Ideally these will serve as a pretty good table
    // for virtually any environment
    // Note: it would be nice to be able to save values from various locations and load
    // the appropriate one here
    for (int i = 0; i < COLORS; i++)
    {
        switch(i)
        {
        case Orange:
            hMin[i] = 0.80f;
            hMax[i] = 0.13f;
            sMin[i] = 0.25f;
            sMax[i] = 1.0f;
            zMin[i] = 0.12f;
            zMax[i] = 1.0f;
            yMin[i] = 34;
            yMax[i] = 145;
            vMin[i] = 115;
            vMax[i] = 171;
            break;
        case Green:
            hMin[i] = 0.37f;
            hMax[i] = 0.45f;
            sMin[i] = 0.28f;
            sMax[i] = 0.46f;
            zMin[i] = 0.22f;
            zMax[i] = 0.53f;
            yMin[i] = 55;
            yMax[i] = 105;
            vMin[i] = 90;
            vMax[i] = 131;
            break;
        case Yellow:
            hMin[i] = 0.17f;
            hMax[i] = 0.26f;
            sMin[i] = 0.32f;
            sMax[i] = 0.69f;
            zMin[i] = 0.27f;
            zMax[i] = 0.48f;
            yMin[i] = 56;
            yMax[i] = 105;
            vMin[i] = 111;
            vMax[i] = 128;
            break;
        case Blue:
            hMin[i] = 0.54f;
            hMax[i] = 0.67f;
            sMin[i] = 0.30f;
            sMax[i] = 0.65f;
            zMin[i] = 0.23f;
            zMax[i] = 0.48f;
            yMin[i] = 33;
            yMax[i] = 105;
            vMin[i] = 109;
            vMax[i] = 127;
            break;
        case White:
            hMin[i] = 0.01f;
            hMax[i] = 0.01f;
            sMin[i] = 0.0f;
            sMax[i] = 0.38f;
            zMin[i] = 0.39f;
            zMax[i] = 1.0f;
            yMin[i] = 102;
            yMax[i] = 250;
            vMin[i] = 99;
            vMax[i] = 128;
            break;
        case Pink:
            hMin[i] = 0.75f;
            hMax[i] = 0.22f;
            sMin[i] = 0.0f;
            sMax[i] = 0.29f;
            zMin[i] = 0.21f;
            zMax[i] = 0.54f;
            yMin[i] = 58;
            yMax[i] = 139;
            vMin[i] = 127;
            vMax[i] = 143;
            break;
        case Navy:
            hMin[i] = 0.57f;
            hMax[i] = 0.68f;
            sMin[i] = 0.23f;
            sMax[i] = 0.42f;
            zMin[i] = 0.17f;
            zMax[i] = 0.45f;
            yMin[i] = 39;
            yMax[i] = 105;
            vMin[i] = 106;
            vMax[i] = 132;
            break;
        default:
            hMin[i] = 0.0f;
            hMax[i] = 0.01f;
            sMin[i] = 0.99f;
            sMax[i] = 1.0f;
            zMin[i] = 0.0f;
            zMax[i] = 1.0f;
            yMin[i] = 30;
            yMax[i] = 230;
            vMin[i] = 40;
            vMax[i] = 150;
            break;
        }
    }
    // set the sliders to start at the correct values
    ui->hMin->setValue(hMin[currentColor] * 100);
    ui->hMax->setValue(hMax[currentColor] * 100);
    ui->sMin->setValue(sMin[currentColor] * 100);
    ui->sMax->setValue(sMax[currentColor] * 100);
    ui->zMin->setValue(zMin[currentColor] * 100);
    ui->zMax->setValue(zMax[currentColor] * 100);
    ui->yMin->setValue(yMin[currentColor]);
    ui->yMax->setValue(yMax[currentColor]);
    ui->vMin->setValue(vMin[currentColor]);
    ui->vMax->setValue(vMax[currentColor]);
    ui->zSlice->setValue(zSlice);
    firstPoint.setX(-1);
    firstPoint.setY(-1);
    setMouseTracking(true);
}

ColorCreator::~ColorCreator()
{
    delete ui;
}


void ColorCreator::mouseMoveEvent(QMouseEvent *event)
{
    QTextStream out(stdout);
    QPoint thePoint = event->pos();
    int x = thePoint.x();
    int y = thePoint.y();
    if (x > IMAGE_X && x < IMAGE_X + IMAGE_WIDTH && y > IMAGE_Y && y < IMAGE_Y + IMAGE_HEIGHT && haveFile) {
        x = x - IMAGE_X;
        y = y - IMAGE_Y;
        int Y = roboimage.getY(x, y);
        int U = roboimage.getU(x, y);
        int V = roboimage.getV(x, y);
        int H = roboimage.getH(x, y);
        int S = roboimage.getS(x, y);
        int Z = roboimage.getZ(x, y);
        out << "YUV :" << Y << " " << U << " " << V << "\n";
        out << "HSZ :" << H << " " << S << " " << Z << "\n";
    }
}

void ColorCreator::mouseReleaseEvent(QMouseEvent *event)
{
    lastPoint = event->pos();
    lastPoint.setX(lastPoint.x() - IMAGE_X);
    lastPoint.setY(lastPoint.y() - IMAGE_Y);
    if (lastPoint.x() - firstPoint.x() > 20) {
        largeDisplay();
    }
}

void ColorCreator::mousePressEvent(QMouseEvent *event)
{
    firstPoint = event->pos();
    firstPoint.setX(firstPoint.x() - IMAGE_X);
    firstPoint.setY(firstPoint.y() - IMAGE_Y);
}

/* When something major changes we need to update all displays and file info.
  */
void ColorCreator::updateDisplays()
{
    QString next;
    next.setNum(currentFrameNumber+1, 10);
    QString prev;
    prev.setNum(currentFrameNumber-1, 10);
    QString plusTen;
    plusTen.setNum(currentFrameNumber+10, 10);
    QString minusTen;
    minusTen.setNum(currentFrameNumber-10, 10);
    nextFrame = currentDirectory + "/" + next + EXTENSION;
    previousFrame = currentDirectory + "/" + prev + EXTENSION;
    tenthFrame = currentDirectory + "/" + plusTen + EXTENSION;
    minusTenthFrame = currentDirectory + "/" + plusTen + EXTENSION;
    haveFile = true;
    updateThresh(true, false, false);
}

/* We display a color wheel to indicate the current color's region.  It is a 2D slice
  in 3D space (slicing on the V dimension (called Z here)) of HSV space.
  Here we just loop through the display area, calculate an angle and distance to the
  center (angle = H, distance = S) and determine whether any given pixel ought to
  be displayed for the current color settings.
  */
void ColorCreator::updateColors()
{
    // we draw by using a QImage - turn it into a Pixmap, then put it on a label
    //QImage img(200, 200, QImage::Format_RGB32);
    bool display;
    QColor c;
    if (!tableMode) {
      /* Our color wheel has a radius of 100.  Loop through the rectangle
      looking for pixels within that radius. For good pixels we calculate
      the H value based on the angle from the origin.  The S value is
      set according to the distance / radius, and the V is fixed (but
      settable by a slider).
      */
        for (int i = 0; i < 200; i++)
        {
            for (int j = 0; j < 200; j++)
            {
                float dist = sqrt((i - 100) * (i - 100) + (j - 100) * (j - 100));
                if (dist < 100.0)
                {
                    display = true;
                    float s = dist / 100.0f;
                    float h = atan2(i - 100, j - 100) / (2.0f * 3.14156);
                    if (h < 0)
                    {
                        h = 1.0f + h;
                    }
                    // Since H is an angle the math is modulo.
                    if (hMax[currentColor] > hMin[currentColor])
                    {
                        if (hMin[currentColor] > h || hMax[currentColor] < h)
                        {
                            display = false;
                        }
                    } else if (hMin[currentColor] > h && hMax[currentColor] < h )
                    {
                        display = false;
                    }
                    if (s < sMin[currentColor] || s > sMax[currentColor])
                    {
                        display = false;
                    }
                    ColorSpace col;
                    col.setHsz(h, s, zSlice);
                    int y = col.getYb();
                    int v = col.getVb();
                    if (y < yMin[currentColor] || y > yMax[currentColor])
                    {
                        display = false;
                    }
                    if (v < vMin[currentColor] || v > vMax[currentColor])
                    {
                        display = false;
                    }
                    if (display)
                    {
                        c.setHsvF(h, s, zSlice);
                    } else{
                        c.setHsvF(0.0, 0.0, 1.0f);
                    }
                } else{
                    c.setHsvF(0.0, 0.0, 1.0f);
                }
                wheel->setPixel(i, j, c.rgb());
            }
        }
        QPixmap pix;
        pix.convertFromImage(*wheel);
        ui->colorWheel->setPixmap(pix);
        ui->colorWheel->repaint();
        updateThresh(false, false, true);
    }
}

/* One of our displays is typically used to display a single color
  channel of our images.  That is controlled here.  Can also display
  edge information if desired.
  */
QColor ColorCreator::getChannelView(int j, int i)
{
    bool found;
    int red, green, blue, edge;
    switch (shape) {
    case V:
        red = green = blue = roboimage.getV(j, i);
        break;
    case Bluec:
        red = green = 0;
        blue = roboimage.getBlue(j, i);
        break;
    case Y:
        red = green = blue = roboimage.getY(j, i);
        break;
    case U:
        red = green = blue = roboimage.getU(j, i);
        break;
    case Greenc:
        red = blue = 0;
        green = roboimage.getGreen(j, i);
        break;
    case Redc:
        blue = green = 0;
        red = roboimage.getRed(j, i);
        break;
    case H:
        red = green = blue = roboimage.getH(j, i);
        break;
    case S:
        red = green = blue = roboimage.getS(j, i);
        break;
    case Z:
        red = green = blue = roboimage.getZ(j, i);
        break;
    case Table:
        if (table->isEnabled())
        {
            return displayColorTable(j, i);
        }
        break;
    case EDGE:
        red = green = blue = 255; //roboimage.getY(j, i);
        found = false;
        if (j > 0 && i > 1) {
            edge = abs(roboimage.getY(j - 1, i) - roboimage.getY(j, i));
            edge = max(abs(roboimage.getY(j, i) - roboimage.getY(j, i - 1)), edge);
            if (edge  > edgediff) {
                red = 255;
                green = 0;
                blue = 0;
                found = true;
            }
            edge = abs(roboimage.getU(j - 1, i) - roboimage.getU(j, i));
            edge = max(abs(roboimage.getU(j, i) - roboimage.getU(j, i - 1)), edge);
            if (edge > edgediff) {
                green = 255;
                blue = 0;
                if (!found) {
                    red = 0;
                }
                found = true;
            }
            edge = abs(roboimage.getV(j - 1, i) - roboimage.getV(j, i));
            edge = max(abs(roboimage.getV(j, i) - roboimage.getV(j, i - 1)), edge);
            if (edge > edgediff) {
                blue = 255;
                if (!found) {
                    red = 0;
                    green = 0;
                }
            }
        }
        break;
    }
    QColor col(red, green, blue);
    return col;
}

/* One of our displays normally displays a thresholded image that is thresholded
  according to our sliders.  However, we can also display an image that is
  thresholded according to a color table.  This is mainly to check that our
  tables/tools are working as desired.
  @param i      The x value in the image
  @param j      The y value in the image
  @return       The thresholded color for that pixel
  */
QColor ColorCreator::displayColorTable(int i, int j)
{
    QColor c;

    int y = roboimage.getY(i, j);
    int u = roboimage.getU(i, j);
    int v = roboimage.getV(i, j);
    unsigned col = table->index(y, u, v);
    if ((col & ORANGE_COL) && (col & RED_COL))
    {
        c = cols[OrangeRed];
    }
    else if (col & ORANGE_COL) {
        c = cols[Orange];
    }
    else if ((col & GREEN_COL) && (col & BLUE_COL))
    {
        c = cols[BlueGreen];
    } else if ((col & GREEN_COL))
    {
        c = cols[Green];
    }
    else if ((col & WHITE_COL))
    {
        c = cols[White];
    }
    else if ((col & BLUE) && (col & NAVY))
    {
        c = cols[BlueNavy];
    }
    else if ((col & BLUE_COL))
    {
        c = cols[Blue];
    }
    else if ((col & NAVY_COL))
    {
        c = cols[Navy];
    }
    else if ((col & RED_COL))
    {
        c = cols[Pink];
    }
    else if ((col & YELLOW_COL))
    {
        c = cols[Yellow];
    }
    else {
        c = cols[Black];
    }
    return c;
}

/* Tests if the given parameters are legal for the given color.
  @param h       the H value
  @param s       the S value
  @param z       the Z (V) value
  @param y       the Y value
  @param v       the V value
  @param color   the color we're checking
  @return        true if the params are in accord with the color definition
  */
bool ColorCreator::testValue(float h, float s, float z, int y, int u, int v, int color)
{
    if (!tableMode || !haveFile)
    {
        if (hMax[color] > hMin[color])
        {
            if (hMin[color] > h || hMax[color] < h)
            {
                return false;
            }
        } else if (hMin[color] > h && hMax[color] < h )
        {
            return false;
        }
        if (s < sMin[color] || s > sMax[color])
        {
            return false;
        }
        else if (z < zMin[color] || z > zMax[color])
        {
            return false;
        }
        else if (y < yMin[color] || y > yMax[color])
        {
            return false;
        } else if (v < vMin[color] || v > vMax[color])
        {
            return false;
        }
        return true;
    } else {
        unsigned col = table->index(y, u, v);
        return col & bitColor[color];
    }
}

/* Controller for the big display.  Normally just displays the current image
  in its raw form.
  */
void ColorCreator::largeDisplay()
{
    bool display;
    QColor c;
    bool regionSet = firstPoint.x() > -1;
    for (int i = 0; i < WIDTH; i++)
    {
        for (int j = 0; j < HEIGHT; j++)
        {
            bool looping = true;
            int start = Orange;
            if (mode == Single) {
                looping = false;
                start = currentColor;
            }
            c.setRgb(255,255,255);
            do {
                display = true;
                int y = roboimage.getY(i, j);
                int u = roboimage.getU(i, j);
                int v = roboimage.getV(i, j);
                float s = (float)roboimage.getS(i, j) / 256.0f;
                float h = (float)roboimage.getH(i, j) / 256.0f;
                float z = (float)roboimage.getZ(i, j) / 256.0f;
                // Since H is an angle the math is modulo.
                display = testValue(h, s, z, y, u, v, start);
                c = cols[start];
                if (display)
                {
                    looping = false;
                    // check for some common overlaps
                    if (start == Orange)
                    {
                        if (testValue(h, s, z, y, u, v, Pink))
                        {
                            c = cols[OrangeRed];
                        }
                    } else if (start == Green)
                    {
                        if (testValue(h, s, z, y, u, v, Blue))
                        {
                            c = cols[BlueGreen];
                        }
                    }
                    else if (start == Blue)
                    {
                        if (testValue(h, s, z, y, u, v, Navy))
                        {
                            c = cols[BlueNavy];
                        }
                    }
                } else{
                    c = cols[Black];
                }
                img->setPixel(i, j, c.rgb());
                start++;
                if (start == Black)
                {
                    looping = false;
                }
            } while (looping);
        }
    }
    if (regionSet && tableMode) {
        c = cols[Black];
        for (int k = firstPoint.x(); k < lastPoint.x(); k++)
        {
            img->setPixel(k, firstPoint.y(), c.rgb());
            img->setPixel(k, firstPoint.y()+1, c.rgb());
            img->setPixel(k, lastPoint.y(), c.rgb());
            img->setPixel(k, lastPoint.y()+1, c.rgb());
        }
        for (int l = firstPoint.y(); l < lastPoint.y(); l++)
        {
            img->setPixel(firstPoint.x(), l, c.rgb());
            img->setPixel(firstPoint.x()+1, l, c.rgb());
            img->setPixel(lastPoint.x(), l, c.rgb());
            img->setPixel(lastPoint.x()+1, l, c.rgb());
        }
    }
    QPixmap pix;
    pix.convertFromImage(*img);
    ui->thresh->setPixmap(pix);
    ui->thresh->repaint();

}

/* This is called when something happens that requires updating the
  thresholded image (e.g. a color definition changed).  It makes sure
  that everything gets update accordingly.
  */
void ColorCreator::updateThresh(bool imageChanged, bool choiceChanged, bool colorsChanged)
{
    if (haveFile)
    {
        bool display;
        bool stats = false;
        QColor c, c2;
        int red, blue, green;
        initStats();
        largeDisplay();
        for (int i = 0; i < WIDTH; i+=2)
        {
            for (int j = 0; j < HEIGHT; j+=2)
            {
                bool looping = true;
                int start = Orange;
                if (mode == Single) {
                    looping = false;
                    stats = true;
                    start = currentColor;
                }
                if (imageChanged) {
                    red = roboimage.getRed(i, j);
                    green = roboimage.getGreen(i, j);
                    blue = roboimage.getBlue(i, j);
                    c.setRgb(red, green, blue);
                    img2->setPixel(i/2, j/2, c.rgb());
                }
                if (imageChanged || choiceChanged) {
                    c = getChannelView(i, j);
                    img3->setPixel(i/2, j/2, c.rgb());
                    c.setRgb(0, 0, 0);
                    c.setRgb(255,255,255);
                }
            }
        }
        if (imageChanged) {
            QPixmap pix2;
            pix2.convertFromImage(*img2);
            ui->view2->setPixmap(pix2);
            ui->view2->repaint();
        }
        if (imageChanged || choiceChanged) {
            QPixmap pix3;
            pix3.convertFromImage(*img3);
            ui->view3->setPixmap(pix3);
            ui->view3->repaint();
        }
        if (stats)
        {
            outputStats();
        }
    }
}

/* We can keep stats on the colors.  Init them for use.
  */
void ColorCreator::initStats()
{
    statsSMin = 1.0f;
    statsSMax = 0.0f;
    statsHMin = 1.0f;
    statsHMax = 0.0f;
    statsZMin = 1.0f;
    statsZMax = 0.0f;
    statsYMin = 255;
    statsYMax = 0;
    statsUMin = 255;
    statsUMax = 0;
    statsVMin = 255;
    statsVMax = 0;
}

/* Collect the stats.  THis is called in a loop.  Each pixel
  is added to the stats.
  Note: check if needed or if this is slowing things down.
  @param x    x value of pixel
  @param y    y value of pixel
  */
void ColorCreator::collectStats(int x, int y)
{
    float s = (float)roboimage.getS(x, y) / 256.0f;
    float h = (float)roboimage.getH(x, y) / 256.0f;
    float z = (float)roboimage.getZ(x, y) / 256.0f;
    int yy = roboimage.getY(x, y);
    int u = roboimage.getU(x, y);
    int v = roboimage.getV(x, y);
    statsSMin = min(statsSMin, s);
    statsSMax = max(statsSMax, s);
    statsHMin = min(statsHMin, h);
    statsHMax = max(statsHMax, h);
    statsZMin = min(statsZMin, z);
    statsZMax = max(statsZMax, z);
    statsYMin = min(statsYMin, yy);
    statsYMax = max(statsYMax, yy);
    statsUMin = min(statsUMin, u);
    statsUMax = max(statsUMax, u);
    statsVMin = min(statsVMin, v);
    statsVMax = max(statsVMax, v);
}

/* Prints out the stats.
  */
void ColorCreator::outputStats()
{
    QTextStream out(stdout);
    out << "Stats for current color\n";
    out << "H: " << statsHMin << " " << statsHMax << "\n";
    out << "S: " << statsSMin << " " << statsSMax << "\n";
    out << "Z: " << statsZMin << " " << statsZMax << "\n";
    out << "Y: " << statsYMin << " " << statsYMax << "\n";
    out << "U: " << statsUMin << " " << statsUMax << "\n";
    out << "V: " << statsVMin << " " << statsVMax << "\n";
}

/* Called when the file button is pushed.  We need a new image.
  */
void ColorCreator::on_pushButton_clicked()
{
    currentDirectory = QFileDialog::getOpenFileName(this, tr("Open Image"),
                                            currentDirectory,
                                            tr("Image Files (*.frm)"));
    roboimage.read(currentDirectory);
    int last = currentDirectory.lastIndexOf("/");
    int period = currentDirectory.lastIndexOf(".") - last - 1;
    QString temp = currentDirectory.mid(last+1, period);
    bool ok;
    currentFrameNumber = temp.toInt(&ok, 10);
    currentDirectory.chop(currentDirectory.size() - last);
    updateDisplays();
}

/* Called when the "previous" button is pushed. We need the previous
  image.
  To Do: Disable the button when no previous image available
  */
void ColorCreator::on_previousButton_clicked()
{
    roboimage.read(previousFrame);
    currentFrameNumber--;
    updateDisplays();
}

/* Called when the "next" button is pushed. We need the previous
  image.
  To Do: Disable the button when no next image available
  */
void ColorCreator::on_nextButton_clicked()
{
    roboimage.read(nextFrame);
    currentFrameNumber++;
    updateDisplays();
}

/* Called when the "H Min" slider is moved.
  @param value    the new value selected
  */
void ColorCreator::on_hMin_valueChanged(int value)
{
    hMin[currentColor] = (float)value / 100.0f;
    updateColors();
    QTextStream out(stdout);
    out << "Set H Min value to " << value << "\n";
}

/* Called when the "H Max" slider is moved.
  @param value    the new value selected
  */
void ColorCreator::on_hMax_valueChanged(int value)
{
    hMax[currentColor] = (float)value / 100.0f;
    updateColors();
    QTextStream out(stdout);
    out << "Set H Max value to " << value << "\n";
}

/* Called when the "S Min" slider is moved.
  @param value    the new value selected
  */
void ColorCreator::on_sMin_valueChanged(int value)
{
    sMin[currentColor] = (float)value / 100.0f;
    updateColors();
    QTextStream out(stdout);
    out << "Set S Min value to " << value << "\n";
}

/* Called when the "S Max" slider is moved.
  @param value    the new value selected
  */
void ColorCreator::on_sMax_valueChanged(int value)
{
    sMax[currentColor] = (float)value / 100.0f;
    updateColors();
    QTextStream out(stdout);
    out << "Set S Max value to " << value << "\n";
}

/* Called when the "Y Min" slider is moved.
  @param value    the new value selected
  */
void ColorCreator::on_yMin_valueChanged(int value)
{
    yMin[currentColor] = value;
    updateColors();
    QTextStream out(stdout);
    out << "Set Y Min value to " << value << "\n";
}

/* Called when the "Y Max" slider is moved.
  @param value    the new value selected
  */
void ColorCreator::on_yMax_valueChanged(int value)
{
    yMax[currentColor] = value;
    updateColors();
    QTextStream out(stdout);
    out << "Set Y Max value to " << value << "\n";
}

/* Called when the "Z Slice" slider is moved. This slider
  controls where you slice through the 3D space to view
  the 2D color wheel
  */
void ColorCreator::on_zSlice_valueChanged(int value)
{
    zSlice = (float)value / 100.0f;
    updateColors();
}

/* Called when the user picks a new color to work on.
  */
void ColorCreator::on_colorSelect_currentIndexChanged(int index)
{
    currentColor = index;
    ui->hMin->setValue(hMin[currentColor] * 100);
    ui->hMax->setValue(hMax[currentColor] * 100);
    ui->sMin->setValue(sMin[currentColor] * 100);
    ui->sMax->setValue(sMax[currentColor] * 100);
    ui->zMin->setValue(zMin[currentColor] * 100);
    ui->zMax->setValue(zMax[currentColor] * 100);
    ui->yMin->setValue(yMin[currentColor]);
    ui->yMax->setValue(yMax[currentColor]);
    ui->vMin->setValue(vMin[currentColor]);
    ui->vMax->setValue(vMax[currentColor]);
    ui->zSlice->setValue((zMin[currentColor] + zMax[currentColor]) * 50);
}

/* Called when the user wants to view something different.
  @param value    the new value selected
  */
void ColorCreator::on_viewChoice_currentIndexChanged(int index)
{
    mode = index;
    updateThresh(false, true, false);
}

/* Called when the "Z Min" slider is moved.
  @param value    the new value selected
  */
void ColorCreator::on_zMin_valueChanged(int value)
{
    zMin[currentColor] = (float)value / 100.0f;
    updateColors();
    QTextStream out(stdout);
    out << "Set Z Min value to " << value << "\n";
}

/* Called when the "Z Max" slider is moved.
  @param value    the new value selected
  */
void ColorCreator::on_zMax_valueChanged(int value)
{
    zMax[currentColor] = (float)value / 100.0f;
    updateColors();
    QTextStream out(stdout);
    out << "Set Z Max value to " << value << "\n";
}

/* Gets a color table.  From there you can view how the color table
  thresholds images.
  */
void ColorCreator::on_getColorTable_clicked()
{
    currentColorDirectory = QFileDialog::getOpenFileName(this, tr("Open Color Table"),
                                            currentColorDirectory,
                                            tr("Table Files (*.mtb)"));
    table->read(currentColorDirectory);
    int last = currentColorDirectory.lastIndexOf("/");
    currentColorDirectory.chop(currentColorDirectory.size() - last);
}

/* Writes out a color table.  The "new" part of the format is that it
  writes the color table using bitwise color definitions instead of the
  old integer definitions.
  @param filename        the name to write
  */
void ColorCreator::writeNewFormat(QString filename)
{
    QFile file(filename);
    QTextStream out(stdout);
    QByteArray temp;
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        out << "The file would not open properly" << "\n";
        return;
    }
    // loop through all possible table values - our tables are v-u-y
    int count = 0;
    for (int z = 0; z < 128; ++z)
    {
        for (int x = 0; x < 128; x ++)
        {
            for (int y = 0; y < 128; y++)
            {
                temp[0] = GREY_COL;
                ColorSpace col;
                col.setYuv(y * 2, x * 2, z * 2);
                for (int c = Orange; c < Black; c++)
                {
                    bool ok = false;
                    if (hMin[c] >= hMax[c])
                    {
                        if (col.getH() >= hMin[c] || col.getH() <= hMax[c])
                        {
                            ok = true;
                        }
                    } else
                    {
                        if (col.getH() >= hMin[c] && col.getH() <= hMax[c])
                        {
                            ok = true;
                        }
                    }
                    if (ok && y * 2 >= yMin[c] && y * 2 <= yMax[c] &&
                            col.getS() >= sMin[c] && col.getS() <= sMax[c] && col.getZ() >= zMin[c] &&
                            col.getZ() <= zMax[c])
                    {
                        if (c == Orange) {
                            count++;
                        }
                        temp[0] = temp[0] | bitColor[c];
                    }
                }
                file.write(temp);
            }
        }
    }
    out << "Count was " << count << "\n" << endl;
    file.close();
}

/* Writes a color table of the old format.  Old meaning integer definitions.
  So we should never use this anymore.
  @param filename        the name of the file to write
  */
void ColorCreator::writeOldFormat(QString filename)
{
    QFile file(filename);
    QTextStream out(stdout);
    QByteArray temp;
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        out << "The file would not open properly" << "\n";
        return;
    }
    // loop through all possible table values - our tables are v-u-y
    int count = 0;
    for (int z = 0; z < 128; ++z)
    {
        for (int x = 0; x < 128; x ++)
        {
            for (int y = 0; y < 128; y++)
            {
                temp[0] = GREY_COL;
                ColorSpace col;
                col.setYuv(y * 2, x * 2, z * 2);
                bool orange = false;
                bool yellow = false;
                bool blue = false;
                for (int c = Orange; c < Black; c++)
                {
                    bool ok = false;
                    if (hMin[c] > hMax[c])
                    {
                        if (col.getH() >= hMin[c] || col.getH() <= hMax[c])
                        {
                            ok = true;
                        }
                    } else
                    {
                        if (col.getH() >= hMin[c] && col.getH() <= hMax[c])
                        {
                            ok = true;
                        }
                    }
                    if (ok && y * 2 >= yMin[c] && y * 2 <= yMax[c] &&
                            col.getS() >= sMin[c] && col.getS() <= sMax[c] && col.getZ() >= zMin[c] &&
                            col.getZ() <= zMax[c])
                    {
                        switch (c)
                        {
                        case Orange:
                            temp[0] = ORANGE_COL;
                            orange = true;
                            count++;
                            break;
                        case Blue:
                            temp[0] = BLUE_COL;
                            blue = true;
                            break;
                        case Yellow:
                            if (orange)
                            {
                                temp[0] = ORANGEYELLOW_COL;
                            } else
                            {
                                temp[0] = YELLOW_COL;
                            }
                            yellow = true;
                            break;
                        case Green:
                            if (blue)
                            {
                                temp[0] = BLUEGREEN_COL;
                            } else{
                                temp[0] = GREEN_COL;
                            }
                            break;
                        case White:
                            if (yellow)
                            {
                                temp[0] = YELLOWWHITE_COL;
                            } else
                            {
                                temp[0] = WHITE;
                            }
                            break;
                        case Pink:
                            if (orange)
                            {
                                temp[0] = ORANGERED_COL;
                            } else
                            {
                                temp[0] = RED_COL;
                            }
                            break;
                        case Navy:
                            temp[0] = NAVY_COL;
                            break;
                        }
                    }
                }
                file.write(temp);
            }
        }
    }
    out << "Count was " << count << "\n";
    file.close();
}

/* The user wants to write a color table.
  */
void ColorCreator::on_writeNew_clicked()
{
    QString filename = baseColorTable + "/new.mtb";
    //writeOldFormat(filename);
    writeNewFormat(filename);
}

/* The user wants the frame with the value 10 higher than the
  current frame.
  To Do: disable button when frame does not exist.
  */
void ColorCreator::on_plusTen_clicked()
{
    roboimage.read(tenthFrame);
    currentFrameNumber += 10;
    updateDisplays();
}

/* The user wants the frame with the value 10 less than the
  current frame.
  To Do: disable button when frame does not exist
  */
void ColorCreator::on_minusTen_clicked()
{
    roboimage.read(minusTenthFrame);
    currentFrameNumber -= 10;
    updateDisplays();
}


/* User wants to display something different.
  */
void ColorCreator::on_channel_currentIndexChanged(int index)
{
    shape = index;
    updateThresh(false, true, false);
}

/* Loads and old style color table.  Note: it will be automatically
  converted to the new format (which is the main reason to do this).
  */
void ColorCreator::on_getOldTable_clicked()
{
    currentColorDirectory = QFileDialog::getOpenFileName(this, tr("Open Old Color Table"),
                                            currentColorDirectory,
                                            tr("Table Files (*.mtb)"));
    table->readOld(currentColorDirectory);
    int last = currentColorDirectory.lastIndexOf("/");
    currentColorDirectory.chop(currentColorDirectory.size() - last);
}

/* User wants a new edge thing.  This was inadvertently added.
  */
void ColorCreator::on_edgeDiff_actionTriggered(int action)
{

}

/* User changes the edge threshold.  When displaying edge
  images we use an int to determine what constitutes an edge.
  @param value      the new value to use
  */
void ColorCreator::on_edgeDiff_valueChanged(int value)
{
    QTextStream out(stdout);
    out << "Set threshold to " << value << "\n";
    edgediff = value;
    updateThresh(false, true, false);
}

/* Called when the "V Min" slider is moved.
  @param value    the new value selected
  */
void ColorCreator::on_vMin_valueChanged(int value)
{
    vMin[currentColor] = value;
    updateColors();
    QTextStream out(stdout);
    out << "Set V Min value to " << value << "\n";
}

/* Called when the "V Max" slider is moved.
  @param value    the new value selected
  */
void ColorCreator::on_vMax_valueChanged(int value)
{
    vMax[currentColor] = value;
    updateColors();
    QTextStream out(stdout);
    out << "Set V Max value to " << value << "\n";
}

void ColorCreator::on_radioButton_clicked()
{
    tableMode = !tableMode;
}

void ColorCreator::on_ColorChange_clicked()
{
    defineMode = !defineMode;
}

void ColorCreator::on_cornerDefine_clicked()
{
    cornerStatus = !cornerStatus;
}

void ColorCreator::on_changeColor_clicked()
{
    if (tableMode && firstPoint.x() > -1)
    {
        int y, u, v, yHigh, uHigh, vHigh, yLow, uLow, vLow;
        if (defineMode)
        {
            // collect up all of the pixels in the region that are not the right color
            y = u = v = 255;
            yLow = uLow = vLow = 255;
            yHigh = uHigh = vHigh = 0;
            for (int k = firstPoint.x(); k < lastPoint.x(); k++)
            {
                for (int l = firstPoint.y(); l < lastPoint.y(); l++)
                {
                    y = roboimage.getY(k, l);
                    u = roboimage.getU(k, l);
                    v = roboimage.getV(k, l);
                    if (y < yLow)
                    {
                        yLow = y;
                    }
                    if (u < uLow)
                    {
                        uLow = u;
                    }
                    if (v < vLow)
                    {
                        vLow = v;
                    }
                    if (y > yHigh)
                    {
                        yHigh = y;
                    }
                    if (u > uHigh)
                    {
                        uHigh = u;
                    }
                    if (v > vHigh)
                    {
                        vHigh = v;
                    }
                }
            }
            QTextStream out(stdout);
            out << "Set Y Stuff value to " << yLow << " " << yHigh << "\n";
            // we have defined a region in the space to make that color
            for (y = yLow; y <= yHigh; y++)
            {
                for (u = uLow; u <= uHigh; u++)
                {
                    for (v = vLow; v <= vHigh; v++)
                    {
                        table->setColor(y, u, v, bitColor[currentColor]);
                    }
                }
            }
        }
        else
        {
            for (int k = firstPoint.x(); k < lastPoint.x(); k++)
            {
                for (int l = firstPoint.y(); l < lastPoint.y(); l++)
                {
                    y = roboimage.getY(k, l);
                    u = roboimage.getU(k, l);
                    v = roboimage.getV(k, l);
                    table->unSetColor(y, u, v, bitColor[currentColor]);
                }
            }
        }
    }
    firstPoint.setX(-1);
    largeDisplay();
}
