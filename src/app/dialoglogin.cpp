#include "dialoglogin.h"
#include "ui_dialoglogin.h"

DialogLogin::DialogLogin(CMgmtConnect *mgtconn, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DialogLogin)
{
  ui->setupUi(this);
  connexion=mgtconn;
  // TODO : get pref somewhere ?
  this->ui->lineEditLogin->setText("");
  this->ui->lineEditLPass->setText("");
  this->ui->lineEditMgmt->setText("");
  state=0;
  QObject::connect(connexion,SIGNAL(endConnection(int,QString)),
                   this,SLOT(endLogin(int,QString)));
}

DialogLogin::~DialogLogin()
{
  delete ui;
}

void DialogLogin::on_pushButtonLogin_clicked()
{
  connexion->setLogin(ui->lineEditLogin->text(),
                      ui->lineEditLPass->text(),
                      ui->lineEditMgmt->text());
  ui->pushButtonLogin->setDisabled(true);
  ui->pushButtonAbort->setDisabled(false);

  ui->labelStatus->setText("Connecting....");
  state=1;
  connexion->doLogin();
}

void DialogLogin::on_pushButtonAbort_clicked()
{
  if (state !=1) return;
  ui->labelStatus->setText("Aborting....");
  ui->pushButtonAbort->setDisabled(true);
  connexion->abortConnect();
}

void DialogLogin::on_pushButtonClose_clicked()
{
  if (state ==CONNECTING)
    {
      this->on_pushButtonAbort_clicked();
      return;
    }
  emit endLoginS(state,"");
  this->close();
}

void DialogLogin::endLogin(int cur_state,QString message)
{
  qDebug() << "end Login signal";
  state=cur_state;
  ui->labelStatus->setText(message);
  if (state == CONNECTED)
    {
      if (message.isEmpty()) message="No login messages from SMS";
      QMessageBox::information(this,"Connected",message);
      emit endLoginS(state,message);
      this->close();
    }
  else
    {
      QMessageBox::warning(this,"Error",message);
      ui->pushButtonLogin->setDisabled(false);
      ui->pushButtonAbort->setDisabled(true);
    }
}
