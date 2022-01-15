/*
File:   Identity.h
Author: J. Ian Lindsay
Date:   2016.08.28

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This might be better-viewed as a data structure. Notions of identity should
  be encapsulated here.
*/

#include "../CppPotpourri.h"
#include "../StringBuilder.h"
#include "../LightLinkedList.h"

// TODO: This was a really good idea. But it implies too much knowledge
//   of build system. There might be a better way.
//#include <Rationalizer.h>

#ifndef __MANUVR_IDENTITY_H__
#define __MANUVR_IDENTITY_H__

class Identity;  // Forward-dec

/*
* Note on NET_ACCEPT / APP_ACCEPT:
* This distinction is needed to isolate policy actions from merely the
*   "authorization to connect". Without this distinction, we have no way
*   of isolating TLS credentials from consideration by any policy layer
*   making choices about the instructions themselves.
* However, failure to draw a distinction is a valid route to support in
*   tight spaces, as there is no need for a separate identity-management
*   apparatus devoted to policy.
* When the non-isolated behavior is desired, the program should set BOTH
*   flags. Certain kinds of identity may not be valid for either, in which
*   case both flags should be cleared (default condition).
*
* TODO: This is covered by an RFC that I forget the name/number for. Dig.
*         Just code against the enum, and as long as the data is not persisted
*         everything will transition smoothly.
*/
#define MANUVR_IDENT_FLAG_DIRTY        0x8000  // This ID will be lost if not persisted.
#define MANUVR_IDENT_FLAG_OUR_OWN      0x4000  // This is our own identity.
#define MANUVR_IDENT_FLAG_LOCAL_CHAIN  0x2000  // An identity required to validate our own.
#define MANUVR_IDENT_FLAG_ORIG_PERSIST 0x1000  // Was loaded from persisted data.
#define MANUVR_IDENT_FLAG_ORIG_GEN     0x0800  // Was generated on this system.
#define MANUVR_IDENT_FLAG_ORIG_EXTER   0x0400  // Came from outside. Usually from TLS.
#define MANUVR_IDENT_FLAG_ORIG_HSM     0x0200  // Came from an HSM, local or not.
#define MANUVR_IDENT_FLAG_ORIG_PKI     0x0100  // Imparted by a PKI.
#define MANUVR_IDENT_FLAG_RESERVED_0   0x0080  //
#define MANUVR_IDENT_FLAG_CRYPT_BACKED 0x0040  // Identity is cryptographically supported.
#define MANUVR_IDENT_FLAG_VALID        0x0020  // Identity is valid and ready-for-use.
#define MANUVR_IDENT_FLAG_3RD_PARTY_CA 0x0010  // This is an intermediary or CA cert.
#define MANUVR_IDENT_FLAG_REVOKED      0x0008  // This identity is no longer valid.
#define MANUVR_IDENT_FLAG_REVOKABLE    0x0004  // This identity can be revoked.
#define MANUVR_IDENT_FLAG_APP_ACCEPT   0x0002  // Is acceptable for the application layer.
#define MANUVR_IDENT_FLAG_NET_ACCEPT   0x0001  // Is acceptable for the network layer.

/* These flags should not be persisted. */
#define MANUVR_IDENT_FLAG_PERSIST_MASK (MANUVR_IDENT_FLAG_VALID | \
                                        MANUVR_IDENT_FLAG_RESERVED_0 | \
                                        MANUVR_IDENT_FLAG_DIRTY)


#define IDENTITY_BASE_PERSIST_LENGTH   6  // The minimum size of the base identity class.
#define IDENTITY_BASE_SERIAL_VERSION   1  // The serializer version.


/* Interface for factory function. */
typedef Identity* (*IdentityInflator)(uint8_t* buf, int len);


/*
* Possible classes of identity.
*/
enum class IdentFormat : uint8_t {
  // Zero-block: The basics.
  UNDETERMINED    = 0x00,  // Nothing here.
  SERIAL_NUM      = 0x01,  // Nearly universal.
  UUID            = 0x02,  // Low-grade. Easy.
  L2_MAC          = 0x03,  // A layer-2 MAC of some sort.
  USER            = 0x04,  // A human actor.

  // Cryptographic
  CERT_FORMAT_DER = 0x10,  // Certificate in DER format.
  PK              = 0x11,  // Asymmetric key.
  PSK_SYM         = 0x12,  // Pre-shared symmetric key.
  PSK_HMAC        = 0x13,  // Pre-shared HMAC key.

  // External frameworks
  OIC_CRED        = 0x40   // OIC credential.
};


/*
* This is our manner of grappling with the concept of "identity". The issue is
*   one of the core-problems of knowledge, and defining it will take effort and
*   refinement.
* This is a base virtual for the common ground...
*/
class Identity {
  public:
    virtual ~Identity();

    /* Provides concise representations... */
    virtual void toString(StringBuilder*)  =0;   // For readability.
    virtual int  serialize(uint8_t*, uint16_t len) =0;   // For storage.
    inline  int  toBuffer(uint8_t* buf) {
      return _serialize(buf, _ident_len);
    };

    inline int   length() {   return _ident_len;  };
    inline bool  isDirty() {  return _ident_flag(MANUVR_IDENT_FLAG_DIRTY);   };
    inline bool  isValid() {  return _ident_flag(MANUVR_IDENT_FLAG_VALID);   };
    inline bool  isSelf() {   return _ident_flag(MANUVR_IDENT_FLAG_OUR_OWN); };

    inline void  isSelf(bool x) {  _ident_set_flag(x, MANUVR_IDENT_FLAG_OUR_OWN); };

    inline char* getHandle() {  return (nullptr == _handle ? (char*) "unnamed":_handle);  };
    Identity* getIdentity(IdentFormat);
    Identity* getIdentity(const char*);

    static Identity* fromBuffer(uint8_t*, int len);   // From storage.
    static void staticToString(Identity*, StringBuilder*);
    static const char* identityTypeString(IdentFormat);
    static const IdentFormat* supportedNotions();


  protected:
    uint16_t _ident_len  = 0;  // How many bytes does the identity occupy in storage?

    Identity(const char*, IdentFormat);

    int  _serialize(uint8_t*, uint16_t len);   // For storage.
    inline void _clobber_format(IdentFormat f) {  _format = f;  };

    inline bool _ident_flag(uint16_t f) {   return ((_flags & f) == f);  };
    inline void _ident_set_flag(bool nu, uint16_t _flag) {
      _flags = (nu) ? (_flags | _flag) : (_flags & ~_flag);
    };


  private:
    uint16_t    _flags  = 0;
    char*       _handle = nullptr;  // Human-readable name of this identity.
    Identity*   _next   = nullptr;  // See notes.
    IdentFormat _format = IdentFormat::UNDETERMINED;   // What is the nature of this identity?

    static const IdentFormat supported_notions[];
};


/**
* Note regarding the linked-list aspect of the Identity class:
* Chaining is probably better than flatness in this case. The problem is that we
*   need to be able to sustain identities that are not self-contained (PKI,
*   suppose). By chaining identities (in-order of relevance where possible), we
*   allow for features like authentication chains, PKI caching,
*   and multiple-self-identity.
*/




/*
* Systems that manage communication with several peers will probably want some
*   means of handling collections of such objects.
*/
class IdentityManager {
  public:
    IdentityManager() {};
    ~IdentityManager() {};

    inline Identity* selfIdentity() {   return _self;  };


  private:
    Identity*   _self      = nullptr;
    LinkedList<Identity*> _idents;   // List of identities that are not ours.
};

#endif // __MANUVR_IDENTITY_H__
