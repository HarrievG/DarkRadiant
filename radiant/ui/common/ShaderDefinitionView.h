#ifndef SHADER_DEFINITION_VIEW_H_
#define SHADER_DEFINITION_VIEW_H_

#include <string>
#include "gtkutil/SourceView.h"
#include <gtkmm/box.h>

namespace Gtk { class Label; }

namespace ui
{

class ShaderDefinitionView :
	public Gtk::VBox
{
private:
	// The shader which should be previewed
	std::string _shader;

	Gtk::Label* _materialName;
	Gtk::Label* _filename;

	// The actual code view
	gtkutil::SourceView* _view;

public:
	ShaderDefinitionView();

	void setShader(const std::string& shader);

	void update();
};

} // namespace ui

#endif /* SHADER_DEFINITION_VIEW_H_ */
