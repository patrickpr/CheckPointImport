#include "cversioncheck.h"

CVersionCheck::CVersionCheck(QObject *parent) : QObject(parent)
{

  // Init connection manager
  mgr = new QNetworkAccessManager(this);
  QObject::connect(mgr,SIGNAL(finished(QNetworkReply*)),
                   this,SLOT(finishConnect(QNetworkReply*)));
  /* Load the human readable error strings for libcrypto */
  ERR_load_crypto_strings();
  /* Load all digest and cipher algorithms */
  OpenSSL_add_all_algorithms();
  /* Load config file, and other important initialisation */
  OPENSSL_config(NULL);

  // Init key
  char pkey[]="-----BEGIN PUBLIC KEY-----\n"
"MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA2NMz3mdR1ArUyJrM5Mos\n"
"GliGEWgKjnm73LOjqeeaxfBTOzOs/5C0x5XljTs8WwcicYgbDI58xMgNWJNuaBbY\n"
"HVHyRshD2F1z+D9bK6/FCjQmLte5NL4e8pa/9Bh2X/Lw6VUhMZtQYeDTzjGwnSUv\n"
"dpY4gA+V6MaVjfGYoZEczeiY0uOkuelIIhBhD1fD/pk/R4w/DdehIIFKuDK3nFvK\n"
"11fqpTdPOwsowFF+hle3jELp2IJpDHIexq4xzWa33rt1EXL/WnresXrYJVZzixCf\n"
"C0o2za0hBeqZ+U+9gsoexJCgElUC8geZ+yQ9uwjIzRS6uTKH09WrM5lcHvl8+Dyn\n"
"3CFjjravIU2WlSZ92FBf0XRwimLPzWFOV0sJ/NXaJXVoRK+aNLzc6eTw/6CXmx6h\n"
"2U00GLhL9afY8H4/e5zyUOsnnCR6K/jrspHgwAeRFrjwf94PAKb/eWjv9fS9H+RE\n"
"pptMfLtyRzjNVp6V5wP4AoBW7U/YLEFjFFnzB0eOcvVKpgbXgsvJ76KqQa/WM1XN\n"
"CfyB5qHp+q1rflgW7OFQjy+5T3ASVaqSF+SJjaZgLWq6ehEIOWaSVMkE7ZNzlpj7\n"
"D8tsYBe34KB/jx/HvMkT6rSm+sR60uNlgqybt26ZxwSSZbfR7DT7AC++Fo9h3Op2\n"
"LN4GHYJbK/EWJif9vFp84MUCAwEAAQ==\n"
"-----END PUBLIC KEY-----\n";

  //ebug() << pkey << "\n";
  BIO *mem;
  mem = BIO_new_mem_buf(pkey, -1); //pkey is of type char*

  publicKey_send = PEM_read_bio_PUBKEY(mem,NULL,NULL,0);
  if (publicKey_send == NULL)
    {
      get_ssl_errors();
      setError(init,"Error setting evp key");
      return;
    }

  qsrand(QDateTime::currentMSecsSinceEpoch()%100000); // TODO : better rand init.
  errorNum=none;
}

CVersionCheck::~CVersionCheck()
{
  // free key structure
  EVP_PKEY_free(publicKey_send);
  /* Removes all digests and ciphers */
  EVP_cleanup();

  /* if you omit the next, a small leak may be left when you make use of the BIO (low level API) for e.g. base64 transformations */
  CRYPTO_cleanup_all_ex_data();

  /* Remove error strings */
  ERR_free_strings();
}


void CVersionCheck::get_ssl_errors()
{

  unsigned long int errcode;
  char linebuf[200]="";
  while ((errcode= ERR_get_error()) != 0)
    {
      sprintf(linebuf,"Lib : %s, doing : %s, reason : %s (%i/%i/%i)\n",
              ERR_lib_error_string(errcode),
              ERR_func_error_string((errcode)),
              ERR_reason_error_string((errcode)),
              ERR_GET_LIB(errcode),
              ERR_GET_FUNC(errcode),
              ERR_GET_REASON(errcode));
      qDebug() << linebuf;
    }
}

CVersionCheck::statusError CVersionCheck::setError(statusError err,QString message)
{
  errorNum=err;
  errorText=message;
  return err;
}

