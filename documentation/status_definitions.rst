DAQ Status
=============

The status can be queried by doing a GET of ``daq-status``.  The payload will include the status name as a string with key ``status``, and the value with key ``status-value``.

============ ===== ==============================
Status       Value Comment
============ ===== ==============================
Deactivated  0     blah
Activating   2     blah
Activated    4     blah
Running      5     blah
Deactivating 6     blah
Canceled     8     blah
Do_Restart   9     blah
Done         10    blah
Error        200   blah

