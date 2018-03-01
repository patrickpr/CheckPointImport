#ifndef CVERSIONCHECK_H
#define CVERSIONCHECK_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QSslConfiguration>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "src/openssl/include/openssl/conf.h"
#include "src/openssl/include/openssl/evp.h"
#include "src/openssl/include/openssl/err.h"
#include "src/openssl/include/openssl/bio.h"
#include "src/openssl/include/openssl/pem.h"

#define MAX_CIPHER_TEXT 500 // Max data sent and received
#define URL_VERSION_CHECK "https://www.proy.org/cpimport.php"

class CVersionCheck : public QObject
{
  Q_OBJECT
public:

  CVersionCheck(QObject *parent = nullptr);
  ~CVersionCheck();

  enum status : int {
    ok,need_update,must_update,action,error,other
  };
  enum statusError : int {
    none,init,cypher
  };

  statusError check_updates(QString version);
  statusError isError();
  QString getError();

private:

  QNetworkAccessManager * mgr;
  QNetworkReply *reply;

  EVP_PKEY * publicKey_send;
  EVP_PKEY * publicKey_receive;
  RSA * rsa_receive;
  QString errorText;
  statusError errorNum;
  statusError setError(statusError err,QString message);

  void get_ssl_errors();

signals:
  void result(CVersionCheck::status state,QString Message);

public slots:
  void finishConnect(QNetworkReply *rep);
};

#endif // CVERSIONCHECK_H
