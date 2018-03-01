#include "mainimportwin.h"
#include "ui_mainimportwin.h"

MainImportWin::MainImportWin(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainImportWin)
{
  // Add icons and images
  QIcon icon(":/icon/res/lock.ico");
  this->setWindowIcon(icon);
  ui->setupUi(this);
  mgtConn=new CMgmtConnect(this);

  if (!QSslSocket::supportsSsl())
    {
      QMessageBox::critical(this,"No SSL","SSL not supported by system. Build ver : "+ QSslSocket::sslLibraryBuildVersionString()
                            + "\n Actual Version : "+QSslSocket::sslLibraryVersionString() );
      }
  // Link signals to end operation & logging
  QObject::connect(mgtConn,SIGNAL(endOperation(int,int,QString)),
                   this,SLOT(endOperation(int,int,QString)));
  QObject::connect(mgtConn,SIGNAL(logging(int,QString)),
                   this,SLOT(logging(int,QString)));

  operation=A_NONE;
  mode=M_HOSTS;
  action_ongoing=false;
  abort=false;

  // create buttons for object type tables
  ui->tableWidget_TMP->hide();
  table_names = QString("Hosts;Nets;Grps;Serv;Rules;Domains;Range").split(";");
  table_width = QList<int>() << 6 << 6 << 12 << 7 << 8 << 5 << 7;
  pushBtnSigMap = new QSignalMapper(this);
  QObject::connect(pushBtnSigMap, SIGNAL(mapped(int)),
                   this, SLOT(on_table_btn_clicked(int)));

  for (int i=0;i<NUM_IMP;i++)
    {
      QTableWidget * tablewidget = new QTableWidget(0,table_width[i],this);

      tables.append(tablewidget);
      QPushButton * button = new QPushButton(table_names[i],this);
      table_buttons.append(button);
      ui->horizontalLayout_Import->insertWidget(i,button);
      ui->verticalLayout_Liste->addWidget(tablewidget);
      pushBtnSigMap->setMapping(button,i);
      QObject::connect(button,SIGNAL(clicked()),
                       pushBtnSigMap,SLOT(map()));
      if (i!=0) // hide all except host table, but activate buttons.
        {
          button->setDisabled(false);
          tablewidget->hide();
        }
      else
        {
          button->setDisabled(true);
        }
      switch(i) {
        case 0: tablewidget->setHorizontalHeaderLabels(QString("Status;Name;IP;Comment;[Tags];[Colour]").split(";"));
          break;
        case 1: tablewidget->setHorizontalHeaderLabels(QString("Status;Name;IP;Comment;[Tags];[Colour]").split(";"));
          break;
        case 2: tablewidget->setHorizontalHeaderLabels(QString("Status;Name;Comment;Tags;Colour;Member...").split(";"));
          break;
        case 3: tablewidget->setHorizontalHeaderLabels(QString("Status;Name;Protocol;Port;Comment;[Tags];[Colour]").split(";"));
          break;
        case 4: tablewidget->setHorizontalHeaderLabels(QString("Status;Name;Action;Source;Dest;Serv;Track;Comment").split(";"));
          break;
        case 5: tablewidget->setHorizontalHeaderLabels(QString("Status;Domain;Comment;[Tags];[Colour]").split(";"));
          break;
        case 6: tablewidget->setHorizontalHeaderLabels(QString("Status;Name;IP Start;IP End;Comment;[Tags];[Colour]").split(";"));
          break;
        }
    }
  liste=tables[0];

  // object search dialog
  dialogObj = new CObjectDlg(mgtConn,this);

  // Version Check
  versionCheck = new CVersionCheck(this);
  QObject::connect(versionCheck,SIGNAL(result(CVersionCheck::status,QString)),
                   this,SLOT(version(CVersionCheck::status,QString)));

  CVersionCheck::statusError statErr  = versionCheck->check_updates("0.9");
  QMessageBox::warning(this,"Alpha version software","This software is in alpha state. Thanks for testing it and check https://github.com/patrickpr/CheckPointImport to get next version when available and get rid of this anoying message");

  if (statErr != CVersionCheck::none)
    {
    qDebug() << "Error in version check.";
    //QMessageBox::warning(this,"Error version Check","Error version check: \n"+versionCheck->getError());
    }
  else
    {
      qDebug() << "version Check init : no error";
    }

}

