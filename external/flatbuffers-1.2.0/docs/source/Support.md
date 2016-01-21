# Platform / Language / Feature support

FlatBuffers is actively being worked on, which means that certain platform /
language / feature combinations may not be available yet.

This page tries to track those issues, to make informed decisions easier.
In general:

  * Languages: language support beyond the ones created by the original
    FlatBuffer authors typically depends on community contributions.
  * Features: C++ was the first language supported, since our original
    target was high performance game development. It thus has the richest
    feature set, and is likely most robust. Other languages are catching up
    however.
  * Platforms: All language implementations are typically portable to most
    platforms, unless where noted otherwise.

NOTE: this table is a start, it needs to be extended.

Feature                        | C++    | Java   | C#     | Go     | Python | JS
------------------------------ | ------ | ------ | ------ | ------ | ------ | ------
Codegen for all basic features | Yes    | Yes    | Yes    | Yes    | Yes    | WIP
JSON parsing                   | Yes    | No     | No     | No     | No     | No
Simple mutation                | Yes    | WIP    | WIP    | No     | No     | No
Reflection                     | Yes    | No     | No     | No     | No     | No
Buffer verifier                | Yes    | No     | No     | No     | No     | No
Testing: basic                 | Yes    | Yes    | Yes    | Yes    | Yes    | WIP
Testing: fuzz                  | Yes    | No     | No     | Yes    | Yes    | No
Performance:                   | Superb | Great  | Great  | Great  | Ok     | ?
Platform: Windows              | VS2010 | Yes    | Yes    | ?      | ?      | ?
Platform: Linux                | GCC282 | Yes    | ?      | Yes    | Yes    | ?
Platform: OS X                 | Xcode4 | ?      | ?      | ?      | Yes    | ?
Platform: Android              | NDK10d | Yes    | ?      | ?      | ?      | ?
Platform: iOS                  | ?      | ?      | ?      | ?      | ?      | ?
Engine: Unity                  | ?      | ?      | Yes    | ?      | ?      | ?
Primary authors (github)       | wvo    | wvo    | (ev/js)| rw     | rw     | (ev)

  * ev = evolutional
  * js = jonsimantov
