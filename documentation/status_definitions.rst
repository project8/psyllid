DAQ Status
=============

The status can be queried by doing a GET of ``daq-status``.  The payload will include the status name as a string with key ``status``, and the value with key ``status-value``.

============ ===== =========================================================================
Status       Value Descriptoin
============ ===== =========================================================================
Deactivated  0     Psyllid is in the idle state and needs to be activated to run
Activating   2     DAQ spinning up
Activated    4     DAQ is ready to start a run
Running      5     A run is ongoing
Deactivating 6     DAQ is spinning down
Canceled     8     Psyllid was canceled and is probably shutting down
Do Restart   9     A non-fatal error occurred; reactivate Psyllid to run again
Done         10    Psyllid operations are complete; restart Psyllid to run again
Error        200   An error occurred; restart Psyllid to run again
============ ===== =========================================================================

