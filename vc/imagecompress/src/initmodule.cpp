#include "module.h"

#define GETSTATE( m ) ( ( struct moduleState* ) PyModule_GetState ( m ) )

struct moduleState {
  PyObject *error;
};

static PyMethodDef ModuleMethods[] = {
  { "func123",  Module::Func123, METH_VARARGS, "Module test function." },
  { NULL, NULL, 0, NULL }
};

static int imageCompressTraverse( PyObject *m, visitproc visit, void *arg ) {
  Py_VISIT( GETSTATE( m )->error );
  return 0;
}//imageCompressTraverse

static int imageCompressClear( PyObject *m ) {
  Py_CLEAR( GETSTATE( m )->error );
  return 0;
}//imageCompressClear

static struct PyModuleDef moduleDef = {
  PyModuleDef_HEAD_INIT,
  "imagecompress",
  NULL,
  sizeof( struct moduleState ),
  ModuleMethods,
  NULL,
  imageCompressTraverse,
  imageCompressClear,
  NULL
};

PyMODINIT_FUNC PyInit_imagecompress() {
  PyObject *module = PyModule_Create( &moduleDef );
  if( module == NULL ) {
    return NULL;
  }
  struct moduleState *st = GETSTATE( module );
  st->error = PyErr_NewException( "imagecompress.Error", NULL, NULL );
  if( st->error == NULL ) {
    Py_DECREF( module );
    return NULL;
  }

  return module;
}//PyInit_imagecompress