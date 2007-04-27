#include "ResponseEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/TreeModel.h"
#include "string/string.h"

#include "EffectEditor.h"

namespace ui {
	
	namespace {
		const std::string LABEL_RESPONSE_EFFECTS = "<b>Response Effects</b>";
	}

ResponseEditor::ResponseEditor(GtkWidget* parent, StimTypes& stimTypes) :
	ClassEditor(stimTypes),
	_parent(parent)
{
	populatePage();
	createContextMenu();
}

void ResponseEditor::setEntity(SREntityPtr entity) {
	// Pass the call to the base class
	ClassEditor::setEntity(entity);
	
	if (entity != NULL) {
		GtkListStore* listStore = _entity->getResponseStore();
		gtk_tree_view_set_model(GTK_TREE_VIEW(_list), GTK_TREE_MODEL(listStore));
		g_object_unref(listStore); // treeview owns reference now
		
		// Clear the treeview (unset the model)
		gtk_tree_view_set_model(GTK_TREE_VIEW(_effectWidgets.view), NULL);
	}
}

void ResponseEditor::update() {
	_updatesDisabled = true;
	
	int id = getIdFromSelection();
	
	if (id > 0 && _entity != NULL) {
		gtk_widget_set_sensitive(_propertyWidgets.vbox, TRUE);
		
		StimResponse& sr = _entity->get(id);
		
		// Get the iter into the liststore pointing at the correct STIM_YYYY type
		GtkTreeIter typeIter = _stimTypes.getIterForName(sr.get("type"));
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(_type.list), &typeIter);
		
		// Active
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.active),
			(sr.get("state") == "1")
		);
		
		// Use Radius
		bool useRandomEffects = (sr.get("random_effects") != "");
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.randomEffectsToggle),
			useRandomEffects
		);
		gtk_entry_set_text(
			GTK_ENTRY(_propertyWidgets.randomEffectsEntry), 
			sr.get("random_effects").c_str()
		);
		gtk_widget_set_sensitive(
			_propertyWidgets.randomEffectsEntry, 
			useRandomEffects
		);
		
		// Use Chance
		bool useChance = (sr.get("chance") != "");
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.chanceToggle),
			useChance
		);
		gtk_spin_button_set_value(
			GTK_SPIN_BUTTON(_propertyWidgets.chanceEntry),
			strToFloat(sr.get("chance"))
		);
		gtk_widget_set_sensitive(
			_propertyWidgets.chanceEntry, 
			useChance
		);
		
		GtkListStore* effectStore = sr.getEffectStore();
		gtk_tree_view_set_model(GTK_TREE_VIEW(_effectWidgets.view), GTK_TREE_MODEL(effectStore));
		g_object_unref(effectStore);
		
		// Disable the editing of inherited properties completely
		if (sr.inherited()) {
			gtk_widget_set_sensitive(_propertyWidgets.vbox, FALSE);
		}
		
		// Update the delete context menu item
		gtk_widget_set_sensitive(_contextMenu.remove, !sr.inherited());
		
		// If there is anything selected, the duplicate item is always active
		gtk_widget_set_sensitive(_contextMenu.duplicate, TRUE);
		
		// Update the "enable/disable" menu items
		bool state = sr.get("state") == "1";
		gtk_widget_set_sensitive(_contextMenu.enable, !state);
		gtk_widget_set_sensitive(_contextMenu.disable, state);
		
		// The response effect list may be empty, so force an update of the
		// context menu sensitivity, in the case the "selection changed" 
		// signal doesn't get called
		updateEffectContextMenu();
	}
	else {
		// Nothing selected
		gtk_widget_set_sensitive(_propertyWidgets.vbox, FALSE);
		// Clear the effect tree view
		gtk_tree_view_set_model(GTK_TREE_VIEW(_effectWidgets.view), NULL);
		
		gtk_widget_set_sensitive(_contextMenu.enable, FALSE);
		gtk_widget_set_sensitive(_contextMenu.disable, FALSE);
		gtk_widget_set_sensitive(_contextMenu.remove, FALSE);
		gtk_widget_set_sensitive(_contextMenu.duplicate, FALSE);
	}
	
	_updatesDisabled = false;
}