MainImportWin::~MainImportWin()
{
  delete ui;
}

void MainImportWin::on_pushButton_Login_clicked()
{
  loginDlg=new DialogLogin(mgtConn,this);

  QObject::connect(loginDlg,SIGNAL(endLoginS(int,QString)),
                   this,SLOT(endLogin(int,QString)));
  loginDlg->show();
}

void MainImportWin::endLogin(int state,QString message)
{
  LastMessage=message;
  if (state == CONNECTED) {
      ui->label_Status->setText("Connected");
      ui->pushButton_Login->setDisabled(true);
      ui->pushButton_Disconnect->setDisabled(false);
      qDebug() << "Connected";
    }
  else
    {
      ui->label_Status->setText("Disconnected");
      ui->pushButton_Login->setDisabled(false);
      ui->pushButton_Disconnect->setDisabled(true);
      qDebug() << "Not connected : " << state << "/" <<message;
    }
  loginDlg->deleteLater();
  get_Layers(-1,"");
}

void MainImportWin::on_pushButton_Disconnect_clicked()
{
  if (action_ongoing) return;
  if (mgtConn->getState() != CONNECTED)
    {
      QMessageBox::warning(this,"No Connection","No active connection");
      return;
    }
  operation=A_LOGOUT,
  ui->pushButton_Disconnect->setDisabled(true);
  mgtConn->doLogout();
  action_ongoing=true;

}

void MainImportWin::endOperation(int cur_state,int oper_state,QString message)
{
  qDebug() << "Main End Operation : " << cur_state << "/" << oper_state<< "/" <<message;
  if (cur_state!=CONNECTED && operation != A_LOGOUT)
    {
      QMessageBox::warning(this,"Error disconnect",message);
      ui->pushButton_Login->setDisabled(false);
      ui->pushButton_Disconnect->setDisabled(true);
    }
  switch (operation)
    {
    case A_LOGOUT:
      action_ongoing=false;
      ui->pushButton_Login->setDisabled(false);
      if (oper_state==O_OK)
        {
          ui->label_Status->setText("Disconnected");
        }
      else
        {
          QMessageBox::warning(this,"Error disconnect",message);
          ui->label_Status->setText("ERROR");
        }
      operation=A_NONE;
      break;
    case A_ADDHOST:
      push_Host(oper_state,message);
      break;
    case A_ADDNET:
      push_Net(oper_state,message);
      break;
    case  A_ADDGRP:
      push_Group(oper_state,message);
      break;
    case  A_ADDSERV:
      push_Serv(oper_state,message);
      break;
    case  A_ADDRULE:
      push_Rule(oper_state,message);
      break;
    case  A_ADDDNS:
      push_Domain(oper_state,message);
      break;
    case A_PUBLISH:
      if (oper_state!=O_OK)
        {
          QMessageBox::warning(this,"Error publish",message);
        }
      else
        {
          QMessageBox::information(this,"Published","Change OK, ID : "+message);
        }
      ui->pushButton_Publish->setDisabled(false);
      break;
    case A_LISTLAY:
      get_Layers(oper_state,message);
      break;
    case  A_ADDRANGE:
      push_Range(oper_state,message);
      break;
    default:
      qDebug() << "Error end operation but no operation";
      break;
    }

}

