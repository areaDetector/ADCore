commonPlugins.cmd
=================

The ``ADCore/iocBoot`` directory contains a file called
``EXAMPLE_commonPlugins.cmd``. This file should be copied to
``commonPlugins.cmd`` and edited for site-specific requirements.
``commonPlugins.cmd`` is loaded by all of the example driver IOC startup
scripts. It loads a set of plugins which are typically useful for
detectors IOCs. Each detector medm screen has links to related displays
for each of the common plugins. While this set of plugins is often
useful and sufficient, users are free to add or remove plugins from this
set for their own IOCs. New medm displays will typically need to be
created if that is done, to have the required links to related displays.

The following medm screen shows the status of all of the common plugins
at a glance, with links to bring up the detailed screen for each.

.. image:: commonPlugins.png
    :align: center

Optional Phoebus profile axes
-----------------------------

``EXAMPLE_commonPlugins.cmd`` loads ``NDPluginStats`` instances (e.g. ``Stats1:``)
that can compute row and column profiles when ``ComputeProfiles`` is enabled.
For Phoebus image viewers that plot those profiles against a pixel (or scaled)
axis, copy ``EXAMPLE_stats_profiles.cmd`` to ``stats_profiles.cmd`` and include
it **after** ``commonPlugins.cmd``::

    < $(ADCORE)/iocBoot/commonPlugins.cmd
    < $(ADCORE)/iocBoot/stats_profiles.cmd

That file loads ``NDStatsProfiles.template``, which provides axis waveforms and
a ``StatsProfInit_`` sequence record. It requires the synApps **calc** module
(``acalcout``). See :doc:`NDPluginStats` for details.

