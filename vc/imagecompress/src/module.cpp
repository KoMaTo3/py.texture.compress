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

PyObject* Module::jpg2rgba( PyObject *self, PyObject *args ) {
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

  //decode
  jpeg_decompress_struct	    cinfo;
  ImgJpg::_gImporterJPEGError	jerr;

  MemoryReader file;
  file.SetSource( memory.GetData(), memory.GetLength() );

  cinfo.err = jpeg_std_error( &jerr.pub );		  // устанавливаем дефолтный менеджер обработки ошибок
  jerr.pub.error_exit = ImgJpg::JPEGErrorExit;	// присваиваем дефолтную функцию для обработки ошибки

  if( setjmp(jerr.setjmp_buffer) )
  {
	  jpeg_destroy_decompress( &cinfo );
	  file.Free();
    return false;
  }
  jpeg_create_decompress( &cinfo );
  ImgJpg::JPEGStdioSrcgMemoryReader( &cinfo, &file );
  jpeg_read_header( &cinfo, TRUE );
  jpeg_start_decompress( &cinfo );
  int row_stride = cinfo.output_width * cinfo.output_components;
  Memory temp_mem( cinfo.output_height * row_stride );
  JSAMPARRAY bufferNew = ( *cinfo.mem->alloc_sarray )( ( j_common_ptr ) &cinfo, JPOOL_IMAGE, row_stride, 1 );
  memset( *bufferNew, 0, sizeof(JSAMPLE) * row_stride );

  int y = 0;
  while( cinfo.output_scanline < cinfo.output_height )
  {
	  jpeg_read_scanlines( &cinfo, bufferNew, 1 );
    uint8_t* temp = ( uint8_t* ) temp_mem.GetData() + ( ( cinfo.output_height - y - 1 ) * row_stride );
	  memcpy( temp, bufferNew[0], row_stride );
	  ++y;
  }

  width = cinfo.image_width;
  height = cinfo.image_height;

  {
    uint32_t x, y;
    int pos3b, pos4b;
    imageDataRGBA.Alloc( height * width * 4 );
    uint8_t *src_mem = (uint8_t*) temp_mem.GetData(), *dest_mem = (uint8_t*) imageDataRGBA.GetData();
    for( y = 0; y < height; ++y )
    for( x = 0; x < width; ++x )
    {
      pos3b = ( x + y * width ) * 3;
      //pos4b = ( x + ( /* height - y - 1 */ y ) * width ) * 4;
      pos4b = ( x + ( height - y - 1 ) * width ) * 4;
      dest_mem[ pos4b    ] = src_mem[ pos3b + 0 ];
      dest_mem[ pos4b + 1] = src_mem[ pos3b + 1 ];
      dest_mem[ pos4b + 2] = src_mem[ pos3b + 2 ];
      dest_mem[ pos4b + 3] = 255;
    }
  }
  temp_mem.Free();

  jpeg_finish_decompress( &cinfo );
  jpeg_destroy_decompress( &cinfo );
  file.Free();

  return Py_BuildValue( "{s:y#,s:i,s:i,s:i}", "data", imageDataRGBA.GetData(), imageDataRGBA.GetLength(), "width", width, "height", height, "length", imageDataRGBA.GetLength() );
}//jpg2rgba


