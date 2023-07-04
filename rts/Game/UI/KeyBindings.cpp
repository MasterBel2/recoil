/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <algorithm>

#include "KeyBindings.h"
#include "KeyCodes.h"
#include "ScanCodes.h"
#include "KeySet.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"


#define LOG_SECTION_KEY_BINDINGS "KeyBindings"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_KEY_BINDINGS)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_KEY_BINDINGS


CONFIG(int, KeyChainTimeout).defaultValue(750).minimumValue(0).description("Timeout in milliseconds waiting for a key chain shortcut.");


CKeyBindings keyBindings;


struct DefaultBinding {
	const char* key;
	const char* action;
};


static const CKeyBindings::KeyBindingComparison compareActionByTriggerOrder = [](const CKeyBindings::KeyBinding& a, const CKeyBindings::KeyBinding& b) {
	bool selfAnyMod = a.keyChain.back().AnyMod();
	bool bindingAnyMod = b.keyChain.back().AnyMod();

	if (selfAnyMod == bindingAnyMod)
		return a.bindingIndex < b.bindingIndex;
	else
		return bindingAnyMod;
};


static const CKeyBindings::KeyBindingComparison compareActionByBindingOrder = [](const CKeyBindings::KeyBinding& a, const CKeyBindings::KeyBinding& b) {
  return (a.bindingIndex < b.bindingIndex);
};

const std::string CKeyBindings::DEFAULT_FILENAME = "uikeys.txt";

