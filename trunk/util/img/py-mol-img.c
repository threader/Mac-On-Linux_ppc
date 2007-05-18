/*********************************************
 * Some Python -> C bindings for mol-img
 * 2007 - Joseph Jezak
 *********************************************/

#include <Python.h>
#include "mol-img.h"

#define MB		1024 * 1024

/* Locally raised exceptions */
static PyObject *MOLerror;
#define onError(message) \
	{ PyErr_SetString(ErrorObject, message); return NULL; }

/******************
 * Python Bindings
 ******************/

/* Create a qcow disk image */
static PyObject * create_qcow(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	char * file;
	int size;

	if (!PyArg_ParseTuple(args, "sl", file, &size))
		return NULL;
	
	return Py_BuildValue("i", create_img_qcow(file, size * MB));
}

/* Create a raw disk image */
static PyObject * create_raw(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	char * file;
	int size;

	if (!PyArg_ParseTuple(args, "sl", file, &size))
		return NULL;
	
	return Py_BuildValue("i", create_img_raw(file, size * MB));
}

/* Exported Methods */
static struct PyMethodDef mol_img_methods[] = {
	{ "create_qcow", (PyCFunction)create_qcow, METH_VARARGS, "Create a QCOW disk image"},
	{ "create_raw", (PyCFunction)create_raw, METH_VARARGS, "Create a raw disk image"},
	{ NULL, NULL, 0, NULL},
};

/* Exported Module Methods */
static struct PyMethodDef module_methods[] = {
	{NULL, NULL, 0, NULL}, 
};

static PyTypeObject mol_imgType = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"mol_img.MOL-img",		/* tp_name */	
	0,				/* tp_basic_size */
	0,				/* tp_item_size */	
	0,				/* tp_dealloc */
	0,				/* tp_print */ 
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	0,				/* tp_getattro */
	0,				/* tp_set_attro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	"MOL Image Objects",		/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	0,				/* tp_iter */
	0,				/* tp_iternext */
	mol_img_methods,		/* tp_methods */
	0,
	0,
	0,
	0, 
	0,
	0,
	0,
	0,
	0,
	0,				/* tp_new */	

};

/* Init Module */
PyMODINIT_FUNC
initmol_img(void) {
	PyObject *m;

	/* Create the module and add functions */
	if (PyType_Ready(&mol_imgType) < 0)
		return;

	m = Py_InitModule3("mol-img", module_methods, "Create QCOW and RAW disk images");
	Py_INCREF(&mol_imgType);
	PyModule_AddObject(m, "MOL-IMG", (PyObject *)&mol_imgType);

	MOLerror = PyErr_NewException("mol-img.error", NULL, NULL);
	Py_INCREF(MOLerror);
	PyModule_AddObject(m, "error", MOLerror);
}