void MainImportWin::on_actionPaste_triggered()
{
  if (action_ongoing) return;
  QClipboard *clipboard = QGuiApplication::clipboard();
  //QMessageBox::information(this,"Paste",clipboard->text());
  qDebug()<<clipboard->text();
  QList<QString> Paste = clipboard->text().split(QRegExp("\\n|\\r"));

  if (liste->rowCount() < Paste.size())
    {
      liste->setRowCount(Paste.size()+1);
    }
  for (int i=0;i<Paste.size();i++)
    {
      QList<QString> line = Paste[i].split("\t");
      if ((mode==M_HOSTS && line.size()>5)
          || (mode==M_NETS && line.size()>5)
          || (mode==M_SERV && line.size()>6)
          || (mode==M_DOMAIN && line.size()>4)
          || (mode==M_RULE && line.size()>7)
          || (mode==M_RANGE && line.size()>6))
        {
          QMessageBox::warning(this,"Too many columns","Too many columns");
          return;
        }
      if (liste->columnCount()< line.size())
        {
          liste->setColumnCount(line.size());
        }
      for (int j=0;j<line.size()+1;j++)
        {
          QTableWidgetItem * item=liste->item(i,j);
          if (item == 0)
            {
              item=new QTableWidgetItem();
              liste->setItem(i,j,item);
            }
          if (j==0)
            {
              item->setText("New");
            }
          else
            {
              item->setText(line[j-1]);
            }
        }
    }
  liste->resizeColumnsToContents();
}

void MainImportWin::on_table_btn_clicked(int n)
{
  if (action_ongoing) return;
  for (int i=0;i<NUM_IMP;i++)
    {
      table_buttons[i]->setDisabled(false);
      tables[i]->hide();
    }
  table_buttons[n]->setDisabled(true);
  tables[n]->show();
  liste=tables[n];
  mode=n;
}

void MainImportWin::on_pushButton_Send_clicked()
{
  if (action_ongoing) return;
  if (mgtConn->getState() != CONNECTED)
    {
      QMessageBox::warning(this,"No Connection","No active connection");
      return;
    }
  ui->pushButton_Send->setDisabled(true);
  ui->pushButton_Abort->setDisabled(false);
  abort=false;
  num_action=-1;
  action_ongoing=true;
  switch(mode)
    {
    case M_HOSTS:
      operation=A_ADDHOST;
      push_Host(-1,"");
      break;
    case M_NETS:
      operation=A_ADDNET;
      push_Net(-1,"");
      break;
    case M_GRPS:
      operation=A_ADDGRP;
      push_Group(-1,"");
      break;
    case M_SERV:
      operation=A_ADDSERV;
      push_Serv(-1,"");
      break;
    case M_RULE:
      operation=A_ADDRULE;
      push_Rule(-1,"");
      break;
    case M_DOMAIN:
      operation=A_ADDDNS;
      push_Domain(-1,"");
      break;
    case M_RANGE:
      operation=A_ADDRANGE;
      push_Range(-1,"");
      break;
    default:
      QMessageBox::warning(this,"Warning","Not implemented");
      ui->pushButton_Send->setDisabled(false);
      ui->pushButton_Abort->setDisabled(true);
      action_ongoing=false;
    }
}

int  MainImportWin::get_next_newline(int *lineNum)
{
  int newline=(*lineNum)+1;
  while (liste->rowCount() > newline)
    {
      if (liste->item(newline,0) ==0)
        {
          qDebug()<<"no item at row "<<newline;
          return 0;
        }
      if (liste->item(newline,0)->text() == "New")
        {
          *lineNum=newline;
          return 1;
        }
      newline++;
    }
  qDebug() << "end parsing at " << newline;
  return 0;
}

void MainImportWin::init_progress_bar()
{
  int line=0,n=0;
  while (get_next_newline(&line) == 1)
    n++;
  ui->progressBar->setMinimum(0);
  ui->progressBar->setMaximum(n);
  ui->progressBar->setValue(0);
  qDebug() << "progress bar calc : " << n;
}

bool MainImportWin::check_val_liste(int line,int count)
{
  while (count > 0)
  {
    if (liste->item(line,count) == 0)
      return false;
    count--;
  }
  return true;
}

int MainImportWin::replace_object(QString regexp,QRegExp::PatternSyntax patern_syntax,QString replace,QList<int> columns)
{
  QRegExp reg(regexp,Qt::CaseSensitive,patern_syntax);
  QString curtext;
  int num_replace=0;
  for (int i=0;i< liste->rowCount();i++)
    {
      for (int j=0;j<columns.size();j++)
        {
          if (liste->item(i,columns.at(j)) != 0) {
              curtext = liste->item(i,columns.at(j))->text();
              if (reg.indexIn(curtext) >=0)
                {
                  num_replace++;
                  curtext.replace(reg,replace);
                  liste->item(i,columns.at(j))->setText(curtext);
                }
            }
        }
    }
  return num_replace;
}

