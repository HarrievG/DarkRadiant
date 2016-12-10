#include "DragManipulator.h"

#include "../SelectionPool.h"
#include "../SelectionTest.h"
#include "../Planes.h"
#include "../SingleItemSelector.h"
#include "../BestSelector.h"

#include "registry/registry.h"

namespace selection
{

const std::string RKEY_TRANSIENT_COMPONENT_SELECTION = "user/ui/transientComponentSelection";

DragManipulator::Component* DragManipulator::getActiveComponent() 
{
    return _dragSelectable.isSelected() ? &_freeDrag : &_freeResize;
}

void DragManipulator::testSelect(const render::View& view, const Matrix4& pivot2world)
{
    SelectionPool selector;

    SelectionVolume test(view);

    if (GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive)
	{
		testSelectPrimitiveMode(view, test, selector);
	}
	else if (GlobalSelectionSystem().Mode() == SelectionSystem::eGroupPart)
    {
    	// Find all primitives that are selectable
		SingleItemSelector booleanSelector;

		GroupChildPrimitiveSelector childPrimitiveTester(booleanSelector, test);
		GlobalSceneGraph().foreachVisibleNodeInVolume(view, childPrimitiveTester);

		if (booleanSelector.hasValidSelectable())
		{
			// Found a selectable primitive
			selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
			_selected = false;
		}
		else
		{
			// Check for selectable faces
			_selected = Scene_forEachPlaneSelectable_selectPlanes(selector, test);
		}
    }
    // Check for entities that can be selected
    else if (GlobalSelectionSystem().Mode() == SelectionSystem::eEntity)
	{
    	// Create a boolean selection pool (can have exactly one selectable or none)
		SingleItemSelector booleanSelector;

		// Find the visible entities
		EntitySelector selectionTester(booleanSelector, test);
		GlobalSceneGraph().foreachVisibleNodeInVolume(view, selectionTester);

		// Check, if an entity could be found
      	if (booleanSelector.hasValidSelectable()) {
        	selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
        	_selected = false;
		}
    }
    else
    {
		BestSelector bestSelector;

		ComponentSelector selectionTester(bestSelector, test, GlobalSelectionSystem().ComponentMode());
		GlobalSelectionSystem().foreachSelected(selectionTester);

		for (ISelectable* selectable : bestSelector.getBestSelectables())
		{
			// greebo: Disabled this, it caused the currently selected patch vertices being deselected.
			if (registry::getValue<bool>(RKEY_TRANSIENT_COMPONENT_SELECTION))
			{
				if (!selectable->isSelected())
				{
					GlobalSelectionSystem().setSelectedAllComponents(false);
				}
			}

			_selected = false;
			selector.addSelectable(SelectionIntersection(0, 0), selectable);
			_dragSelectable.setSelected(true);
		}
	}

	for (SelectionPool::const_iterator i = selector.begin(); i != selector.end(); ++i)
	{
		i->second->setSelected(true);
	}
}

void DragManipulator::testSelectPrimitiveMode(const render::View& view, SelectionVolume& test, SelectionPool& selector)
{
	SingleItemSelector itemSelector;

	// First try to select entities (including func_* groups)
	EntitySelector selectionTester(itemSelector, test);
	GlobalSceneGraph().foreachVisibleNodeInVolume(view, selectionTester);

	_selected = false;

	if (itemSelector.hasValidSelectable())
	{
		// Found a selectable entity
		selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
		return;
	}
	
	// Find all worldspawn primitives
	PrimitiveSelector primitiveTester(itemSelector, test);
	GlobalSceneGraph().foreachVisibleNodeInVolume(view, primitiveTester);

	if (itemSelector.hasValidSelectable())
	{
		// Found a selectable primitive
		selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
		return;
	}

	// Entities and worldspawn primitives failed, so check for group children too
	// Find all group child primitives that are selectable
	GroupChildPrimitiveSelector childPrimitiveTester(itemSelector, test);
	GlobalSceneGraph().foreachVisibleNodeInVolume(view, childPrimitiveTester);

	if (itemSelector.hasValidSelectable())
	{
		// Found a selectable group child primitive
		selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
		return;
	}

	// all direct hits failed, check for drag-selectable faces
	_selected = Scene_forEachPlaneSelectable_selectPlanes(selector, test);
}

void DragManipulator::setSelected(bool select) 
{
    _selected = select;
    _dragSelectable.setSelected(select);
}

bool DragManipulator::isSelected() const
{
	return _selected || _dragSelectable.isSelected();
}

}