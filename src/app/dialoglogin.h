#ifndef DIALOGLOGIN_H
#define DIALOGLOGIN_H

#include <QDialog>
#include <QMessageBox>

#include "cmgmtconnect.h"

namespace Ui {
  class DialogLogin;
}

class DialogLogin : public QDialog
{
  Q_OBJECT

public:
  explicit DialogLogin(CMgmtConnect * mgtconn,QWidget *parent = 0);
  ~DialogLogin();
  CMgmtConnect *connexion;
  int state;
public slots:
  void endLogin(int cur_state,QString message);
private slots:
  void on_pushButtonLogin_clicked();

  void on_pushButtonAbort_clicked();

  void on_pushButtonClose_clicked();

private:
  Ui::DialogLogin *ui;
signals:
  void endLoginS(int cur_state,QString message);
};

#endif // DIALOGLOGIN_H
