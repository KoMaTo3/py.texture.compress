#pragma once

#include "Python.h"
#include <deque>
#include <stdint.h>
#include "tools.h"

class Memory;

class Module {
public:
  static PyObject* Func123( PyObject *self, PyObject *args );
  static PyObject* free( PyObject *self, PyObject *args );

  static PyObject* tga2rgba( PyObject *self, PyObject *args );
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
