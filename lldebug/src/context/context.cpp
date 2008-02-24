/*
 * Copyright (c) 2005-2008  cielacanth <cielacanth AT s60.xrea.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "precomp.h"
#include "lldebug.h"
#include "configfile.h"
#include "net/remoteengine.h"
#include "context/codeconv.h"
#include "context/context.h"
#include "context/luautils.h"
#include "context/luaiterate.h"

#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <fstream>

/// Dummy function name for eval.
#define DUMMY_FUNCNAME "__LLDEBUG_DUMMY_TEMPORARY_FUNCTION_NAME__"

namespace lldebug {

/**
 * @brief lua_State*����R���e�L�X�g���擾�ł���悤�ɂ���N���X�ł��B
 */
class Context::ContextManager {
public:
	explicit ContextManager() {
	}

	~ContextManager() {
		scoped_lock lock(m_mutex);

		while (!m_map.empty()) {
			Context *ctx = (*m_map.begin()).second;
			ctx->Delete();
		}
	}

	/// Get weather this object is empty.
	bool IsEmpty() {
		scoped_lock lock(m_mutex);

		return m_map.empty();
	}

	/// Add the pair of (L, ctx).
	void Add(Context *ctx, lua_State *L) {
		scoped_lock lock(m_mutex);

		if (ctx == NULL || L == NULL) {
			return;
		}

		m_map.insert(std::make_pair(L, ctx));
	}

	/// Erase the 'ctx' and corresponding lua_State objects.
	void Erase(Context *ctx) {
		scoped_lock lock(m_mutex);

		if (ctx == NULL) {
			return;
		}

		for (Map::iterator it = m_map.begin(); it != m_map.end(); ) {
			Context *p = (*it).second;
 
			if (p == ctx) {
				m_map.erase(it++);
			}
			else {
				++it;
			}
		}
	}

	/// Find the Context object from a lua_State object.
	Context *Find(lua_State *L) {
		scoped_lock lock(m_mutex);

		if (L == NULL) {
			return NULL;
		}

		Map::iterator it = m_map.find(L);
		if (it == m_map.end()) {
			return NULL;
		}

		return (*it).second;
	}

private:
	typedef std::map<lua_State *, Context *> Map;
	Map m_map;
	mutex m_mutex;
};


/*-----------------------------------------------------------------*/
shared_ptr<Context::ContextManager> Context::ms_manager;
int Context::ms_idCounter = 0;

Context *Context::Create() {
	// It's impossible to use 'xxx_ptr' classes,
	// because ctx don't have public delete.
	Context *ctx = new Context;

	try {
		if (ctx->Initialize() != 0) {
			ctx->Delete();
			return NULL;
		}

		// After the all initialization was done,
		// we create a new frame for this context.
		if (ctx->CreateDebuggerFrame() != 0) {
			ctx->Delete();
			return NULL;
		}

		if (ms_manager == NULL) {
			ms_manager.reset(new ContextManager);
		}

		ms_manager->Add(ctx, ctx->GetLua());
	}
	catch (...) {
		ctx->Delete();
		return NULL;
	}

	return ctx;
}

Context::Context()
	: m_id(0), m_lua(NULL)
	, m_state(STATE_INITIAL), m_isEnabled(true), m_updateCount(0)
	, m_waitUpdateCount(0), m_isMustUpdate(false)
	, m_engine(new RemoteEngine)
	, m_sourceManager(&*m_engine), m_breakpoints(&*m_engine) {

	m_id = ms_idCounter++;
}

/// Create the new debug frame.
int Context::CreateDebuggerFrame() {
	scoped_lock lock(m_mutex);

	/*HINSTANCE inst = ::ShellExecuteA(
		NULL, NULL,
		"..\\debug\\lldebug_frame.exe",
		"localhost 51123",
		"",
		SW_SHOWNORMAL);
	if ((unsigned long int)inst <= 32) {
		return -1;
	}*/

	if (m_engine->StartContext("localhost", "51123", 10) != 0) {
		return -1;
	}

	return 0;
}

int Context::Initialize() {
	scoped_lock lock(m_mutex);

	// dll����̌Ăяo���Ȃ̂ŕ��ʂɎ��s����\��������܂��B
	lua_State *L = lua_open();
	if (L == NULL) {
		return -1;
	}

	if (LuaInitialize(L) != 0) {
		lua_close(L);
		return -1;
	}

	/*lldebug_InitState init = lldebug_getinitstate();
	if (init != NULL && init(L) != 0) {
		lua_close(L);
		return -1;
	}*/

	SetHook(L);
	m_lua = L;
	m_state = STATE_STEPINTO; //NORMAL;
	m_coroutines.push_back(CoroutineInfo(L));
	return 0;
}