static const DefaultBinding defaultBindings[] = {
	{            "esc", "quitmessage" },
	{      "Shift+esc", "quitmenu"    },
	{ "Ctrl+Shift+esc", "quitforce"   },
	{  "Alt+Shift+esc", "reloadforce" },
	{      "Any+pause", "pause"       },

	{ "c", "controlunit"      },
	{ "Any+h", "sharedialog"  },
	{ "Any+i", "gameinfo"     },

	{ "Any+j",         "mouse2" },
	{ "backspace", "mousestate" },
	{ "Shift+backspace", "togglecammode" },
	{  "Ctrl+backspace", "togglecammode" },
	{         "Any+tab", "toggleoverview" },

	{               "Any+enter", "chat"           },
	// leave this unbound, takes as many keypresses as exiting ally/spec modes
	// { "Alt+ctrl+z,Alt+ctrl+z", "chatswitchall"  },
	{ "Alt+ctrl+a,Alt+ctrl+a", "chatswitchally" },
	{ "Alt+ctrl+s,Alt+ctrl+s", "chatswitchspec" },

	{       "Any+tab", "edit_complete"  },
	{ "Any+backspace", "edit_backspace" },
	{    "Any+delete", "edit_delete"    },
	{      "Any+home", "edit_home"      },
	{      "Alt+left", "edit_home"      },
	{       "Any+end", "edit_end"       },
	{     "Alt+right", "edit_end"       },
	{        "Any+up", "edit_prev_line" },
	{      "Any+down", "edit_next_line" },
	{      "Any+left", "edit_prev_char" },
	{     "Any+right", "edit_next_char" },
	{     "Ctrl+left", "edit_prev_word" },
	{    "Ctrl+right", "edit_next_word" },
	{     "Any+enter", "edit_return"    },
	{    "Any+escape", "edit_escape"    },

	{ "Ctrl+v", "pastetext" },

	{ "Any+home", "increaseViewRadius" },
	{ "Any+end",  "decreaseViewRadius" },

	{ "Alt+insert",  "speedup"  },
	{ "Alt+delete",  "slowdown" },
	{ "Alt+=",       "speedup"  },
	{ "Alt++",       "speedup"  },
	{ "Alt+-",       "slowdown" },
	{ "Alt+numpad+", "speedup"  },
	{ "Alt+numpad-", "slowdown" },

	{       ",", "prevmenu" },
	{       ".", "nextmenu" },
	{ "Shift+,", "decguiopacity" },
	{ "Shift+.", "incguiopacity" },

	{      "1", "specteam 0"  },
	{      "2", "specteam 1"  },
	{      "3", "specteam 2"  },
	{      "4", "specteam 3"  },
	{      "5", "specteam 4"  },
	{      "6", "specteam 5"  },
	{      "7", "specteam 6"  },
	{      "8", "specteam 7"  },
	{      "9", "specteam 8"  },
	{      "0", "specteam 9"  },
	{ "Ctrl+1", "specteam 10" },
	{ "Ctrl+2", "specteam 11" },
	{ "Ctrl+3", "specteam 12" },
	{ "Ctrl+4", "specteam 13" },
	{ "Ctrl+5", "specteam 14" },
	{ "Ctrl+6", "specteam 15" },
	{ "Ctrl+7", "specteam 16" },
	{ "Ctrl+8", "specteam 17" },
	{ "Ctrl+9", "specteam 18" },
	{ "Ctrl+0", "specteam 19" },

	{ "Any+0", "group0" },
	{ "Any+1", "group1" },
	{ "Any+2", "group2" },
	{ "Any+3", "group3" },
	{ "Any+4", "group4" },
	{ "Any+5", "group5" },
	{ "Any+6", "group6" },
	{ "Any+7", "group7" },
	{ "Any+8", "group8" },
	{ "Any+9", "group9" },

	{       "[", "buildfacing inc"  },
	{ "Shift+[", "buildfacing inc"  },
	{       "]", "buildfacing dec"  },
	{ "Shift+]", "buildfacing dec"  },
	{   "Any+z", "buildspacing inc" },
	{   "Any+x", "buildspacing dec" },

	{            "a", "attack"       },
	{      "Shift+a", "attack"       },
	{        "Alt+a", "areaattack"   },
	{  "Alt+Shift+a", "areaattack"   },
	{        "Alt+b", "debug"        },
	{        "Alt+v", "debugcolvol"  },
	{        "Alt+p", "debugpath"    },
	{            "d", "manualfire"   },
	{      "Shift+d", "manualfire"   },
	{       "Ctrl+d", "selfd"        },
	{ "Ctrl+Shift+d", "selfd queued" },
	{            "e", "reclaim"      },
	{      "Shift+e", "reclaim"      },
	{            "f", "fight"        },
	{      "Shift+f", "fight"        },
	{        "Alt+f", "forcestart"   },
	{            "g", "guard"        },
	{      "Shift+g", "guard"        },
	{            "k", "cloak"        },
	{      "Shift+k", "cloak"        },
	{            "l", "loadunits"    },
	{      "Shift+l", "loadunits"    },
	{            "m", "move"         },
	{      "Shift+m", "move"         },
	{        "Alt+o", "singlestep"   },
	{            "p", "patrol"       },
	{      "Shift+p", "patrol"       },
	{            "q", "groupselect"  },
	{            "q", "groupadd"     },
	{       "Ctrl+q", "aiselect"     },
	{      "Shift+q", "groupclear"   },
	{            "r", "repair"       },
	{      "Shift+r", "repair"       },
	{            "s", "stop"         },
	{      "Shift+s", "stop"         },
	{            "u", "unloadunits"  },
	{      "Shift+u", "unloadunits"  },
	{            "w", "wait"         },
	{      "Shift+w", "wait queued"  },
	{            "x", "onoff"        },
	{      "Shift+x", "onoff"        },


	{  "Ctrl+t", "trackmode" },
	{   "Any+t", "track" },

	{ "Ctrl+f1", "viewfps"  },
	{ "Ctrl+f2", "viewta"   },
	{ "Ctrl+f3", "viewspring" },
	{ "Ctrl+f4", "viewrot"  },
	{ "Ctrl+f5", "viewfree" },

	{ "Any+f1",  "ShowElevation"         },
	{ "Any+f2",  "ShowPathTraversability"},
	{ "Any+f3",  "LastMsgPos"            },
	{ "Any+f4",  "ShowMetalMap"          },
	{ "Any+f5",  "HideInterface"         },
	{ "Any+f6",  "MuteSound"             },
	{ "Any+l",   "togglelos"             },

	{ "Ctrl+Shift+f8",  "savegame" },
	{ "Ctrl+Shift+f10", "createvideo" },
	{ "Any+f11", "screenshot"     },
	{ "Any+f12", "screenshot"     },
	{ "Alt+enter", "fullscreen"  },

	{ "Any+`,Any+`",    "drawlabel" },
	{ "Any+\\,Any+\\",  "drawlabel" },
	{ "Any+~,Any+~",    "drawlabel" },
	{ "Any+§,Any+§",    "drawlabel" },
	{ "Any+^,Any+^",    "drawlabel" },

	{    "Any+`",    "drawinmap"  },
	{    "Any+\\",   "drawinmap"  },
	{    "Any+~",    "drawinmap"  },
	{    "Any+§",    "drawinmap"  },
	{    "Any+^",    "drawinmap"  },

	{    "Any+up",       "moveforward"  },
	{    "Any+down",     "moveback"     },
	{    "Any+right",    "moveright"    },
	{    "Any+left",     "moveleft"     },
	{    "Any+pageup",   "moveup"       },
	{    "Any+pagedown", "movedown"     },

	{    "Any+ctrl",     "moveslow"     }, // decreases delta for move/zoom camera transitions
	{    "Any+shift",    "movefast"     }, // increases delta for move/zoom camera transitions

	{    "Any+ctrl",     "movetilt"     }, // rotates the camera over the x axis on mousewheel move
	{    "Any+alt",      "movereset"    }, // resets camera state to maxzoom/minzoom on mousewheel move, additionally resets tilt on Overhead cam
	{    "Any+alt",      "moverotate"   }, // rotates the camera in x and y axis on mmb move (Spring cam)

	// selection keys
	{ "Ctrl+a",    "select AllMap++_ClearSelection_SelectAll+"                                         },
	{ "Ctrl+b",    "select AllMap+_Builder_Idle+_ClearSelection_SelectOne+"                            },
	{ "Ctrl+c",    "select AllMap+_ManualFireUnit+_ClearSelection_SelectOne+"                          },
	{ "Ctrl+r",    "select AllMap+_Radar+_ClearSelection_SelectAll+"                                   },
	{ "Ctrl+v",    "select AllMap+_Not_Builder_Not_Commander_InPrevSel_Not_InHotkeyGroup+_SelectAll+"  },
	{ "Ctrl+w",    "select AllMap+_Not_Aircraft_Weapons+_ClearSelection_SelectAll+"                    },
	{ "Ctrl+x",    "select AllMap+_InPrevSel_Not_InHotkeyGroup+_SelectAll+"                            },
	{ "Ctrl+z",    "select AllMap+_InPrevSel+_ClearSelection_SelectAll+"                               }
};


