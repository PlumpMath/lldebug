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

#ifndef __LLDEBUG_LUAINFO_H__
#define __LLDEBUG_LUAINFO_H__

namespace lldebug {

/// A dummy object that offers original address for lua.
extern const int LuaOriginalObject;

/// Get the typename.
std::string LuaGetTypeName(int type);

#ifndef LLDEBUG_VISUAL
/// Convert the lua object placed on idx to string.
/// It doesn't use any lua functions.
std::string LuaToString(lua_State *L, int idx);

/// Convert the lua object placed on idx to string.
/// It uses lua functions.
std::string LuaConvertString(lua_State *L, int idx);

/// Get the original name of the lua function.
std::string LuaMakeFuncName(lua_Debug *ar);
#endif


/**
 * @brief �قȂ�A�v���P�[�V�����Ԃ�lua_State�̒l�����L���邽�߂ɕK�v�ȃN���X�ł��B
 */
class LuaHandle {
public:
	explicit LuaHandle()
		: m_luaState(0) {
	}

	LuaHandle(const LuaHandle &x)
		: m_luaState(x.m_luaState) {
	}

	~LuaHandle() {
	}

	LuaHandle &operator =(const LuaHandle &x) {
		m_luaState = x.m_luaState;
		return *this;
	}

#ifndef LLDEBUG_VISUAL
	LuaHandle(lua_State *L) {
		SetState(L);
	}

	LuaHandle &operator =(lua_State *x) {
		SetState(x);
		return *this;
	}

	lua_State *oprator() const {
		return GetState();
	}

	lua_State *GetState() const {
		return reinterpret_cast<lua_State *>(m_luaState);
	}

	void SetState(lua_State *L) {
		m_luaState = reinterpret_cast<boost::uint64_t>(L);
	}
#endif

	friend bool operator==(const LuaHandle &x, const LuaHandle &y) {
		return (x.m_luaState == y.m_luaState);
	}
	friend bool operator!=(const LuaHandle &x, const LuaHandle &y) {
		return !(x == y);
	}
	friend bool operator <(const LuaHandle &x, const LuaHandle &y) {
		return (x.m_luaState < y.m_luaState);
	}
	friend bool operator >(const LuaHandle &x, const LuaHandle &y) {
		return (y < x);
	}
	friend bool operator<=(const LuaHandle &x, const LuaHandle &y) {
		return !(x > y);
	}
	friend bool operator>=(const LuaHandle &x, const LuaHandle &y) {
		return !(x < y);
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int) {
		ar & LLDEBUG_MEMBER_NVP(luaState);
	}

private:
	boost::uint64_t m_luaState;
};

/**
 * @brief Stackframe object.
 */
class LuaStackFrame {
public:
	explicit LuaStackFrame(const LuaHandle &lua = LuaHandle(), int level = 0);
	~LuaStackFrame();

	/// Get the lua state object.
	LuaHandle &GetLua() {
		return m_lua;
	}

	/// Get the const lua state object.
	const LuaHandle &GetLua() const {
		return m_lua;
	}

	/// Get the level of the local stack frame.
	int GetLevel() const {
		return m_level;
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int) {
		ar & LLDEBUG_MEMBER_NVP(lua);
		ar & LLDEBUG_MEMBER_NVP(level);
	}

private:
	LuaHandle m_lua;
	int m_level;
};


/**
 * @brief lua�̕ϐ�����ێ����܂��B
 */
class LuaVar {
public:
	explicit LuaVar();
	virtual ~LuaVar();

#ifndef LLDEBUG_VISUAL
	LuaVar(const LuaHandle &lua, const std::string &name, int valueIdx);
	LuaVar(const LuaHandle &lua, const std::string &name, const std::string &value);

	/// Push the table value.
	int PushTable(lua_State *L) const;
#endif
	
	/// ���̃I�u�W�F�N�g���L�����ǂ����擾���܂��B
	bool IsOk() const {
		return (m_valueType >= 0);
	}

