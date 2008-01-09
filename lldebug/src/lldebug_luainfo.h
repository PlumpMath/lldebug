//---------------------------------------------------------------------------
//
// Name:        Project1App.h
// Author:      �딎
// Created:     2007/11/23 0:05:32
// Description: 
//
//---------------------------------------------------------------------------

#ifndef __LLDEBUG_LUAINFO_H__
#define __LLDEBUG_LUAINFO_H__

namespace lldebug {

std::string LuaToString(lua_State *L, int idx);

enum VarRootType {
	VARROOT_GLOBAL,
	VARROOT_LOCAL,
	VARROOT_REGISTRY,
	VARROOT_ENVIRON,
	VARROOT_STACK,
};

/**
 * @brief lua�̕ϐ�����ێ����܂��B
 */
class LuaVar {
public:
	explicit LuaVar();
	LuaVar(lua_State *L, VarRootType type, int level = -1);
	LuaVar(shared_ptr<LuaVar> parent, const std::string &name, int valueIdx);
	LuaVar(shared_ptr<LuaVar> parent, int keyIdx, int valueIdx);
	virtual ~LuaVar();

	/// ���̃I�u�W�F�N�g���L�����ǂ����擾���܂��B
	bool IsOk() const {
		return (m_parent != NULL);
	}

	/// ���̕ϐ�������lua_State *���擾���܂��B
	lua_State *GetLua() const {
		return m_L;
	}

	/// ���̃e�[�u���^�C�v���擾���܂��B
	VarRootType GetRootType() const {
		return m_rootType;
	}

	/// ���e�[�u�������[�J���������Ƃ��̃X�^�b�N�t���[�����x�����擾���܂��B
	int GetLevel() const {
		return m_level;
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
		return lua_typename(NULL, m_valueType);
	}

	/// �ϐ����t�B�[���h�������ǂ������擾���܂��B
	bool HasFields() const {
		return m_hasFields;
	}

	/// �ϐ����t�B�[���h�������ǂ�����ݒ肵�܂��B
	void SetHasFields(bool hasFields) {
		m_hasFields = hasFields;
	}
	/// �e���擾���܂��B
	shared_ptr<LuaVar> GetParent() const {
		return m_parent;
	}

	friend bool operator==(const LuaVar &x, const LuaVar &y) {
		bool xhasp = (x.m_parent == NULL);
		bool yhasp = (y.m_parent == NULL);
		if (xhasp != yhasp) {
			return false;
		}

		if (x.m_L != y.m_L || x.m_name != y.m_name) {
			return false;
		}

		if (x.m_parent == NULL) {
			return (
				x.m_rootType == y.m_rootType &&
				x.m_level == y.m_level);
		}
		else {
			return (*x.m_parent == *y.m_parent);
		}
	}

	friend bool operator!=(const LuaVar &x, const LuaVar &y) {
		return !(x == y);
	}

private:
	lua_State *m_L;
	VarRootType m_rootType;
	int m_level;  ///< ��ꂪ���[�J���������ꍇ�̃X�^�b�N�t���[�����x���ł��B
	shared_ptr<LuaVar> m_parent;

	std::string m_name;
	std::string m_value;
	int m_valueType;
	bool m_hasFields;
};


/**
 * @brief �֐��̌Ăяo��������ێ����܂��B
 */
class LuaBackTraceInfo {
public:
	explicit LuaBackTraceInfo(lua_State *L = NULL, int level = 0,
							  lua_Debug *ar = NULL,
							  const std::string &sourceTitle = std::string(""));
	~LuaBackTraceInfo();

	/// ���̃R�[�����Ȃ��ꂽlua_State�ł��B
	lua_State *GetLua() const {
		return m_lua;
	}

	/// �֐��̃X�^�b�N���x���ł��B
	int GetLevel() const {
		return m_level;
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

	/// �Ăяo�����̊֐��̖��O�ł��B
	const std::string &GetFuncName() const {
		return m_funcName;
	}

public:
	lua_State *m_lua;
	int m_level;

	std::string m_key;
	std::string m_sourceTitle;
	int m_line;
	std::string m_funcName;
};

typedef std::vector<LuaVar> LuaVarList;
typedef std::vector<LuaVar> LuaStackList;
typedef std::vector<LuaBackTraceInfo> LuaBackTrace;

}

#endif
