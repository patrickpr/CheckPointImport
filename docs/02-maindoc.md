Documentation
===============

Main Window
---------------

![MAIN](img/main_1.png)


* 1 : login/logout
* 2 : Object type to import (independant tables : you can switch from one to another and all data will be kept)
* 3 : Send : send data to Management
* 4 : Layer selection when importing rules. You must login for the soft to fill this.
* 5 : Publish : publish the changes on management. See below for other buttons
* 6 : Abort button : when sending large amout of data.
* 7 : Logs. All logs are displayed here. Check "Error only" to only display errors.

Status : 

* New : the object will be pushed to management when “send” is clicked.
* OK : the object was sent to the management and accepted without warnings
* ERR <message> : the object was sent to management who replied with a warning (object created) or error (object not created).

------------------


Login
-----


Enter login/pass / IP information and click Login  (note : abort doesn’t work yet !)

A keepalive is sent after successful login, so you will never be disconnected

![LOGIN](img/login_1.png)

A message box appears at the end of successful login that displays information sent by management.

