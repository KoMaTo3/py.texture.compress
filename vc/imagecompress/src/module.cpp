#include "module.h"

#include "stdio.h"
#include "filemanager.h"
#include "tools.h"
#include "squish.h"

Module::MemoryList Module::memoryBlocksList;

PyObject* Module::Func123( PyObject *self, PyObject *args ) {
  const char *fileName;
  if( !PyArg_ParseTuple( args, "s", &fileName ) ) {
    LOGE( "fileName is NULL" );
    return NULL;
  }

  return Py_BuildValue( "i", strlen( fileName ) );
}//Func123

PyObject* Module::tga2rgba( PyObject *self, PyObject *args ) {
  const char *fileName;
  if( !PyArg_ParseTuple( args, "s", &fileName ) ) {
    LOGE( "fileName is NULL" );
    return NULL;
  }
  size_t
    width = 0,
    height = 0;
  bool isTransparent = false;

  FileManagerType fileManager;
  if( !fileManager.FileExists( fileName ) ) {
    LOGE( "File '%s' not fount", fileName );
    return NULL;
  }

  Memory memory, imageDataRGBA;
  fileManager.GetFile( fileName, memory );
  //Module::memoryBlocksList.push_back( imageDataRGBA );

  //decode
  if( !memory.GetLength() ) {
    LOGE( "No data or file too short" );
    return false;
  }

  unsigned char *data = memory.GetData();
  unsigned char idLength = data[ 0 ];
  if( data[ 1 ] ) {
    LOGE( "ImageLoader::LoadTGA => idLength" );
    return false;
  }

  unsigned char compressed = data[ 2 ]; //RLE-compression
  if( !( compressed == 2 || compressed == 0x0A ) ) {
    LOGE( "ImageLoader::LoadTGA => unknown compression" );
    return false;
  }

  unsigned short
    *w = ( unsigned short* ) ( data + 12 ),
    *h = ( unsigned short* ) ( data + 14 );
  width = *w;
  height = *h;

  unsigned char bpp = data[ 16 ];
  if( !( bpp == 24 || bpp == 32 ) ) {
    LOGE( "ImageLoader::LoadTGA => unknown bpp[%d]", bpp );
    return false;
  }

  size_t src_pos = 18 + idLength;

  isTransparent = false;
  imageDataRGBA.Alloc( width * height * 4 );
  unsigned char *dest = imageDataRGBA.GetData();

  if( compressed == 2 ) { //not compressed
    size_t x, y;
    unsigned char mult = bpp >> 3;
    for( y = 0; y < height; y++) {
      for( x = 0; x < width; x++) {
        size_t dest_pos = ( x + y * width ) << 2;
        //dest_pos = ( x + ( height - y - 1 ) * width ) << 2;
        src_pos  = ( x + y * width ) * mult + 18;
        dest[ dest_pos     ] = data[ src_pos + 2 ];
        dest[ dest_pos + 1 ] = data[ src_pos + 1 ];
        dest[ dest_pos + 2 ] = data[ src_pos + 0 ];
        dest[ dest_pos + 3 ] = ( bpp == 32 ? data[ src_pos + 3 ] : 255);
        if( bpp == 32 && data[ src_pos + 3 ] != 0xFF ) {
          isTransparent = true;
        }
      }//x
    }//y
  } else { //RLE-compression 0x0A
    size_t src_pos = 18 + idLength, dest_pos = 0;
    unsigned char q;
    unsigned char r, g, b, a = 255;
    size_t x = 0, y = 0;
    while( y < height ) {
      unsigned char block = data[ src_pos++ ];
      unsigned char num = block & 127;

      if( block & 128 ) { //compressed block
        b = data[ src_pos ];
        g = data[ src_pos + 1 ];
        r = data[ src_pos + 2 ];
        if( bpp == 32 ) {
          a = data[ src_pos + 3 ];
          src_pos += 4;
        } else {
          src_pos += 3;
        }
        for( q = 0; q < num + 1; ++q ) {
          //dest_pos = ( x + y * width ) << 2;
          dest_pos = ( x + ( height - y - 1 ) * width ) << 2;
          dest[ dest_pos ] = r;
          dest[ dest_pos + 1 ] = g;
          dest[ dest_pos + 2 ] = b;
          dest[ dest_pos + 3 ] = a;
          if( a != 0xFF ) {
            isTransparent = true;
          }

          x = ( x + 1 ) % width;
          if( !x ) {
            ++y;
          }
        }
      } else { //not compressed block
        for( q = 0; q < num + 1; ++q ) {
          //dest_pos = ( x + y * width ) << 2;
          dest_pos = ( x + ( height - y - 1 ) * width ) << 2;
          b = data[ src_pos ];
          g = data[ src_pos + 1 ];
          r = data[ src_pos + 2 ];
          if( bpp == 32 ) {
            a = data[ src_pos + 3 ];
            src_pos += 4;
          } else {
            src_pos += 3;
          }

          dest[ dest_pos ] = r;
          dest[ dest_pos + 1 ] = g;
          dest[ dest_pos + 2 ] = b;
          dest[ dest_pos + 3 ] = a;
          if( a != 0xFF ) {
            isTransparent = true;
          }

          x = ( x + 1 ) % width;
          if( !x ) {
            ++y;
          }
        }//for q < num
      }//not compressed block
    }//while y < height
  }//RLE-compression

  return Py_BuildValue( "{s:y#,s:i,s:i,s:i}", "data", imageDataRGBA.GetData(), imageDataRGBA.GetLength(), "width", width, "height", height, "length", imageDataRGBA.GetLength() );
}//tga2rgba