PyObject* Module::png2rgba( PyObject *self, PyObject *args ) {
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

  //decode
  png_structp   png_ptr;
  png_infop     info_ptr;
  png_uint_32   img_width = 0, img_height = 0;
  int           bit_depth = 0, color_type = 0, interlace_type = 0;

  MemoryReader memReader;
  memReader.SetSource( memory.GetData(), memory.GetLength() );
  Memory data_mem( 256 );
  char *pdata = ( char* ) data_mem.GetData();

  memReader.Read( pdata, 4 );
  if( !png_check_sig( ( uint8_t* ) pdata ,4 ) )
  {
    memReader.Free();
    return false;
  }
  png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, 0, 0, 0 );
  if( png_ptr == NULL )
  {
    memReader.Free();
    return false;
  }
  info_ptr = png_create_info_struct( png_ptr );
  if( info_ptr == NULL )
  {
    memReader.Free();
    return false;
  }
  memReader.SeekFromStart( 4 );
  png_set_read_fn( png_ptr, ( void* ) &memReader, ImgPng::PNGReadFunctiongMemoryReader );//!!!

  png_set_sig_bytes( png_ptr, 4 );

  // Тут можно настроить прогрессбар, таким макаром:
  // описываем функцию где-то выше
  // void read_row_callback(png_ptr, png_uint_32 row, int pass) { }
  // информируем библиотеку об этой функции
  //	png_set_read_status_fn(png_ptr, read_row_callback);

  png_read_info( png_ptr, info_ptr );
  png_get_IHDR( png_ptr, info_ptr, &img_width, &img_height, &bit_depth,
    &color_type, &interlace_type, 0, 0 );
  if( bit_depth == 16 )
    png_set_strip_16( png_ptr );
  if( color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8 )
    png_set_palette_to_rgb( png_ptr );
  if( color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8 )
    png_set_gray_1_2_4_to_8( png_ptr );
  if( png_get_valid( png_ptr, info_ptr, PNG_INFO_tRNS ) )
    png_set_tRNS_to_alpha( png_ptr );
  if( color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
    png_set_gray_to_rgb( png_ptr );
  double gamma = 0.0;
  if( png_get_gAMA( png_ptr, info_ptr, &gamma ) )
    png_set_gamma( png_ptr, 2.2, gamma );
  else
    png_set_gamma( png_ptr, 2.2, 0.45455 );
  png_read_update_info( png_ptr, info_ptr );
  png_get_IHDR( png_ptr, info_ptr, &img_width, &img_height, &bit_depth, &color_type, 0, 0, 0 );
  png_uint_32 row_bytes = png_get_rowbytes( png_ptr, info_ptr );
  data_mem.Alloc( img_height * row_bytes );
  pdata = ( char* ) data_mem.GetData();
  png_byte **row_pointers = new png_byte * [ img_height ];
  for( unsigned int i = 0; i < img_height; ++i )
    row_pointers[ i ] = ( uint8_t* ) pdata + i * row_bytes;
  png_read_image( png_ptr, row_pointers );
  png_destroy_read_struct( &png_ptr, 0, 0 );
  if( row_pointers ) {
    delete row_pointers;
    row_pointers = 0;
  }
  memReader.Free();

  width = img_width;
  height = img_height;
  imageDataRGBA.Alloc( width * height * 4 );

  //__log.PrintInfo( Filelevel_DEBUG, "color_type = %d, interlace_type = %d", color_type, interlace_type );
  if( color_type == PNG_COLOR_TYPE_RGB )  //2
  {
    uint32_t x, y, pos1, pos2;
    Byte *dataDest = ( Byte* ) imageDataRGBA.GetData();
    for( y = 0; y < height; ++y )
    for( x = 0; x < width; ++x )
    {
      pos1 = x * 3 + row_bytes * y;
      pos2 = ( x + y * width ) << 2;
      dataDest[ pos2     ] = pdata[ pos1 + 0 ];
      dataDest[ pos2 + 1 ] = pdata[ pos1 + 1 ];
      dataDest[ pos2 + 2 ] = pdata[ pos1 + 2 ];
      dataDest[ pos2 + 3 ] = 0xFF;
    }
  }
  else
  if( color_type == PNG_COLOR_TYPE_RGB_ALPHA )  //6
  {
    uint32_t x, y, pos;
    Byte *dataDest = ( Byte* ) imageDataRGBA.GetData();
    for( y = 0; y < height; ++y)
    for( x = 0; x < width; ++x)
    {
      pos = ( x + y * width ) << 2;
      dataDest[ pos     ] = pdata[ pos + 0 ];
      dataDest[ pos + 1 ] = pdata[ pos + 1 ];
      dataDest[ pos + 2 ] = pdata[ pos + 2 ];
      dataDest[ pos + 3 ] = pdata[ pos + 3 ];
    }
  }

  return Py_BuildValue( "{s:y#,s:i,s:i,s:i}", "data", imageDataRGBA.GetData(), imageDataRGBA.GetLength(), "width", width, "height", height, "length", imageDataRGBA.GetLength() );
}

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



const int ImgJpg::c_JPEGInputBufferSize = 4096;

// наша функция обработки ошибок
void ImgJpg::JPEGErrorExit(j_common_ptr cinfo)
{
    // получаем ссылочку на нашу структуру
  ImgJpg::_gImporterJPEGError* myerr = (ImgJpg::_gImporterJPEGError*)cinfo->err;
    // выводим сообщение об ошибке (наверное можно убрать)
    (*cinfo->err->output_message)(cinfo);
    // делаем прыжок на очистку данных и ретурн ошибки
    longjmp(myerr->setjmp_buffer, 1);
}

/*
struct _gImporterJPEGSourcegFile
{
    jpeg_source_mgr  pub;    // ссылочка на стандартный менеджер
    MemoryReader      *file;  // открытый файл
    JOCTET    *buffer;  // буфер данных
    bool      sof;    // признак того, что файл только что открыли
};
*/

struct _gImporterJPEGSourcegMemoryReader
{
    jpeg_source_mgr  pub;    // ссылочка на стандартный менеджер
    MemoryReader *file;  // открытый 'файл'
    JOCTET    *buffer;  // буфер данных
    bool      sof;    // признак того, что файл только что открыли
};

