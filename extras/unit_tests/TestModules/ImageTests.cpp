/*
File:   ImageTests.cpp
Author: J. Ian Lindsay
Date:   2023.09.04


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


This program tests Image.
*/

#include "Image/Image.h"

/*******************************************************************************
* Test routines
*******************************************************************************/
typedef struct {
  Image* img_monochrome; // Monochrome
  Image* img_grey8;      // 8-bit greyscale
  Image* img_r8g8b8;     // 24-bit color
  Image* img_r5g6b5;     // 16-bit color
  Image* img_r3g3b2;     // 8-bit color
} C3PImgTestObjs;

C3PImgTestObjs test_obj = {
  .img_monochrome = nullptr,
  .img_grey8      = nullptr,
  .img_r8g8b8     = nullptr,
  .img_r5g6b5     = nullptr,
  .img_r3g3b2     = nullptr
};


/*
* The Image API uses a binder type for locations in a pixel map. This makes
*   function calls cleaner to read and allows better visibility into the intent
*   of image manipulation code (for both humans and the toolchain).
*/
int test_img_pixaddr() {
  int ret = -1;
  printf("Testing PixAddr...\n");
  printf("\tGiven no arguments, PixUInt constructs as (0, 0)... ");
  const PixAddr ZERO_ADDR;
  if ((0 == ZERO_ADDR.x) && (0 == ZERO_ADDR.y)) {
    printf("Pass\n\tExplicit construction works as expected... ");
    const PixUInt TEST_X = (31 + (randomUInt32() % 151));
    const PixUInt TEST_Y = (31 + (randomUInt32() % 151));
    PixAddr construct_test_0(TEST_X, TEST_Y);
    if ((TEST_X == construct_test_0.x) && (TEST_Y == construct_test_0.y)) {
      printf("Pass\n\tCopy construction works as expected... ");
      PixAddr construct_test_1(construct_test_0);
      if ((TEST_X == construct_test_1.x) && (TEST_Y == construct_test_1.y)) {
        printf("PASS\n");
        ret = 0;
      }
    }
  }
  return ret;
}


/*
* The Image API supports an optional indirection class that defines a frustum
*   within a larger pixel map, thereby eliminating the need for locally-scoped
*   drawing code to do absolute pixel arithmetic.
*/
int test_img_pixboundingbox() {
  int ret = -1;
  return ret;
}


