//---------------------------------------------------------------------------
//
// Name:        Project1App.h
// Author:      �딎
// Created:     2007/11/23 0:05:32
// Description: 
//
//---------------------------------------------------------------------------

#ifndef __LLDEBUG_BOOTSTRAP_H__
#define __LLDEBUG_BOOTSTRAP_H__

namespace lldebug {

/**
 * @brief wxWidgets��ʃX���b�h�ŊJ�n���邽�߂̃N���X�ł��B
 * 
 * wxWidgets��ʃX���b�h����J�n����Ƃ��ɖ��ɂȂ�_��
 * �EwxEntry���Ăяo������łȂ��ƃX���b�h���g�p�ł��Ȃ�
 * �EwxEntryStart��wxApp::OnRun�𓯂��X���b�h����Ăяo���Ȃ��Ƃ����Ȃ�
 * �Ȃǂł��B
 * 
 * �܂�ŏ������́AwxThread�ȊO�̃X���b�h�@�\��p����K�v������܂��B
 */
class Bootstrap {
public:
	explicit Bootstrap();
	virtual ~Bootstrap();

	virtual int Initialize();
	virtual void Exit();
	virtual void WaitExit();

protected:
	friend class Application;
	static Bootstrap *Get();
	virtual void Success(bool flag);

private:
	/// Initializer�̏�Ԃ����ʂ��܂��B
	enum InitState {
		INITSTATE_INITIAL,
		INITSTATE_INITED,
		INITSTATE_EXITED,
	};

	static Bootstrap *ms_instance;
	shared_ptr<thread> m_thread;
	mutex m_mutex;
	condition m_cond;
	InitState m_state;
};

}

#endif
