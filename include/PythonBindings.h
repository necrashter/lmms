#ifndef LMMS_PYTHON_BINDINGS_H
#define LMMS_PYTHON_BINDINGS_H

#include "Engine.h"
#include "GuiApplication.h"
#include "MidiClip.h"
#include "PianoRoll.h"
#include "Song.h"
#include "SongEditor.h"
#include "LmmsTypes.h"

// Python.h conflicts with Qt's "slots" keyword
#pragma push_macro("slots")
#undef slots
#include "Python.h"
#pragma pop_macro("slots")

namespace lmms::gui
{

static PyObject* LmmsError;

static int clearMidiClip(MidiClip* clip)
{
	if (clip == nullptr) return 1;
	clip->clearNotes();
	return 0;
}

static int clearPianoRoll()
{
	return clearMidiClip(getGUI()->pianoRoll()->getMidiClip());
}

static int addToMidiClip(MidiClip* clip, int len, int pos, int key, volume_t vol, panning_t pan)
{
	if (clip == nullptr) return 1;

	Note new_note(TimePos(len), TimePos(pos), key);
	new_note.setVolume(vol);
	new_note.setPanning(pan);
	clip->addNote(new_note);

	getGUI()->pianoRoll()->update();
	getGUI()->songEditor()->update();
	Engine::getSong()->setModified();
	return 0;
}

static int addToPianoRoll(int len, int pos, int key, volume_t vol, panning_t pan)
{
	return addToMidiClip(getGUI()->pianoRoll()->getMidiClip(), len, pos, key, vol, pan);
}

static int getTrackCount()
{
	return static_cast<int>(Engine::getSong()->tracks().size());
}

static PyObject* emb_addToPianoRoll(PyObject* /*self*/, PyObject* args)
{
	PyObject* a = PyObject_GetItem(args, PyLong_FromLong(0));

	if (addToPianoRoll(
		static_cast<int>(PyLong_AsLong(PyMapping_GetItemString(a, "len"))),
		static_cast<int>(PyLong_AsLong(PyMapping_GetItemString(a, "pos"))),
		static_cast<int>(PyLong_AsLong(PyMapping_GetItemString(a, "key"))),
		static_cast<volume_t>(PyLong_AsLong(PyMapping_GetItemString(a, "vol"))),
		static_cast<panning_t>(PyLong_AsLong(PyMapping_GetItemString(a, "pan")))
	) != 0)
	{
		PyErr_SetString(LmmsError, "Note addition failed");
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject* emb_clearPianoRoll(PyObject* /*self*/, PyObject* /*args*/)
{
	if (clearPianoRoll() != 0)
	{
		PyErr_SetString(LmmsError, "Piano roll clear failed");
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject* emb_getTrackCount(PyObject* /*self*/, PyObject* /*args*/)
{
	return PyLong_FromLong(getTrackCount());
}

static PyObject* emb_createClip(PyObject* /*self*/, PyObject* args)
{
	long tnum, pos;
	if (!PyArg_ParseTuple(args, "ll", &tnum, &pos))
		return nullptr;
	auto& tracks = Engine::getSong()->tracks();
	Clip* clip = tracks[tnum]->createClip(TimePos(static_cast<int>(pos)));
	clip->movePosition(TimePos(static_cast<int>(pos)));
	return PyCapsule_New(static_cast<void*>(clip), nullptr, nullptr);
}

static PyObject* emb_appendClip(PyObject* /*self*/, PyObject* args)
{
	long tnum;
	if (!PyArg_ParseTuple(args, "l", &tnum))
		return nullptr;
	auto& tracks = Engine::getSong()->tracks();
	const auto& clips = tracks[tnum]->getClips();
	TimePos pos = clips.empty() ? TimePos(0) : clips.back()->endPosition();
	Clip* clip = tracks[tnum]->createClip(pos);
	clip->movePosition(pos);
	return PyCapsule_New(static_cast<void*>(clip), nullptr, nullptr);
}

static PyObject* emb_clearTrack(PyObject* /*self*/, PyObject* args)
{
	long tnum;
	if (!PyArg_ParseTuple(args, "l", &tnum))
		return nullptr;
	auto& tracks = Engine::getSong()->tracks();
	Track* t = tracks[tnum];
	t->addJournalCheckPoint();
	t->lock();
	t->deleteClips();
	t->unlock();
	Py_RETURN_NONE;
}

static PyObject* emb_addToClip(PyObject* /*self*/, PyObject* args)
{
	PyObject* capsule = PyObject_GetItem(args, PyLong_FromLong(0));
	PyObject* a       = PyObject_GetItem(args, PyLong_FromLong(1));

	auto* clip = static_cast<MidiClip*>(PyCapsule_GetPointer(capsule, nullptr));

	if (addToMidiClip(clip,
		static_cast<int>(PyLong_AsLong(PyMapping_GetItemString(a, "len"))),
		static_cast<int>(PyLong_AsLong(PyMapping_GetItemString(a, "pos"))),
		static_cast<int>(PyLong_AsLong(PyMapping_GetItemString(a, "key"))),
		static_cast<volume_t>(PyLong_AsLong(PyMapping_GetItemString(a, "vol"))),
		static_cast<panning_t>(PyLong_AsLong(PyMapping_GetItemString(a, "pan")))
	) != 0)
	{
		PyErr_SetString(LmmsError, "Note addition failed");
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyMethodDef EmbMethods[] = {
	{"addToPianoRoll", emb_addToPianoRoll, METH_VARARGS, "Add note to piano roll"},
	{"clearPianoRoll", emb_clearPianoRoll, METH_VARARGS, "Clear piano roll"},
	{"getTrackCount",  emb_getTrackCount,  METH_VARARGS, "Return number of song tracks"},
	{"createClip",     emb_createClip,     METH_VARARGS, "Create a clip at a position on a track"},
	{"clearTrack",     emb_clearTrack,     METH_VARARGS, "Delete all clips from a track"},
	{"appendClip",     emb_appendClip,     METH_VARARGS, "Append a clip at the end of a track"},
	{"addToClip",      emb_addToClip,      METH_VARARGS, "Add a note to a clip"},
	{nullptr, nullptr, 0, nullptr}
};

static PyModuleDef EmbModule = {
	PyModuleDef_HEAD_INIT, "lmms", nullptr, -1, EmbMethods,
	nullptr, nullptr, nullptr, nullptr
};

static PyObject* PyInit_lmms()
{
	PyObject* m = PyModule_Create(&EmbModule);
	if (m == nullptr)
		return nullptr;

	LmmsError = PyErr_NewException("lmms.error", nullptr, nullptr);
	Py_INCREF(LmmsError);
	PyModule_AddObject(m, "error", LmmsError);
	return m;
}

} // namespace lmms::gui

#endif // LMMS_PYTHON_BINDINGS_H