/******************************************************************************/
//
// CKeyBindings
//

void CKeyBindings::Init()
{
	fakeMetaKey = -1;
	keyChainTimeout = 750;

	buildHotkeyMap = true;
	debugEnabled = false;

	codeBindings.reserve(32);
	scanBindings.reserve(32);
	hotkeys.reserve(32);

	statefulCommands.reserve(16);
	statefulCommands.insert("drawinmap");
	statefulCommands.insert("moveforward");
	statefulCommands.insert("moveback");
	statefulCommands.insert("moveright");
	statefulCommands.insert("moveleft");
	statefulCommands.insert("moveup");
	statefulCommands.insert("movedown");
	statefulCommands.insert("moveslow");
	statefulCommands.insert("movefast");
	statefulCommands.insert("movetilt");
	statefulCommands.insert("movereset");
	statefulCommands.insert("moverotate");

	RegisterAction("bind");
	RegisterAction("unbind");
	RegisterAction("unbindall");
	RegisterAction("unbindaction");
	RegisterAction("unbindkeyset");
	RegisterAction("fakemeta");
	RegisterAction("keydebug");
	RegisterAction("keyload");
	RegisterAction("keyreload");
	RegisterAction("keysave");
	RegisterAction("keysyms");
	RegisterAction("keycodes");
	RegisterAction("keyprint");
	SortRegisteredActions();

	configHandler->NotifyOnChange(this, {"KeyChainTimeout"});
}

void CKeyBindings::Kill()
{
	codeBindings.clear();
	scanBindings.clear();
	hotkeys.clear();
	loadStack.clear();
	statefulCommands.clear();

	configHandler->RemoveObserver(this);
}


/******************************************************************************/

void FilterByKeychain(const CKeyBindings::KeyBindingList & in, const CKeyChain & kc, CKeyBindings::KeyBindingList & out)
{
	for (const CKeyBindings::KeyBinding& action: in)
		if (kc.fit(action.keyChain))
			out.push_back(action);
}


