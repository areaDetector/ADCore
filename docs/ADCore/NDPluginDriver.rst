NDPluginDriver
==============

NDPluginDriver inherits from
`asynNDArrayDriver <areaDetectorDoc.html#asynNDArrayDriver>`__.
NDPluginDriver is the class from which actual plugins are directly
derived. The EPICS database NDArrayBase.template provides access to each
of the parameters defined in asynNDArrayDriver, and the
`asynNDArrayDriver <areaDetectorDoc.html#asynNDArrayDriver>`__
documentation describes that database. The NDPluginDriver class handles
most of the details of processing NDArray callbacks from the driver.
Plugins derived from this class typically need to implement the
processCallbacks method, and one or more of the write(Int32, Float64,
Octet) methods. The `NDPluginDriver class
documentation <areaDetectorDoxygenHTML/class_n_d_plugin_driver.html>`__ 
describes this class in detail.

NDPluginDriver defines parameters that all plugin drivers should
implement if possible. These parameters are defined by strings (drvInfo
strings in asyn) with an associated asyn interface, and access
(read-only or read-write). The EPICS database NDPluginBase.template
provides access to these standard plugin parameters, listed in the
following table. Note that to reduce the width of this table the
parameter index variable names have been split into 2 lines, but these
are just a single name, for example ``NDPluginDriverArrayPort``.

