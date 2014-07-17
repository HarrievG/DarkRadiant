#include "TreeView.h"

#include <wx/popupwin.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

namespace wxutil
{

TreeView::TreeView(wxWindow* parent, TreeModel* model, long style) :
	wxDataViewCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style),
	_searchPopup(NULL)
{
	EnableAutoColumnWidthFix();

	if (model != NULL)
	{
		AssociateModel(model);
		model->DecRef();
	}

	Connect(wxEVT_CHAR, wxKeyEventHandler(TreeView::_onChar), NULL, this);
}

TreeView* TreeView::Create(wxWindow* parent, long style)
{
	return new TreeView(parent, NULL, style);
}

// Construct a TreeView using the given TreeModel, which will be associated
// with this view (refcount is automatically decreased by one).
TreeView* TreeView::CreateWithModel(wxWindow* parent, TreeModel* model, long style)
{
	return new TreeView(parent, model, style);
}

TreeView::~TreeView()
{}

// Enable the automatic recalculation of column widths
void TreeView::EnableAutoColumnWidthFix(bool enable)
{
	if (enable)
	{
		Connect(wxEVT_DATAVIEW_ITEM_EXPANDED, wxDataViewEventHandler(TreeView::_onItemExpanded), NULL, this);
	}
	else
	{
		Disconnect(wxEVT_DATAVIEW_ITEM_EXPANDED, wxDataViewEventHandler(TreeView::_onItemExpanded), NULL, this);
	}
}

void TreeView::_onItemExpanded(wxDataViewEvent& ev)
{
	// This should force a recalculation of the column width
	if (GetModel() != NULL)
	{
		GetModel()->ItemChanged(ev.GetItem());
	}

	ev.Skip();
}

// The custom popup window containing our search box
class TreeView::SearchPopupWindow :
	public wxPopupTransientWindow
{
private:
	TreeView* _owner;
	wxTextCtrl* _entry;

public:
	SearchPopupWindow(TreeView* owner) :
		wxPopupTransientWindow(owner),
		_owner(owner),
		_entry(NULL)
	{
		SetSizer(new wxBoxSizer(wxVERTICAL));

		_entry = new wxTextCtrl(this, wxID_ANY);

		GetSizer()->Add(_entry, 1, wxEXPAND | wxALL, 6);

		Layout();
		Fit();

		// Position this control in the bottom right corner
		wxPoint popupPos = owner->GetScreenPosition() + owner->GetSize() - GetSize();
		Position(popupPos, wxSize(0, 0));

		Connect(wxEVT_CHAR, wxKeyEventHandler(SearchPopupWindow::OnChar), NULL, this);
	}

	void OnChar(wxKeyEvent& ev)
	{
		HandleKey(ev);
	}

	virtual void OnDismiss()
	{
		// Let the owner know that we've been destroyed, just clear the pointer
		if (_owner->_searchPopup)
		{
			_owner->_searchPopup = NULL;
		}

		wxPopupTransientWindow::OnDismiss();
	}

	void HandleKey(wxKeyEvent& ev)
	{
		// Adapted this from the wxWidgets docs
		wxChar uc = ev.GetUnicodeKey();

		if (uc != WXK_NONE)
		{
			// It's a "normal" character. Notice that this includes
			// control characters in 1..31 range, e.g. WXK_RETURN or
			// WXK_BACK, so check for them explicitly.
			if (uc >= 32)
			{
				_entry->SetValue(_entry->GetValue() + ev.GetUnicodeKey());
			}
			else if (ev.GetKeyCode() == WXK_ESCAPE)
			{
				DismissAndNotify();
			}
		}
		else // No Unicode equivalent.
		{
			// TODO: Cursor events are special 
		}
	}
};


void TreeView::_onChar(wxKeyEvent& ev)
{
	if (_searchPopup == NULL)
	{
		_searchPopup = new SearchPopupWindow(this);
		_searchPopup->Popup();
	}

	// Handle the first key immediately
	_searchPopup->HandleKey(ev);

	// Don't eat the event
	ev.Skip();
}

} // namespace