Context::~Context() {
	SaveConfig();
	m_engine.reset();

	scoped_lock lock(m_mutex);

	if (m_lua != NULL) {
		lua_close(m_lua);
		m_lua = NULL;
	}

	// �ŏ��̃R���e�L�X�g�̍쐬�Ɏ��s���Ă���\��������܂��B
	if (ms_manager != NULL) {
		ms_manager->Erase(this);

		if (ms_manager->IsEmpty()) {
			ms_manager.reset();
		}
	}
}

void Context::Delete() {
	delete this;
}

int Context::LoadConfig() {
	scoped_lock lock(m_mutex);

	try {
		std::string filename = EncodeToFilename(m_rootFileKey);
		std::fstream ifs;
		if (OpenConfigFile(filename + "xml", ifs, false) != 0) {
			return -1;
		}
	
		boost::archive::xml_iarchive ar(ifs);
		BreakpointList bps(&*m_engine);
		ar & BOOST_SERIALIZATION_NVP(bps);

		// set values
		m_breakpoints = bps;
		m_engine->ChangedBreakpointList(m_breakpoints);
	}
	catch (std::exception &) {
		OutputError("Couldn't open the config file.");
	}

	return 0;
}

int Context::SaveConfig() {
	scoped_lock lock(m_mutex);

	try {
		std::string filename = EncodeToFilename(m_rootFileKey);
		std::fstream ofs;
		if (OpenConfigFile(filename + "xml", ofs, true) != 0) {
			return -1;
		}

		boost::archive::xml_oarchive ar(ofs);
		ar & LLDEBUG_MEMBER_NVP(breakpoints);
	}
	catch (std::exception &) {
		OutputError("Couldn't save the config file.");
	}

	return 0;
}

Context *Context::Find(lua_State *L) {
	if (ms_manager == NULL) {
		return NULL;
	}

	return ms_manager->Find(L);
}

void Context::Quit() {
	scoped_lock lock(m_mutex);

	SetState(STATE_QUIT);
}

void Context::SetDebugEnable(bool enabled) {
	scoped_lock lock(m_mutex);

	m_isEnabled = enabled;
}

/// Parse the lua error that forat is like 'FILENAME:LINE:str...'.
Context::ParseData Context::ParseLuaError(const std::string &str) {
	scoped_lock lock(m_mutex);

	// FILENAME:LINE:str...
	bool found = false;
	int line;
	std::string::size_type pos = 0, prevPos = 0;
	while ((prevPos = str.find(':', prevPos)) != str.npos) {
		pos = prevPos;

		// parse line number
		line = 0;
		while (++pos < str.length()) {
			char c = str[pos];

			if (c < '0' || '9' < c) {
				break;
			}
			line = line * 10 + (c - '0');
		}

		// If there is ':', parse is success.
		if (pos < str.length() && str[pos] == ':') {
			found = true;
			break;
		}

		// str[prevPos] is ':', so advance prevPos.
		++prevPos;
	}

	// source is file, key:line: str...
	// source is string, [string "***"]:line: str...
	//               ,or [string "***..."]:line: str...
	// If found is true, str[prevPos] is first ':', str[pos] is second ':'.
	if (found) {
		std::string filename = str.substr(0, prevPos);
		const char *beginStr = "[string \"";
		const char *endStr = "\"]";

		// msg is what is removed filename and line from str.
		std::string msg =
			str.substr(std::min(pos + 2, str.length())); // skip ": "

		// If source is string, key is manipulated and may be shorten.
		if (filename.find(beginStr) == 0 &&
			filename.rfind(endStr) == filename.length() - strlen(endStr)) {
			filename = filename.substr(
				strlen(beginStr),
				filename.length() - strlen(beginStr) - strlen(endStr));

			// If key is "***...", "..." must be removed.
			if (filename.rfind("...") == filename.length() - 3) {
				filename = filename.substr(0, filename.length() - 3);
			}
 
			// Replace key if any.
			const Source *source = m_sourceManager.GetString(filename);
			if (source != NULL) {
				return ParseData(msg, source->GetKey(), line, false);
			}
			else if (filename == DUMMY_FUNCNAME) {
				return ParseData(msg, "", -1, false);
			}
		}
		else {
			return ParseData(msg, std::string("@") + filename, line, false);
		}
	}

	// Come here when str can't be parsed or filename is invalid.
	return ParseData(str, "", -1, false);
}

