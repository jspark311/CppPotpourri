#include "StegImage.h"
#include <math.h>    // for ceil

StegImage::StegImage()
  : Image(),
    _enable_red(true),
    _enable_green(true),
    _enable_blue(true),
    _offset(0),
    _max_stride(14),
    _stride_seed(0),
    _usable_pixels(0),
    _payload_size(0),
    _max_payload_size(0),
    _strides(nullptr),
    _stride_count(0),
    _plaintext(nullptr),
    _plaintext_len(0),
    _ciphertext(nullptr),
    _ciphertext_len(0),
    _file_name_info(nullptr),
    _compress(false),
    _store_filename(true),
    _rescale(true),
    _visible_result(false),
    _write_directory(nullptr),
    _bit_cursor(0),
    _iv_size(0) {
}

StegImage::~StegImage() {
  destroyImage();
}

bool StegImage::setChannels(bool red, bool green, bool blue) {
  _enable_red = red;
  _enable_green = green;
  _enable_blue = blue;
  _find_max_payload_size();
  int bpp = _get_bits_per_pixel();
  if (bpp == 0) {
    return false;
  }
  c3p_log(LOG_LEV_DEBUG, "StegImage", "Channel settings: %d bits/pixel", bpp);
  return true;
}


bool StegImage::loadCarrier(const char* path) {
  destroyImage();
  setSize(img->x(), img->y());
  return setBufferByCopy(img->buffer(), ImgBufferFormat::R8_G8_B8);
}


bool StegImage::setMessage(StringBuilder* message, const char* nameOverride) {
  uint32_t len = (uint32_t)message->length();
  _plaintext.concat(message);
  if (nameOverride && *nameOverride) {
    size_t namelen = strlen(nameOverride) + 1;
    _file_name_info = (char*)malloc(namelen);
    strcpy(_file_name_info, nameOverride);
  }
  _derive_params_from_key(_file_name_info);
  _demarcate_strides();
  return true;
}

bool StegImage::getMessage(const char* outputDirectory, StringBuilder* out) {
  // TODO: _demodulate, _decrypt, optionally write file
  return true;
}

bool StegImage::outputImage(const char* outputPath) {
  // TODO: serialize image buffer to PNG or other format
  return false;
}

const char* StegImage::filename() const {
  return _file_name_info;
}


void StegImage::destroyImage() {
  if (_strides) {
    free(_strides);
    _strides = nullptr;
  }
  _plaintext.clear();
  _ciphertext.clear();
  if (_file_name_info) {
    free(_file_name_info);
    _file_name_info = nullptr;
  }
  if (_write_directory) {
    free(_write_directory);
    _write_directory = nullptr;
  }
}

void StegImage::dumpParams() const {
  c3p_log(2, "StegImage", "Channels (R,G,B): %d,%d,%d",
    _enable_red ? 1 : 0,
    _enable_green ? 1 : 0,
    _enable_blue ? 1 : 0);
  c3p_log(2, "StegImage", "Offset: %u", _offset);
  c3p_log(2, "StegImage", "Max stride: %u", _max_stride);
  c3p_log(2, "StegImage", "Stride seed: %llu", (unsigned long long)_stride_seed);
}

uint8_t* StegImage::_encrypt_data(const uint8_t* plain, uint32_t len, uint32_t* out_len) {
  *out_len = len;
  uint8_t* buf = (uint8_t*)malloc(len);
  memcpy(buf, plain, len);
  return buf;
}

uint8_t* StegImage::_decrypt_data(const uint8_t* cipher, uint32_t len, uint32_t* out_len) {
  *out_len = len;
  uint8_t* buf = (uint8_t*)malloc(len);
  memcpy(buf, cipher, len);
  return buf;
}


void StegImage::_derive_params_from_key(const char* password) {
  uint32_t pass_len = (uint32_t)strlen(password);
  uint8_t digest[32];
  sha256((uint8_t*)password, pass_len, digest);

  // Set header offset from most-significant byte
  _offset = (uint32_t)digest[0];

  // Determine number of additional hash rounds
  uint16_t rounds = (uint16_t)((digest[1] << 8) | digest[2]);
  for (uint16_t i = 0; i < rounds; i++) {
    sha256(digest, 32, digest);
  }

  // Derive maximum stride size from fourth byte
  _max_stride = digest[3];

  // XOR remaining bytes to form RNG seed
  uint8_t xor_val = 0;
  for (int i = 4; i < 32; i++) {
    xor_val ^= digest[i];
  }
  _stride_seed = (uint64_t)xor_val;
}


void StegImage::_demarcate_strides() {
  // TODO: fill _strides array of length _stride_count via C3P_pRNG
}

uint32_t StegImage::_find_max_payload_size() {
  int bpp = _get_bits_per_pixel();
  uint32_t total = x() * y();
  _usable_pixels = total > _offset ? total - _offset : 0;
  _max_payload_size = (_usable_pixels * bpp) / 8;
  c3p_log(1, "StegImage", "Max payload: %u bytes", _max_payload_size);
  return _max_payload_size;
}

int StegImage::_steg_bits_per_pixel() const {
  return (_enable_red ? 1 : 0) + (_enable_green ? 1 : 0) + (_enable_blue ? 1 : 0);
}

void StegImage::_set_channel_spec() {
  // TODO: write channel flags into header pixel at _offset
}

bool StegImage::_encrypt() {
  return true;  // stub
}

bool StegImage::_modulate() {
  return true;  // stub
}

int StegImage::_get_bit() {
  return 0;     // stub
}

uint32_t StegImage::_get_x_coords_by_linear(uint32_t linear) const {
  return linear % x();
}

uint32_t StegImage::_get_y_coords_by_linear(uint32_t linear) const {
  return linear / x();
}

void StegImage::_get_channel_spec() {
  // TODO: read header pixel flags
}


bool StegImage::_decrypt() {
  return true;  // stub
}


bool StegImage::_demodulate() {
  return true;  // stub
}


// Packs a StegHeader into the provided StringBuilder
void StegImage::_pack_header(const StegHeader* hdr, StringBuilder* sb) {
  sb->append((const char*)hdr, sizeof(StegHeader));
}