// вызывается, когда переменная bytes_in_buffer достигает 0 и возникает
// необходимость в новой порции информации. возвращает TRUE, если буфер
// перезаполнен успешно, и FALSE если произошла ошибка ввода/вывода.
/*
boolean JPEGFillInputBuffergFile(j_decompress_ptr cinfo)
{
  _gImporterJPEGSourcegFile* src = (_gImporterJPEGSourcegFile*)cinfo->src;
  // читаем кусками по ImgJpg::c_JPEGInputBufferSize байт
  size_t nbytes = src->file->Read(src->buffer, sizeof(JOCTET), ImgJpg::c_JPEGInputBufferSize);
  // если мы ничего не считали :(
  if (nbytes <= 0) {
    if ( src->sof )  return(FALSE); // блин, нам дали пустой файл - заорем "нехорошо" :)
    // если уже читали до этого, то вставляем в буфер инфу о конце файла
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    nbytes = 2;
  }

  // загоняем инфу в буфер, и размер скока прочли
  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = nbytes;
  src->sof = false;  // файл не пустой, пронесло :)))
  // возвращаем успешное выполнение операции
  return(TRUE);
}
*/

boolean JPEGFillInputBuffergMemoryReader(j_decompress_ptr cinfo)
{
  _gImporterJPEGSourcegMemoryReader* src = (_gImporterJPEGSourcegMemoryReader*)cinfo->src;
  // читаем кусками по ImgJpg::c_JPEGInputBufferSize байт
  size_t nbytes = src->file->Read(src->buffer, sizeof(JOCTET), ImgJpg::c_JPEGInputBufferSize);
  // если мы ничего не считали :(
  if (nbytes <= 0) {
    if ( src->sof )  return(FALSE); // блин, нам дали пустой файл - заорем "нехорошо" :)
    // если уже читали до этого, то вставляем в буфер инфу о конце файла
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    nbytes = 2;
  }

  // загоняем инфу в буфер, и размер скока прочли
  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = nbytes;
  src->sof = false;  // файл не пустой, пронесло :)))
  // возвращаем успешное выполнение операции
  return(TRUE);
}

// инициализация источника. вызывается до того, как какая-нибудь информация
// будет из него прочтена.
/*
void JPEGInitSourcegFile(j_decompress_ptr cinfo)
{
	_gImporterJPEGSourcegFile* src = (_gImporterJPEGSourcegFile*)cinfo->src;
	// говорим, шо файл тока шо открыт, выдыруг он пустой? :)
	src->sof = true;
}
*/

void JPEGInitSourcegMemoryReader(j_decompress_ptr cinfo)
{
	_gImporterJPEGSourcegMemoryReader* src = (_gImporterJPEGSourcegMemoryReader*)cinfo->src;
	// говорим, шо файл тока шо открыт, выдыруг он пустой? :)
	src->sof = true;
}

// происходит, когда необходимо пропустить num_bytes. в случае опустошения
// буфера, его необходимо перезагрузить.
/*
void JPEGSkipInputDatagFile(j_decompress_ptr cinfo, long num_bytes)
{
	_gImporterJPEGSourcegFile* src = (_gImporterJPEGSourcegFile*)cinfo->src;
	// если нужно снести 0 байт :) обижено уходим
	if (num_bytes <= 0) return;
	// выкидываем инфу из буфера и перегружаем его, пока num_bytes не станет
	// меньше размера буфера
	while (num_bytes > (long) src->pub.bytes_in_buffer) {
		num_bytes -= (long) src->pub.bytes_in_buffer;
		JPEGFillInputBuffergFile(cinfo);
	}
	// а теперь просто правильно настраиваем указатели на оставшуюся часть
	src->pub.next_input_byte += (size_t) num_bytes;
	src->pub.bytes_in_buffer -= (size_t) num_bytes;
}
*/

void JPEGSkipInputDatagMemoryReader(j_decompress_ptr cinfo, long num_bytes)
{
	_gImporterJPEGSourcegMemoryReader* src = (_gImporterJPEGSourcegMemoryReader*)cinfo->src;
	// если нужно снести 0 байт :) обижено уходим
	if (num_bytes <= 0) return;
	// выкидываем инфу из буфера и перегружаем его, пока num_bytes не станет
	// меньше размера буфера
	while (num_bytes > (long) src->pub.bytes_in_buffer) {
		num_bytes -= (long) src->pub.bytes_in_buffer;
		JPEGFillInputBuffergMemoryReader(cinfo);
	}
	// а теперь просто правильно настраиваем указатели на оставшуюся часть
	src->pub.next_input_byte += (size_t) num_bytes;
	src->pub.bytes_in_buffer -= (size_t) num_bytes;
}

// убить ресурс. вызывается функцией jpeg_finish_decompress когда все данные будут
// прочтены. у нас ничего сносить не надо.
void JPEGTermSource(j_decompress_ptr cinfo) { }