void MainImportWin::push_Host(int res,QString message)
{
  qDebug() << "push host state :" << res << " / mess : " << message
           << " / state " << mgtConn->getState() << " / line  "<<num_action;
  switch(res) {
    case -1: // first call
      init_progress_bar();
      break;
    case O_OK:
      if (liste->item(num_action,0) == 0)
        {
          qDebug() << "Error in row : " << num_action;
        }
      else
        {
          liste->item(num_action,0)->setText("OK");
          liste->item(num_action,0)->setTextColor(QColor::fromRgb(0,255,0));
        }
      break;
    case O_ERR:
      liste->item(num_action,0)->setText("ERR "+message);
      liste->item(num_action,0)->setTextColor(QColor::fromRgb(255,0,0));
      break;
    default:
      qDebug() << "Err push_host res val";
    }

  if (!abort && mgtConn->getState() == CONNECTED && get_next_newline(&num_action) == 1)
    {
      qDebug() << "do push line : "<<num_action;
      QList<QString> tags;
      QString color;
      if (liste->item(num_action,4) != 0 && !liste->item(num_action,4)->text().isEmpty())
        {
          tags= liste->item(num_action,4)->text().split(";");
        }
      if (liste->item(num_action,5) != 0 && !liste->item(num_action,5)->text().isEmpty())
        {
          color=liste->item(num_action,5)->text();
        }
      if (!check_val_liste(num_action,3))
        {
          push_Host(O_ERR,"Not enough data in line");
          return;
        }
      mgtConn->doPushHost(liste->item(num_action,1)->text(),
                          liste->item(num_action,2)->text(),
                          liste->item(num_action,3)->text(),
                          tags,color);
      ui->progressBar->setValue(ui->progressBar->value()+1);
    }
  else
    {
      qDebug() << "end host push";
      ui->pushButton_Send->setDisabled(false);
      ui->pushButton_Abort->setDisabled(true);
      action_ongoing=false;
    }
}

void MainImportWin::push_Net(int res,QString message)
{
  qDebug() << "push net " << res << " / mess : " << message
           << " / state " << mgtConn->getState() << " /action "<<num_action;
  switch(res) {
    case -1: // first call
      init_progress_bar();
      break;
    case O_OK:
      if (liste->item(num_action,0) == 0)
        {
          qDebug() << "Error in row : " << num_action;
        }
      else
        {
          liste->item(num_action,0)->setText("OK");
          liste->item(num_action,0)->setTextColor(QColor::fromRgb(0,255,0));
        }
      break;
    case O_ERR:
      liste->item(num_action,0)->setText("ERR "+message);
      liste->item(num_action,0)->setTextColor(QColor::fromRgb(255,0,0));
      break;
    default:
      qDebug() << "Err net res val";
    }

  if (mgtConn->getState() == CONNECTED && get_next_newline(&num_action) == 1)
    {
      qDebug() << "do net push line : "<<num_action;
      if (!check_val_liste(num_action,3))
        {
          push_Net(O_ERR,"Not enough data in line");
          return;
        }
      QList<QString> ipMask=liste->item(num_action,2)->text().split("/");
      if (ipMask.size() != 2 && ipMask[1].toInt()>0 && ipMask[1].toInt()<33) {
          push_Net(O_ERR,"Invalid IP/Mask");
        }
      else
        {
          QList<QString> tags;
          QString color;
          if (liste->item(num_action,4) != 0 && !liste->item(num_action,4)->text().isEmpty())
            {
              tags= liste->item(num_action,4)->text().split(";");
            }
          if (liste->item(num_action,5) != 0 && !liste->item(num_action,5)->text().isEmpty())
            {
              color=liste->item(num_action,5)->text();
            }
          mgtConn->doPushNetwork(liste->item(num_action,1)->text(),
                                 ipMask[0],ipMask[1].toInt(),
              liste->item(num_action,3)->text(),
              tags,color);
          ui->progressBar->setValue(ui->progressBar->value()+1);
        }
    }
  else
    {
      qDebug() << "end net push";
      ui->pushButton_Send->setDisabled(false);
      ui->pushButton_Abort->setDisabled(true);
      action_ongoing=false;
    }
}

