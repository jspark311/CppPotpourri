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

int test_img_construction() {
  int ret = -1;
  printf("Testing Image construction...\n");
  const uint32_t TEST_X_sz     = (37 + (randomUInt32() % 151));
  const uint32_t TEST_Y_sz     = (37 + (randomUInt32() % 151));
  const uint32_t TEST_PX_COUNT = (TEST_X_sz * TEST_Y_sz);
  printf("\tCreating test images of size (%u x %u)... ", TEST_X_sz, TEST_Y_sz);

  //Image(TEST_X_sz, TEST_Y_sz, ImgBufferFormat, uint8_t*);
  Image img_trivial;
  Image img_0(TEST_X_sz, TEST_Y_sz, ImgBufferFormat::MONOCHROME);  // Monochrome
  Image img_1(TEST_X_sz, TEST_Y_sz, ImgBufferFormat::GREY_8);      // 8-bit greyscale
  Image img_2(TEST_X_sz, TEST_Y_sz, ImgBufferFormat::R8_G8_B8);    // 24-bit color
  Image img_3(TEST_X_sz, TEST_Y_sz, ImgBufferFormat::R5_G6_B5);    // 16-bit color
  Image img_4(TEST_X_sz, TEST_Y_sz, ImgBufferFormat::R3_G3_B2);    // 8-bit color
  Image img_5(TEST_X_sz, TEST_Y_sz);
  printf("Done.\n\tAllocation works for all (and only) fully-specified Images... ");

  bool step_pass = !img_trivial.reallocate();
  step_pass &= img_0.reallocate();
  step_pass &= img_1.reallocate();
  step_pass &= img_2.reallocate();
  step_pass &= img_3.reallocate();
  step_pass &= img_4.reallocate();
  step_pass &= !img_5.reallocate();

  if (step_pass) {
    printf("Pass\n\tAllocation sizes match expectations...\n");
    const uint32_t EXPECTED_SZ_1BIT  = ((TEST_PX_COUNT >> 3) + ((TEST_PX_COUNT & 7) ? 1:0));
    const uint32_t EXPECTED_SZ_1BYTE = TEST_PX_COUNT;
    const uint32_t EXPECTED_SZ_2BYTE = (TEST_PX_COUNT * 2);
    const uint32_t EXPECTED_SZ_3BYTE = (TEST_PX_COUNT * 3);
    printf("Pass.\n\t\tUnder-specified images report 0 for bytesUsed()... ");
    if ((0 == img_trivial.bytesUsed()) & (0 == img_5.bytesUsed())) {
      printf("Pass.\n\t\tMONOCHROME image reports %u for bytesUsed()... ", EXPECTED_SZ_1BIT);
      if (EXPECTED_SZ_1BIT == img_0.bytesUsed()) {
        printf("Pass.\n\t\tGREY_8 image reports %u for bytesUsed()... ", EXPECTED_SZ_1BYTE);
        if (EXPECTED_SZ_1BYTE == img_1.bytesUsed()) {
          printf("Pass.\n\t\tR8_G8_B8 image reports %u for bytesUsed()... ", EXPECTED_SZ_3BYTE);
          if (EXPECTED_SZ_3BYTE == img_2.bytesUsed()) {
            printf("Pass.\n\t\tR5_G6_B5 image reports %u for bytesUsed()... ", EXPECTED_SZ_2BYTE);
            if (EXPECTED_SZ_2BYTE == img_3.bytesUsed()) {
              printf("Pass.\n\t\tR3_G3_B2 image reports %u for bytesUsed()... ", EXPECTED_SZ_1BYTE);
              if (EXPECTED_SZ_1BYTE == img_4.bytesUsed()) {
                printf("Pass\n\tAllocation tests pass.\n");
                ret = 0;
              }
            }
          }
        }
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
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

#define CHKLST_IMG_TESTS_ALL ( \
  CHKLST_IMG_TEST_ALLOCATION | CHKLST_IMG_TEST_SET_BUF_BY_COPY | \
  CHKLST_IMG_TEST_REALLOCATE | CHKLST_IMG_TEST_COLOR_CONVERSION | \
  CHKLST_IMG_TEST_PIXEL_MANIPULATION | CHKLST_IMG_TEST_MIRRORING | \
  CHKLST_IMG_TEST_ROTATION | CHKLST_IMG_TEST_PARSE_PACK | \
  CHKLST_IMG_TEST_BITMAP_DRAW | CHKLST_IMG_TEST_FRAMEBUFFER_LOCK | \
  CHKLST_IMG_TEST_GEO_PRIMITIVES | CHKLST_IMG_TEST_TEXT_BASICS | \
  CHKLST_IMG_TEST_TEXT_PLACEMENT | CHKLST_IMG_TEST_TEXT_FONT | \
  CHKLST_IMG_TEST_TEXT_ABUSE | CHKLST_IMG_TEST_GEO_DERIVED)


const StepSequenceList TOP_LEVEL_IMG_TEST_LIST[] = {
  //
  { .FLAG         = CHKLST_IMG_TEST_ALLOCATION,
    .LABEL        = "Construction and allocation",
    .DEP_MASK     = (0),
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

  img_test_plan.requestSteps(CHKLST_IMG_TEST_ALLOCATION);
  //img_test_plan.requestSteps(CHKLST_IMG_TESTS_ALL);
  while (!img_test_plan.request_completed() && (0 == img_test_plan.failed_steps(false))) {
    img_test_plan.poll();
  }

  StringBuilder report_output;
  img_test_plan.printDebug(&report_output, "Image test report");
  printf("%s\n", (char*) report_output.string());

  return (img_test_plan.request_fulfilled() ? 0 : 1);
}