void Context::OutputLog(LogType type, const std::string &str) {
	scoped_lock lock(m_mutex);

	ParseData data = ParseLuaError(str);
	assert(!data.isDummyFunc);
	m_engine->OutputLog(type, data.message, data.key, data.line);
}

void Context::OutputLuaError(const char *str) {
	scoped_lock lock(m_mutex);

	if (str == NULL) {
		return;
	}

	OutputLog(LOGTYPE_ERROR, str);
}

void Context::OutputLog(const std::string &str) {
	scoped_lock lock(m_mutex);

	OutputLog(LOGTYPE_MESSAGE, str);
}

void Context::OutputError(const std::string &str) {
	scoped_lock lock(m_mutex);

	OutputLog(LOGTYPE_ERROR, str);
}

void Context::BeginCoroutine(lua_State *L) {
	scoped_lock lock(m_mutex);

	CoroutineInfo info(L);
	m_coroutines.push_back(info);
}

void Context::EndCoroutine(lua_State *L) {
	scoped_lock lock(m_mutex);

	if (m_coroutines.empty() || m_coroutines.back().L != L) {
		assert(0 && "Couldn't end coroutine.");
		return;
	}

	// �X�e�b�v�I�[�o�[���ݒ肳�ꂽ�R���[�`���𔲂���Ƃ���
	// �����I�Ƀu���C�N���܂��B
	if (m_state == STATE_STEPOVER || m_state == STATE_STEPRETURN) {
		if (m_stepinfo.L == L) {
			SetState(STATE_BREAK);
		}
	}

	m_coroutines.pop_back();
}

void Context::SetHook(lua_State *L) {
	int mask = LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET;
	lua_sethook(L, Context::s_HookCallback, mask, 0);
}

void Context::s_HookCallback(lua_State *L, lua_Debug *ar) {
	Context *ctx = Context::Find(L);

	if (ctx != NULL) {
		ctx->HookCallback(L, ar);
	}
}

void Context::SetState(State state) {
	assert(state != STATE_INITIAL);
	scoped_lock lock(m_mutex);

	if (state == m_state) {
		return;
	}

	if (state == STATE_QUIT) {
		m_state = state;
		return;
	}

	// ��ԑJ�ڂ��s���܂��B
	switch (m_state) {
	case STATE_INITIAL:
		m_state = state;
		break;
	case STATE_NORMAL: // running or stable
		switch (state) {
		case STATE_BREAK:
			m_state = state;
			m_engine->ChangedState(true);
			break;
		case STATE_STEPOVER:
		case STATE_STEPINTO:
		case STATE_STEPRETURN:
			/* ignore */
			break;
		default:
			/* error */
			assert(false && "Value of 'state' is illegal.");
			return;
		}
		break;
	case STATE_BREAK:
		if (m_waitUpdateCount > 0) {
			return; // ignore
		}
		switch (state) {
		case STATE_NORMAL:
			m_state = state;
			m_engine->ChangedState(false);
			break;
		case STATE_STEPOVER:
		case STATE_STEPINTO:
		case STATE_STEPRETURN:
			m_state = state;
			m_engine->ChangedState(false);
			break;
		default:
			/* error */
			assert(false && "Value of 'state' is illegal.");
			return;
		}
		break;
	case STATE_STEPOVER:
	case STATE_STEPINTO:
	case STATE_STEPRETURN:
		switch (state) {
		case STATE_NORMAL:
		case STATE_STEPOVER:
		case STATE_STEPINTO:
		case STATE_STEPRETURN:
			/* ignore */
			break;
		case STATE_BREAK:
			m_engine->ChangedState(true);
			m_state = state;
			break;
		default:
			/* error */
			assert(false && "Value of 'state' is illegal.");
			return;
		}
		break;
	case STATE_QUIT:
		/* ignore */
		break;
	default:
		/* error */
		assert(false && "Value of 'm_state' is illegal.");
		return;
	}

	// �X�e�b�v�I�[�o�[�Ȃ����ݒ肵�܂��B
	if (m_state == STATE_STEPOVER || m_state == STATE_STEPRETURN) {
		m_stepinfo = m_coroutines.back();
	}
}

