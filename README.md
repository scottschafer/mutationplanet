mutationplanet
(c) 2013 Scott Schafer, GPL
==============

An artificial life simulation written in C++ and based on the Blackberry Gameplay engine, supporting iOS, Android, Mac and Windows.

This has currently only been built for Mac OS X and iOS, although supporting other platforms should be fairly trivial.

Instructions for OS X / iOS:
1) Create a parent directory (GamePlayWS)
2) Checkout out GamePlay (https://github.com/blackberry/GamePlay) into that directory
3) Checkout MutationPlanet into that directory

The result should look like:
- GamePlayWS
   |__GamePlay
   |  |__(many files)
   |  |__gameplay.xcworkspace
   |
   |__MutationPlanet
      |__(many files)
      |__MutationPlanet.xcodeproj
  
Open up GamePlayWS\GamePlay\gameplay.xcworkspace in XCode. Next, drag MutationPlanet.xcodeproj into the workspace, but not
into any of the sample applications. You should see MutationPlanet-macosx and MutationPlanet-ios show up in the targets on the
upper left (if not visible, ensure that 'Show Toolbar' is selected under the View menu).

The easiest way I've found to run a release version is to Profile it, which gives you the added benefit of performance information.
Select Profile, then 'Time Profiler' to see it rip.
