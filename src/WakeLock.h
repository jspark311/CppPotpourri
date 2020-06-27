/*
* 2020.06.07
*
* A simple class for "Wake Locks", as an Android dev would understand it.
* This class was patterned after android.os.PowerManager.WakeLock:
*   https://developer.android.com/reference/kotlin/android/os/PowerManager.WakeLock
*
* At the time of writing, this API deviates in the following ways:
*   1) Obviously, no Java embroidery.
*   2) No encryption.
*   3) No discrete "Units" for specific functionality. Yet. Might still happen.
*   4) No notions of "WorkSource", but rather a reference to the faculty being
*        Locked (typically a piece of hardware). The interface class is implemented
*        by any class that wants to receive callbacks from the WakeLock object
*        which may be held by numerous other classes.
*
* NOTE/TODO: This class was not intended to be used under true multithreading.
*   There is no effort at concurrency control, apart from keeping things brief.
*   Might be OK if you confine WakeLock's entire call-stack to a single thread.
*   Just be warned....
*/

#include <inttypes.h>
#include <stdint.h>


#ifndef __MANUVR_WAKELOCK_H__
#define __MANUVR_WAKELOCK_H__


#define WL_FLAG_IDEMPOTENT_LOCK    0x01  //
#define WL_FLAG_DEFER_STATE_CHANGE 0x02  //

class WakeLock;  // Forward declaration.



/**
* This is the interface for classes that support WakeLocking.
* Implementing classes should create a WakeLock object and keep it as a local
*   reference.
*/
class WakeLockable {
  public:
    virtual const WakeLock* getWakeLock()     =0;

  protected:
    friend class WakeLock;

    /**
    * This is called by the WakeLock object to notify the owner of a change. If
    *   the owner returns true, the WakeLock can be considered "cleared". A
    *   return of false indicates that the WakeLock is being ignored by the
    *   owner.
    * TODO: Formalize this contract... The owner might need time to clean up I/O
    *   or carry out its own notifications, and we don't want to falsely report
    *   that we are in a certain state if a synchropnous call is made later on.
    */
    virtual bool _wakelock_notify(bool is_held) =0;
};


/**
*/
class WakeLock {
  public:
    WakeLock(WakeLockable* obj) : _OWNER(obj) {};  // Constructor.
    ~WakeLock() {};   // Trivial destructor.

    /**
    * Releases a WakeLock.
    *
    * @return true if the call resulted in a state change.
    */
    bool release();

    /*
    * Aquires a WakeLock, with a given timeout.
    * TODO: Properly supporting this will be tricky without making Platform
    *   more complicated.
    *
    * @return true if the call resulted in a state change.
    */
    bool acquire(uint32_t timeout_ms);
    inline bool acquire() {    return acquire(0);   };


    /*
    * Wake locks are reference counted by default. If a wake lock is reference
    *   counted, then each call to acquire() must be balanced by an equal number
    *   of calls to release(). If a wake lock is not reference counted, then one
    *   call to release() is sufficient to undo the effect of all previous calls
    *   to acquire().
    */
    void referenceCounted(bool);
    inline bool referenceCounted() {   return !(_wl_flag(WL_FLAG_IDEMPOTENT_LOCK));  };


    /* Public accessors. */
    inline bool isHeld() {        return (0 < _refs);  };


  private:
    WakeLockable* _OWNER   = nullptr;
    uint16_t  _refs        = 0;
    uint8_t   _wl_flags    = 0;

    void _process_wl_callback(bool);

    inline bool _wl_flag(uint8_t f) {   return ((_wl_flags & f) == f);   };
    inline void _wl_clear_flag(uint8_t _flag) {  _wl_flags &= ~_flag;    };
    inline void _wl_set_flag(uint8_t _flag) {    _wl_flags |= _flag;     };
    inline void _wl_set_flag(bool nu, uint8_t _flag) {
      _wl_flags = (nu) ? (_wl_flags | _flag) : (_wl_flags & ~_flag);
    };
};

#endif // __MANUVR_WAKELOCK_H__