/*
* Construction can be done with or without an existing memory range.
* Image dimensions are arbitrary, but must be greater than zero. They must also
*   be less-than the maximum value representable by PixUInt (defaults to 16-bit).
*
*/
int test_img_construction() {
  int ret = -1;
  printf("Testing Image construction...\n");
  const PixUInt TEST_X_sz     = (PixUInt) (37 + (randomUInt32() % 151));
  const PixUInt TEST_Y_sz     = (PixUInt) (37 + (randomUInt32() % 151));
  const PixUInt TEST_PX_COUNT = (TEST_X_sz * TEST_Y_sz);
  printf("\tCreating test images of size (%u x %u)... ", TEST_X_sz, TEST_Y_sz);
  uint8_t stack_img_buf[TEST_PX_COUNT];       // Create a pre-allocated buffer
  random_fill(stack_img_buf, TEST_PX_COUNT);  //   and fill it with junk.

  Image img_trivial;
  Image img_0(TEST_X_sz, TEST_Y_sz, ImgBufferFormat::MONOCHROME);  // Monochrome
  Image img_1(TEST_X_sz, TEST_Y_sz, ImgBufferFormat::GREY_8);      // 8-bit greyscale
  Image img_2(TEST_X_sz, TEST_Y_sz, ImgBufferFormat::R8_G8_B8);    // 24-bit color
  Image img_3(TEST_X_sz, TEST_Y_sz, ImgBufferFormat::R5_G6_B5);    // 16-bit color
  Image img_4(TEST_X_sz, TEST_Y_sz, ImgBufferFormat::R3_G3_B2);    // 8-bit color
  Image img_5(TEST_X_sz, TEST_Y_sz);
  Image img_6(TEST_X_sz, TEST_Y_sz, ImgBufferFormat::GREY_8, stack_img_buf);

  // Allocation isn't done on construction.
  printf("Done.\n\tAllocation works for all (and only) fully-specified Images... ");
  bool step_pass = !img_trivial.reallocate(); // Should fail because one or both dimensions is zero.
  step_pass &= img_0.reallocate();    // Should pass.
  step_pass &= img_1.reallocate();    // Should pass.
  step_pass &= img_2.reallocate();    // Should pass.
  step_pass &= img_3.reallocate();    // Should pass.
  step_pass &= img_4.reallocate();    // Should pass.
  step_pass &= !img_5.reallocate();   // Should fail because the format isn't specified.
  step_pass &= !img_6.reallocate();   // Should fail because the buffer was given.

  if (step_pass) {
    printf("Pass.\n\tUnder-specified images report 0 for bytesUsed()... ");
    if ((0 == img_trivial.bytesUsed()) & (0 == img_5.bytesUsed())) {
      //Image* test_images[] = {
      //  &img_0, &img_1, &img_2, &img_3, &img_4, &img_6  // NOTE: 5 should fail.
      //};
      printf("Pass\n\tbuffer() returns the same pointer as was passed to the constructor... ");
      if (stack_img_buf == img_6.buffer()) {
        printf("Pass\n\tAllocation sizes match expectations...");
        const uint32_t EXPECTED_SZ_1BIT  = ((TEST_PX_COUNT >> 3) + ((TEST_PX_COUNT & 7) ? 1:0));
        const uint32_t EXPECTED_SZ_1BYTE = (TEST_PX_COUNT);
        const uint32_t EXPECTED_SZ_2BYTE = (TEST_PX_COUNT * 2);
        const uint32_t EXPECTED_SZ_3BYTE = (TEST_PX_COUNT * 3);
        printf("\n\t\tMONOCHROME image reports %u for bytesUsed()... ", EXPECTED_SZ_1BIT);
        if (EXPECTED_SZ_1BIT == img_0.bytesUsed()) {
          printf("Pass.\n\t\tGREY_8 image reports %u for bytesUsed()... ", EXPECTED_SZ_1BYTE);
          if (EXPECTED_SZ_1BYTE == img_1.bytesUsed()) {
            printf("Pass.\n\t\tR8_G8_B8 image reports %u for bytesUsed()... ", EXPECTED_SZ_3BYTE);
            if (EXPECTED_SZ_3BYTE == img_2.bytesUsed()) {
              printf("Pass.\n\t\tR5_G6_B5 image reports %u for bytesUsed()... ", EXPECTED_SZ_2BYTE);
              if (EXPECTED_SZ_2BYTE == img_3.bytesUsed()) {
                printf("Pass.\n\t\tR3_G3_B2 image reports %u for bytesUsed()... ", EXPECTED_SZ_1BYTE);
                if (EXPECTED_SZ_1BYTE == img_4.bytesUsed()) {
                  // Finally, allocate an image in each color format we plan to test.
                  //test_obj.img_monochrome = nullptr;
                  //test_obj.img_grey8      = nullptr;
                  //test_obj.img_r8g8b8     = nullptr;
                  //test_obj.img_r5g6b5     = nullptr;
                  //test_obj.img_r3g3b2     = nullptr;
                  ret = 0;
                }
              }
            }
          }
        }
      }
    }
  }

  if (0 == ret) {
    const uint32_t IMG_BYTE_LEN = img_1.bytesUsed();
    uint8_t* IMG_BUFFER         = img_1.buffer();

    printf("Pass\n\tCalling reallocate() zeroes the buffer memory... ");
    for (PixUInt i = 0; i < IMG_BYTE_LEN; i++) {
      if (0 != *(IMG_BUFFER + i)) {  ret = -1;  }
    }
    if (0 == ret) {
      printf("Pass\n\tCalling reallocate() on already-allocated memory still wipes the buffer... ");
      random_fill(IMG_BUFFER, IMG_BYTE_LEN);  // Fill it with junk again.
      if (img_1.reallocate()) {
        for (PixUInt i = 0; i < IMG_BYTE_LEN; i++) {
          if (0 != *(IMG_BUFFER + i)) {  ret = -1;  }
        }
      }
      else {
        ret = -1;
      }
    }
  }

  if (0 == ret) {
    printf("PASS.\n");
  }
  else {
    printf("Fail.\n");
    dump_image(&img_1);
  }
  return ret;
}



