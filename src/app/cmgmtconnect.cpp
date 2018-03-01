#include "cmgmtconnect.h"


CCPObject::CCPObject()
{
  oType=other;
}

void CCPObject::setType(QString typestrI)
{

  typestr=typestrI;
  QList<QString> typesO={"host","network","group","service-tcp","service-udp","dns-domain","access-rule"};
  QList<oTypeEnum> typesI={host,net,group,service,service,domain,rule};

  for (int i=0;i<typesO.size();i++)
    {
      if (typestrI==typesO[i])
        {
          oType=typesI[i];
          return;
        }
    }
  oType=other;
  
}

CMgmtConnect::CMgmtConnect(QObject *parent) : QObject(parent)
{
  login=pass=mgmt=error="";
  state=DISCONNECTED;
  action=A_NONE;
  request = new QNetworkRequest();
  // Allow all certificates
  QSslConfiguration sslconf(QSslConfiguration::defaultConfiguration());
  sslconf.setPeerVerifyMode(QSslSocket::QueryPeer);
  request->setSslConfiguration(sslconf);

  mgr=new QNetworkAccessManager(this);
  connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinishConnect(QNetworkReply*)));

  keepaliveRequest= new QNetworkRequest();
  keepaliveMgr= new QNetworkAccessManager(this);
  keepaliveRequest->setSslConfiguration(sslconf);
  connect(keepaliveMgr,SIGNAL(finished(QNetworkReply*)),
          this,SLOT(onfinishKeepalive(QNetworkReply*)));
  //connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));
  qDebug() << "SSL built : " << QSslSocket::sslLibraryBuildVersionString();
  qDebug() << "SSL version : " << QSslSocket::sslLibraryVersionString();
  qDebug() << "support SSL : " << QSslSocket::supportsSsl() <<"\n";
  timer=new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(keepalive()));
  timer->start(CONN_TIMEOUT/2*1000);
}

CMgmtConnect::~CMgmtConnect() {

  delete mgr;
  delete request;
}

void CMgmtConnect::keepalive()
{
  if (state != CONNECTED) return;
  QUrl url("https://"+mgmt+KEEPALIVE_URL);
  keepaliveRequest->setUrl(url);
  keepaliveRequest->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  keepaliveRequest->setRawHeader(QByteArray("x-chkp-sid"),QByteArray(sid.toLatin1()));
  QJsonObject jsonData;
  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();
  qDebug() << "sending keepalive : "<<url.url() << " / " << data;
  keepaliveMgr->post(*keepaliveRequest,data);

}

void CMgmtConnect::onfinishKeepalive(QNetworkReply *rep)
{
  QByteArray bts = rep->readAll();
  QString strReply(bts);
  qDebug() << "keepalive response : " << strReply;
  //TODO : check response ?
}

void CMgmtConnect::onfinishSearch(QJsonObject jsonObj)
{
  QList<CCPObject> liste_response;
  CCPObject single;

  if (jsonObj["objects"].isNull())
    {
      single.name="Error :" +  jsonObj["message"].toString();
      liste_response << single;
      emit endSearch(O_ERR,liste_response);
    }
  else
    {
      QJsonArray jsonObjLay=jsonObj["objects"].toArray();
      for (int i=0;i< jsonObjLay.size();i++)
        {
          QJsonObject elmt=jsonObjLay.at(i).toObject();
          single.name=elmt["name"].toString();
          single.uid=elmt["uid"].toString();
          single.setType(elmt["type"].toString());
          if (single.oType == CCPObject::host)
            {
              single.hostIPv4=(elmt["ipv4-address"].toString());
            }
          if (single.oType == CCPObject::service)
            {
              single.servPort=(elmt["port"].toString());
            }
          liste_response << single;
        }
      emit endSearch(CONNECTED,liste_response);
    }

}

int CMgmtConnect::getState() { return this->state;}

void CMgmtConnect::setLogin(QString Slogin,QString Spass,QString Smgmt) {
  login=Slogin;
  pass=Spass;
  mgmt=Smgmt;
  state=DISCONNECTED;
}

void CMgmtConnect::doLogin() {
  action=A_LOGIN;
  QUrl url("https://"+mgmt+LOGIN_URL);
  qDebug() << "URL : "<< url.url();
  request->setUrl(url);

  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QJsonObject jsonData;
  jsonData["user"]=login;
  jsonData["password"]=pass;
  jsonData["session-timeout"]=300;

  //QJsonDocument * doc = new QJsonDocument(jsonData);
  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << "jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
  //mgr->post(*request,data);
  state=CONNECTING;
}

