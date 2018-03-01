#ifndef MAINIMPORTWIN_H
#define MAINIMPORTWIN_H

#include <QMainWindow>
#include <QMessageBox>
#include <QInputDialog>
#include <QTableWidget>
#include <QClipboard>
#include <QSignalMapper>
#include <QColor>
#include "cmgmtconnect.h"
#include "dialoglogin.h"
#include "cobjectdlg.h"
#include "cversioncheck.h"

#define M_NONE -1
#define M_HOSTS 0
#define M_NETS 1
#define M_GRPS 2
#define M_SERV 3
#define M_RULE 4
#define M_DOMAIN 5
#define M_RANGE 6

#define NUM_IMP 7

namespace Ui {
  class MainImportWin;
}

class MainImportWin : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainImportWin(QWidget *parent = 0);
  ~MainImportWin();

  CMgmtConnect *mgtConn;
  DialogLogin *loginDlg;
  CObjectDlg *dialogObj;
  QString LastMessage;

  CVersionCheck *versionCheck;

  QTableWidget *liste; // current table displayed

public slots:
  void endLogin(int state,QString message);
  void endOperation(int cur_state,int oper_state,QString message);
  void logging(int state,QString text);
  void version(CVersionCheck::status status,QString message);
private slots:

  void on_pushButton_Login_clicked();

  void on_pushButton_Disconnect_clicked();

  void on_actionPaste_triggered();

  void on_table_btn_clicked(int n);
  void on_pushButton_Send_clicked();

  void on_pushButton_Publish_clicked();

  void on_pushButton_Clear_OK_clicked();

  void on_pushButton_Clear_All_clicked();

  void on_pushButton_Abort_clicked();

  void on_pushButton_Clear_Log_clicked();

  void on_actionCopy_triggered();

  void on_pushButton_Err_To_New_clicked();

  void on_actionQuit_triggered();

  void on_pushButton_Get_clicked();

private:
  Ui::MainImportWin *ui;
  int operation;
  int mode;
  int num_action;
  QList<QTableWidget *> tables;
  QList<QString> table_names;
  QList<QPushButton *> table_buttons;
  QList<int> table_width;
  QSignalMapper *pushBtnSigMap;

  bool abort; // abort sendind host/net/....
  bool action_ongoing;
  
  int get_next_newline(int *lineNum);
  int replace_object(QString regexp, QRegExp::PatternSyntax patern_syntax, QString replace, QList<int> columns);
  void init_progress_bar();
  bool check_val_liste(int line, int count);
  void push_Host(int res, QString message);
  void push_Net(int res,QString message);
  void push_Group(int res,QString message);
  void push_Serv(int res,QString message);
  void push_Rule(int res,QString message);
  void push_Domain(int res,QString message);
  void get_Layers(int res,QString message);
  void push_Range(int res,QString message);

};

#endif // MAINIMPORTWIN_H