void MainImportWin::push_Group(int res,QString message)
{
  qDebug() << "push grp " << res << " / mess : " << message
           << " / state " << mgtConn->getState() << " /action "<<num_action;
  switch(res) {
    case -1: // first call
      init_progress_bar();
      break;
    case O_OK:
      if (liste->item(num_action,0) == 0)
        {
          qDebug() << "Error in row : " << num_action;
        }
      else
        {
          liste->item(num_action,0)->setText("OK");
          liste->item(num_action,0)->setTextColor(QColor::fromRgb(0,255,0));
        }
      break;
    case O_ERR:
      liste->item(num_action,0)->setText("ERR "+message);
      liste->item(num_action,0)->setTextColor(QColor::fromRgb(255,0,0));
      break;
    default:
      qDebug() << "Err grp res val";
    }

  if (mgtConn->getState() == CONNECTED && get_next_newline(&num_action) == 1)
    {
      qDebug() << "do grp push line : "<<num_action;
      if (!check_val_liste(num_action,2))
        {
          push_Group(O_ERR,"Not enough data in line");
          return;
        }
      QList<QString> members;
      int i=5;
      while (i<(liste->columnCount())+1
             && liste->item(num_action,i) != 0)
        {
          QString memberItem=liste->item(num_action,i)->text();
          if (!memberItem.isEmpty())
            {
              members << memberItem;
            }
          i++;
        }
      QList<QString> tags;
      QString color;
      if (liste->item(num_action,3) != 0 && !liste->item(num_action,3)->text().isEmpty())
        {
          tags= liste->item(num_action,3)->text().split(";");
        }
      if (liste->item(num_action,4) != 0 && !liste->item(num_action,4)->text().isEmpty())
        {
          color=liste->item(num_action,4)->text();
        }
      mgtConn->doPushGroup(liste->item(num_action,1)->text(), // name
                           liste->item(num_action,2)->text(), // comment
                           members, // QList<QString> members
                           tags,color);//colour
      ui->progressBar->setValue(ui->progressBar->value()+1);
    }
  else
    {
      qDebug() << "end grp push";
      ui->pushButton_Send->setDisabled(false);
      ui->pushButton_Abort->setDisabled(true);
      action_ongoing=false;
    }
}

void MainImportWin::push_Serv(int res,QString message)
{
  qDebug() << "push serv " << res << " / mess : " << message
           << " / state " << mgtConn->getState() << " /action "<<num_action;
  switch(res) {
    case -1: // first call
      init_progress_bar();
      break;
    case O_OK:
      if (liste->item(num_action,0) == 0)
        {
          qDebug() << "Error in row : " << num_action;
        }
      else
        {
          liste->item(num_action,0)->setText("OK");
          liste->item(num_action,0)->setTextColor(QColor::fromRgb(0,255,0));
        }
      break;
    case O_ERR:
      liste->item(num_action,0)->setText("ERR "+message);
      liste->item(num_action,0)->setTextColor(QColor::fromRgb(255,0,0));
      break;
    default:
      qDebug() << "Err serv res val";
    }

  if (mgtConn->getState() == CONNECTED && get_next_newline(&num_action) == 1)
    {
      qDebug() << "do serv push line : "<<num_action;
      if (!check_val_liste(num_action,4))
        {
          push_Serv(O_ERR,"Not enough data in line");
          return;
        }
      QList<QString> tags;
      QString color;
      if (liste->item(num_action,5) != 0 && !liste->item(num_action,5)->text().isEmpty())
        {
          tags= liste->item(num_action,5)->text().split(";");
        }
      if (liste->item(num_action,6) != 0 && !liste->item(num_action,6)->text().isEmpty())
        {
          color=liste->item(num_action,6)->text();
        }
      mgtConn->doPushService(liste->item(num_action,1)->text(),//name
                             liste->item(num_action,2)->text(),   // protocol
                             liste->item(num_action,3)->text(),//Port
                             liste->item(num_action,4)->text(),//Comment
                             tags,color);  //colour
      ui->progressBar->setValue(ui->progressBar->value()+1);
    }
  else
    {
      qDebug() << "end serv push";
      ui->pushButton_Send->setDisabled(false);
      ui->pushButton_Abort->setDisabled(true);
      action_ongoing=false;
    }
}

