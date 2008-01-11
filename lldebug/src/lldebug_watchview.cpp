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
#include "lldebug_watchview.h"
#include "lldebug_context.h"

namespace lldebug {

class WatchViewItemData : public wxTreeItemData {
public:
	explicit WatchViewItemData(const LuaVar &var)
		: m_var(var) {
	}

	virtual ~WatchViewItemData() {
	}

	const LuaVar &GetVar() const {
		return m_var;
	}

private:
	LuaVar m_var;
};

BEGIN_EVENT_TABLE(WatchView, wxTreeListCtrl)
	EVT_SIZE(WatchView::OnSize)
	EVT_SHOW(WatchView::OnShow)
	EVT_LLDEBUG_CHANGED_STATE(ID_LOCALWATCHVIEW, WatchView::OnChangedState)
	EVT_LLDEBUG_UPDATE_SOURCE(ID_LOCALWATCHVIEW, WatchView::OnUpdateSource)

	EVT_TREE_ITEM_EXPANDED(wxID_ANY, WatchView::OnExpanded)
	EVT_TREE_END_LABEL_EDIT(wxID_ANY, WatchView::OnEndLabelEdit)
	EVT_LIST_COL_END_DRAG(wxID_ANY, WatchView::OnColEndDrag)
END_EVENT_TABLE()

static int GetWatchViewId(WatchView::Type type) {
	switch (type) {
	case WatchView::TYPE_LOCALWATCH:
		return ID_LOCALWATCHVIEW;
	case WatchView::TYPE_GLOBALWATCH:
		return ID_GLOBALWATCHVIEW;
	case WatchView::TYPE_REGISTRYWATCH:
		return ID_REGISTRYWATCHVIEW;
	case WatchView::TYPE_ENVIRONWATCH:
		return ID_ENVIRONWATCHVIEW;
	case WatchView::TYPE_STACKWATCH:
		return ID_STACKWATCHVIEW;
	case WatchView::TYPE_WATCH:
		return ID_WATCHVIEW;
	}

	return -1;
}

WatchView::WatchView(Context *ctx, wxWindow *parent, Type type)
	: wxTreeListCtrl(parent
		, GetWatchViewId(type) + ctx->GetId()
		, wxDefaultPosition, wxDefaultSize
		, wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT
		| wxTR_EDIT_LABELS | wxTR_ROW_LINES | wxTR_COL_LINES
		| wxTR_FULL_ROW_HIGHLIGHT | wxALWAYS_SHOW_SB)
	, m_ctx(ctx), m_type(type), m_lua(NULL), m_level(0) {
	CreateGUIControls();
}

WatchView::~WatchView() {
}

void WatchView::CreateGUIControls() {
	scoped_lock lock(m_mutex);

	if (m_type == TYPE_STACKWATCH) {
		AddColumn(_("Index"), 80, wxALIGN_LEFT, -1, true, true);
	}
	else {
		AddColumn(_("Name"), 80, wxALIGN_LEFT, -1, true, true);
	}

	AddColumn(_("Value"), 120, wxALIGN_LEFT, -1, true, true);
	AddColumn(_("Type"), 60, wxALIGN_LEFT, -1, true, true);
	SetLineSpacing(2);

	AddRoot(wxT(""));
}

WatchView::wxTreeItemIdList WatchView::GetItemChildren(const wxTreeItemId &item) {
	scoped_lock lock(m_mutex);
	wxTreeItemIdList result;
	wxTreeItemIdValue cookie;

	for (wxTreeItemId child = GetFirstChild(item, cookie);
		child.IsOk();
		child = GetNextChild(item, cookie)) {
		result.push_back(child);
	}

	return result;
}

WatchViewItemData *WatchView::GetItemData(const wxTreeItemId &item) {
	scoped_lock lock(m_mutex);
	return static_cast<WatchViewItemData *>(wxTreeListCtrl::GetItemData(item));
}

LuaVarList WatchView::GetLuaVarList(lua_State *L, int level) {
	scoped_lock lock(m_mutex);

	switch (m_type) {
	case TYPE_LOCALWATCH:
		return m_ctx->LuaGetLocals(L, level);
	case TYPE_GLOBALWATCH:
		return m_ctx->LuaGetFields(TABLETYPE_GLOBAL);
	case TYPE_REGISTRYWATCH:
		return m_ctx->LuaGetFields(TABLETYPE_REGISTRY);
	case TYPE_ENVIRONWATCH:
		return m_ctx->LuaGetFields(TABLETYPE_ENVIRON);
	case TYPE_STACKWATCH: {
		LuaVarList vars = m_ctx->LuaGetStack();
		std::reverse(vars.begin(), vars.end());
		return vars;
		}
	case TYPE_WATCH:
		return LuaVarList();
	}

	return LuaVarList();
}

struct CompareItem {
	WatchView *watch;
	const LuaVar &var;

	explicit CompareItem(WatchView *watch_, const LuaVar &var_)
		: watch(watch_), var(var_) {
	}