void CMgmtConnect::onfinishConnect(QNetworkReply *rep) {
  QByteArray bts = rep->readAll();
  QString strReply(bts);

  QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
  QJsonObject jsonObj = jsonResponse.object();

  qDebug() << "action : " << action << " / Output" << strReply << "\n";

  if (action == A_SEARCHOBJ)
  {
      onfinishSearch(jsonObj);
      rep->deleteLater();
      return;
  }

  if (rep->error() == QNetworkReply::NoError) {
      emit logging(O_OK,strReply);
      switch (action) {
      case A_LOGIN:
        if (jsonObj["sid"].isNull())
          {
            QString Message=jsonObj["message"].toString();
            qDebug() << "Error conn : " << Message;
            emit endConnection(ERROR,Message);
            state=ERROR;
          }
        else
          {
            state=CONNECTED;
            sid=jsonObj["sid"].toString();
            uid=jsonObj["uid"].toString();
            qDebug() << "sid : " << sid << " / uid : " << uid;
            QString Message = jsonObj["login-message"].toString() + "\n" +
                              jsonObj["disk-space-message"].toString();
            emit endConnection(CONNECTED,Message);
          }
          break;
       case A_LOGOUT:
            if (jsonObj["message"].isNull())
              {
                emit endOperation(DISCONNECTED,O_ERR,"Error disconnect response");
                state=DISCONNECTED;
              }
            else
              {
                emit endOperation(DISCONNECTED,O_OK,jsonObj["message"].toString());
                state=DISCONNECTED;
              }
            break;
       case A_ADDHOST:
       case A_ADDNET:
       case A_ADDGRP:
       case A_ADDSERV:
       case A_ADDRULE:
       case A_ADDDNS:
       case A_ADDRANGE:
            if (jsonObj["uid"].isNull())
              {
                emit endOperation(CONNECTED,O_ERR,"Error in response");
                state=CONNECTED;
              }
            else
              {
                emit endOperation(CONNECTED,O_OK,jsonObj["uid"].toString());
                state=CONNECTED;
              }
            break;
         case A_PUBLISH:
          if (jsonObj["task-id"].isNull())
            {
              emit endOperation(CONNECTED,O_ERR,"Error in response");
              state=CONNECTED;
            }
          else
            {
              emit endOperation(CONNECTED,O_OK,jsonObj["task-id"].toString());
              state=CONNECTED;
            }
          break;
        case A_LISTLAY:
          if (jsonObj["access-layers"].isNull())
            {
              emit endOperation(CONNECTED,O_ERR,"Error in response");
              state=CONNECTED;
            }
          else
            {
              QJsonArray jsonObjLay=jsonObj["access-layers"].toArray();
              QString liste;
              for (int i=0;i< jsonObjLay.size();i++)
                {
                  if (!liste.isEmpty()) liste.append(";");
                  QJsonObject elmt=jsonObjLay.at(i).toObject();
                  liste.append(elmt["name"].toString());
                }
              emit endOperation(CONNECTED,O_OK,liste);
              state=CONNECTED;
            }
          break;
       default:
          qDebug() << "reply with no action ongoing (connect-OK) :"<<action;
       }

  }
  else
  {
      qDebug() << "Error in SSL" << rep->errorString();
      qDebug() << "Output" << strReply << "\n";
      emit logging(O_ERR,strReply);
      switch (action) {
        case A_LOGIN:
        if (jsonObj["message"].isNull())
          {
            state=ERROR;
            emit endConnection(ERROR,"Connection err : "+rep->errorString());
          }
        else
          {
            state=ERROR;
            emit endConnection(ERROR,
                               jsonObj["message"].toString()
                                + " (" + jsonObj["code"].toString()+")");
          }
          break;
        case A_LOGOUT:
             if (jsonObj["message"].isNull())
               {
                 emit endOperation(ERROR,O_ERR,"Error disconnect response");
                 state=DISCONNECTED;
               }
             else
               {
                 emit endOperation(ERROR,O_ERR,jsonObj["message"].toString());
                 state=DISCONNECTED;
               }
             break;
        case A_ADDHOST:
        case A_PUBLISH:
        case A_ADDNET:
        case A_ADDGRP:
        case A_ADDSERV:
        case A_ADDRULE:
        case A_ADDDNS:
        case A_LISTLAY:
        case A_ADDRANGE:
          if (jsonObj["message"].isNull())
            {
              emit endOperation(CONNECTED,O_ERR,strReply);
            }
          else
            {
              emit endOperation(CONNECTED,O_ERR,jsonObj["message"].toString());
            }
          break;
        default:
          qDebug() << "reply with no action ongoing (connect-NOK) :"<<action;
        }
  }
  rep->deleteLater();
  return;
}