void MainImportWin::push_Rule(int res,QString message)
{
  qDebug() << "push rule " << res << " / mess : " << message
           << " / state " << mgtConn->getState() << " /action "<<num_action;
  QRegExp object_err;
  QString newVal;
  int num_replace;
  switch(res) {
    case -1: // first call
      init_progress_bar();
      break;
    case O_OK:
      if (liste->item(num_action,0) == 0)
        {
          qDebug() << "Error in row : " << num_action;
        }
      else
        {
          liste->item(num_action,0)->setText("OK");
          liste->item(num_action,0)->setTextColor(QColor::fromRgb(0,255,0));
        }
      break;
    case O_ERR:
      qDebug() << "end rule push on ERROR";
      object_err.setPattern("Requested object \\[(.*)\\] not found");
      if (object_err.indexIn(message)>=0)
        {
          bool change;
          newVal=QInputDialog::getText(this,"Object not found",
                      "Object : "+ object_err.cap(1)+" not found, replace (in all rules) with :\n",
                                QLineEdit::Normal,object_err.cap(1),&change);
          if (change)
            {
              QList<int> columns;
              columns << 3 << 4 << 5;
              qDebug() << "replace :" << object_err.cap(1) << "with " << newVal << " on :" <<columns;
              num_replace=replace_object(object_err.cap(1),QRegExp::FixedString,newVal,columns);
              if (QMessageBox::question(this,"Continue",QVariant(num_replace).toString()+" items replaced. Continue ?",QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes)
                == QMessageBox::Yes)
                {
                  num_action--;
                  break;
                }
             }
        }
      liste->item(num_action,0)->setText("ERR "+message);
      liste->item(num_action,0)->setTextColor(QColor::fromRgb(255,0,0));
      ui->pushButton_Send->setDisabled(false);
      ui->pushButton_Abort->setDisabled(true);
      action_ongoing=false;
      return;
      break;
    default:
      qDebug() << "Err rule result val";
      action_ongoing=false;
    }

  if (mgtConn->getState() == CONNECTED && get_next_newline(&num_action) == 1)
    {
      qDebug() << "do rule push line : "<<num_action;
      if (!check_val_liste(num_action,7))
        {
          push_Rule(O_ERR,"Not enough data in line");
          return;
        }
      QString layer=ui->comboBox_Layers->currentData().toString();
      //int index=num_action+1;
      int index=0; // bottom
      QList<QString> src,dst,srv;
      if (liste->item(num_action,3) != 0 && !liste->item(num_action,3)->text().isEmpty())
        {
          src=liste->item(num_action,3)->text().split(";");
          // remove last element if empty (trailing ";"
          if (src.at(src.size()-1).isEmpty()) src.removeAt(src.size()-1);
        }
      if (liste->item(num_action,4) != 0 && !liste->item(num_action,4)->text().isEmpty())
        {
          dst=liste->item(num_action,4)->text().split(";");
          if (dst.at(dst.size()-1).isEmpty()) dst.removeAt(dst.size()-1);
        }
      if (liste->item(num_action,5) != 0 && !liste->item(num_action,5)->text().isEmpty())
        {
          srv=liste->item(num_action,5)->text().split(";");
          if (srv.at(srv.size()-1).isEmpty()) srv.removeAt(srv.size()-1);
        }
      mgtConn->doPushRule(layer,index, // layer and index
                          liste->item(num_action,1)->text(),//name
                          liste->item(num_action,2)->text(),// action
                          src,dst,srv,
                          liste->item(num_action,6)->text(),//Track
                          liste->item(num_action,7)->text());//Comment
      ui->progressBar->setValue(ui->progressBar->value()+1);
    }
  else
    {
      qDebug() << "end rule push";
      ui->pushButton_Send->setDisabled(false);
      ui->pushButton_Abort->setDisabled(true);
      action_ongoing=false;
    }
}