int Context::HandleCommand() {
	scoped_lock lock(m_mutex);

	while (m_engine->HasCommands()) {
		Command command = m_engine->GetCommand();
		m_engine->PopCommand();

		if (command.IsResponse()) {
			command.CallResponse();
			continue;
		}

		switch (command.GetType()) {
		case REMOTECOMMANDTYPE_END_CONNECTION:
			Quit();
			return -1;
		case REMOTECOMMANDTYPE_BREAK:
			SetState(STATE_BREAK);
			break;
		case REMOTECOMMANDTYPE_RESUME:
			SetState(STATE_NORMAL);
			break;
		case REMOTECOMMANDTYPE_STEPINTO:
			SetState(STATE_STEPINTO);
			break;
		case REMOTECOMMANDTYPE_STEPOVER:
			SetState(STATE_STEPOVER);
			break;
		case REMOTECOMMANDTYPE_STEPRETURN:
			SetState(STATE_STEPRETURN);
			break;

		case REMOTECOMMANDTYPE_FORCE_UPDATESOURCE:
			m_isMustUpdate = true;
			break;

		case REMOTECOMMANDTYPE_SAVE_SOURCE:
			{
				std::string key;
				string_array sources;
				command.GetData().Get_SaveSource(key, sources);
				m_sourceManager.Save(key, sources);
			}
			break;

		case REMOTECOMMANDTYPE_SET_UPDATECOUNT:
			{
				int count;
				command.GetData().Get_SetUpdateCount(count);
				if (count > m_updateCount) {
					m_updateCount = count;
				}
			}
			break;

		case REMOTECOMMANDTYPE_EVALS_TO_VARLIST:
			{
				string_array evals;
				LuaStackFrame stackFrame;
				command.GetData().Get_EvalsToVarList(evals, stackFrame);
				m_engine->ResponseVarList(command, LuaEvalsToVarList(evals, stackFrame));
			}
			break;
		case REMOTECOMMANDTYPE_EVAL_TO_MULTIVAR:
			{
				std::string eval;
				LuaStackFrame stackFrame;
				command.GetData().Get_EvalToMultiVar(eval, stackFrame);
				m_engine->ResponseVarList(command, LuaEvalToMultiVar(eval, stackFrame));
			}
			break;
		case REMOTECOMMANDTYPE_EVAL_TO_VAR:
			{
				std::string eval;
				LuaStackFrame stackFrame;
				command.GetData().Get_EvalToVar(eval, stackFrame);
				m_engine->ResponseVar(command, LuaEvalToVar(eval, stackFrame));
			}
			break;
		

		case REMOTECOMMANDTYPE_SET_BREAKPOINT:
			{
				Breakpoint bp;
				command.GetData().Get_SetBreakpoint(bp);
				m_breakpoints.Set(bp);
			}
			break;
		case REMOTECOMMANDTYPE_REMOVE_BREAKPOINT:
			{
				Breakpoint bp;
				command.GetData().Get_RemoveBreakpoint(bp);
				m_breakpoints.Remove(bp);
			}
			break;

		case REMOTECOMMANDTYPE_REQUEST_FIELDSVARLIST:
			{
				LuaVar var;
				command.GetData().Get_RequestFieldVarList(var);
				m_engine->ResponseVarList(command, LuaGetFields(var));
			}
			break;
		case REMOTECOMMANDTYPE_REQUEST_LOCALVARLIST:
			{
				LuaStackFrame stackFrame;
				command.GetData().Get_RequestLocalVarList(stackFrame);
				m_engine->ResponseVarList(command, LuaGetLocals(stackFrame));
			}
			break;
		case REMOTECOMMANDTYPE_REQUEST_ENVIRONVARLIST:
			{
				LuaStackFrame stackFrame;
				command.GetData().Get_RequestEnvironVarList(stackFrame);
				m_engine->ResponseVarList(command, LuaGetEnviron(stackFrame));
			}
			break;
		case REMOTECOMMANDTYPE_REQUEST_GLOBALVARLIST:
			m_engine->ResponseVarList(command, LuaGetFields(TABLETYPE_GLOBAL));
			break;
		case REMOTECOMMANDTYPE_REQUEST_REGISTRYVARLIST:
			m_engine->ResponseVarList(command, LuaGetFields(TABLETYPE_REGISTRY));
			break;
		case REMOTECOMMANDTYPE_REQUEST_STACKLIST:
			m_engine->ResponseVarList(command, LuaGetStack());
			break;
		case REMOTECOMMANDTYPE_REQUEST_SOURCE:
			{
				std::string key;
				command.GetData().Get_RequestSource(key);
				const Source *source = m_sourceManager.Get(key);
				m_engine->ResponseSource(command, (source != NULL ? *source : Source()));
			}
			break;
		case REMOTECOMMANDTYPE_REQUEST_BACKTRACE:
			//m_engine->ResponseBacktrace(command, LuaGetBacktrace());
			break;
		}
	}

	return 0;
}