PyObject* Module::free( PyObject *self, PyObject *args ) {
  for( auto& memory: Module::memoryBlocksList ) {
    delete memory;
  }
  Module::memoryBlocksList.clear();
  return Py_BuildValue( "i", 1 );
}//free

bool Module::GetImageFromArguments( PyObject *args, size_t M_OUT &width, size_t M_OUT &height, Memory M_OUT &rgbaData ) {
  PyObject *parameters;
  if( !PyArg_ParseTuple( args, "O!", &PyDict_Type, &parameters ) ) {
    LOGE( "rgba2dxt1 => need dict as first parameter" );
    return NULL;
  }
  PyObject *items = PyDict_Items( parameters );
  PyObject *keys = PyDict_Keys( parameters );
  rgbaData.Free();
  width = 0;
  height = 0;

  auto size = PyList_Size( items );
  for( int q = 0; q < size; ++q ) {
    PyObject *key = PyList_GetItem( keys, q );
    PyObject *item = PyList_GetItem( items, q );
    std::string name = PyBytes_AsString( PyUnicode_AsASCIIString( key ) );
    if( name == "width" ) {
      auto widthItem = PyTuple_GetItem( item, 1 );
      width = PyLong_AsLong( widthItem );
    } else if( name == "height" ) {
      auto heightItem = PyTuple_GetItem( item, 1 );
      height = PyLong_AsLong( heightItem );
    } else if( name == "data" ) {
      PyObject *bytes = PyTuple_GetItem( item, 1 );
      size_t length = PyBytes_Size( bytes );
      rgbaData.Alloc( length );
      memcpy( rgbaData.GetData(), PyBytes_AsString( bytes ), rgbaData.GetLength() );
    }
  }

  return width && height && rgbaData.GetLength();
}//GetImageFromArguments


PyObject* Module::rgba2dxt1( PyObject *self, PyObject *args ) {
  return Module::DoDXTCompressFromArgs( args, squish::kDxt1 );
}//rgba2dxt1


PyObject* Module::rgba2dxt3( PyObject *self, PyObject *args ) {
  return Module::DoDXTCompressFromArgs( args, squish::kDxt3 );
}//rgba2dxt3


PyObject* Module::rgba2dxt5( PyObject *self, PyObject *args ) {
  return Module::DoDXTCompressFromArgs( args, squish::kDxt5 );
}//rgba2dxt5


PyObject* Module::DoDXTCompressFromArgs( PyObject* M_IN args, int format ) {
  Memory compressedData;
  size_t
    width = 0,
    height = 0;
  if( Module::CompressSquish( args, compressedData, width, height, format ) ) {
    return Py_BuildValue( "{s:y#,s:i,s:i,s:i}", "data", compressedData.GetData(), compressedData.GetLength(), "width", width, "height", height, "length", compressedData.GetLength() );
  }

  return Py_BuildValue( "i", 0 );
}//DoDXTCompressFromArgs


bool Module::CompressSquish( PyObject* M_IN args, Memory M_OUT &compressedData, size_t M_OUT &width, size_t M_OUT &height, int format ) {
  Memory rgbaData;
  GetImageFromArguments( args, width, height, rgbaData );
  //LOGI( "image size[%dx%d] length[%d]", width, height, rgbaData.GetLength() );

  if( width % 4 || height % 4 ) {
    LOGE( "Bad image size[%dx%d]: must be multiplies by 4", width, height );
    return false;
  }

  const size_t resultSize = squish::GetStorageRequirements( width, height, format );
  compressedData.Alloc( resultSize );
  squish::CompressImage( rgbaData.GetData(), width, height, compressedData.GetData(), format );

  return true;
}//CompressSquish