void MainImportWin::push_Domain(int res,QString message)
{
  qDebug() << "push dns " << res << " / mess : " << message
           << " / state " << mgtConn->getState() << " /action "<<num_action;
  switch(res) {
    case -1: // first call
      init_progress_bar();
      break;
    case O_OK:
      if (liste->item(num_action,0) == 0)
        {
          qDebug() << "Error in row : " << num_action;
        }
      else
        {
          liste->item(num_action,0)->setText("OK");
          liste->item(num_action,0)->setTextColor(QColor::fromRgb(0,255,0));
        }
      break;
    case O_ERR:
      liste->item(num_action,0)->setText("ERR "+message);
      liste->item(num_action,0)->setTextColor(QColor::fromRgb(255,0,0));
      break;
    default:
      qDebug() << "Err dns res val";
    }

  if (mgtConn->getState() == CONNECTED && get_next_newline(&num_action) == 1)
    {
      qDebug() << "do dns push line : "<<num_action;
      if (!check_val_liste(num_action,2))
        {
          push_Domain(O_ERR,"Not enough data in line");
          return;
        }
      QList<QString> tags;
      QString color;
      if (liste->item(num_action,3) != 0 && !liste->item(num_action,3)->text().isEmpty())
        {
          tags= liste->item(num_action,3)->text().split(";");
        }
      if (liste->item(num_action,4) != 0 && !liste->item(num_action,4)->text().isEmpty())
        {
          color=liste->item(num_action,4)->text();
        }
      mgtConn->doPushDomain(liste->item(num_action,1)->text(),//name
                            liste->item(num_action,2)->text(),//Comment
                            tags,color);  //colour
      ui->progressBar->setValue(ui->progressBar->value()+1);
    }
  else
    {
      qDebug() << "end dns push";
      ui->pushButton_Send->setDisabled(false);
      ui->pushButton_Abort->setDisabled(true);
      action_ongoing=false;
    }
}

void MainImportWin::push_Range(int res,QString message)
{
  qDebug() << "push range state :" << res << " / mess : " << message
           << " / state " << mgtConn->getState() << " / line  "<<num_action;
  switch(res) {
    case -1: // first call
      init_progress_bar();
      break;
    case O_OK:
      if (liste->item(num_action,0) == 0)
        {
          qDebug() << "Error in row : " << num_action;
        }
      else
        {
          liste->item(num_action,0)->setText("OK");
          liste->item(num_action,0)->setTextColor(QColor::fromRgb(0,255,0));
        }
      break;
    case O_ERR:
      liste->item(num_action,0)->setText("ERR "+message);
      liste->item(num_action,0)->setTextColor(QColor::fromRgb(255,0,0));
      break;
    default:
      qDebug() << "Err push_range res val";
    }

  if (!abort && mgtConn->getState() == CONNECTED && get_next_newline(&num_action) == 1)
    {
      qDebug() << "do push line : "<<num_action;
      QList<QString> tags;
      QString color;
      if (liste->item(num_action,5) != 0 && !liste->item(num_action,5)->text().isEmpty())
        {
          tags= liste->item(num_action,5)->text().split(";");
        }
      if (liste->item(num_action,6) != 0 && !liste->item(num_action,6)->text().isEmpty())
        {
          color=liste->item(num_action,6)->text();
        }
      if (!check_val_liste(num_action,4))
        {
          push_Host(O_ERR,"Not enough data in line");
          return;
        }
      mgtConn->doPushRange(liste->item(num_action,1)->text(),
                          liste->item(num_action,2)->text(),
                          liste->item(num_action,3)->text(),
                          liste->item(num_action,4)->text(),
                          tags,color);
      ui->progressBar->setValue(ui->progressBar->value()+1);
    }
  else
    {
      qDebug() << "end range push";
      ui->pushButton_Send->setDisabled(false);
      ui->pushButton_Abort->setDisabled(true);
      action_ongoing=false;
    }
}

