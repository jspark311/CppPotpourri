/*
File:   GfxUICryptoBurrito.h
Author: J. Ian Lindsay
Date:   2023.04.08

NOTE: Do not include this file directly. Only via GfxUI.h.

These classes are built on top of the GfxUI classes, and implement a toolkit
  for C3P's cryptography components.
*/

#ifndef _C3P_GFXUI_KIT_CRYPTOBURRITO_H
#define __C3P_GFXUI_KIT_CRYPTOBURRITO_H

#include "../../AbstractPlatform.h"
#include "../Image.h"
#include "../ImageUtils.h"

//#if (!defined(__C3P_GFXUI_KIT_CRYPTOBURRITO_H) & defined(__HAS_CRYPT_WRAPPER))


/*******************************************************************************
* Graphical tool for RNGs
*******************************************************************************/

/*
* A pane that shows the RNG. This will be available from the platform with or
*   without the rest of the cryptography classes.
*/
class GfxUICryptoRNG : public GfxUIElement, public PollableObj {
  public:
    GfxUICryptoRNG(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUICryptoRNG();

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);

    /* Implementation of PollableObj. */
    PollingResult poll();


  private:
    SensorFilter<uint32_t> _rng_buffer;
    GfxUISensorFilter<uint32_t> _vis_0;
    C3PScheduledPolling _schedule_rng_update;
};


#if defined(__HAS_CRYPT_WRAPPER)
#include "../../CryptoBurrito/CryptoBurrito.h"

/*******************************************************************************
* Graphical tool for symetric ciphers
*******************************************************************************/

/*******************************************************************************
* Graphical tool for asymetric ciphers
*******************************************************************************/

/*******************************************************************************
* Graphical tool for hash algorithms
*******************************************************************************/

/*******************************************************************************
* Graphical tool for keys and Identities
*******************************************************************************/

/*******************************************************************************
* Top-level graphical toolbox for CryptoBurrito
*******************************************************************************/

/* This is the top-level cryptography toolkit. */
class GfxUICryptoBurrito : public GfxUITabbedContentPane {
  public:
    GfxUICryptoBurrito(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0) :
      GfxUITabbedContentPane(lay, sty, (f | GFXUI_FLAG_ALWAYS_REDRAW)),
      _pane_rng(
        GfxUILayout(
          internalPosX(), (internalPosY() + _tab_bar.elementHeight()),
          internalWidth(), (internalHeight() - _tab_bar.elementHeight()),
          1, 0, 0, 0,   // Margins_px(t, b, l, r)
          0, 0, 0, 0               // Border_px(t, b, l, r)
        ),
        sty,
        (GFXUI_FLAG_ALWAYS_REDRAW)
      ),
      _pane_kvp_utils(
        GfxUILayout(
          internalPosX(), (internalPosY() + _tab_bar.elementHeight()),
          internalWidth(), (internalHeight() - _tab_bar.elementHeight()),
          1, 0, 0, 0,   // Margins_px(t, b, l, r)
          0, 0, 0, 0               // Border_px(t, b, l, r)
        ),
        sty,
        0
      ),
      _pane_burrito_info(0, 0, 0, 0)
    {
      // Note our subordinate objects...
      //_pane_config.add_child(&_txt1);
      addTab("Info", &_pane_burrito_info);
      addTab("KVP", &_pane_kvp_utils);
      addTab("RNG", &_pane_rng, true);
    };

    ~GfxUICryptoBurrito() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    //virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);

    int fast_forward_data();


  protected:
    GfxUICryptoRNG  _pane_rng;
    GfxUIKVPUtil    _pane_kvp_utils;
    GfxUIGroup      _pane_burrito_info;

    int8_t _filter_alignment_check();
};

#endif  // __HAS_CRYPT_WRAPPER
#endif  // __C3P_GFXUI_KIT_CRYPTOBURRITO_H
