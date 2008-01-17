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

#ifndef __LLDEBUG_CONTEXT_H__
#define __LLDEBUG_CONTEXT_H__

#include "lldebug_sysinfo.h"
#include "lldebug_luainfo.h"
#include "lldebug_remoteengine.h"

namespace lldebug {

class MainFrame;

enum TableType {
	TABLETYPE_GLOBAL,
	TABLETYPE_REGISTRY,
	TABLETYPE_ENVIRON,
};

class Context {
public:
	enum State {
		STATE_INITIAL,
		STATE_NORMAL,
		STATE_STEPOVER,
		STATE_STEPINTO,
		STATE_STEPRETURN,
		STATE_BREAK,
		STATE_QUIT,
	};

	class scoped_lua;
	friend class scoped_lua;

public:
	static Context *Create();
	virtual void Delete();

	static Context *Find(lua_State *L);
	virtual void Quit();

	/// ��������E�B���h�E�ɏo�͂��܂��B
	void Output(const std::string &str);
	void OutputF(const char *fmt, ...);

	int LuaOpenBase(lua_State *L);
	void LuaOpenLibs(lua_State *L);

	LuaVarList LuaGetLocals(lua_State *L, int level);
	LuaVarList LuaGetFields(TableType type);
	LuaVarList LuaGetFields(const LuaVar &var);
	LuaStackList LuaGetStack();
	LuaBackTrace LuaGetBackTrace();
	int LuaEval(const std::string &str, lua_State *L = NULL);

	/// �R���e�L�X�g�̂h�c���擾���܂��B
	int GetId() const {
		return m_id;
	}

	/// lua�I�u�W�F�N�g���擾���܂��B
	lua_State *GetLua() {
		return m_coroutines.back().L;
	}

	/// ��ԍŏ��ɍ쐬���ꂽlua�I�u�W�F�N�g���擾���܂��B
	lua_State *GetMainLua() {
		return m_lua;
	}

	/// �R�}���h��o�^���܂��B
	int PushCommand(Command::Type type) {
		return m_cmdQueue.PushCommand(type);
	}

	/// �\�[�X�t�@�C������ێ������I�u�W�F�N�g���擾���܂��B
	const Source *GetSource(const std::string &key) {
		return m_sourceManager.Get(key);
	}

	/// �\�[�X���Z�[�u���܂��B
	int SaveSource(const std::string &key, const string_array &source) {
		return m_sourceManager.Save(key, source);
	}

	/// �u���C�N�|�C���g���擾���܂��B
	const BreakPoint &GetBreakPoint(size_t i) {
		return m_breakPoints.Get(i);
	}

	/// �u���C�N�|�C���g�̍��v���ł��B
	size_t GetBreakPointSize() {
		return m_breakPoints.GetSize();
	}

	/// �u���C�N�|�C���g�𔭌����܂��B
	const BreakPoint *FindBreakPoint(const std::string &key, int line) {
		return m_breakPoints.Find(key, line);
	}

	/// �u���C�N�|�C���g��ǉ����܂��B
	void AddBreakPoint(const BreakPoint &bp) {
		m_breakPoints.Add(bp);
	}

	/// �u���C�N�|�C���g�̃I���^�I�t��؂�ւ��܂��B
	void ToggleBreakPoint(const std::string &key, int line) {
		m_breakPoints.Toggle(key, line);
	}

	MainFrame *GetFrame() {
		return m_frame;
	}

private:
	explicit Context();
	virtual ~Context();
	virtual int Initialize();
	virtual int CreateDebuggerFrame();
	virtual int LoadConfig();
	virtual int SaveConfig();
	virtual void SetState(State state);
	virtual int HandleCommand();

	static void SetHook(lua_State *L, bool enable);
	virtual void HookCallback(lua_State *L, lua_Debug *ar);
	static void s_HookCallback(lua_State *L, lua_Debug *ar);

	class LuaImpl;
	friend class LuaImpl;
	int LuaInitialize(lua_State *L);
	void BeginCoroutine(lua_State *L);
	void EndCoroutine(lua_State *L);

protected:
	friend class MainFrame;
	void SetFrame(MainFrame *frame);

private:
	class ContextManager;
	static shared_ptr<ContextManager> ms_manager;
	static int ms_idCounter;

	int m_id;
	lua_State *m_lua;
	State m_state;

	class scoped_current_source;
	friend class scoped_current_source;
	const char *m_currentSourceKey;
	int m_currentLine;

	/// lua_State *���Ƃ̊֐��Ăяo���񐔂��L�^���邱�Ƃ�
	/// �X�e�b�v�I�[�o�[�����S�Ɏ������܂��B
	struct CoroutineInfo {
		CoroutineInfo(lua_State *L_ = NULL, int call_ = 0)
			: L(L_), call(call_) {
		}
		lua_State *L;
		int call;
	};
	typedef std::vector<CoroutineInfo> CoroutineList;
	CoroutineList m_coroutines;
	CoroutineInfo m_stepinfo;

	RemoteEngine m_engine;
	CommandQueue m_cmdQueue;
	SourceManager m_sourceManager;
	BreakPointList m_breakPoints;
	std::string m_rootFileKey;

	MainFrame *m_frame;
	mutex m_mutex;
	condition m_condFrame;
};

/**
 * @brief ����̃X�R�[�v��lua���g�����߂̃N���X�ł��B
 */
class Context::scoped_lua {
public:
	explicit scoped_lua(lua_State *L, int n, int npop = 0)
		: m_L(L), m_npop(npop) {
		m_pos = lua_gettop(L) + n;
		m_oldhook = (lua_gethook(L) != NULL);
		Context::SetHook(L, false);
	}

	~scoped_lua() {
		Context::SetHook(m_L, m_oldhook);
		//wxASSERT(m_pos == lua_gettop(m_L));
		lua_pop(m_L, m_npop);
	}

private:
	lua_State *m_L;
	int m_pos;
	int m_npop;
	bool m_oldhook;
};

}

#endif

