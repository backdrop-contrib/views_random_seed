Views Random Seed
=================

Views Random Seed provides a new sort handler to views that allows rows to be
sorted in a pseudo-random order. The random order is kept consistent across
requests for a configurable period of time. This makes it so that random ordered
views can be used with pagers, or multiple displays of the same view can share
the same random order.


Requirements
------------
No dependencies beyond Backdrop core.

Installation
------------
- Install this module using the official Backdrop CMS instructions at
  https://docs.backdropcms.org/documentation/extend-with-modules.

- When configuring a view, add a new "Sort criteria". Select the new
  "Global: Random seed" handler.

Issues
------
Bugs and Feature Requests should be reported in the Issue Queue:
https://github.com/backdrop-contrib/views_random_seed/issues.


Current Maintainers
-------------------

- [Nate Lampton](https://github.com/quicksketch)


Credits
-------
- Ported to Backdrop CMS by [Nate Lampton](https://github.com/quicksketch)
- Originally written for Drupal by [Kristof De Jaeger](https://www.drupal.org/u/swentel).
- Port to Backdrop sponsered by [Exceljet](https://exceljet.net).

License
-------

This project is GPL v2 software.
See the LICENSE.txt file in this directory for complete text.
