#ifndef QTPLAY_H
#define QTPLAY_H

#include <QtGui/QMainWindow>
#include "ui_qtplay.h"

class QtPlay : public QMainWindow
{
	Q_OBJECT

public:
	QtPlay(QWidget *parent = 0, Qt::WFlags flags = 0);
	~QtPlay();

private:
	Ui::QtPlayClass ui;
};

#endif // QTPLAY_H
