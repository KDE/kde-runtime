#include <QtGui/QImage>
#include <QtCore/QString>

#include <ksvgrenderer.h>
#include <QtGui/QPainter>
#include <iostream>

using std::cout;
using std::endl;

int main(int argc, char **argv)
{
	if(argc < 5)
	{
		cout << "Usage : ksvgtopng width height svgfilename outputfilename" << endl;
		cout << "Please use full path name for svgfilename" << endl;
		return -1;
	}

	int width = atoi(argv[1]);
	int height = atoi(argv[2]);

	QImage img(width, height, QImage::Format_ARGB32_Premultiplied);
	img.fill(0);

	KSvgRenderer renderer(QString::fromLocal8Bit(argv[3]));
	if(renderer.isValid())
	{
	        QPainter p(&img);
		renderer.render(&p);
/*
		// Apply icon sharpening
		double factor = 0;

		if(width == 16)
			factor = 30;
		else if(width == 32)
			factor = 20;
		else if(width == 48)
			factor = 10;
		else if(width == 64)
			factor = 5;

		*img = KImageEffect::sharpen(*img, factor); // use QImageBlitz::sharpen()
*/
	}

	img.save(argv[4], "PNG");

	return 0;
}
