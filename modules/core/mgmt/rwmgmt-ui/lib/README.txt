Everything in the directory should be considered read-only and managed by bower.  If you must make a change, create a git submodule pointing to your fork on github or elsewhere, or copy the source into ./ridgets/js and use from there.

Items of note:
* v0.1.5 of circliful would flicker if you changed the value.  HEAD as of 8/7/14 was flakey.  Tim M. found a version that worked but it's unclear where he got it.  We could resolve this, but we're likely to switch to kendo ui's donuts at some point.
