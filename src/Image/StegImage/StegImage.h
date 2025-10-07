
#ifndef __C3P_STEG_IMG_H
#define __C3P_STEG_IMG_H

#include "../Image.h"
#include "../../StringBuilder.h"
#include "../../C3PRandom/C3PRandom.h"
#include "../../Identity/Identity.h"



/*
* Subclass of Image providing LSB-based steganographic embedding and extraction.
* Theory of operation can be found in the original PHP class:
* https://github.com/jspark311/BuriedUnderTheNoiseFloor
*/

#pragma pack(push, 1)
struct StegHeader {
  uint8_t active_channels;  // bit0=red, bit1=green, bit2=blue
  uint16_t version;         // writer version
  uint8_t header_length;    // sizeof(StegHeader)
  uint8_t message_params;   // message control bits
  uint8_t channel_params;   // reserved for carrier preprocessing
  uint32_t payload_size;    // size of payload+checksum, excluding header
};
#pragma pack(pop)


class StegImage : public Image {
  public:
    StegImage();
    ~StegImage();

    bool setChannels(bool red = true, bool green = true, bool blue = true);
    bool loadCarrier(Image* img);
    bool setMessage(StringBuilder*, const char* nameOverride = nullptr);
    bool getMessage(const char* outputDirectory, StringBuilder* out);
    bool outputImage(const char* outputPath);
    const char* filename() const;
    void destroyImage();

    void dumpParams() const;


  private:
    bool _enable_red;
    bool _enable_green;
    bool _enable_blue;
    uint32_t _offset;
    uint8_t _max_stride;
    uint64_t _stride_seed;
    uint32_t _usable_pixels;
    uint32_t _payload_size;
    uint32_t _max_payload_size;
    uint32_t* _strides;
    uint32_t _stride_count;
    StringBuilder _plaintext;
    StringBuilder _ciphertext;
    char* _file_name_info;
    bool _compress;
    bool _store_filename;
    bool _rescale;
    bool _visible_result;
    char* _write_directory;
    int _bit_cursor;
    uint32_t _iv_size;

    // Stub cryptographic functions (pass-through)
    uint8_t* _encrypt_data(const uint8_t* plain, uint32_t len, uint32_t* out_len);
    uint8_t* _decrypt_data(const uint8_t* cipher, uint32_t len, uint32_t* out_len);

    // Internal parameter derivation
    void _derive_params_from_key(const char* password);
    void _demarcate_strides();
    uint32_t _find_max_payload_size();
    int _steg_bits_per_pixel() const;

    // Header packing
    void _pack_header(const StegHeader* hdr, StringBuilder* sb);

    // Modulation helpers
    void _set_channel_spec();
    bool _encrypt();
    bool _modulate();
    int _get_bit();
    uint32_t _get_x_coords_by_linear(uint32_t linear) const;
    uint32_t _get_y_coords_by_linear(uint32_t linear) const;
    void _get_channel_spec();
    bool _decrypt();
    bool _demodulate();
};





/*
* This is a transform class that generates an authentication code for the source Image,
*   and then steganographically embeds it into the Image itself, along with an
*   optional payload. That is, it modifies the source Image. For theory and
*   operation, see BuriedUnderTheNoiseFloor.
*/
class ImageSigner {
  public:
    ImageSigner(
      Image* i_s, Identity* signing_ident, uint8_t* payload = nullptr, uint32_t payload_len = 0
    );
    ~ImageSigner() {};

    int8_t sign();
    int8_t signWithParameters();
    bool   busy();


  private:
    Image*    _source;
    Identity* _signing_ident;
    uint8_t*  _pl;
    uint32_t  _pl_len;
};


/*
* This is a class that tries to authenticate a given Image against a given
*   Identity, and extract any payloads that may be steganographically embedded
*   within it. It does not modify the source Image. For theory and
*   operation, see BuriedUnderTheNoiseFloor.
*/
class ImageAuthenticator {
  public:
    ImageAuthenticator(Image* i_s, Identity* verify_ident);
    ~ImageAuthenticator() {};

    int8_t verify();
    int8_t verifyWithParameters();
    bool   busy();
    bool   authenticated();
    bool   foundSig();

    inline uint8_t* payload() {         return  _pl;        };
    inline uint32_t payloadLength() {   return  _pl_len;    };


  private:
    Image*    _source;
    Identity* _verify_ident;
    uint8_t*  _pl;
    uint32_t  _pl_len;
};



#endif   // __C3P_STEG_IMG_H