struct UpdateResponseWaiter {
	explicit UpdateResponseWaiter(int *count)
		: m_count(count) {
		++*count;
	}

	void operator()(const Command &command) {
		--*m_count;
	}

	private:
	int *m_count;
	};

void Context::HookCallback(lua_State *L, lua_Debug *ar) {
	if (!m_isEnabled) {
		return;
	}

	scoped_lock lock(m_mutex);
	assert(m_state != STATE_INITIAL && "Not initialized !!!");

	switch (ar->event) {
	case LUA_HOOKCALL:
		++m_coroutines.back().call;
		return;
	case LUA_HOOKRET:
	case LUA_HOOKTAILRET:
		if (m_state == STATE_STEPRETURN) {
			const CoroutineInfo &info = m_coroutines.back();
			if (m_stepinfo.L == info.L  && info.call <= m_stepinfo.call) {
				SetState(STATE_BREAK);
			}
		}
		--m_coroutines.back().call;
		return;
	default:
		break;
	}

	// Stop running if need.
	switch (m_state) {
	case STATE_QUIT:
		luaL_error(L, "quited");
		return;
	case STATE_STEPOVER: {
		const CoroutineInfo &info = m_coroutines.back();
		if (m_stepinfo.L == info.L  && info.call <= m_stepinfo.call) {
			SetState(STATE_BREAK);
		}
		}
		break;
	case STATE_STEPINTO:
		SetState(STATE_BREAK);
		break;
	case STATE_NORMAL:
	case STATE_STEPRETURN:
	case STATE_BREAK:
		break;
	case STATE_INITIAL:
		/* error */
		assert(false && "Value of 'state' is illegal.");
		break;
	}

	// Get the infomation of the current function.
	lua_getinfo(L, "nSl", ar);

	// Break and stop program, if any.
	Breakpoint bp = m_breakpoints.Find(ar->source, ar->currentline - 1);
	if (bp.IsOk()) {
		SetState(STATE_BREAK);
	}

	// �u���C�N�ɂ����t���[�����X�V���邽�߂ɕK�v�ł��B
	State prevState = STATE_NORMAL;
	for (;;) {
		// handle event and message queue
		if (HandleCommand() != 0 || !m_engine->IsConnecting()) {
			luaL_error(L, "quited");
			return;
		}

		// Break this loop if the state isn't STATE_BREAK.
		if (m_state != STATE_BREAK) {
			break;
		}
		else {
			if (m_isMustUpdate || prevState != STATE_BREAK) {
				if (m_sourceManager.Get(ar->source) == NULL) {
					m_sourceManager.Add(ar->source);
				}
				m_isMustUpdate = false;

				m_engine->UpdateSource(
					ar->source, ar->currentline,
					++m_updateCount,
					UpdateResponseWaiter(&m_waitUpdateCount));
			}
			prevState = m_state;

			if (!m_engine->HasCommands()) {
				boost::xtime xt;
				boost::xtime_get(&xt, boost::TIME_UTC);
				xt.nsec += 10 * 1000 * 1000;
				boost::thread::sleep(xt);
			}
		}
	}
}

/**
 * @brief lua�p�̊֐����������Ă��܂��B
 */
class Context::LuaImpl {
public:
	static int atpanic(lua_State *L) {
		printf("%s\n", lua_tostring(L, -1));
		return -1;
	}

	static int cocreate(lua_State *L) {
		lua_State *NL = lua_newthread(L);
		luaL_argcheck(L, lua_isfunction(L, 1) && !lua_iscfunction(L, 1), 1,
			"Lua function expected");
		lua_pushvalue(L, 1);  /* move function to top */
		lua_xmove(L, NL, 1);  /* move function from L to NL */

		lua_atpanic(NL, atpanic);
		Context::SetHook(NL);
		Context::ms_manager->Add(Context::Find(L), NL);
		return 1;
	}

	static int auxresume (lua_State *L, lua_State *co, int narg) {
		Context *ctx = Context::Find(L);
		int status;
		if (!lua_checkstack(co, narg))
			luaL_error(L, "too many arguments to resume");
		lua_xmove(L, co, narg);

		ctx->BeginCoroutine(co);
		status = lua_resume(co, narg);
		ctx->EndCoroutine(co);

		if (status == 0 || status == LUA_YIELD) {
			int nres = lua_gettop(co);
			if (!lua_checkstack(L, nres))
				luaL_error(L, "too many results to resume");
			lua_xmove(co, L, nres);  /* move yielded values */
			return nres;
		}
		else {
			lua_xmove(co, L, 1);  /* move error message */
			return -1;  /* error flag */
		}
	}

