#include "cobjectdlg.h"
#include "ui_cobjectdlg.h"

CObjectDlg::CObjectDlg(CMgmtConnect* connection, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::CObjectDlg)
{
  ui->setupUi(this);
  mgtConn=connection;
  connect(mgtConn,SIGNAL(endSearch(int,QList<CCPObject>)),
          this,SLOT(result(int,QList<CCPObject>)));
}

CObjectDlg::~CObjectDlg()
{
  delete ui;
}

void CObjectDlg::on_pushButton_Search_clicked()
{
    QString filter=ui->lineEdit_Search->text();

    mgtConn->doSearch(filter, "", 500);
}

void CObjectDlg::result(int status,QList<CCPObject> listeObjects)
{
  //empty tree
  QTreeWidgetItem *removeItem;
  while ((removeItem=ui->treeView_Object->takeTopLevelItem(0))!=0)
    {
      delete removeItem;
    }
  if (status == O_ERR)
    {
      QMessageBox::warning(this,"Error","Search error : " + listeObjects[0].name);
      return;
    }
  // Get all types returned.
  QList<QString> objType;
  for (int i=0;i<listeObjects.size();i++)
    {
      if (! objType.contains(listeObjects.at(i).typestr))
        {
          objType.append(listeObjects.at(i).typestr);
        }
    }
  QTreeWidgetItem * type;
  QTreeWidgetItem * elmt;
  for (int i=0;i<objType.size();i++)
    {
      type = new QTreeWidgetItem(QTreeWidgetItem::Type);
      type->setText(0,objType.at(i));
      ui->treeView_Object->insertTopLevelItem(i,type);
      for (int j=0;j<listeObjects.size();j++)
        {
          if (listeObjects.at(j).typestr ==objType.at(i) )
            {
              elmt=new QTreeWidgetItem(type);
              elmt->setText(0,listeObjects.at(j).name);
              if (listeObjects.at(j).oType==CCPObject::host)
                {
                  elmt->setText(1,listeObjects.at(j).hostIPv4);
                }
              if (listeObjects.at(j).oType==CCPObject::service)
                {
                  elmt->setText(1,listeObjects.at(j).servPort);
                }
              elmt->setText(2,listeObjects.at(j).uid);
            }
        }
    }
}