void CMgmtConnect::abortConnect() {
  if (reply != NULL && state==1) {
    reply->abort();
    qDebug() << "aborting connection";
  }
  else
    {
      qDebug() << "ERROR : aborting connection not in connect state";
    }
}

void CMgmtConnect::doLogout() {
  if (state != CONNECTED) return;
  action=A_LOGOUT;
  QUrl url("https://"+mgmt+LOGOUT_URL);
  qDebug() << "URL : "<< url.url();
  request->setUrl(url);
  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  setSid();
  QJsonObject jsonData;

  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << request->rawHeaderList();
  qDebug() << "jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
}

void CMgmtConnect::setSid()
{
    request->setRawHeader(QByteArray("x-chkp-sid"),QByteArray(sid.toLatin1()));
}

void CMgmtConnect::add_tags(QJsonObject* jsonData,QList<QString> tags)
{
  if (tags.isEmpty()) return;
  if (tags.size() == 1)
    {
      jsonData->insert("tags",tags.at(0));
      return;
    }
  QJsonArray jsonArray;
  for (int i=0;i<tags.size();i++)
    {
      jsonArray.append(tags.at(i));
    }
  jsonData->insert("tags",jsonArray);
}

void CMgmtConnect::doPushHost(QString name, QString IP, QString comment,QList<QString> tags,QString color )
{

  action=A_ADDHOST;
  QUrl url("https://"+mgmt+ADDHOST_URL);
  qDebug() << "URL : "<< url.url();
  request->setUrl(url);

  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  setSid();
  QJsonObject jsonData;
  jsonData["name"]=name;
  jsonData["ip-address"]=IP;
  jsonData["comments"]=comment;
  jsonData["color"]=color.isEmpty()?"black":color;
  add_tags(&jsonData,tags);

  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << "action " << action <<  " jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
}

void CMgmtConnect::doPushNetwork(QString name, QString IP, int mask,QString comment,QList<QString> tags,QString color )
{

  action=A_ADDNET;
  QUrl url("https://"+mgmt+ADDNET_URL);
  qDebug() << "URL : "<< url.url();
  request->setUrl(url);

  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  setSid();
  QJsonObject jsonData;
  jsonData["name"]=name;
  jsonData["subnet"]=IP;
  jsonData["mask-length"]=mask;
  jsonData["comments"]=comment;
  jsonData["color"]=color.isEmpty()?"black":color;
  add_tags(&jsonData,tags);

  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << "action " << action <<  " jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
}

void CMgmtConnect::doPushGroup(QString name,QString comment,QList<QString> members,QList<QString> tags,QString color)
{

  action=A_ADDGRP;
  QUrl url("https://"+mgmt+ADDGRPT_URL);
  qDebug() << "URL : "<< url.url();
  request->setUrl(url);

  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  setSid();
  QJsonArray jsonArray;
  for (int i=0;i<members.size();i++)
    {
      jsonArray.append(members[i]);
    }
  QJsonObject jsonData;
  jsonData["name"]=name;
  jsonData["members"]=jsonArray;
  jsonData["comments"]=comment;
  jsonData["color"]=color.isEmpty()?"black":color;
  add_tags(&jsonData,tags);

  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << "action " << action <<  " jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
}

void CMgmtConnect::doPushService(QString name,QString protocol,QString port,QString comment,QList<QString> tags,QString color)
{

  action=A_ADDNET;
  QUrl url;
  if (protocol == "tcp")
    {
      url.setUrl("https://"+mgmt+ADDSERV_TCP_URL);
    }
  else if (protocol == "udp")
    {
      url.setUrl("https://"+mgmt+ADDSERV_UDP_URL);
    }
  else
    {
       emit endOperation(CONNECTED,O_ERR,"Unkown protocol");
    }

  qDebug() << "URL : "<< url.url();
  request->setUrl(url);

  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  setSid();
  QJsonObject jsonData;
  jsonData["name"]=name;
  jsonData["port"]=port;
  jsonData["comments"]=comment;
  jsonData["color"]=color.isEmpty()?"black":color;
  add_tags(&jsonData,tags);

  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << "action " << action <<  " jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
}