	static int coresume(lua_State *L) {
		lua_State *co = lua_tothread(L, 1);
		int r;
		luaL_argcheck(L, co, 1, "coroutine expected");
		r = auxresume(L, co, lua_gettop(L) - 1);
		if (r < 0) {
			lua_pushboolean(L, 0);
			lua_insert(L, -2);
			return 2;  /* return false + error message */
		}
		else {
			lua_pushboolean(L, 1);
			lua_insert(L, -(r + 1));
			return r + 1;  /* return true + `resume' returns */
		}
	}

	static void override_baselib(lua_State *L) {
		const luaL_reg s_coregs[] = {
			{"create", cocreate},
			{"resume", coresume},
			{NULL, NULL}
		};

		luaL_openlib(L, LUA_COLIBNAME, s_coregs, 0);
	}

	/*---------------------------------------*/
	static int index_for_eval(lua_State *L) {
		Context *ctx = Context::Find(L);
		if (ctx == NULL) {
			return 0;
		}

		return ctx->LuaIndexForEval(L);
	}

	static int newindex_for_eval(lua_State *L) {
		Context *ctx = Context::Find(L);
		if (ctx == NULL) {
			return 0;
		}

		return ctx->LuaNewindexForEval(L);
	}
};

int Context::LuaInitialize(lua_State *L) {
	lua_atpanic(L, LuaImpl::atpanic);
	lldebug::LuaInitialize(L);
	return 0;
}

int Context::LoadFile(const char *filename) {
	scoped_lock lock(m_mutex);

	if (filename == NULL) {
		return -1;
	}

	if (luaL_loadfile(m_lua, filename) != 0) {
		m_sourceManager.Add(std::string("@") + filename);
		OutputLuaError(lua_tostring(m_lua, -1));
		return -1;
	}

	// Save the first key.
	if (m_rootFileKey.empty()) {
		m_rootFileKey = "@";
		m_rootFileKey += filename;
		LoadConfig();
	}

	m_sourceManager.Add(std::string("@") + filename);
	return 0;
}

int Context::LoadString(const char *str) {
	scoped_lock lock(m_mutex);
	
	if (str == NULL) {
		return -1;
	}

	if (luaL_loadstring(m_lua, str) != 0) {
		m_sourceManager.Add(str);
		OutputLuaError(lua_tostring(m_lua, -1));
		return -1;
	}

	// Save the first key.
	if (m_rootFileKey.empty()) {
		m_rootFileKey = str;
		LoadConfig();
	}

	m_sourceManager.Add(str);
	return 0;
}

int Context::LuaOpenBase(lua_State *L) {
	scoped_lock lock(m_mutex);

	if (luaopen_base(L) == 0) {
		return 0;
	}

	LuaImpl::override_baselib(L);
	return 2;
}

void Context::LuaOpenLibs(lua_State *L) {
	scoped_lock lock(m_mutex);

	luaL_openlibs(L);
	LuaImpl::override_baselib(L);
}


/*-----------------------------------------------------------------*/
LuaVarList Context::LuaGetFields(TableType type) {
	scoped_lock lock(m_mutex);
	int rootType = 0;

	// Detect table type.
	switch (type) {
	case TABLETYPE_GLOBAL:
		rootType = LUA_GLOBALSINDEX;
		break;
	case TABLETYPE_REGISTRY:
		rootType = LUA_REGISTRYINDEX;
		break;
	default:
		assert(0 && "type [:TableType] is invalid");
		return LuaVarList();
	}

	// Get the fields of the table.
	varlist_maker callback;
	if (iterate_fields(callback, GetLua(), rootType) != 0) {
		return LuaVarList();
	}

	return callback.get_result();
}

LuaVarList Context::LuaGetFields(const LuaVar &var) {
	scoped_lock lock(m_mutex);

	// Get the fields of var.
	varlist_maker callback;
	if (iterate_var(callback, var) != 0) {
		return LuaVarList();
	}

	return callback.get_result();
}

LuaVarList Context::LuaGetLocals(const LuaStackFrame &stackFrame) {
	scoped_lock lock(m_mutex);
	lua_State *L = stackFrame.GetLua().GetState();
	
	varlist_maker callback;
	if (iterate_locals(callback, (L != NULL ? L : GetLua()),
		stackFrame.GetLevel(), true, true, false) != 0) {
		return LuaVarList();
	}

	return callback.get_result();
}

