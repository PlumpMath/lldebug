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

#ifndef __LLDEBUG_SYSINFO_H__
#define __LLDEBUG_SYSINFO_H__

namespace lldebug {

std::string GetConfigFileName(const std::string &filename);

/**
 * @brief �X�N���v�g�₻�̎��s�ɑ΂��Ė��߂������N���X�ł��B
 */
class Command {
public:
	/**
	 * @brief �R�}���h�����ʂ��܂��B
	 */
	enum Type {
		TYPE_NONE,
		TYPE_PAUSE,
		TYPE_RESTART,
		TYPE_TOGGLE,
		TYPE_STEPOVER,
		TYPE_STEPINTO,
		TYPE_STEPRETURN,
		TYPE_QUIT,
	};

public:
	explicit Command(Type type);
	virtual ~Command();

	/// �R�}���h�̎�ނ��擾���܂��B
	Type GetType() const {
		return m_type;
	}

private:
	Type m_type;
};

/**
 * @brief �R�}���h�L���[�ł��B
 */
class CommandQueue {
public:
	explicit CommandQueue();
	virtual ~CommandQueue();

	/// �R�}���h���ő�maxSec�b�ԑ҂��܂��B
	virtual bool Wait(size_t maxSec);

	/// �R�}���h���擾���܂��B
	virtual const Command &Get();

	/// �R�}���h����ǉ����܂��B
	virtual int Push(const Command &cmd);

	/// �R�}���h����ǉ����܂��B
	virtual int PushCommand(Command::Type type);

	/// �R�}���h����폜���܂��B
	virtual void Pop();

	/// �R�}���h���󂩂ǂ������ׂ܂��B
	virtual bool IsEmpty();

private:
	typedef std::list<Command> QueueImpl;
	QueueImpl m_queue;

	mutex m_mutex;
	condition m_cond;
};


/**
 * @brief �f�o�b�K�̃u���C�N�|�C���g�I�u�W�F�N�g�ł��B
 */
class BreakPoint {
public:
	explicit BreakPoint(const std::string &key = std::string(""), int line = 0,
						bool isInternal = false, bool isTemp = false);
	~BreakPoint();

	/// �ݒ肳��Ă���t�@�C���̎��ʎq���擾���܂��B
	const std::string &GetKey() const {
		return m_key;
	}

	/// �ݒ肳��Ă���s�����擾���܂��B
	int GetLine() const {
		return m_line;
	}

	/// �����Ŏg���u���[�N�|�C���g���H
	bool IsInternal() const {
		return m_isInternal;
	}

	/// �ꎞ�I�ȃu���[�N�|�C���g���H
	bool IsTemp() const {
		return m_isTemp;
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar & LLDEBUG_MEMBER_NVP(key);
		ar & LLDEBUG_MEMBER_NVP(line);
	}

private:
	std::string m_key;
	int m_line;
	bool m_isInternal;
	bool m_isTemp;
};

/**
 * @brief �u���C�N�|�C���g�̃��X�g���擾���܂��B
 */
class BreakPointList {
public:
	explicit BreakPointList();
	virtual ~BreakPointList();

	/// �u���C�N�|�C���g���擾���܂�(i��0origin)�B
	const BreakPoint &Get(size_t i);

	/// �u���C�N�|�C���g�̍��v�T�C�Y���擾���܂��B
	size_t GetSize();

	/// ����̈ʒu�ɂ���u���C�N�|�C���g��T���o���܂��B
	const BreakPoint *Find(const std::string &key, int line);

	/// �u���C�N�|�C���g��V�K�ɒǉ����܂��B
	void Add(const BreakPoint &bp);

	/// �u���C�N�|�C���g�̃I���^�I�t��؂�ւ��܂��B
	void Toggle(const std::string &key, int line);

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar & LLDEBUG_MEMBER_NVP(breakPoints);
	}

private:
	mutex m_mutex;

	typedef std::vector<BreakPoint> ImplList;
	ImplList m_breakPoints;
};


/**
 * @brief �f�o�b�O���ɕ\�������\�[�X�t�@�C���Ȃǂ��Ǘ����܂��B
 */
class Source {
public:
	explicit Source(const std::string &key = std::string(""),
					const std::string &title = std::string(""),
					const string_array &sources = string_array(),
					const std::string &path = std::string(""));
	~Source();

	/// �\�[�X�̎��ʎq���擾���܂��B
	const std::string &GetKey() const {
		return m_key;
	}

	/// �\�[�X�̃^�C�g����UTF8�`���Ŏ擾���܂��B
	const std::string &GetTitle() const {
		return m_title;
	}

	/// �\�[�X�̃p�X������΂�����l�C�e�B�u�̃G���R�[�f�B���O�`���Ŏ擾���܂��B
	const std::string &GetPath() const {
		return m_path;
	}

	/// �\�[�X�̕������UTF8�`���Ŏ擾���܂��B
	const string_array &GetSources() const {
		return m_sources;
	}

	/// �\�[�X�̍s�����擾���܂��B
	string_array::size_type GetNumberOfLines() const {
		return m_sources.size();
	}

	/// �\�[�X����s�̕������UTF8�`���Ŏ擾���܂��B
	const std::string &GetSourceLine(string_array::size_type l) const {
		return m_sources[l];
	}

	/// �ǂݍ��ݎ��̃\�[�X�̃G���R�[�f�B���O���擾���܂��B
	int GetSourceEncoding() const {
		return m_sourceEncoding;
	}

private:
	std::string m_key;
	std::string m_title;
	std::string m_path;
	string_array m_sources;
	int m_sourceEncoding;
};

/**
 * @brief �f�o�b�O���ɕ\�������\�[�X�t�@�C���Ȃǂ��Ǘ����܂��B
 */
class SourceManager {
public:
	explicit SourceManager();
	~SourceManager();

	/// �\�[�X�t�@�C����UTF8�`���Ŏ擾���܂��B
	const Source *Get(const std::string &key);

	/// �\�[�X���L�[����ǉ����܂��B
	/// (key��'@dir/filename.ext'���\�[�X���̂܂܂̕�����œn����܂��B)
	int Add(const std::string &key);

	/// �\�[�X���Z�[�u���܂��B
	int Save(const std::string &key, const string_array &source);

private:
	typedef std::map<std::string, Source> ImplMap;
	ImplMap m_sourceMap;
	int m_textCounter;

	mutex m_mutex;
};

}

#endif
