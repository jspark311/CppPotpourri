/*
File:   GfxUIGroup.cpp
Author: J. Ian Lindsay
Date:   2022.07.09

*/

#include "GfxUIKit.h"


/*******************************************************************************
* GfxUIRoot
*******************************************************************************/

/**
* Constructor that automatically sizes the element to take up the entire image.
*/
GfxUIRoot::GfxUIRoot(UIGfxWrapper* ui_gfx) :
  GfxUIGroup(0, 0, ui_gfx->img()->x(), ui_gfx->img()->y())
{}