CVersionCheck::statusError CVersionCheck::check_updates(QString version)
{
  if (errorNum != none)
    {
      return errorNum;
    }
  EVP_CIPHER_CTX *ctx;
  int ciphertext_len;
  char* ciphertext=(char*)malloc(sizeof(char)*((MAX_CIPHER_TEXT+1)*2));
  if (ciphertext==NULL)
    {
      return setError(init,"Memory alloc failed [ref ciphertext]");
    }

  unsigned char *plaintext=(unsigned char *)malloc(sizeof(unsigned char)*(MAX_CIPHER_TEXT+1));;
  int plaintext_len;
  if (plaintext==NULL)
    {
      return setError(init,"Memory alloc failed [ref ciphertext]");
    }

  if (version.size()>MAX_CIPHER_TEXT-1)
    {
      return setError(init,"version String too long");
    }
  strncpy((char*)plaintext,version.toLatin1().data(),version.size());
  plaintext_len=strlen((const char*)plaintext);
  // Normalize length
  for (int i=plaintext_len;i<500;i++)
    {
      //plaintext[i]=(qrand()%90)+33;
      plaintext[i]='a';
    }
  plaintext[500]=0;
  plaintext_len=500;

  int len;

  unsigned char *encrypted_key[1];
  int encrypted_key_len[1];
  encrypted_key[0]=(unsigned char *)malloc(sizeof(unsigned char *)*(EVP_PKEY_size(publicKey_send)+1));
  if (encrypted_key[0]==NULL)
    {
      return setError(init,"Memory alloc failed [ref encrypted_key]");
    }

  unsigned char iv[EVP_MAX_IV_LENGTH+1];
  for (int i=0;i<EVP_MAX_IV_LENGTH;i++)
    {
      //iv[i]=qrand();
      iv[i]=0;
    }
  iv[EVP_MAX_IV_LENGTH]=0;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new()))
    {
      return setError(init,"Error ctx init");
    }

  /* Initialise the envelope seal operation. This operation generates
   * a key for the provided cipher, and then encrypts that key a number
   * of times (one for each public key provided in the pub_key array). In
   * this example the array size is just one. This operation also
   * generates an IV and places it in iv. */
  if(1 != EVP_SealInit(ctx, EVP_aes_256_ecb(), encrypted_key,
          encrypted_key_len, iv, &publicKey_send, 1))
    {
      get_ssl_errors();
      return setError(init,"Error EVP Seal");
    }

  /* Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_SealUpdate can be called multiple times if necessary
   */
  //qDebug()<<"Plain Text (" << plaintext_len << ") "<< QString((const char*)plaintext);
  if(1 != EVP_SealUpdate(ctx, (unsigned char*)ciphertext, &len, plaintext, plaintext_len))
    {
      get_ssl_errors();
      return setError(init,"Error EVP Update");
    }

  ciphertext_len = len;

  /* Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.
   */
  if(1 != EVP_SealFinal(ctx, (unsigned char*)ciphertext + len, &len))
    {
      get_ssl_errors();
      return setError(init,"Error EVP SealFinal");
    }
  ciphertext_len += len;

  // send stuff
  QUrl url(URL_VERSION_CHECK);
  //qDebug() << "URL : "<< url.url();
  QNetworkRequest * request = new QNetworkRequest();
  request->setUrl(url);
  request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QJsonObject jsonData;
  jsonData["a"]=QByteArray((const char*)encrypted_key[0],encrypted_key_len[0]).toBase64().data();
  jsonData["b"]=QByteArray(ciphertext,ciphertext_len).toBase64().data();
  jsonData["c"]=qrand()%65535;
  jsonData["d"]=QByteArray((const char*)iv,strlen((const char*)iv)).toBase64().data();

  // Free CTX context
  EVP_CIPHER_CTX_free(ctx);
  //QJsonDocument * doc = new QJsonDocument(jsonData);
  QJsonDocument doc (jsonData);
  QByteArray data = doc.toJson();

  //qDebug() << "plainT EncT Key : " << plaintext_len <<"/"<< ciphertext_len  <<"/"<< encrypted_key_len[0];
  //qDebug() << "jsondata : " << QString::fromUtf8(data.data(), data.size())<< "\n";

  reply=mgr->post(*request,data);
  //qDebug() << "end verify query";

  return none;
}

void CVersionCheck::finishConnect(QNetworkReply *rep)
{
  QByteArray bts = rep->readAll();
  QString strReply(bts);

  QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
  QJsonObject jsonObj = jsonResponse.object();

  //qDebug() << "Output verCheck" << strReply << "\n";

  if (rep->error() == QNetworkReply::NoError)
    {
      if (jsonObj["message"].isNull())
        {
           emit result(error,"Error in returned message");
        }
      QString message_ver=jsonObj["message"].toString();
      QString status_ver=jsonObj["version"].toString();
      QString status_num=jsonObj["status"].toString();
      if (status_num=="1")
        {
          emit result(ok,message_ver);
        }
      else
        {
          emit result(need_update,message_ver);
        }
    }
  else
    {
      emit result(error,"Error connecting");
    }

  // ?? rep->deleteLater();
  return;
}

CVersionCheck::statusError CVersionCheck::isError()
{
  return errorNum;
}

QString CVersionCheck::getError()
{
  return errorText;
}
