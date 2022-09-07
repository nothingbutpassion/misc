
/**NOTES:
1) Build: 
a. manual build:
    g++ -I/usr/include/python3.6 py_extention_example.cpp -shared -o example.so
b. Or write setup.py and then run "python3 setup.py build" 
    from distutils.core import setup, Extension
    module = Extension('example', sources = ['py_extention_example.cpp'])
    setup(name = 'example', version = '0.1', description = 'Just an example extention', ext_modules = [module])     

2) Usage: 
a. First set env: 
    export PYTHONPATH=<the dir of example.so>:$PYTHONPATH
b. Then in python script:
    import example
    example.hello()
    example.add(1, 1)
    example.system('ls -l')
    example.show_time(1980, 12, 12)
    example.show_time(year=1980, month=10, day=10, hour=11, minute=11)
*/

#define PY_SSIZE_T_CLEAN        // Make "s#" use Py_ssize_t rather than int.
#include <Python.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

static PyObject* example_hello(PyObject* self) {
    return Py_BuildValue("s", "Hello, Python extension!!!");
}

static PyObject* example_add(PyObject* self, PyObject* args) {
    int a;
    int b;
    if (!PyArg_ParseTuple(args, "ii", &a, &b))
        return NULL;
    return Py_BuildValue("i", a + b);
}

static PyObject* example_system(PyObject* self, PyObject* args) {
    const char* command;
    if (!PyArg_ParseTuple(args, "s", &command))
        return NULL;
    int ret = system(command);
    if (ret < 0)
        return NULL;
    return PyLong_FromLong(ret);
}

static PyObject* example_show_time(PyObject* self, PyObject* args, PyObject* keywds)
{
    int year;
    int month;
    int day;
    int hour = 0;
    int minute = 0;
    static char *kwlist[] = {"year", "month", "day", "hour", "minute", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "iii|ii", kwlist, &year, &month, &day, &hour, &minute))
        return NULL;
    printf("time: %04d-%2d-%2d %02d:%02d\n", year, month, day, hour, minute);
    Py_RETURN_NONE;
}


static PyMethodDef methods[] = {
    {"hello",       (PyCFunction)(void(*)(void))example_hello,  METH_NOARGS,    "Return a hello string"},
    {"add",         example_add,                                METH_VARARGS,   "Add two integer numer."},
    {"system",      example_system,                             METH_VARARGS,   "Execute a shell command."},
    {"show_time",  (PyCFunction)(void(*)(void))example_show_time,  METH_VARARGS|METH_KEYWORDS,   "Print time with given args"},
    {NULL, NULL, 0, NULL}       // last item flags
};




static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "example",                  // name of module
    "an extention example",     // module documentation, may be NULL
    -1,                         // size of per-interpreter state of the module, 
                                // or -1 if the module keeps state in global variables.
    methods   
};


PyMODINIT_FUNC 
PyInit_example(void)
{
    return PyModule_Create(&module);
}