void MergeActionListsByTrigger(const CKeyBindings::KeyBindingList& keyBindingListA, const CKeyBindings::KeyBindingList& keyBindingListB, CKeyBindings::KeyBindingList & out)
{
	// When we are retrieving actionlists for a given keyboard state we need to
	// remove duplicate actions that might arise from binding the same action
	// to both key and scancodes
	//
	// If the user bound same action to both keys and scancodes we want to not
	// include action duplicates keeping the action with lower bindingIndex.
	//
	// A duplicate is an action for which theres a correspondent and only one
	// identical action present, but bound to a different key.
	// This is guaranteed by the logic in ::Bind, which also imples it will
	// invariably be the opposite of the action keytype (scancode<->keycode)
	// Duplicates will not exist in a single list but would exist if both lists
	// were merged without care.
	//
	// For each list we have a list of ordered actions that is partitioned in two sections:
	// [k, k, ..., Any+k, Any+k,...]

	// We assume that both lists are already sorted by binding id.
	// We assume that each list does not contain, itself, duplicates.
	// We assume that the two lists are either both non-Any lists, or both Any lists.
	//
	// Their merged result is appended to out.
	if (keyBindingListA.empty()) {
		out.insert(std::end(out), std::begin(keyBindingListB), std::end(keyBindingListB));
		return;
	}

	// Only items from list A
	size_t aBeginId = out.size();
	out.insert(std::end(out), std::begin(keyBindingListA), std::end(keyBindingListA));
	size_t aEndId = out.size();

	if (keyBindingListB.empty()) return;

	// Only add items from list B
	// - If there are duplicates with >= bindingId, don't add them
	// - If there are duplicates with < bindingId, remove the A item
	for (const auto & aB : keyBindingListB) {
		bool toAdd = true;
		for (size_t a = aBeginId; a < aEndId; ++a) {
			// B is a duplicate...
			if (aB.action.line == out[a].action.line) {
				// ...with a higher id, so do not add it.
				if (aB.bindingIndex >= out[a].bindingIndex) {
					toAdd = false;
				// ...with a lower id, so remove its A equivalent.
				} else {
					out.erase(std::next(std::begin(out), a));
					--aEndId;
				}
				break;
			}
		}
		if (toAdd) out.push_back(aB);
	}

	// Merge the two parts.
	std::inplace_merge(std::next(std::begin(out), aBeginId), std::next(std::begin(out), aEndId), std::end(out), compareActionByTriggerOrder);
}

ActionList CKeyBindings::KeyBindingListToActionList(const KeyBindingList& keyBindingList)
{
	ActionList actionList;
	actionList.reserve(keyBindingList.size());
	for (int i = 0; i < keyBindingList.size(); i++) {
		actionList.emplace_back(keyBindingList[i].action);
	}
	return actionList;
}

const CKeyBindings::KeyBindingList & CKeyBindings::GetKeyBindingList(const CKeySet& ks, bool forceAny) const
{
	static KeyBindingList empty;

	if (ks.Key() < 0)
		return empty;

	const auto & bindings = ks.IsKeyCode() ? codeBindings : scanBindings;

	CKeySet toUse = ks;
	if (forceAny) toUse.SetAnyBit();

	const auto it = bindings.find(toUse);
	if (it != bindings.end())
		return (it->second);

	return empty;
}


CKeyBindings::KeyBindingList CKeyBindings::GetKeyBindingList(const CKeyChain& kc) const
{
	KeyBindingList out;

	if (kc.empty())
		return out;

	const auto & keyBindingList = GetKeyBindingList(kc.back(), false);
	FilterByKeychain(keyBindingList, kc, out);

	if (!kc.back().AnyMod()) {
		const auto & keyBindingList = GetKeyBindingList(kc.back(), true);
		FilterByKeychain(keyBindingList, kc, out);
	}

	return out;
}


CKeyBindings::KeyBindingList CKeyBindings::GetKeyBindingList(const CKeyChain& kc, const CKeyChain& sc) const
{
	// Recover the actionLists we need to merge.
	KeyBindingList merged;

	// First get non-Any lists.
	KeyBindingList kList, sList;
	if (!kc.back().AnyMod()) FilterByKeychain(GetKeyBindingList(kc.back(), false), kc, kList);
	if (!sc.back().AnyMod()) FilterByKeychain(GetKeyBindingList(sc.back(), false), sc, sList);

	MergeActionListsByTrigger(kList, sList, merged);

	// Now do Any
	kList.clear();
	sList.clear();
	FilterByKeychain(GetKeyBindingList(kc.back(), true), kc, kList);
	FilterByKeychain(GetKeyBindingList(sc.back(), true), sc, sList);

	MergeActionListsByTrigger(kList, sList, merged);

	if (debugEnabled) {
		LOG(
			"GetKeyBindingList: codeChain=\"%s\" scanChain=\"%s\" keyCode=\"%s\" scanCode=\"%s\":",
			kc.GetString().c_str(),
			sc.GetString().c_str(),
			kc.back().GetCodeString().c_str(),
			sc.back().GetCodeString().c_str()
		);

		DebugKeyBindingList(merged);
	}

	return merged;
}


