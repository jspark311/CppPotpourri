# GfxUI

## NOTE:

Do not include this file directly. It was created in an effort to restrict
clutter and keep a firm grasp on inclusion order. All of these capabilities
should be attainable in top-level code with `#include "Image/GfxUI.h"`.


## Scope and purpose

The GfxUI classes are built on top of the Image and graphics classes, and
implement bi-directional UI flows between the user and firmware. These classes
only make sense if the mode of user input is a 2-axis surface, such as a
mouse or touchscreen).

Touch and render coordinates are assumed to be isometric. That is, to have the
same origin and orientation as does the `Image` class that they are based upon.
See the [Image class documentation](../../extras/doc/Image.md) for details.

Arrangements where this is not true must do transform work prior to providing
input events.


# Usage guide

## The GfxUI API

There are three base classes that define a GUI object:

   * `GfxUILayout`:  A parameter class with helper methods. Defines such planar UI tropes as size, position, margin, border.
   * `GfxUIStyle`:   Another parameter class that defines colors, text-sizes and typefaces, and so on.
   * `GfxUIElement`: Extends `GfxUILayout`, composes `GfxUIStyle`, adds a DAG implementation, and provides the API and base handling common to all objects in the GUI.

### Rules and Principles

#### Isomorphism of DAG handling

  * Any `GfxUIElement` can have children. These children are assumed to fall into the area defined by their parent.
  * Any `GfxUIElement` may be a child of any other, provided it does not lead to violation of the DAG (must not be cyclic).
  * Although parent objects can exchange children, no `GfxUIElement` may be a child of more than one parent.

## Writing a new GfxUI object