int test_img_buffer_by_copy() {
  int ret = -1;
  return ret;
}



int test_img_reallocation() {
  int ret = -1;
  return ret;
}


int test_img_endian_flip() {
  int ret = -1;
  printf("Testing pixel endian flip...\n");
  // Determine our endianess with a magic number and a pointer dance.
  const uint16_t TEST = 0xAA55;
  const bool PF_IS_BIG_ENDIAN = (0xAA == *((uint8_t*) &TEST));
  const PixUInt  TEST_X_SZ = (PixUInt) (37 + (randomUInt32() % 151));
  const PixUInt  TEST_Y_SZ = (PixUInt) (37 + (randomUInt32() % 151));
  const uint16_t COLOR_WRITE_0 = 0x1234;
  const uint16_t COLOR_READ_0  = endianSwap16(COLOR_WRITE_0);
  const uint16_t COLOR_WRITE_1 = 0x8844;
  const uint16_t COLOR_READ_1  = endianSwap16(COLOR_WRITE_1);
  const PixUInt  TEST_0_X = (TEST_X_SZ >> 1);
  const PixUInt  TEST_0_Y = (TEST_Y_SZ >> 1);
  const PixUInt  TEST_1_X = (TEST_X_SZ >> 2);
  const PixUInt  TEST_1_Y = (TEST_Y_SZ >> 2);
  printf("\tCreating test image of size (%u x %u), native format is %s-endian... ", TEST_X_SZ, TEST_Y_SZ, (PF_IS_BIG_ENDIAN?"big":"little"));
  Image test_img(TEST_X_SZ, TEST_Y_SZ, ImgBufferFormat::R5_G6_B5);    // 16-bit color
  if (test_img.reallocate()) {
    printf("Done.\n\tWriting a pixel works... ");
    if (test_img.setPixel(TEST_0_X, TEST_0_Y, COLOR_WRITE_0)) {
      printf("Pass.\n\tThe pixel reads back the same way... ");
      if (COLOR_WRITE_0 == test_img.getPixel(TEST_0_X, TEST_0_Y)) {
        printf("Pass.\n\tSwapping the endianness of the image makes the same pixel read back flipped... ");
        test_img.bigEndian(!PF_IS_BIG_ENDIAN);
        if (COLOR_READ_0 == test_img.getPixel(TEST_0_X, TEST_0_Y)) {
          printf("Pass.\n\tWriting another pixel produces the same result with non-native endianness... ");
          if (test_img.setPixel(TEST_1_X, TEST_1_Y, COLOR_WRITE_1)) {
            if (COLOR_WRITE_1 == test_img.getPixel(TEST_1_X, TEST_1_Y)) {
              printf("Pass.\n\tSwapping the endianness of the image back to native flips the second pixel... ");
              test_img.bigEndian(PF_IS_BIG_ENDIAN);
              if (COLOR_READ_1 == test_img.getPixel(TEST_1_X, TEST_1_Y)) {
                ret = 0;
              }
            }
          }
        }
      }
    }
  }

  if (0 == ret) {
    printf("PASS.\n");
  }
  else {
    printf("Fail.\n");
    dump_image(&test_img);
  }
  return ret;
}


int test_img_color() {
  int ret = -1;
  return ret;
}


int test_img_pixel_level() {
  int ret = -1;
  return ret;
}


int test_img_mirroring() {
  int ret = -1;
  return ret;
}


int test_img_rotation() {
  int ret = -1;
  return ret;
}


int test_img_fb_lock() {
  int ret = -1;
  return ret;
}


int test_img_bitmap() {
  int ret = -1;
  return ret;
}


int test_img_geo_primitives() {
  int ret = -1;
  return ret;
}


int test_img_txt_basics() {
  int ret = -1;
  return ret;
}


int test_img_txt_placement() {
  int ret = -1;
  return ret;
}


int test_img_txt_font() {
  int ret = -1;
  return ret;
}


int test_img_txt_abuse() {
  int ret = -1;
  return ret;
}


int test_img_txt_geo_derived() {
  int ret = -1;
  return ret;
}





/*******************************************************************************
* Test plan
*******************************************************************************/

