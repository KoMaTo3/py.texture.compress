#pragma once

#include "Python.h"
#include <deque>

class Memory;

class Module {
public:
  static PyObject* Func123( PyObject *self, PyObject *args );
  static PyObject* free( PyObject *self, PyObject *args );

  static PyObject* tga2rgba( PyObject *self, PyObject *args );
  static PyObject* rgba2dxt1( PyObject *self, PyObject *args );

private:
  typedef std::deque< Memory* > MemoryList;
  static MemoryList memoryBlocksList;
};