.. raw:: html

    
  <table class="table table-bordered">
    <tbody>
      <tr>
        <td align="center" colspan="7,">
          <b>Parameter Definitions in NDPluginDriver.h and EPICS Record Definitions in NDPluginBase.template</b>
        </td>
      </tr>
      <tr>
        <th>
          Parameter index variable
        </th>
        <th>
          asyn interface
        </th>
        <th>
          Access
        </th>
        <th>
          Description
        </th>
        <th>
          drvInfo string
        </th>
        <th>
          EPICS record name
        </th>
        <th>
          EPICS record type
        </th>
      </tr>
      <tr>
        <td align="center" colspan="7,">
          <b>Information about this plugin</b></td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          PluginType</td>
        <td>
          asynOctet</td>
        <td>
          r/o</td>
        <td>
          A string describing the plugin type.</td>
        <td>
          PLUGIN_TYPE</td>
        <td>
          $(P)$(R)PluginType_RBV</td>
        <td>
          stringin</td>
      </tr>
      <tr>
        <td align="center" colspan="7,">
          <b>asyn NDArray driver doing callbacks to this plugin</b></td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          ArrayPort</td>
        <td>
          asynOctet</td>
        <td>
          r/w</td>
        <td>
          asyn port name for NDArray driver that will make callbacks to this plugin. This
          port can be changed at run time, connecting the plugin to a different NDArray driver.
        </td>
        <td>
          NDARRAY_PORT</td>
        <td>
          $(P)$(R)NDArrayPort<br />
          (P)$(R)NDArrayPort_RBV</td>
        <td>
          stringout<br />
          stringin</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          ArrayAddr</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          asyn port address for NDArray driver that will make callbacks to this plugin. This
          address can be changed at run time, connecting the plugin to a different address
          in the NDArray driver.</td>
        <td>
          NDARRAY_ADDR</td>
        <td>
          $(P)$(R)NDArrayAddress<br />
          $(P)$(R)NDArrayAddress_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td align="center" colspan="7,">
          <b>Queue size and status</b></td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          QueueSize</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          The total queue size for callbacks when BlockingCallbacks=0. This can be changed
          at run time to increase or decrease the size of the queue and thus the buffering
          in this plugin. This changes the memory requirements of the plugin. When the queue
          size is changed the plugin temporarily stops the callbacks from the input driver
          and waits for all NDArrays currently in the queue to process.</td>
        <td>
          QUEUE_SIZE</td>
        <td>
          $(P)$(R)QueueSize<br />
          $(P)$(R)QueueSize_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          QueueFree</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          The number of free queue elements. This record goes into minor alarm when the queue
          is 75% full and major alarm when the queue is 100% full.</td>
        <td>
          QUEUE_FREE</td>
        <td>
          $(P)$(R)QueueFree</td>
        <td>
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          QueueUse</td>
        <td>
          N/A</td>
        <td>
          r/o</td>
        <td>
          The number of used queue elements.</td>
        <td>
          N/A</td>
        <td>
          $(P)$(R)QueueUse</td>
        <td>
          calc</td>
      </tr>
      <tr>
        <td align="center" colspan="7,">
          <b>Number of threads</b></td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          MaxThreads</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          The maximum number of threads that this plugin is allowed to use. This is defined
          when the plugin is created, and cannot be changed at run-time. Note that some plugins
          are not thread-safe for multiple threads running in the same plugin object, and
          these must force MaxThreads=1.</td>
        <td>
          MAX_THREADS</td>
        <td>
          $(P)$(R)MaxThreads_RBV</td>
        <td>
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          NumThreads</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          The number of threads to use for this plugin. The value must be between 1 and MaxThreads.
        </td>
        <td>
          NUM_THREADS</td>
        <td>
          $(P)$(R)NumThreads<br />
          $(P)$(R)NumThreads_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td align="center" colspan="7,">
          <b>Sorting of output NDArrays</b></td>
      </tr>
      <tr>
        <td colspan="7,">
          When using a plugin with multiple threads, or when the input plugin is NDPluginGather
          it is likely that the NDArray output will be slightly out of order, i.e. NDArray::uniqueId
          fields will not be monotonically increasing. This is because the threads are running
          asynchronously and at slightly different speeds.&nbsp; As a consequence a file plugin
          downstream of this plugin would write NDArrays to the file in the "wrong" order.
          Plugins have an option to sort the NDArrays by uniqueId to attempt to output them
          in the correct order. This sorting option is enabled by setting SortMode=Sorted,
          and works using the following algorithm:
          <ul>
            <li>An std::multiset object is created to store the NDArray output pointers as they
              are received in NDArrayDriver::doNDArrayCallbacks. This is the method that all derived
              classes must call to output NDArrays to downstream plugins. This std::multiset also
              stores the time at which each NDArray was received by the NDArrayDriver::doNDArrayCallbacks
              method. This multiset is automatically sorted by the uniqueId of each NDArray.</li>
            <li>A worker thread is created which processes at the time interval specified by SortTime.
              This thread outputs the next array (NDArray[N]) in the multiset if any of the following
              are true:
              <ul>
                <li>NDArray[N].uniqueId = NDArray[N-1].uniqueId. This allows for the case where multiple
                  upstream plugins are processing the same NDArray. This may happen, for example,
                  if NDPluginGather is being used and not all of its inputs are getting their NDArrays
                  from from NDPluginScatter.</li>
                <li>NDArray[N].uniqueId = NDArray[N-1].uniqueId + 1. This is the normal case.</li>
                <li>NDArray[N] has been in the multiset for longer than SortTime. This will be the
                  case if the next array that <i>should</i> have been output has not arrived, perhaps
                  because it has been dropped by some upstream plugin and will never arrive. Increasing
                  the SortTime will allow longer for out of order arrays to arrive, at the expense
                  of more memory because the multiset will grow larger before outputting the arrays.</li>
              </ul>
            </li>
          </ul>
          When NDArrays are added to the multiset they have their reference count increased,
          and so will still be consuming memory. The multiset is limited in size to SortSize.
          If the multiset would grow larger than this because arrays are arriving faster than
          they are being removed with the specified SortTime, then they will be dropped in
          the same manner as when NDArrays are dropped from the normal input queue. In this
          case DroppedOutputArrays will be incremented. Note that because NDArrays can be
          stored in both the normal input queue and the multiset the total memory potentially
          used by the plugin is determined by both QueueSize and SortSize.<br />
          If the plugin is receiving 500 NDArrays/s (2 ms period), and the maximum time the
          plugin threads require to execute is 20 msec, then the minimum value of SortTime
          should be 0.02 sec, and the minimum value of SortSize would be 10. It is a good
          idea to add a safety margin to these values, so perhaps SortSize=50 and SortTime=0.04
          sec.</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          SortMode</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Selects whether the plugin outputs NDArrays in the order in which they arrive (Unsorted=1)
          or sorted by UniqueId (Sorted=1).</td>
        <td>
          SORT_MODE</td>
        <td>
          $(P)$(R)SortMode<br />
          $(P)$(R)SortMode_RBV</td>
        <td>
          mbbo<br />
          mbbi</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          SortTime</td>
        <td>
          asynFloat64</td>
        <td>
          r/w</td>
        <td>
          Sets the minimum time that the plugin will wait for preceeding arrays to arrive
          before outputting array N when SortMode=Sorted.</td>
        <td>
          SORT_TIME</td>
        <td>
          $(P)$(R)SortTime<br />
          $(P)$(R)SortTime_RBV</td>
        <td>
          ao<br />
          ai</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          SortSize</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          The maximum allowed size of the std::multiset. This can be changed at run time to
          increase or decrease the size of the queue and thus the buffering in this plugin.
          This changes the memory requirements of the plugin.</td>
        <td>
          SORT_SIZE</td>
        <td>
          $(P)$(R)SortSize<br />
          $(P)$(R)SortSize_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          SortFree</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          The number of NDArrays remaining before the std::multiset will not be allowed to
          grow larger and the plugin may begin to drop output frames.</td>
        <td>
          SORT_FREE</td>
        <td>
          $(P)$(R)SortFree</td>
        <td>
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          DisorderedArrays</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          The number of NDArrays that have been output in the "wrong" order. The definition
          of the wrong order for NDArray[N] is that NDArray[N].uniqueId=NDArray[N-1].uniqueId
          or NDArray[N].uniqueId=NDArray[N-1].uniqueId+1. The reason for the equality test
          is explained above.</td>
        <td>
          DISORDERED_ARRAYS</td>
        <td>
          $(P)$(R)DisorderedArrays<br />
          $(P)$(R)DisorderedArrays_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          DroppedOutputArrays</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Counter that increments by 1 each time an NDArray callback occurs when SortMode=1
          and the std::multiset is full (SortFree=0), so the NDArray cannot be added to the
          std::multiset.</td>
        <td>
          DROPPED_OUTPUT_ARRAYS</td>
        <td>
          $(P)$(R)DroppedOutputArrays<br />
          $(P)$(R)DroppedOutputArrays_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td align="center" colspan="7,">
          <b>Callback enable, throttling, and statistics</b></td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          EnableCallbacks</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Enable (1) or disable (0) callbacks from the driver to this plugin. If callbacks
          are disabled then the plugin will normally be idle and consume no CPU resources.
          When disabling the plugin it will continue to process any NDArrays that are already
          in the queue. </td>
        <td>
          ENABLE_CALLBACKS</td>
        <td>
          $(P)$(R)EnableCallbacks<br />
          $(P)$(R)EnableCallbacks_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          BlockingCallbacks</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          0 = callbacks from the driver do not block; the NDArray data is put on a queue and
          the callback processes in one of the plugin threads.
          <br />
          1 = callbacks from the driver block; the callback processes in the driver callback
          thread.</td>
        <td>
          BLOCKING_CALLBACKS</td>
        <td>
          $(P)$(R)BlockingCallbacks<br />
          $(P)$(R)BlockingCallbacks_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          ProcessPlugin</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          NDPluginDriver maintains a pointer to the last NDArray that the plugin received.
          If the ProcessPlugin record is processed then the plugin runs again using this same
          NDArray. This can be used to change the plugin parameters and observe the effects
          on downstream plugins and image viewers without requiring the underlying detector
          to collect another NDArray. When the plugin is disabled the cached NDArray is released
          back to the NDArrayPool.</td>
        <td>
          PROCESS_PLUGIN</td>
        <td>
          $(P)$(R)ProcessPlugin</td>
        <td>
          bo</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          ExecutionTime</td>
        <td>
          asynFloat64</td>
        <td>
          r/o</td>
        <td>
          The execution time when the plugin processes. This is useful for measuring the performance
          of the plugin</td>
        <td>
          EXECUTION_TIME</td>
        <td>
          $(P)$(R)ExecutionTime_RBV</td>
        <td>
          ai</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          MinCallbackTime</td>
        <td>
          asynFloat64</td>
        <td>
          r/w</td>
        <td>
          The minimum time in seconds between calls to processCallbacks. Any callbacks occuring
          before this minimum time has elapsed will be ignored. 0 means no minimum time, i.e.
          process all callbacks.</td>
        <td>
          MIN_CALLBACK_TIME</td>
        <td>
          $(P)$(R)MinCallbackTime<br />
          $(P)$(R)MinCallbackTime_RBV</td>
        <td>
          ao<br />
          ai</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          MaxByteRate</td>
        <td>
          asynFloat64</td>
        <td>
          r/w</td>
        <td>
          The maximum data output rate in bytes/s. If the output rate would exceed this then
          the output array is dropped and DroppedOutputArrays is incremented. This can be
          useful, for example, to limit the network bandwidth from a plugin. For most plugins
          this logic is implemented in NDPluginDriver::endProcessCallbacks() when the plugin
          is finishing its operation and is doing callbacks to any downstream plugins. However,
          the NDPluginPva and NDPluginStdArrays plugins are treated differently because the
          output we generally want to throttle is not the NDArray passed to downstream plugins,
          but rather the size of the output for the pvaServer (NDPluginPva) or the size of
          the arrays passed back to device support for waveform records (NDPluginStdArrays).
          For these plugins the throttling logic is thus also implemented inside the plugin.
          If these plugins are throttled then they really do no useful work, and so ArrayCounter
          is not incremented. This makes the ArrayRate reflect the rate at which the plugin
          is actually doing useful work. For NDPluginStdArrays this is also important because
          clients (e.g. ImageJ) may monitor the ArrayCounter_RBV field to decide when to read
          the array and update the display.</td>
        <td>
          MAX_BYTE_RATE</td>
        <td>
          $(P)$(R)MaxByteRate<br />
          $(P)$(R)MaxByteRate_RBV</td>
        <td>
          ao<br />
          ai</td>
      </tr>
      <tr>
        <td>
          NDPluginDriver<br />
          DroppedArrays</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Counter that increments by 1 each time an NDArray callback occurs when NDPluginDriverBlockingCallbacks=0
          and the plugin driver queue is full, so the callback cannot be processed.</td>
        <td>
          DROPPED_ARRAYS</td>
        <td>
          $(P)$(R)DroppedArrays<br />
          $(P)$(R)DroppedArrays_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td align="center" colspan="7,">
          <b>Debugging control</b></td>
      </tr>
      <tr>
        <td>
          N/A</td>
        <td>
          N/A</td>
        <td>
          N/A</td>
        <td>
          N/A</td>
        <td>
          $(P)$(R)AsynIO</td>
        <td>
          asyn</td>
      </tr>
    </tbody>
  </table>