CKeyBindings::KeyBindingList CKeyBindings::GetKeyBindingList(int keyCode, int scanCode) const
{
	return GetKeyBindingList(keyCode, scanCode, CKeySet::GetCurrentModifiers());
}


CKeyBindings::KeyBindingList CKeyBindings::GetKeyBindingList(int keyCode, int scanCode, unsigned char modifiers) const
{
	CKeyChain codeChain;
	CKeyChain scanChain;

	codeChain.emplace_back(CKeySet(keyCode, modifiers, CKeySet::KSKeyCode));
	scanChain.emplace_back(CKeySet(scanCode, modifiers, CKeySet::KSScanCode));

	return GetKeyBindingList(codeChain, scanChain);
}


CKeyBindings::KeyBindingList CKeyBindings::GetKeyBindingList() const
{
	KeyBindingList merged;

	// If hotkey map is built hotkey size is often equal to action count, + 1 for recently bound action
	if (!hotkeys.empty())
		merged.reserve(hotkeys.size() + 1);

	for (const auto& p: codeBindings) {
		const KeyBindingList& keyBindingList = p.second;

		merged.insert(merged.end(), keyBindingList.begin(), keyBindingList.end());
	}

	for (const auto& p: scanBindings) {
		const KeyBindingList& keyBindingList = p.second;

		merged.insert(merged.end(), keyBindingList.begin(), keyBindingList.end());
	}

	std::sort(merged.begin(), merged.end(), compareActionByBindingOrder);

	return merged;
}


void CKeyBindings::DebugKeyBindingList(const KeyBindingList& keyBindingList) const {
	LOG("Key Binding List:");
	if (keyBindingList.empty()) {
		LOG("   EMPTY");
	} else {
		int i = 1;
		for (const auto& keyBinding: keyBindingList) {
			LOG("   %i.  action=\"%s\"  rawline=\"%s\"  shortcut=\"%s\"  index=\"%i\"", i++, keyBinding.action.command.c_str(), keyBinding.action.rawline.c_str(), keyBinding.boundWith.c_str(), keyBinding.bindingIndex);
		}
	}
};


const CKeyBindings::HotkeyList& CKeyBindings::GetHotkeys(const std::string& action) const
{
	const auto it = hotkeys.find(action);
	if (it == hotkeys.end()) {
		static HotkeyList empty;
		return empty;
	}
	return it->second;
}


/******************************************************************************/

static bool ParseSingleChain(const std::string& keystr, CKeyChain* kc)
{
	kc->clear();
	CKeySet ks;

	// note: this will fail if keystr contains spaces
	std::stringstream ss(keystr);

	while (ss.good()) {
		char kcstr[256];
		ss.getline(kcstr, 256, ',');
		std::string kstr(kcstr);

		if (!ks.Parse(kstr, false))
			return false;

		kc->emplace_back(ks);
	}

	return true;
}


static bool ParseKeyChain(std::string keystr, CKeyChain* kc, const size_t pos = std::string::npos)
{
	// recursive function to allow "," as separator-char & as shortcut
	// -> when parsing fails, this functions replaces one by one all "," by their hexcode
	//    and tries then to reparse it
	// -> i.e. ",,," will at the end parsed as "0x2c,0x2c"

	const size_t cpos = keystr.rfind(',', pos);

	if (ParseSingleChain(keystr, kc))
		return true;

	if (cpos == std::string::npos)
		return false;

	// if cpos is 0, cpos - 1 will equal std::string::npos (size_t::max)
	const size_t nextpos = cpos - 1;

	if ((nextpos != std::string::npos) && ParseKeyChain(keystr, kc, nextpos))
		return true;

	keystr.replace(cpos, 1, IntToString(keyCodes.GetCode(","), "%#x"));
	return ParseKeyChain(keystr, kc, cpos);
}