	/// ���̕ϐ�������lua_State *���擾���܂��B
	const LuaHandle &GetLua() const {
		return m_lua;
	}

	/// �ϐ���or�e�[�u���̃L�[�����擾���܂��B
	const std::string &GetName() const {
		return m_name;
	}

	/// �ϐ��̒l�𕶎���Ŏ擾���܂��B
	const std::string &GetValue() const {
		return m_value;
	}

	/// �ϐ��̒l�^���擾���܂��B
	int GetValueType() const {
		return m_valueType;
	}

	/// �ϐ��̒l�^�𕶎���Ŏ擾���܂��B
	std::string GetValueTypeName() const {
		return LuaGetTypeName(m_valueType);
	}

	/// �ϐ����t�B�[���h�������ǂ������擾���܂��B
	bool HasFields() const {
		return m_hasFields;
	}

protected:
#ifndef LLDEBUG_VISUAL
	/// Check weather the variable has fields.
	bool CheckHasFields(lua_State *L, int valueIdx) const;

	/// Register and save valueIdx to the internal table.
	int RegisterTable(lua_State *L, int valueIdx);
#endif

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int) {
		ar & LLDEBUG_MEMBER_NVP(lua);
		ar & LLDEBUG_MEMBER_NVP(name);
		ar & LLDEBUG_MEMBER_NVP(value);
		ar & LLDEBUG_MEMBER_NVP(valueType);
		ar & LLDEBUG_MEMBER_NVP(tableIdx);
		ar & LLDEBUG_MEMBER_NVP(hasFields);
	}

private:
	LuaHandle m_lua;
	std::string m_name;
	std::string m_value;
	int m_valueType;
	int m_tableIdx;
	bool m_hasFields;
};


/**
 * @brief �֐��̌Ăяo��������ێ����܂��B
 */
class LuaBacktrace {
public:
	explicit LuaBacktrace();
	~LuaBacktrace();

#ifndef LLDEBUG_VISUAL
	explicit LuaBacktrace(const LuaHandle &lua,
						  const std::string &name,
						  const std::string &sourceKey,
						  const std::string &sourceTitle,
						  int line, int level);
#endif

	/// ���̃R�[�����Ȃ��ꂽlua_State�ł��B
	const LuaHandle &GetLua() const {
		return m_lua;
	}

	/// �Ăяo�����̊֐��̖��O�ł��B
	const std::string &GetFuncName() const {
		return m_funcName;
	}

	/// ���̃R�[�����Ȃ��ꂽ�\�[�X�̎��ʎq�ł��B
	const std::string &GetKey() const {
		return m_key;
	}

	/// ���̃R�[�����Ȃ��ꂽ�\�[�X�̃^�C�g���ł��B
	const std::string &GetTitle() const {
		return m_sourceTitle;
	}

	/// ���̃R�[�����Ȃ��ꂽ�\�[�X�̍s�ԍ��ł��B
	int GetLine() const {
		return m_line;
	}

	/// �֐��̃X�^�b�N���x���ł��B
	int GetLevel() const {
		return m_level;
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int) {
		ar & LLDEBUG_MEMBER_NVP(lua);
		ar & LLDEBUG_MEMBER_NVP(funcName);
		ar & LLDEBUG_MEMBER_NVP(key);
		ar & LLDEBUG_MEMBER_NVP(sourceTitle);
		ar & LLDEBUG_MEMBER_NVP(line);
		ar & LLDEBUG_MEMBER_NVP(level);
	}

public:
	LuaHandle m_lua;
	std::string m_funcName;
	std::string m_key;
	std::string m_sourceTitle;
	int m_line;
	int m_level;
};

typedef std::vector<LuaVar> LuaVarList;
typedef std::vector<LuaVar> LuaStackList;
typedef std::vector<LuaBacktrace> LuaBacktraceList;

}

#endif