#define CHKLST_IMG_TEST_ALLOCATION         0x00000001  // Tests the constructors and allocation semantics.
#define CHKLST_IMG_TEST_SET_BUF_BY_COPY    0x00000002  // setBufferByCopy(uint8_t*, ImgBufferFormat);
#define CHKLST_IMG_TEST_REALLOCATE         0x00000004  // reallocate()
#define CHKLST_IMG_TEST_COLOR_CONVERSION   0x00000008  // Does the color handling make sense?
#define CHKLST_IMG_TEST_PIXEL_MANIPULATION 0x00000010  // Can pixels be read and written?
#define CHKLST_IMG_TEST_MIRRORING          0x00000020  //
#define CHKLST_IMG_TEST_ROTATION           0x00000040  //
#define CHKLST_IMG_TEST_PARSE_PACK         0x00000080  // Can image metadata and content be moved over a wire?
#define CHKLST_IMG_TEST_BITMAP_DRAW        0x00000100  // Can bitmaps be drawn?
#define CHKLST_IMG_TEST_FRAMEBUFFER_LOCK   0x00000200  // Framebuffer support flags.
#define CHKLST_IMG_TEST_GEO_PRIMITIVES     0x00000400  // Lines, circles, squares, etc
#define CHKLST_IMG_TEST_TEXT_BASICS        0x00000800  //
#define CHKLST_IMG_TEST_TEXT_PLACEMENT     0x00001000  //
#define CHKLST_IMG_TEST_TEXT_FONT          0x00002000  //
#define CHKLST_IMG_TEST_TEXT_ABUSE         0x00004000  // Mis-use of the API.
#define CHKLST_IMG_TEST_GEO_DERIVED        0x00008000  // More sophisticated geometry fxns.
#define CHKLST_IMG_TEST_PIXADDR            0x00010000  // Tests the PixAddr class.
#define CHKLST_IMG_TEST_PIXBOUNDINGBOX     0x00020000  // Tests the PixBoundingBox class.
#define CHKLST_IMG_TEST_ENDIAN_FLIP        0x00040000  // Framebuffers often need a specific storage order.

#define CHKLST_IMG_TESTS_ALL ( \
  CHKLST_IMG_TEST_ALLOCATION | CHKLST_IMG_TEST_SET_BUF_BY_COPY | \
  CHKLST_IMG_TEST_REALLOCATE | CHKLST_IMG_TEST_COLOR_CONVERSION | \
  CHKLST_IMG_TEST_PIXEL_MANIPULATION | CHKLST_IMG_TEST_MIRRORING | \
  CHKLST_IMG_TEST_ROTATION | CHKLST_IMG_TEST_PARSE_PACK | \
  CHKLST_IMG_TEST_BITMAP_DRAW | CHKLST_IMG_TEST_FRAMEBUFFER_LOCK | \
  CHKLST_IMG_TEST_GEO_PRIMITIVES | CHKLST_IMG_TEST_TEXT_BASICS | \
  CHKLST_IMG_TEST_TEXT_PLACEMENT | CHKLST_IMG_TEST_TEXT_FONT | \
  CHKLST_IMG_TEST_TEXT_ABUSE | CHKLST_IMG_TEST_GEO_DERIVED | \
  CHKLST_IMG_TEST_PIXADDR | CHKLST_IMG_TEST_PIXBOUNDINGBOX)