void CMgmtConnect::doPushDomain(QString name, QString comment, QList<QString> tags, QString color)
{
  action=A_ADDDNS;
  QUrl url;
  url.setUrl("https://"+mgmt+ADDDOM_URL);

  qDebug() << "URL : "<< url.url();
  request->setUrl(url);

  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  setSid();
  QJsonObject jsonData;
  jsonData["name"]=name;
  jsonData["comments"]=comment;
  jsonData["color"]=color.isEmpty()?"black":color;
  jsonData["is-sub-domain"]="false";
  add_tags(&jsonData,tags);

  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << "action " << action <<  " jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
}

void CMgmtConnect::doPushRule(QString layer, int index,QString name, QString actionR, QList<QString> src,QList<QString> dst,QList<QString> srv, QString track,QString comment)
{
  action=A_ADDRULE;
  QUrl url;
  url.setUrl("https://"+mgmt+ADDRULE_URL);

  qDebug() << "URL : "<< url.url();
  request->setUrl(url);

  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  setSid();
  QJsonObject jsonData;
  jsonData["layer"]=layer;
  if (index > 0) jsonData["position"]=index;
      else jsonData["position"]="bottom";
  jsonData["action"]=actionR;
  jsonData["name"]=name;
  jsonData["comments"]=comment;
  QJsonObject tracking;
  tracking["type"]=track;
  jsonData["track"]=tracking;
  QJsonArray srcA,dstA,srvA;
  if (!src.isEmpty())
    {
      if (src.size() == 1) jsonData.insert("source",src.at(0));
      else
        {
          for (int i=0;i<src.size();i++) srcA.append(src.at(i));
          jsonData.insert("source",srcA);
        }
    }
  if (!dst.isEmpty())
    {
      if (dst.size() == 1) jsonData.insert("destination",dst.at(0));
      else
        {
          for (int i=0;i<dst.size();i++) dstA.append(dst.at(i));
          jsonData.insert("destination",dstA);
        }
    }
  if (!srv.isEmpty())
    {
      if (srv.size() == 1) jsonData.insert("service",srv.at(0));
      else
        {
          for (int i=0;i<srv.size();i++) srvA.append(srv.at(i));
          jsonData.insert("service",srvA);
        }
    }
  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << "action " << action <<  " jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
}

void CMgmtConnect::doPushRange(QString name, QString IP, QString IP2,QString comment,QList<QString> tags, QString color)
{

  action=A_ADDRANGE;
  QUrl url("https://"+mgmt+ADDRANGE_URL);
  qDebug() << "URL : "<< url.url();
  request->setUrl(url);

  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  setSid();
  QJsonObject jsonData;
  jsonData["name"]=name;
  jsonData["ip-address-first"]=IP;
  jsonData["ip-address-last"]=IP2;
  jsonData["comments"]=comment;
  jsonData["color"]=color.isEmpty()?"black":color;
  add_tags(&jsonData,tags);

  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << "action " << action <<  " jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
}

void CMgmtConnect::doSearch(QString filter, QString type_filter, int limit=50)
{

  action=A_SEARCHOBJ;
  QUrl url("https://"+mgmt+GETOBJ_URL);
  qDebug() << "URL : "<< url.url();
  request->setUrl(url);

  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  setSid();
  QJsonObject jsonData;
  jsonData["filter"]=filter;
  jsonData["type"]=type_filter.isEmpty()?"object":type_filter;
  jsonData["limit"]=limit;

  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << "action " << action <<  " jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
}


void CMgmtConnect::doPublish() {
  if (state != CONNECTED) return;
  action=A_PUBLISH;
  QUrl url("https://"+mgmt+PUBLISH_URL);
  qDebug() << "URL : "<< url.url();
  request->setUrl(url);
  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  setSid();
  QJsonObject jsonData;

  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << request->rawHeaderList();
  qDebug() << "jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
}

void CMgmtConnect::doGetLayers()
{
  if (state != CONNECTED) return;
  action=A_LISTLAY;
  QUrl url("https://"+mgmt+GETLAYERS_URL);
  qDebug() << "URL : "<< url.url();
  request->setUrl(url);
  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  setSid();
  QJsonObject jsonData;

  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  // FIXME for debug
  qDebug() << request->rawHeaderList();
  qDebug() << "jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
}
