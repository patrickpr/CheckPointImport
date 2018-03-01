#ifndef COBJECTDLG_H
#define COBJECTDLG_H

#include <QDialog>
#include <QMessageBox>
#include <QTreeWidget>
#include "cmgmtconnect.h"

namespace Ui {
  class CObjectDlg;
}

class CObjectDlg : public QDialog
{
  Q_OBJECT

public:
  explicit CObjectDlg(CMgmtConnect *connection, QWidget *parent = 0);
  ~CObjectDlg();

private:
  Ui::CObjectDlg *ui;
  CMgmtConnect * mgtConn;

public slots:
  void result(int status,QList<CCPObject> listeObjects);
private slots:
  void on_pushButton_Search_clicked();
};

#endif // COBJECTDLG_H