void ResponseEditor::populatePage() {
	GtkWidget* srHBox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(_pageVBox), GTK_WIDGET(srHBox), TRUE, TRUE, 0);
	
	// List and buttons below
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), 
		gtkutil::ScrolledFrame(_list), TRUE, TRUE, 0);
	
	// Create the type selector plus buttons and pack them
	gtk_box_pack_start(GTK_BOX(vbox), createListButtons(), FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(srHBox),	vbox, FALSE, FALSE, 0);
	
	// Response property section
	_propertyWidgets.vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(srHBox), _propertyWidgets.vbox, TRUE, TRUE, 0);
	
	_type = createStimTypeSelector();
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), _type.hbox, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(_type.list), "changed", G_CALLBACK(onStimTypeSelect), this);
	
	// Create the table for the widget alignment
	GtkTable* table = GTK_TABLE(gtk_table_new(3, 2, FALSE));
	gtk_table_set_row_spacings(table, 6);
	gtk_table_set_col_spacings(table, 6);
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), GTK_WIDGET(table), FALSE, FALSE, 0);	
	
	// Active
	_propertyWidgets.active = gtk_check_button_new_with_label("Active");
	gtk_table_attach_defaults(table, _propertyWidgets.active, 0, 2, 0, 1);
		
	// Random Effects Toggle
	_propertyWidgets.randomEffectsToggle = gtk_check_button_new_with_label("Random Effects:");
	_propertyWidgets.randomEffectsEntry = gtk_entry_new();
	
	gtk_table_attach(table, _propertyWidgets.randomEffectsToggle, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(table, _propertyWidgets.randomEffectsEntry, 1, 2, 2, 3);
	
	// Chance variable
	_propertyWidgets.chanceToggle = gtk_check_button_new_with_label("Chance:");
	_propertyWidgets.chanceEntry = gtk_spin_button_new_with_range(0.0f, 1.0f, 0.1f);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(_propertyWidgets.chanceEntry), 2);
	
	gtk_table_attach(table, _propertyWidgets.chanceToggle, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(table, _propertyWidgets.chanceEntry, 1, 2, 1, 2);
	
	// Connect the signals
	g_signal_connect(G_OBJECT(_propertyWidgets.active), "toggled", G_CALLBACK(onCheckboxToggle), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.randomEffectsToggle), "toggled", G_CALLBACK(onCheckboxToggle), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.chanceToggle), "toggled", G_CALLBACK(onCheckboxToggle), this);
	
	g_signal_connect(G_OBJECT(_propertyWidgets.randomEffectsEntry), "changed", G_CALLBACK(onEntryChanged), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.chanceEntry), "value-changed", G_CALLBACK(onSpinButtonChanged), this);
	
	// The map associating entry fields to response property keys
	_entryWidgets[GTK_EDITABLE(_propertyWidgets.randomEffectsEntry)] = "random_effects";
	_spinWidgets[GTK_SPIN_BUTTON(_propertyWidgets.chanceEntry)] = "chance";
	
	gtk_box_pack_start(
		GTK_BOX(_propertyWidgets.vbox), 
		gtkutil::LeftAlignedLabel(LABEL_RESPONSE_EFFECTS),
		FALSE, FALSE, 0
	);
	gtk_box_pack_start(
		GTK_BOX(_propertyWidgets.vbox), 
		createEffectWidgets(),
		TRUE, TRUE, 0
	);
}