void CKeyBindings::AddActionToKeyMap(KeyMap& bindings, KeyBinding& keyBinding)
{
	CKeySet& ks = keyBinding.keyChain.back();

	const auto it = bindings.find(ks);

	if (it == bindings.end()) {
		// create new keyset entry and push it command
		KeyBindingList& keyBindingList = bindings[ks];
		keyBinding.bindingIndex = ++bindingsCount;
		keyBindingList.push_back(keyBinding);
	} else {
		KeyBindingList& keyBindingList = it->second;
		assert(it->first == ks);

		auto it = std::find_if(keyBindingList.begin(), keyBindingList.end(), [&keyBinding](KeyBinding k) {
			return keyBinding.action.line == k.action.line;
		});

		// check if the command is already bound to the given keyset
		if (it == std::end(keyBindingList)) {
			// not yet bound, push it
			keyBinding.bindingIndex = ++bindingsCount;
			keyBindingList.push_back(keyBinding);
		}
	}
}


bool CKeyBindings::Bind(const std::string& keystr, const std::string& line)
{
	if (debugEnabled)
		LOG("[CKeyBindings::%s] index=%i keystr=%s line=%s", __func__, bindingsCount + 1, keystr.c_str(), line.c_str());

	KeyBinding keyBinding;
	keyBinding.action = Action(line);
	keyBinding.boundWith = keystr;
	if (keyBinding.action.command.empty()) {
		LOG_L(L_WARNING, "Bind: empty action: %s", line.c_str());
		return false;
	}

	if (!ParseKeyChain(keystr, &keyBinding.keyChain) || keyBinding.keyChain.empty()) {
		LOG_L(L_WARNING, "Bind: could not parse key: %s", keystr.c_str());
		return false;
	}
	CKeySet& ks = keyBinding.keyChain.back();

	// Try to be safe, force AnyMod mode for stateful commands
	if (statefulCommands.find(keyBinding.action.command) != statefulCommands.end())
		ks.SetAnyBit();

	KeyMap& bindings = ks.IsKeyCode() ? codeBindings : scanBindings;
	AddActionToKeyMap(bindings, keyBinding);

	return true;
}


bool CKeyBindings::UnBind(const std::string& keystr, const std::string& command)
{
	CKeySet ks;
	if (!ks.Parse(keystr)) {
		LOG_L(L_WARNING, "UnBind: could not parse key: %s", keystr.c_str());
		return false;
	}

	if (debugEnabled)
		LOG("[CKeyBindings::%s] keystr=%s command=%s", __func__, keystr.c_str(), command.c_str());

	KeyMap& bindings = ks.IsKeyCode() ? codeBindings : scanBindings;
	const auto it = bindings.find(ks);

	if (it == bindings.end())
		return false;

	KeyBindingList& keyBindingList = it->second;
	const bool success = RemoveCommandFromList(keyBindingList, command);

	if (keyBindingList.empty())
		bindings.erase(it);

	return success;
}


bool CKeyBindings::UnBindKeyset(const std::string& keystr)
{
	if (debugEnabled)
		LOG("[CKeyBindings::%s] keystr=%s", __func__, keystr.c_str());

	CKeySet ks;
	if (!ks.Parse(keystr)) {
		LOG_L(L_WARNING, "UnBindKeyset: could not parse key: %s", keystr.c_str());
		return false;
	}

	KeyMap& bindings = ks.IsKeyCode() ? codeBindings : scanBindings;

	const auto it = bindings.find(ks);

	if (it == bindings.end())
		return false;

	bindings.erase(it);
	return true;
}


bool CKeyBindings::RemoveActionFromKeyMap(const std::string& command, KeyMap& bindings)
{
	bool success = false;

	auto it = bindings.begin();

	while (it != bindings.end()) {
		KeyBindingList& keyBindingList = it->second;

		if (RemoveCommandFromList(keyBindingList, command))
			success = true;

		if (keyBindingList.empty()) {
			it = bindings.erase(it);
		} else {
			++it;
		}
	}

	return success;
}


bool CKeyBindings::UnBindAction(const std::string& command)
{
	if (debugEnabled)
		LOG("[CKeyBindings::%s] command=%s", __func__, command.c_str());
	return RemoveActionFromKeyMap(command, codeBindings) || RemoveActionFromKeyMap(command, scanBindings);
}


bool CKeyBindings::SetFakeMetaKey(const std::string& keystr)
{
	CKeySet ks;
	if (StringToLower(keystr) == "none") {
		fakeMetaKey = -1;
		return true;
	}
	if (!ks.Parse(keystr)) {
		LOG_L(L_WARNING, "SetFakeMetaKey: could not parse key: %s", keystr.c_str());
		return false;
	}
	if (!ks.IsKeyCode()) {
		LOG_L(L_WARNING, "SetFakeMetaKey: can't assign to scancode: %s", keystr.c_str());
		return false;
	}
	fakeMetaKey = ks.Key();
	return true;
}