	bool operator()(const wxTreeItemId &item) {
		WatchViewItemData *data = watch->GetItemData(item);
		return (data == NULL ? false : data->GetVar() == var);
	}
};

void WatchView::UpdateVars(wxTreeItemId parent, const LuaVarList &vars) {
	// �����̎q�A�C�e�����X�g�ŁA�A�C�e���̒ǉ��E�폜���Ɏg���܂��B
	wxTreeItemIdList children = GetItemChildren(parent);

	for (LuaVarList::size_type i = 0; i < vars.size(); ++i) {
		const LuaVar &var = vars[i];
		wxTreeItemIdList::iterator it;
		wxTreeItemId item;

		// �q�A�C�e�������łɓo�^����Ă��邩�T���܂��B
		it = std::find_if(children.begin(), children.end(), CompareItem(this, var));
		if (it == children.end()) {
			item = AppendItem(parent, wxEmptyString, -1, -1, new WatchViewItemData(var));

			wxString name = wxConvFromUTF8(var.GetName());
			SetItemText(item, 0, name);
		}
		else {
			// �����̃A�C�e�����������炻����g���܂��B
			item = *it;
			children.erase(it);

			// �Â��f�[�^�͍폜���Ȃ��ƃ��[�N���܂��B
			WatchViewItemData *oldData = GetItemData(item);
			if (oldData != NULL) {
				delete oldData;
			}
			SetItemData(item, new WatchViewItemData(var));
		}

		// �J������ݒ肵�܂��B
		wxString value = wxConvFromUTF8(var.GetValue());
		wxString type = wxConvFromUTF8(var.GetValueTypeName());
		SetItemText(item, 1, value);
		SetItemText(item, 2, type);

		// �q�A�C�e��������΂�����X�V���܂��B
		if (var.HasFields()) {
			if (HasChildren(item)) {
				if (IsExpanded(item)) {
					LuaVarList childrenVars = m_ctx->LuaGetFields(var);
					UpdateVars(item, childrenVars);
				}
			}
			else {
				// �J���Ă��Ȃ��c���[�A�C�e����
				// [���̃A�C�e���͎q������]�L�����o���܂��B
				AppendItem(item, wxT("$<dummy item for lazy evalution>"));
			}
		}
		else {
			// �A�C�e�����J����Ă��邩�̏�Ԃ͎q�����Ȃ��Ȃ��Ă��ۑ�����܂��B
			// �Ȃ̂ňꉞ���Ă����܂��B
			Collapse(item);

			// �A�C�e���������Ȃ�q�A�C�e���������܂��B
			DeleteChildren(item);
		}
	}

	// �q�A�C�e���Ɏc���Ă���A�C�e���͍폜�ΏۂɂȂ�܂��B
	for (wxTreeItemIdList::iterator it = children.begin();
		it != children.end();
		++it) {
		Delete(*it);
	}
}

void WatchView::OnChangedState(wxChangedStateEvent &event) {
	scoped_lock lock(m_mutex);
	event.Skip();

	Enable(event.GetValue());
	/*if (event.GetValue()) {
		if (IsEnabled() && IsShown()) {
			UpdateVars(GetRootItem(), GetLuaVarList(0));
		}
	}
	else {
	}*/
}

void WatchView::OnUpdateSource(wxSourceLineEvent &event) {
	scoped_lock lock(m_mutex);
	event.Skip();

	if (IsEnabled() && IsShown()) {
		UpdateVars(GetRootItem(),
			GetLuaVarList(event.GetLua(), event.GetLevel()));
		m_lua = event.GetLua();
		m_level = event.GetLevel();
	}
}

void WatchView::OnShow(wxShowEvent &event) {
	scoped_lock lock(m_mutex);
	event.Skip();

	if (event.GetShow() && IsEnabled() && IsShown()) {
		UpdateVars(GetRootItem(), GetLuaVarList(m_lua, m_level));
	}
}

void WatchView::OnExpanded(wxTreeEvent &event) {
	scoped_lock lock(m_mutex);
	event.Skip();

	wxTreeItemId item = event.GetItem();
	WatchViewItemData *data = GetItemData(item);
	if (IsEnabled() && IsShown()) {
		UpdateVars(item, m_ctx->LuaGetFields(data->GetVar()));
	}
}

void WatchView::OnEndLabelEdit(wxTreeEvent &event) {
	scoped_lock lock(m_mutex);
	event.Skip();

	if (m_type == TYPE_WATCH) {
		wxTreeItemId item = event.GetItem();
		WatchViewItemData *data = GetItemData(item);
	}
}

void WatchView::LayoutColumn(int selectedColumn) {
	scoped_lock lock(m_mutex);

	// ���v�J�����T�C�Y���擾���܂��B
	int col_w = 0;
	int sel_w = 0;
	for (int i = 0; i < GetColumnCount(); ++i) {
		if (i <= selectedColumn && i != GetColumnCount() - 1) {
			sel_w += GetColumnWidth(i); 
		}
		else {
			col_w += GetColumnWidth(i);
		}
	}
	
	// ����������ꍇ�̓G���[�Ƃ��Ĉ����܂��B
	int width = GetClientSize().GetWidth();
				//- wxSystemSettings::GetMetric(wxSYS_VSCROLL_X) - 2;
	double rate = (double)(width - sel_w) / col_w;
	if (rate < 0.0001) {
		return;
	}

	// �J�����T�C�Y�𓯂��䗦�����ω������܂��B
	for (int i = 0; i < GetColumnCount(); ++i) {
		if (i <= selectedColumn && i != GetColumnCount() - 1) {
			continue;
		}

		int w = GetColumnWidth(i);
		SetColumnWidth(i, (int)(w * rate));
	}
}

void WatchView::OnSize(wxSizeEvent &event) {
	event.Skip();
	LayoutColumn(-1);
}

void WatchView::OnColEndDrag(wxListEvent &event) {
	event.Skip();
	LayoutColumn(event.GetColumn());
}

}