// Create the response effect list widgets
GtkWidget* ResponseEditor::createEffectWidgets() {

	_effectWidgets.view = gtk_tree_view_new();
	_effectWidgets.selection = 
		gtk_tree_view_get_selection(GTK_TREE_VIEW(_effectWidgets.view));
	gtk_widget_set_size_request(_effectWidgets.view, -1, 150);
	
	// Connect the signals
	g_signal_connect(G_OBJECT(_effectWidgets.selection), "changed",
					 G_CALLBACK(onEffectSelectionChange), this);
	g_signal_connect(G_OBJECT(_effectWidgets.view), "key-press-event", 
					 G_CALLBACK(onTreeViewKeyPress), this);
	g_signal_connect(G_OBJECT(_effectWidgets.view), "button-press-event",
					 G_CALLBACK(onTreeViewButtonPress), this);
	g_signal_connect(G_OBJECT(_effectWidgets.view), "button-release-event",
					 G_CALLBACK(onTreeViewButtonRelease), this);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_effectWidgets.view), 
		gtkutil::TextColumn("#", EFFECT_INDEX_COL)
	);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_effectWidgets.view), 
		gtkutil::TextColumn("Effect", EFFECT_CAPTION_COL)
	);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_effectWidgets.view), 
		gtkutil::TextColumn("Details (double-click to edit)", EFFECT_ARGS_COL)
	);
	
	// Return the tree view in a frame
	return gtkutil::ScrolledFrame(_effectWidgets.view);
}

void ResponseEditor::checkBoxToggled(GtkToggleButton* toggleButton) {
	GtkWidget* toggleWidget = GTK_WIDGET(toggleButton);
	bool active = gtk_toggle_button_get_active(toggleButton);
	
	if (toggleWidget == _propertyWidgets.active) {
		setProperty("state", active ? "1" : "0");
	}
	else if (toggleWidget == _propertyWidgets.randomEffectsToggle) {
		std::string entryText = 
			gtk_entry_get_text(GTK_ENTRY(_propertyWidgets.randomEffectsEntry));
	
		// Enter a default value for the entry text, if it's empty up till now.
		if (active) {
			entryText += (entryText.empty()) ? "1" : "";	
		}
		else {
			entryText = "";
		}
		setProperty("random_effects", entryText);
	}
	else if (toggleWidget == _propertyWidgets.chanceToggle) {
		std::string entryText = floatToStr(gtk_spin_button_get_value_as_float(
			GTK_SPIN_BUTTON(_propertyWidgets.chanceEntry)
		));

		setProperty("chance", active ? entryText : "");
	}
}

void ResponseEditor::addEffect() {
	if (_entity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		// Make sure we have a response
		if (sr.get("class") == "R") {
			// Add a new effect and update all the widgets
			sr.addEffect(effectIndex);
			update();
		}
	}
}

void ResponseEditor::removeEffect() {
	if (_entity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		// Make sure we have a response and anything selected
		if (sr.get("class") == "R" && effectIndex > 0) {
			// Remove the effect and update all the widgets
			sr.deleteEffect(effectIndex);
			update();
		}
	}
}

void ResponseEditor::editEffect() {
	if (_entity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		// Make sure we have a response and anything selected
		if (sr.get("class") == "R" && effectIndex > 0) {
			// Create a new effect editor (self-destructs)
			new EffectEditor(GTK_WINDOW(_parent), sr, 
							 effectIndex, _stimTypes, *this);
			
			// The editor is modal and will destroy itself, our work is done
		}
	}
}

void ResponseEditor::moveEffect(int direction) {
	if (_entity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		if (sr.get("class") == "R" && effectIndex > 0) {
			// Move the index (swap the specified indices)
			sr.moveEffect(effectIndex, effectIndex + direction);
			update();
			// Select the moved effect after the update
			selectEffectIndex(effectIndex + direction);
		}
	}
}

void ResponseEditor::updateEffectContextMenu() {
	// Check if we have anything selected at all
	int curEffectIndex = getEffectIdFromSelection();
	int highestEffectIndex = 0;
	
	bool anythingSelected = curEffectIndex >= 0;
	
	int srId = getIdFromSelection();
	if (srId > 0) {
		StimResponse& sr = _entity->get(srId);
		highestEffectIndex = sr.highestEffectIndex();
	}
	
	bool upActive = anythingSelected && curEffectIndex > 1;
	bool downActive = anythingSelected && curEffectIndex < highestEffectIndex;
	
	// Enable or disable the "Delete" context menu items based on the presence
	// of a selection.
	gtk_widget_set_sensitive(_effectWidgets.deleteMenuItem, anythingSelected);
	gtk_widget_set_sensitive(_effectWidgets.editMenuItem, anythingSelected);
		
	gtk_widget_set_sensitive(_effectWidgets.upMenuItem, upActive);
	gtk_widget_set_sensitive(_effectWidgets.downMenuItem, downActive);
}