bool CKeyBindings::AddKeySymbol(const std::string& keysym, const std::string& code)
{
	CKeySet ks;
	if (!ks.Parse(code)) {
		LOG_L(L_WARNING, "AddKeySymbol: could not parse key: %s", code.c_str());
		return false;
	}
	if (!(ks.GetKeys()->AddKeySymbol(keysym, ks.Key()))) {
		LOG_L(L_WARNING, "AddKeySymbol: could not add: %s", keysym.c_str());
		return false;
	}
	return true;
}


bool CKeyBindings::RemoveCommandFromList(KeyBindingList& keyBindingList, const std::string& command)
{
	bool success = false;

	auto it = keyBindingList.begin();

	while (it != keyBindingList.end()) {
		if (it->action.command == command) {
			it = keyBindingList.erase(it);
			success = true;
		} else {
			++it;
		}
	}

	return success;
}


void CKeyBindings::ConfigNotify(const std::string& key, const std::string& value)
{
	keyChainTimeout = atoi(value.c_str());
}


void CKeyBindings::LoadDefaults()
{
	const bool tmpBuildHotkeyMap = buildHotkeyMap;
	buildHotkeyMap = false;

	if (debugEnabled)
		LOG("[CKeyBindings::%s]", __func__);

	SetFakeMetaKey("space");

	for (const auto& b: defaultBindings) {
		Bind(b.key, b.action);
	}

	buildHotkeyMap = tmpBuildHotkeyMap;
}


/******************************************************************************/

void CKeyBindings::PushAction(const Action& action)
{
	if (action.command == "keysave") {
		static const std::string defaultOutFilename = "uikeys.tmp"; // tmp, not txt

		const std::vector<std::string> args = CSimpleParser::Tokenize(action.extra, 2);
		const std::string& filename = args.empty() ? defaultOutFilename : args[0];

		if (Save(filename)) {
			LOG("Saved active keybindings at %s", filename.c_str());
		} else {
			LOG_L(L_WARNING, "Could not save %s", filename.c_str());
		}
	}
	else if (action.command == "keyprint") {
		Print();
	}
	else if (action.command == "keysyms") {
		keyCodes.PrintNameToCode(); //TODO move to CKeyCodes?
	}
	else if (action.command == "keycodes") {
		keyCodes.PrintCodeToName(); //TODO move to CKeyCodes?
	}
	else {
		ExecuteCommand(action.rawline);
	}
}

bool CKeyBindings::ExecuteCommand(const std::string& line)
{
	const std::vector<std::string> words = CSimpleParser::Tokenize(line, 2);

	if (words.empty())
		return false;

	const std::string command = StringToLower(words[0]);

	if (command == "keydebug") {
		if (words.size() == 1) {
			// toggle
			debugEnabled = !debugEnabled;
		} else if (words.size() >= 2) {
			// set
			debugEnabled = atoi(words[1].c_str());
		}
	}
	else if (command == "keyload") {
		const std::string& filename = words.size() > 1 ? words[1] : DEFAULT_FILENAME;

		if (debugEnabled)
			LOG("[CKeyBindings::%s] line=%s", __func__, line.c_str());

		// Backward-compatibility from before `/keydefaults` existed
		if (loadStack.empty() && words.size() == 1)
			LoadDefaults();

		Load(filename);
	}
	else if (command == "keyreload") {
		const std::string& filename = words.size() > 1 ? words[1] : DEFAULT_FILENAME;

		if (debugEnabled)
			LOG("[CKeyBindings::%s] line=%s", __func__, line.c_str());

		ExecuteCommand("unbindall");
		ExecuteCommand("unbind enter chat");

		if (loadStack.empty() && words.size() == 1)
			LoadDefaults();

		Load(filename);
	}
	else if (command == "keydefaults") {
		LoadDefaults();
	}
	else if ((command == "fakemeta") && (words.size() > 1)) {
		if (!SetFakeMetaKey(words[1])) { return false; }
	}
	else if ((command == "keysym") && (words.size() > 2)) {
		if (!AddKeySymbol(words[1], words[2])) { return false; }
	}
	else if ((command == "bind") && (words.size() > 2)) {
		if (!Bind(words[1], words[2])) { return false; }
	}
	else if ((command == "unbind") && (words.size() > 2)) {
		if (!UnBind(words[1], words[2])) { return false; }
	}
	else if ((command == "unbindaction") && (words.size() > 1)) {
		if (!UnBindAction(words[1])) { return false; }
	}
	else if ((command == "unbindkeyset") && (words.size() > 1)) {
		if (!UnBindKeyset(words[1])) { return false; }
	}
	else if (command == "unbindall") {
		codeBindings.clear();
		scanBindings.clear();
		keyCodes.Reset();
		scanCodes.Reset();
		bindingsCount = 0;
		Bind("enter", "chat"); // bare minimum

		if (debugEnabled)
			LOG("[CKeyBindings::%s] line=%s", __func__, line.c_str());
	}
	else {
		return false;
	}

	if (buildHotkeyMap)
		BuildHotkeyMap();

	return false;
}