LuaVarList Context::LuaGetEnviron(const LuaStackFrame &stackFrame) {
	scoped_lock lock(m_mutex);
	lua_State *L = stackFrame.GetLua().GetState();
	
	varlist_maker callback;
	if (iterate_locals(callback, (L != NULL ? L : GetLua()),
		stackFrame.GetLevel(), false, false, true) != 0) {
		return LuaVarList();
	}

	return callback.get_result();
}

LuaVarList Context::LuaGetStack() {
	scoped_lock lock(m_mutex);

	varlist_maker callback;
	if (iterate_stacks(callback, GetLua()) != 0) {
		return LuaVarList();
	}

	return callback.get_result();
}

LuaBacktraceList Context::LuaGetBacktrace() {
	const int LEVEL_MAX = 1024;	// maximum size of the stack
	scoped_lock lock(m_mutex);
	LuaBacktraceList array;
	
	CoroutineList::reverse_iterator it;
	for (it = m_coroutines.rbegin(); it != m_coroutines.rend(); ++it) {
		lua_State *L1 = it->L;
		scoped_lua scoped(this, L1, 0);
		lua_Debug ar;

		// level 0 may be this own function
		for (int level = 0; lua_getstack(L1, level, &ar); ++level) {
			if (level > LEVEL_MAX) {
				assert(false && "stack size is too many");
				return array;
			}

			lua_getinfo(L1, "Snl", &ar);

			// Source title is also set,
			// because it is always used when backtrace is shown.
			std::string sourceTitle;
			const Source *source = m_sourceManager.Get(ar.source);
			if (source != NULL) {
				sourceTitle = source->GetTitle();
			}

			std::string name = LuaMakeFuncName(&ar);
			array.push_back(LuaBacktrace(L1, name, ar.source, sourceTitle, ar.currentline, level));
		}
	}

	return array;
}

int Context::LuaIndexForEval(lua_State *L) {
	scoped_lock lock(m_mutex);
	scoped_lua scoped(this, L, 1);

	if (!lua_isstring(L, 2)) {
		lua_pushnil(L);
		return 1;
	}
	std::string target(lua_tostring(L, 2));
	int level = (int)lua_tointeger(L, lua_upvalueindex(1));

	// level 0: this C function
	// level 1: __lldebug_dummy__ function
	// level 2: loaded string in LuaEval (includes __lldebug_dummy__)
	// level 3: current running function
	if (find_localvalue(L, level + 1, target, true, true, false) == 0) {
		return 1;
	}

	if (find_localvalue(L, level + 3, target, true, true, true) == 0) {
		return 1;
	}

	lua_pushnil(L);
	return 1; // return nil
}

int Context::LuaNewindexForEval(lua_State *L) {
	scoped_lock lock(m_mutex);
	scoped_lua scoped(this, L, 0);

	if (!lua_isstring(L, 2)) {
		return 0;
	}
	std::string target(lua_tostring(L, 2));
	int level = (int)lua_tointeger(L, lua_upvalueindex(1));
	
	// level 0: this C function
	// level 1: __lldebug_dummy__ function
	// level 2: loaded string in LuaEval (includes __lldebug_dummy__)
	// level 3: current running function
	if (set_localvalue(L, level + 1, target, 3, true, true, false, false) == 0) {
		return 0;
	}

	if (set_localvalue(L, level + 3, target, 3, true, true, true, true) == 0) {
		return 0;
	}

	return 0;
}

/**
 * @brief string reader for LuaEval.
 */
struct eval_string_reader {
	std::string str;
	const char *beginning;
	const char *ending;
	int state;

	explicit eval_string_reader(const std::string &str_,
								const char *beginning_, const char *ending_)
		: str(str_), beginning(beginning_), ending(ending_), state(0) {
	}

	static const char *exec(lua_State *L, void *data, size_t *size) {
		eval_string_reader *reader = (eval_string_reader *)data;
		
		switch (reader->state) {
		case 0: // beginning function part
			++reader->state;
			if (reader->beginning != NULL) {
				*size = strlen(reader->beginning);
				return reader->beginning;
			}
			// fall through

		case 1: // content part
			++reader->state;
			if (!reader->str.empty()) {
				*size = reader->str.length();
				return reader->str.c_str();
			}
			// fall through

		case 2: // ending function part
			++reader->state;
			if (reader->ending != NULL) {
				*size = strlen(reader->ending);
				return reader->ending;
			}
			// fall through
		}

		return NULL;
	}
	};