// Create the context menus
void ResponseEditor::createContextMenu() {
	// Menu widgets
	_contextMenu.menu = gtk_menu_new();
	_effectWidgets.contextMenu = gtk_menu_new();
	
	// Each menu gets a delete item
	_contextMenu.remove = gtkutil::StockIconMenuItem(GTK_STOCK_DELETE, "Delete");
	//_contextMenu.add = gtkutil::StockIconMenuItem(GTK_STOCK_ADD, "Add");
	_contextMenu.enable = gtkutil::StockIconMenuItem(GTK_STOCK_YES, "Activate");
	_contextMenu.disable = gtkutil::StockIconMenuItem(GTK_STOCK_NO, "Deactivate");
	_contextMenu.duplicate = gtkutil::StockIconMenuItem(GTK_STOCK_COPY, "Duplicate");
	
	//gtk_menu_shell_append(GTK_MENU_SHELL(_contextMenu.menu), _contextMenu.add);
	gtk_menu_shell_append(GTK_MENU_SHELL(_contextMenu.menu), _contextMenu.enable);
	gtk_menu_shell_append(GTK_MENU_SHELL(_contextMenu.menu), _contextMenu.disable);
	gtk_menu_shell_append(GTK_MENU_SHELL(_contextMenu.menu), _contextMenu.duplicate);
	gtk_menu_shell_append(GTK_MENU_SHELL(_contextMenu.menu), _contextMenu.remove);
	
	_effectWidgets.addMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_ADD,
															   "Add new Effect");
	_effectWidgets.editMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_EDIT,
															   "Edit");
	_effectWidgets.deleteMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_DELETE,
															   "Delete");
	_effectWidgets.upMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_GO_UP,
															   "Move Up");
	_effectWidgets.downMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_GO_DOWN,
															   "Move Down");
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectWidgets.contextMenu), 
						  _effectWidgets.addMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectWidgets.contextMenu), 
						  _effectWidgets.editMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectWidgets.contextMenu), 
						  _effectWidgets.upMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectWidgets.contextMenu), 
						  _effectWidgets.downMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectWidgets.contextMenu), 
						  _effectWidgets.deleteMenuItem);

	// Connect up the signals
	g_signal_connect(G_OBJECT(_contextMenu.remove), "activate",
					 G_CALLBACK(onContextMenuDelete), this);
	/*g_signal_connect(G_OBJECT(_contextMenu.add), "activate",
					 G_CALLBACK(onContextMenuAdd), this);*/
	g_signal_connect(G_OBJECT(_contextMenu.enable), "activate",
					 G_CALLBACK(onContextMenuEnable), this);
	g_signal_connect(G_OBJECT(_contextMenu.disable), "activate",
					 G_CALLBACK(onContextMenuDisable), this);
	g_signal_connect(G_OBJECT(_contextMenu.duplicate), "activate",
					 G_CALLBACK(onContextMenuDuplicate), this);
	
	g_signal_connect(G_OBJECT(_effectWidgets.deleteMenuItem), "activate",
					 G_CALLBACK(onContextMenuDelete), this);
	g_signal_connect(G_OBJECT(_effectWidgets.editMenuItem), "activate",
					 G_CALLBACK(onContextMenuEdit), this);			 
	g_signal_connect(G_OBJECT(_effectWidgets.addMenuItem), "activate",
					 G_CALLBACK(onContextMenuAdd), this);
	g_signal_connect(G_OBJECT(_effectWidgets.upMenuItem), "activate",
					 G_CALLBACK(onContextMenuEffectUp), this);
	g_signal_connect(G_OBJECT(_effectWidgets.downMenuItem), "activate",
					 G_CALLBACK(onContextMenuEffectDown), this);
					 
	// Show menus (not actually visible until popped up)
	gtk_widget_show_all(_contextMenu.menu);
	gtk_widget_show_all(_effectWidgets.contextMenu);
}