bool CKeyBindings::Load(const std::string& filename)
{
	if (std::find(loadStack.begin(), loadStack.end(), filename) != loadStack.end()) {
		LOG_L(L_WARNING, "[CKeyBindings::%s] Cyclic keys file inclusion: %s, load stack:", __func__, filename.c_str());
		LOG_L(L_WARNING, " !-> %s", filename.c_str());
		for (auto it = loadStack.rbegin(); it != loadStack.rend(); ++it)
			LOG_L(L_WARNING, "  -> %s", (*it).c_str());

		return false;
	}

	const bool tmpBuildHotkeyMap = buildHotkeyMap;
	buildHotkeyMap = false;

	if (debugEnabled) {
		LOG("[CKeyBindings::%s] filename=%s%s", __func__, filename.c_str(), loadStack.empty() ? "" : ", load stack:");
		for (auto it = loadStack.rbegin(); it != loadStack.rend(); ++it)
			LOG("  -> %s", (*it).c_str());
	}

	loadStack.push_back(filename);

	CFileHandler ifs(filename);
	CSimpleParser parser(ifs);

	while (!parser.Eof()) {
		ExecuteCommand(parser.GetCleanLine());
	}

	loadStack.pop_back();

	buildHotkeyMap = tmpBuildHotkeyMap;

	return true;
}


void CKeyBindings::BuildHotkeyMap()
{
	if (debugEnabled)
		LOG("[CKeyBindings::%s]", __func__);

	// create reverse map of bindings ([action] -> key shortcuts)
	hotkeys.clear();

	for (const auto& keyBinding: GetKeyBindingList()) {
		HotkeyList& hl = hotkeys[keyBinding.action.command + (keyBinding.action.extra.empty() ? "" : " " + keyBinding.action.extra)];
		hl.push_back(keyBinding.boundWith);
	}
}


/******************************************************************************/

void CKeyBindings::Print() const
{
	FileSave(stdout);
}


bool CKeyBindings::Save(const std::string& filename) const
{
	FILE* out = fopen(filename.c_str(), "wt");
	if (out == nullptr)
		return false;

	const bool success = FileSave(out);
	fclose(out);
	return success;
}


bool CKeyBindings::FileSave(FILE* out) const
{
	if (out == nullptr)
		return false;

	// clear the defaults
	fprintf(out, "\n");
	fprintf(out, "unbindall          // clear the defaults\n");
	fprintf(out, "unbind enter chat  // clear the defaults\n");
	fprintf(out, "\n");

	// save the user defined key symbols
	keyCodes.SaveUserKeySymbols(out);
	scanCodes.SaveUserKeySymbols(out);

	// save the fake meta key (if it has been defined)
	if (fakeMetaKey >= 0)
		fprintf(out, "fakemeta  %s\n\n", keyCodes.GetName(fakeMetaKey).c_str());

	for (const KeyBinding& keybinding: GetKeyBindingList()) {
		std::string comment;

		if (unitDefHandler && (keybinding.action.command.find("buildunit_") == 0)) {
			const std::string unitName = keybinding.action.command.substr(10);
			const UnitDef* unitDef = unitDefHandler->GetUnitDefByName(unitName);

			if (unitDef != nullptr)
				comment = "  // " + unitDef->humanName + " - " + unitDef->tooltip;
		}

		if (comment.empty()) {
			fprintf(out, "bind %18s  %s\n", keybinding.boundWith.c_str(), keybinding.action.rawline.c_str());
		} else {
			fprintf(out, "bind %18s  %-20s%s\n", keybinding.boundWith.c_str(), keybinding.action.rawline.c_str(), comment.c_str());
		}
	}

	return true;
}


/******************************************************************************/
