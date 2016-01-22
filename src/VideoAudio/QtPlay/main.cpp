#include "stdafx.h"
#include "qtplay.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QtPlay w;
	w.show();
	return a.exec();
}