int Context::LuaEval(lua_State *L, int level, const std::string &str) {
	scoped_lock lock(m_mutex);

	if (str.empty()) {
		return 0;
	}

	const char *beginning = NULL;
	const char *ending = NULL;
	if (level >= 0) {
		beginning =
			"local function __lldebug_eval_function__()\n";
		ending =
			"\nend\n"
			"setfenv("
			"  __lldebug_eval_function__,"
			"  setmetatable({}, __lldebug_eval_metatable__))\n"
			"return __lldebug_eval_function__()\n";

		// Create __lldebug_eval_metatable__ in the global table.
		// table = {__index=func1, __newindex=func2}
		lua_newtable(L);
		lua_pushliteral(L, "__index");
		lua_pushinteger(L, level);
		lua_pushcclosure(L, LuaImpl::index_for_eval, 1);
		lua_rawset(L, -3);
		lua_pushliteral(L, "__newindex");
		lua_pushinteger(L, level);
		lua_pushcclosure(L, LuaImpl::newindex_for_eval, 1);
		lua_rawset(L, -3);

		// globals[__lldebug_eval_metatable__] = table
		lua_pushliteral(L, "__lldebug_eval_metatable__");
		lua_pushvalue(L, -2);
		lua_rawset(L, LUA_GLOBALSINDEX);
		lua_pop(L, 1);
	}

	// on exit: globals[__lldebug_eval_metatable__] = nil
	struct call_on_exit {
		lua_State *L;
		call_on_exit(lua_State *L_) : L(L_) {}
		~call_on_exit() {
			lua_pushliteral(L, "__lldebug_eval_metatable__");
			lua_pushnil(L);
			lua_rawset(L, LUA_GLOBALSINDEX);
		}
	} exit_obj(L);

	// Load string using lua_load.
	// (existence of luaL_loadstring depends on the lua version)
	eval_string_reader reader(str, beginning, ending);
	if (lua_load(L, eval_string_reader::exec, &reader, DUMMY_FUNCNAME) != 0) {
		return -1;
	}

	// Do execute !
	if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0) {
		return -1;
	}

	return 0;
}

LuaVarList Context::LuaEvalsToVarList(const string_array &evals,
									  const LuaStackFrame &stackFrame) {
	lua_State *L = stackFrame.GetLua().GetState();
	if (L == NULL) L = GetLua();
	scoped_lock lock(m_mutex);
	scoped_lua scoped(this, L, 0);
	LuaVarList result;

	string_array::const_iterator it;
	for (it = evals.begin(); it != evals.end(); ++it) {
		result.push_back(LuaEvalToVar(*it, stackFrame));
	}

	return result;
}

LuaVarList Context::LuaEvalToMultiVar(const std::string &eval,
									  const LuaStackFrame &stackFrame) {
	lua_State *L = stackFrame.GetLua().GetState();
	if (L == NULL) L = GetLua();
	scoped_lock lock(m_mutex);
	scoped_lua scoped(this, L, 0);
	LuaVarList result;
	int beginningtop = lua_gettop(L);

	if (LuaEval(L, stackFrame.GetLevel(), eval) != 0) {
		std::string error = lua_tostring(L, -1);
		lua_pop(L, 1);
		result.push_back(LuaVar(LuaHandle(L), "<error>", ParseLuaError(error).message));
		return result;
	}

	// Convert the result to LuaVar objects.
	int top = lua_gettop(L);
	for (int idx = beginningtop + 1; idx <= top; ++idx) {
		result.push_back(LuaVar(LuaHandle(L), eval, idx));
	}

	lua_settop(L, beginningtop); // adjust the stack top
	return result;
}

LuaVar Context::LuaEvalToVar(const std::string &eval,
							 const LuaStackFrame &stackFrame) {
	lua_State *L = stackFrame.GetLua().GetState();
	if (L == NULL) L = GetLua();
	scoped_lock lock(m_mutex);
	scoped_lua scoped(this, L, 0);
	int beginningtop = lua_gettop(L);

	if (LuaEval(L, stackFrame.GetLevel(), eval) != 0) {
		std::string error = lua_tostring(L, -1);
		lua_pop(L, 1);
		return LuaVar(LuaHandle(L), "<error>", ParseLuaError(error).message);
	}

	// Convert the result to a LuaVar object.
	int top = lua_gettop(L);
	if (top == beginningtop) {
		return LuaVar();
	}
	else {
		LuaVar result(LuaHandle(L), eval, beginningtop + 1);
		lua_settop(L, beginningtop); // adjust the stack top
		return result;
	}
}

} // end of namespace lldebug