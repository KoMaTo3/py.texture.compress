#pragma once

#include "stdio.h"

#define XMD_H
extern "C" {
#include "jpeglib/jpeglib.h"
}
#include "setjmp.h"
#include "pnglib/png.h"

#include "Python.h"
#include <deque>
#include <stdint.h>
#include "tools.h"
#include "memoryreader.h"

class Memory;

class Module {
public:
  static PyObject* Func123( PyObject *self, PyObject *args );
  static PyObject* free( PyObject *self, PyObject *args );

  static PyObject* tga2rgba( PyObject *self, PyObject *args );
  static PyObject* jpg2rgba( PyObject *self, PyObject *args );
  static PyObject* png2rgba( PyObject *self, PyObject *args );
  static PyObject* rgba2dxt1( PyObject *self, PyObject *args );
  static PyObject* rgba2dxt3( PyObject *self, PyObject *args );
  static PyObject* rgba2dxt5( PyObject *self, PyObject *args );

private:
  typedef std::deque< Memory* > MemoryList;
  static MemoryList memoryBlocksList;

  static bool CompressSquish( PyObject* M_IN args, Memory M_OUT &compressedData, size_t M_OUT &width, size_t M_OUT &height, int format );
  static PyObject* DoDXTCompressFromArgs( PyObject* M_IN args, int format );
  static bool GetImageFromArguments( PyObject* M_IN args, size_t M_OUT &width, size_t M_OUT &height, Memory M_OUT &rgbaData );
};

enum MODULE_COMPRESSION_IMAGE_TYPE {
  MODULE_COMPRESSION_IMAGE_TYPE_RGBA = 0,
  MODULE_COMPRESSION_IMAGE_TYPE_RGB_S3TC_DXT1 = 1,
  MODULE_COMPRESSION_IMAGE_TYPE_RGBA_S3TC_DXT1 = 2,
  MODULE_COMPRESSION_IMAGE_TYPE_RGBA_S3TC_DXT3 = 3,
  MODULE_COMPRESSION_IMAGE_TYPE_RGBA_S3TC_DXT5 = 4,
};


class ImgJpg
{
public:

  struct  _gImporterJPEGError {
          jpeg_error_mgr  pub;
          jmp_buf setjmp_buffer;
  };

  static const int c_JPEGInputBufferSize;

  static void JPEGErrorExit             ( j_common_ptr cinfo );
  static void JPEGStdioSrcgMemoryReader ( j_decompress_ptr cinfo, MemoryReader* file );
};

class ImgPng
{
public:
  static void PNGReadFunctiongFile        ( png_structp png_ptr, png_bytep data, png_size_t length );
  static void PNGReadFunctiongMemoryReader( png_structp png_ptr, png_bytep data, png_size_t length );

};