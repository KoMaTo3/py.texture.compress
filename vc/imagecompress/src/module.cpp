#include "module.h"

#include "stdio.h"
#include "Python.h"

PyObject* Module::Func123( PyObject *self, PyObject *args ) {
  const char *str;
  if( !PyArg_ParseTuple( args, "s", &str ) ) {
    return NULL;
  }
  //return Py_BuildValue( "i", strlen( str ) );
  return Py_BuildValue( "ssi", str, "ok", 0 );
}//Func123
