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

#include "lldebug_prec.h"
#include "lldebug_bootstrap.h"
#include <boost/function.hpp>

namespace lldebug {

Bootstrap *Bootstrap::ms_instance = NULL;

Bootstrap *Bootstrap::Get() {
	return ms_instance;
}

Bootstrap::Bootstrap()
	: m_state(INITSTATE_INITIAL) {
	wxASSERT(ms_instance == NULL);
	ms_instance = this;
}

Bootstrap::~Bootstrap() {
	Exit();
	WaitExit();
	ms_instance = NULL;
}

void Bootstrap::Success(bool flag) {
	scoped_lock lock(m_mutex);

	if (m_state != INITSTATE_INITIAL) {
		return;
	}

	// inited��true�ɂ����̂��Acond�̕ω���ʒm���܂��B
	// ���̎��_��cond.wait���Ă�ł���ӏ��݂̂��̃E�F�C�g����������܂��B
	//
	// ��
	// void thread1() {
	//   scoped_lock lock(mutex);
	//   inited = true;
	//   cond.notify_all();
	// }
	// 
	// void thread2() {
	//   scoped_lock lock(mutex);
	//   while (! inited)
	//     cond.wait(mutex); // wait�Ɠ�����mutex�̃��b�N�������B
	// }
	// 
	// 1, notify �� wait �̑O�ɌĂ΂ꂽ�ꍇ
	//   mutex�ŕϐ���ی삵����Ainited��true�ɂ������m�点�܂��B
	//   wait���ł�mutex�̃��b�N���������ꂽ���inited��true�Ȃ̂�
	//   �؂�Ȃ��������I�����܂��B
	// 
	// 2, notify �� wait �̌�ɌĂ΂ꂽ�ꍇ
	//   mutex�ŕϐ���ی삵����Ainited�l�𒲂ׁAfalse�Ȃ�wait�B
	//   wait�����ł͂�����mutex�̃��b�N���������܂��B
	//   ���̌�Athread1����notify_all���Ă΂�inited�̒l�𒲂׏������I�����܂��B
	//
	m_state = (flag ? INITSTATE_INITED : INITSTATE_EXITED);
	m_cond.notify_all();
}
class ThreadObj {
public:
	explicit ThreadObj(int argc, char **argv)
		: m_argc(argc), m_argv(argv) {
	}
	void operator()() {
		wxEntry(m_argc, m_argv);
	}
private:
	int m_argc;
	char **m_argv;
};

int Bootstrap::Initialize() {
	scoped_lock lock(m_mutex);

	if (m_thread != NULL || m_state != INITSTATE_INITIAL) {
		return -1;
	}

	boost::function0<void> fn = ThreadObj(0, NULL);
	shared_ptr<thread> th(new thread(fn));
	if (th == NULL) {
		return -1;
	}

	// �܂�Application�N���X�̏�������҂��܂��B
	while (true) {
		switch (m_state) {
		case INITSTATE_INITIAL:
			break;
		case INITSTATE_INITED:
			goto loop_quit;
		case INITSTATE_EXITED:
			th->join();
			return -1;
		}

		m_cond.wait(lock);
	}
loop_quit:;

	// �I�����Ɏg����wxTheApp->Exit��wxAppBase�N���X������
	// MainLoopManual�I�u�W�F�N�g�ɑ΂��ďI���������o���܂����A
	// ���C�����[�v���n�܂��Ă��Ȃ��Ƃ��̃I�u�W�F�N�g���̂�
	// ����Ă��Ȃ��\�����o�Ă��܂��B
	boost::xtime end;
	boost::xtime_get(&end, boost::TIME_UTC);
	end.sec += 10;
	while (wxApp::GetInstance() == NULL || !wxTheApp->IsMainLoopRunning()) {
		boost::xtime current;
		boost::xtime_get(&current, boost::TIME_UTC);

		// �P�O�b�ȏソ�����炠����߂܂��B
		if (boost::xtime_cmp(current, end) >= 0) break;
		boost::thread::yield();
		
	}

	m_thread = th;
	return 0;
}

void Bootstrap::Exit() {
	if (wxApp::GetInstance() != NULL) {
		wxTheApp->Exit();
		wxWakeUpIdle();
	}
}

void Bootstrap::WaitExit() {
	scoped_lock lock(m_mutex);
	if (m_thread == NULL) {
		return;
	}

	m_thread->join();
	m_thread.reset();
	m_state = INITSTATE_EXITED;
}

}
