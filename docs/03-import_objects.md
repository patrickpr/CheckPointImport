Documentation
===============

Import Host/nets/…. (all except rules)
--------------------------------------

Copy the data, then paste (ctrl-v or File->Paste)

![paste](img/paste_1.png)

Then click “Send”:

![send](img/send_1.png)

In case of error or warning : the object is not created and line turns to "ERR".
The push will continue on other objects.

![send](img/send_err.png)

You can see the error in the log window : 

![send](img/error_log.png)

When all objects have been tried push the “Clear Ok” Button : all the objects with OK status will be erased.

You then only have objects in "ERR" state.

Make the changes nedded in the objects or clear the line if you don't want to import them anymore.
(note : you can also enter "OK" in state and hit "Clear Ok" button)

Then push the err->new button to turn all "ERR" into "New" status

before : 
![clear_ok_2.png](img/clear_ok_2.png)

after :
![err_to_new.png](img/err_to_new.png)


Repeat until you get fed up of the migration

Don’t forget to “publish” by hitting "publish" button
-----------------------------------------------------


Next : ![import rules](04-import_rules.md)