/*
void JPEGStdioSrcgFile(j_decompress_ptr cinfo, MemoryReader* file)
{
  _gImporterJPEGSourcegFile* src = 0;
  // смотрим, выделена ли память под JPEG-декомпрессор менеджер?
  // возможна ситуация, когда происходит одновременное обращение к источнику
  // от нескольких библиотек
  if (cinfo->src == 0) {
    // выделим память под наш менеджер, и установим на него указатель глобальной структуры
    // библиотеки. так как я использую менеджер памяти библиотеки JPEG то позаботится об
    // освобождении она сама. JPOOL_PERMANENT - означает что эта память выделяется на все
    // время работы с библиотекой
    cinfo->src = (struct jpeg_source_mgr *) (*cinfo->mem->alloc_small) 
      ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(_gImporterJPEGSourcegFile));
    src = (_gImporterJPEGSourcegFile*) cinfo->src;
    // выделяю память для буффера данных, прочитанных из файла
    src->buffer = (JOCTET*) (*cinfo->mem->alloc_small) 
      ((j_common_ptr) cinfo, JPOOL_PERMANENT, ImgJpg::c_JPEGInputBufferSize * sizeof(JOCTET));
    memset(src->buffer, 0, ImgJpg::c_JPEGInputBufferSize * sizeof(JOCTET));
  }
  // для краткости - сестры таланта
  src = (_gImporterJPEGSourcegFile*)cinfo->src;
  // настраиваем обработчики событий на наши функции
  src->pub.init_source = JPEGInitSourcegFile;
  src->pub.fill_input_buffer = JPEGFillInputBuffergFile;
  src->pub.skip_input_data = JPEGSkipInputDatagFile;
  src->pub.resync_to_restart = jpeg_resync_to_restart; // use default method
  src->pub.term_source = JPEGTermSource;
  // теперь заполняем поля нашей структуры
  src->file = file;
  // настраиваем указатели на буфера
  src->pub.bytes_in_buffer = 0;  // forces fill_input_buffer on first read
  src->pub.next_input_byte = 0;  // until buffer loaded
}
*/


void ImgJpg::JPEGStdioSrcgMemoryReader(j_decompress_ptr cinfo, MemoryReader* file)
{
  _gImporterJPEGSourcegMemoryReader * src = 0;
  // смотрим, выделена ли память под JPEG-декомпрессор менеджер?
  // возможна ситуация, когда происходит одновременное обращение к источнику
  // от нескольких библиотек
  if (cinfo->src == 0) {
    // выделим память под наш менеджер, и установим на него указатель глобальной структуры
    // библиотеки. так как я использую менеджер памяти библиотеки JPEG то позаботится об
    // освобождении она сама. JPOOL_PERMANENT - означает что эта память выделяется на все
    // время работы с библиотекой
    cinfo->src = (struct jpeg_source_mgr *) (*cinfo->mem->alloc_small) 
      ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(_gImporterJPEGSourcegMemoryReader));
    src = (_gImporterJPEGSourcegMemoryReader*) cinfo->src;
    // выделяю память для буффера данных, прочитанных из файла
    src->buffer = (JOCTET*) (*cinfo->mem->alloc_small) 
      ((j_common_ptr) cinfo, JPOOL_PERMANENT, ImgJpg::c_JPEGInputBufferSize * sizeof(JOCTET));
    memset(src->buffer, 0, ImgJpg::c_JPEGInputBufferSize * sizeof(JOCTET));
  }
  // для краткости - сестры таланта
  src = (_gImporterJPEGSourcegMemoryReader*)cinfo->src;
  // настраиваем обработчики событий на наши функции
  src->pub.init_source = JPEGInitSourcegMemoryReader;
  src->pub.fill_input_buffer = JPEGFillInputBuffergMemoryReader;
  src->pub.skip_input_data = JPEGSkipInputDatagMemoryReader;
  src->pub.resync_to_restart = jpeg_resync_to_restart; // use default method
  src->pub.term_source = JPEGTermSource;
  // теперь заполняем поля нашей структуры
  src->file = file;
  // настраиваем указатели на буфера
  src->pub.bytes_in_buffer = 0;  // forces fill_input_buffer on first read
  src->pub.next_input_byte = 0;  // until buffer loaded
}

/*
void ImgPng::PNGReadFunctiongFile( png_structp png_ptr, png_bytep data, png_size_t length )
{
  File *file = ( File* ) png_get_io_ptr( png_ptr );
  file->Read( data, Dword( length ) );
}
*/

void ImgPng::PNGReadFunctiongMemoryReader( png_structp png_ptr, png_bytep data, png_size_t length )
{
  MemoryReader *file = ( MemoryReader* ) png_get_io_ptr( png_ptr );
  file->Read( data, long( length ) );
}