const StepSequenceList TOP_LEVEL_IMG_TEST_LIST[] = {
  //
  { .FLAG         = CHKLST_IMG_TEST_PIXADDR,
    .LABEL        = "PixAddr class",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_pixaddr()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_ALLOCATION,
    .LABEL        = "Construction and allocation",
    .DEP_MASK     = (CHKLST_IMG_TEST_PIXADDR),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_construction()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_SET_BUF_BY_COPY,
    .LABEL        = "setBufferByCopy()",
    .DEP_MASK     = (CHKLST_IMG_TEST_ALLOCATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_buffer_by_copy()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_REALLOCATE,
    .LABEL        = "Re-allocation",
    .DEP_MASK     = (CHKLST_IMG_TEST_ALLOCATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_reallocation()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_COLOR_CONVERSION,
    .LABEL        = "Color conversion",
    .DEP_MASK     = (CHKLST_IMG_TEST_ALLOCATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_color()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_PIXEL_MANIPULATION,
    .LABEL        = "Pixel manipulation",
    .DEP_MASK     = (CHKLST_IMG_TEST_COLOR_CONVERSION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_pixel_level()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_MIRRORING,
    .LABEL        = "Mirroring",
    .DEP_MASK     = (CHKLST_IMG_TEST_PIXEL_MANIPULATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_mirroring()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_ROTATION,
    .LABEL        = "Rotation",
    .DEP_MASK     = (CHKLST_IMG_TEST_MIRRORING),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_rotation()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_FRAMEBUFFER_LOCK,
    .LABEL        = "Framebuffer locking",
    .DEP_MASK     = (CHKLST_IMG_TEST_PIXEL_MANIPULATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_fb_lock()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_ENDIAN_FLIP,
    .LABEL        = "Endian flip",
    .DEP_MASK     = (CHKLST_IMG_TEST_PIXEL_MANIPULATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_endian_flip()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_BITMAP_DRAW,
    .LABEL        = "Bitmap draw",
    .DEP_MASK     = (CHKLST_IMG_TEST_PIXEL_MANIPULATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_bitmap()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_GEO_PRIMITIVES,
    .LABEL        = "Geometric primitives",
    .DEP_MASK     = (CHKLST_IMG_TEST_PIXEL_MANIPULATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_geo_primitives()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_TEXT_BASICS,
    .LABEL        = "Text basics",
    .DEP_MASK     = (CHKLST_IMG_TEST_PIXEL_MANIPULATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_txt_basics()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_TEXT_PLACEMENT,
    .LABEL        = "Text placement",
    .DEP_MASK     = (CHKLST_IMG_TEST_TEXT_BASICS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_txt_placement()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_TEXT_FONT,
    .LABEL        = "Fonts",
    .DEP_MASK     = (CHKLST_IMG_TEST_TEXT_PLACEMENT),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_txt_font()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_TEXT_ABUSE,
    .LABEL        = "Text abuse",
    .DEP_MASK     = (CHKLST_IMG_TEST_TEXT_FONT | CHKLST_IMG_TEST_TEXT_PLACEMENT | CHKLST_IMG_TEST_TEXT_BASICS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_txt_abuse()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_GEO_DERIVED,
    .LABEL        = "Derived geometrics",
    .DEP_MASK     = (CHKLST_IMG_TEST_GEO_PRIMITIVES),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_txt_geo_derived()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_IMG_TEST_PIXBOUNDINGBOX,
    .LABEL        = "PixBoundingBox",
    .DEP_MASK     = (CHKLST_IMG_TEST_PIXADDR),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_img_pixboundingbox()) ? 1:-1);  }
  },
};

AsyncSequencer img_test_plan(TOP_LEVEL_IMG_TEST_LIST, (sizeof(TOP_LEVEL_IMG_TEST_LIST) / sizeof(TOP_LEVEL_IMG_TEST_LIST[0])));


/*******************************************************************************
* The main function.
*******************************************************************************/

void print_types_image() {
  printf("\tImage                 %u\t%u\n", sizeof(Image),    alignof(Image));
  printf("\tPixUInt               %u\t%u\n", sizeof(PixUInt),  alignof(PixUInt));
  printf("\tGFXfont               %u\t%u\n", sizeof(GFXfont),  alignof(GFXfont));
  printf("\tGFXglyph              %u\t%u\n", sizeof(GFXglyph), alignof(GFXglyph));
}


int c3p_image_test_main() {
  const char* const MODULE_NAME = "Image";
  printf("===< %s >=======================================\n", MODULE_NAME);

  img_test_plan.requestSteps(CHKLST_IMG_TEST_ALLOCATION | CHKLST_IMG_TEST_ENDIAN_FLIP);
  //img_test_plan.requestSteps(CHKLST_IMG_TESTS_ALL);
  while (!img_test_plan.request_completed() && (0 == img_test_plan.failed_steps(false))) {
    img_test_plan.poll();
  }

  StringBuilder report_output;
  img_test_plan.printDebug(&report_output, "Image test report");
  printf("%s\n", (char*) report_output.string());

  return (img_test_plan.request_fulfilled() ? 0 : 1);
}