void ResponseEditor::selectEffectIndex(const unsigned int index) {
	// Setup the selectionfinder to search for the index string
	gtkutil::TreeModel::SelectionFinder finder(index, EFFECT_INDEX_COL);

	gtk_tree_model_foreach(
		gtk_tree_view_get_model(GTK_TREE_VIEW(_effectWidgets.view)),
		gtkutil::TreeModel::SelectionFinder::forEach,
		&finder
	);
	
	if (finder.getPath() != NULL) {
		GtkTreeIter iter = finder.getIter();
		// Set the active row of the list to the given effect
		gtk_tree_selection_select_iter(_effectWidgets.selection, &iter);
	}
}

int ResponseEditor::getEffectIdFromSelection() {
	GtkTreeIter iter;
	GtkTreeModel* model;
	bool anythingSelected = gtk_tree_selection_get_selected(
		_effectWidgets.selection, &model, &iter
	);
	
	if (anythingSelected && _entity != NULL) {
		return gtkutil::TreeModel::getInt(model, &iter, EFFECT_INDEX_COL);
	}
	else {
		return -1;
	}
}

void ResponseEditor::openContextMenu(GtkTreeView* view) {
	// Check the treeview this remove call is targeting
	if (view == GTK_TREE_VIEW(_list)) {
		gtk_menu_popup(GTK_MENU(_contextMenu.menu), NULL, NULL, 
					   NULL, NULL, 1, GDK_CURRENT_TIME);	
	}
	else if (view == GTK_TREE_VIEW(_effectWidgets.view)) {
		gtk_menu_popup(GTK_MENU(_effectWidgets.contextMenu), NULL, NULL, 
					   NULL, NULL, 1, GDK_CURRENT_TIME);
	}
}

void ResponseEditor::selectionChanged() {
	update();
}

void ResponseEditor::addSR() {
	if (_entity == NULL) return;

	// Create a new StimResponse object
	int id = _entity->add();
	
	// Get a reference to the newly allocated object
	StimResponse& sr = _entity->get(id);
	sr.set("class", "R");
	
	// Get the selected stim type name from the combo box
	std::string name = getStimTypeIdFromSelector(GTK_COMBO_BOX(_addType.list));
	sr.set("type", (!name.empty()) ? name : _stimTypes.getFirstName());

	sr.set("state", "1");
	
	// Update the list stores AFTER the type has been set
	_entity->updateListStores();
	
	// Select the newly created response
	selectId(id);
}

// Static GTK Callbacks
// Button click events on TreeViews
gboolean ResponseEditor::onTreeViewButtonPress(
	GtkTreeView* view, GdkEventButton* ev, ResponseEditor* self)
{
	if (ev->type == GDK_2BUTTON_PRESS) {
		// Call the effect editor upon double click
		self->editEffect();
	}
	
	return FALSE;
}

// Delete context menu items activated
void ResponseEditor::onContextMenuDelete(GtkWidget* w, ResponseEditor* self) {
	if (w == self->_contextMenu.remove) {
		// Delete the selected stim from the list
		self->removeSR(GTK_TREE_VIEW(self->_list));
	}
	else if (w == self->_effectWidgets.deleteMenuItem) {
		self->removeEffect();
	}
}

// Delete context menu items activated
void ResponseEditor::onContextMenuAdd(GtkWidget* w, ResponseEditor* self) {
	if (w == self->_contextMenu.add) {
		self->addSR();
	}
	else if (w == self->_effectWidgets.addMenuItem) {
		self->addEffect();
	}
}

// Callback for effects treeview selection changes
void ResponseEditor::onEffectSelectionChange(GtkTreeSelection* selection, 
										   ResponseEditor* self) 
{
	if (self->_updatesDisabled)	return; // Callback loop guard

	// Update the sensitivity
	self->updateEffectContextMenu();
}

void ResponseEditor::onContextMenuEffectUp(GtkWidget* widget, ResponseEditor* self) {
	self->moveEffect(-1);
}

void ResponseEditor::onContextMenuEffectDown(GtkWidget* widget, ResponseEditor* self) {
	self->moveEffect(+1);
}

void ResponseEditor::onContextMenuEdit(GtkWidget* widget, ResponseEditor* self) {
	self->editEffect();
}

} // namespace ui