void MainImportWin::get_Layers(int res,QString message)
{
  QList<QString> llayers;
  switch (res)
    {
    case -1: // init
      if (mgtConn->getState() != CONNECTED)
      {
        QMessageBox::warning(this,"No Connection","No active connection");
        return;
      }
      if (action_ongoing) return;
      action_ongoing=true;
      operation=A_LISTLAY;
      mgtConn->doGetLayers();
      break;
    case O_OK:
      action_ongoing=false;
      llayers = message.split(";");
      while(ui->comboBox_Layers->count() > 0)
        {
          ui->comboBox_Layers->removeItem(0);
        }
      for (int i=0;i<llayers.size();i++)
        {
          ui->comboBox_Layers->addItem(llayers.at(i),llayers.at(i));
        }
      break;
    case O_ERR:
      action_ongoing=false;
      while(ui->comboBox_Layers->count() > 0)
        ui->comboBox_Layers->removeItem(0);
       ui->comboBox_Layers->addItem("Error get layers","error");
      break;
    default:
      qDebug() << "Err Layers result val";
      action_ongoing=false;
    }

}

void MainImportWin::on_pushButton_Publish_clicked()
{
  if (mgtConn->getState() != CONNECTED)
    {
      QMessageBox::warning(this,"No Connection","No active connection");
      return;
    }
  if (action_ongoing) return;

  operation=A_PUBLISH;
  ui->pushButton_Publish->setDisabled(true);
  mgtConn->doPublish();
}

void MainImportWin::on_pushButton_Clear_OK_clicked()
{
  if (action_ongoing) return;
  for (int i=0;i<liste->rowCount();i++)
    {
      if (liste->item(i,0)== 0 || liste->item(i,0)->text() == "OK")
        {
          liste->removeRow(i);
          i--;
        }
    }
}

void MainImportWin::on_pushButton_Clear_All_clicked()
{
  if (action_ongoing) return;
  while(liste->rowCount() > 0) liste->removeRow(0);
}

void MainImportWin::on_pushButton_Abort_clicked()
{
  abort=true;
  ui->pushButton_Abort->setDisabled(true);
}

void MainImportWin::logging(int state,QString text)
{
  if (state == O_OK && ui->checkBox_Log_Error->isChecked())
    return;
  ui->plainTextEdit_Log->appendPlainText(text);
}

void MainImportWin::on_pushButton_Clear_Log_clicked()
{
    ui->plainTextEdit_Log->document()->setPlainText("");
}

void MainImportWin::on_actionCopy_triggered()
{
  QList<QTableWidgetItem *> selected = liste->selectedItems();
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (selected.isEmpty())
    {
      QMessageBox::warning(this,"Copy","Nothing selected");
      return;
    }
  QString clip;
  QTableWidgetItem* current;
  QTableWidgetItem* previous=selected.first();
  selected.removeFirst();
  clip.append(previous->text());
  foreach (current, selected) {
      if (current->row() != previous->row())
        {
          clip.append("\n");
        }
      else
        {
          clip.append("\t");
        }
      clip.append(current->text());
      previous=current;
    }
  //QMessageBox::information(this,"Paste",clipboard->text());
  clipboard->setText(clip);
  qDebug()<< "Copy : " << clipboard->text();
}

void MainImportWin::on_pushButton_Err_To_New_clicked()
{
  if (action_ongoing) return;
  for (int i=0;i<liste->rowCount();i++)
    {
      if (liste->item(i,0)!= 0 && liste->item(i,0)->text().contains("ERR"))
        {
          liste->item(i,0)->setText("New");
          liste->item(i,0)->setTextColor(QColor::fromRgb(0,0,0));

        }
    }
}

void MainImportWin::on_actionQuit_triggered()
{
    exit(0);
}

void MainImportWin::on_pushButton_Get_clicked()
{
  dialogObj->show();
}

void MainImportWin::version(CVersionCheck::status status,QString message)
{
  if (status == CVersionCheck::ok)
    {
      qDebug() << "Version OK : " << message;
    }
  else
    {
      qDebug() << "Version NOK : " << message;
    }

}
