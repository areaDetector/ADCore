Destructible drivers
====================

Since asyn R4-45, subclasses of ``asynPortDriver`` can declare themselves as
destructible. This makes asyn call ``asynPortDriver::shudownPortDriver()`` on
IOC shutdown, after which, the driver is deleted. This means that on recent
versions of asyn, destructors are run, whereas on older versions, they are not.
Performing shutdown correctly is important so that the driver disconnects from
the device properly, releasing device resources and allowing clean reconnection.

Here are the guidelines for making a destructible driver.

When backwards compatibility is not needed
------------------------------------------

If the driver only supports asyn R4-45 or newer, the recipe is as follows:

#. Pass the ``ASYN_DESTRUCTIBLE`` flag to the base constructor (i.e.,
   ``NDPluginDriver`` or ``ADDriver``).

#. Override ``shutdownPortDriver()`` and put there code that needs to be
   executed with the driver intact. This is a good place to stop threads, for
   example. ``shutdownPortDriver()`` is a virtual function, so don't forget to
   call the base implementation.

#. Implement the destructor. Do the cleanup as best you can. Note that
   ``shutdownPortDriver()`` will only be called when the IOC shuts down, so, if
   the driver could be used outside an IOC (e.g. in unit tests), you should call
   ``shutdownPortDriver()`` from the destructor. To determine if it has already
   been run, call ``shutdownNeeded()`` which will return ``false`` if the
   shutdown has already happened.

When you need backwards compatibility
-------------------------------------

If the driver needs to support older asyn versions, or if you are adding
destructability to a base class whose subclasses will not be upgraded at the
same time, the recipe is as follows:

#. The constructor should not add ``ASYN_DESTRUCTIBLE`` to the flags. Instead,
   it should accept flags as an argument, and ``ASYN_DESTRUCTIBLE`` should be
   put there by the iocsh command that instantiates the driver. This allows the
   driver to be subclassed when the derived class is not destructible. Use of
   ``ASYN_DESTRUCTIBLE`` needs to be gated with an ``#ifdef``.

#. Override ``shutdownPortDriver()`` and put there code that needs to run on IOC
   shutdown. ``shutdownPortDriver()`` is a virtual function, so don't forget to
   call the base implementation.

#. Implement the destructor. Note that newer versions of asyn will call it, but
   older versions **will not**. So, use it to release memory and such, but
   anything that needs to happen in order to disconnect from the device must go
   into ``shutdownPortDriver()``.

   Note that ``shutdownPortDriver()`` will only be called when the IOC shuts
   down, so, if the driver could be used outside an IOC (e.g. in unit tests),
   you should call ``shutdownPortDriver()`` from the destructor. To determine if
   it has already been run, you will need to set a variable in
   ``shutdownPortDriver()`` yourself because the ``shutdownNeeded()`` function is
   only available in newer asyn versions.

#. Add a wrapper function to the IOC exit hook using ``epicsAtExit()`` which
   calls ``shutdownPortDriver()`` on your driver. This must happen only if
   ``ASYN_DESTRUCTIBLE`` is not defined.
