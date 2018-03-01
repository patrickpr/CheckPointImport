#ifndef CMGMTCONNECT_H
#define CMGMTCONNECT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QSslConfiguration>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
//#include <QMessageBox>
#include <QSslSocket>
#include <QTimer>


#define DISCONNECTED 0
#define CONNECTING 1
#define CONNECTED 2
#define ERROR 3

#define O_OK 0
#define O_ERR 1

#define A_NONE    -1
#define A_LOGIN   0
#define A_LOGOUT  1
#define A_ADDHOST 2
#define A_PUBLISH 3
#define A_ADDNET  4
#define A_ADDGRP  5
#define A_ADDSERV 6
#define A_ADDRULE 7
#define A_ADDDNS 8
#define A_LISTLAY 9
#define A_ADDRANGE 10
#define A_SEARCHOBJ 11

#define LOGIN_URL "/web_api/login"
#define LOGOUT_URL "/web_api/logout"
#define ADDHOST_URL "/web_api/add-host"
#define PUBLISH_URL "/web_api/publish"
#define ADDNET_URL "/web_api/add-network"
#define ADDGRPT_URL "/web_api/add-group"
#define ADDSERV_TCP_URL "/web_api/add-service-tcp"
#define ADDSERV_UDP_URL "/web_api/add-service-udp"
#define ADDDOM_URL "/web_api/add-dns-domain"
#define ADDRULE_URL "/web_api/add-access-rule"
#define GETLAYERS_URL "/web_api/show-access-layers"
#define KEEPALIVE_URL "/web_api/keepalive"
#define ADDRANGE_URL "/web_api/add-address-range"
#define GETOBJ_URL "/web_api/show-objects"

#define CONN_TIMEOUT 300

class CCPObject
{
public:
  explicit CCPObject();
  QString name;
  QString uid;
  QString typestr;

  QString hostIPv4;
  QString servPort;
  enum oTypeEnum : int {
    host,net,group,service,domain,rule,other
  };
  enum oTypesService : int {
    tcp,udp,icmp,other_service
  };

  oTypeEnum oType;
  oTypesService serviceType;
  void setType(QString typestrI);

};

class CMgmtConnect : public QObject
{
  Q_OBJECT
public:
  explicit CMgmtConnect(QObject *parent = nullptr);
  ~CMgmtConnect();
  int getState();
  QString getuid();
  void setLogin(QString Slogin,QString Spass,QString Smgmt);
  void doLogin();
  void doLogout();
  void doPushHost(QString name, QString IP, QString comment,QList<QString> tags,QString color );
  void doPushNetwork(QString name, QString IP, int mask, QString comment, QList<QString> tags, QString color );
  void doPushGroup(QString name, QString comment, QList<QString> members, QList<QString> tags, QString color);
  void doPushService(QString name, QString protocol, QString port, QString comment, QList<QString> tags, QString color);
  void doPushDomain(QString name, QString comment, QList<QString> tags, QString color);
  void doPushRule(QString layer, int index, QString name, QString actionR, QList<QString> src, QList<QString> dst, QList<QString> srv, QString track, QString comment);
  void doPushRange(QString name, QString IP, QString IP2, QString comment, QList<QString> tags, QString color);
  void doGetLayers();
  void doSearch(QString filter, QString type_filter, int limit);
  void doPublish();
  QString error;
  void abortConnect();
private:
  QTimer *timer;
  QString login,pass,mgmt;
  int state;
  int action;
  QNetworkAccessManager * mgr;
  QNetworkAccessManager * keepaliveMgr;
  QNetworkRequest *request;
  QNetworkRequest *keepaliveRequest;
  QNetworkReply *reply;
  QString sid; // token sent after auth
  QString uid; //uid of session
  void setSid(); // set x-chkp-sid
  void add_tags(QJsonObject* jsonData,QList<QString> tags);
  void onfinishSearch(QJsonObject jsonObj);
  

signals:
  void endConnection(int cur_state,QString message);
  void endOperation(int cur_state,int oper_state,QString message);
  void logging(int state,QString text);
  void endSearch(int cur_state,QList<CCPObject> listeObjects);
public slots:
  void onfinishConnect(QNetworkReply *rep);
  void onfinishKeepalive(QNetworkReply *rep);

private slots:
  void keepalive();

};

#endif // CMGMTCONNECT_H
