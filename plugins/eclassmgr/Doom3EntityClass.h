#ifndef DOOM3ENTITYCLASS_H_
#define DOOM3ENTITYCLASS_H_

#include "ieclass.h"
#include "irender.h"

#include "math/Vector3.h"
#include "math/aabb.h"

#include "parser/DefTokeniser.h"

#include <vector>
#include <map>

/* FORWARD DECLS */

class Shader;

namespace eclass
{

/** 
 * Implementation of the IEntityClass interface. This represents a single
 * Doom 3 entity class, such as "light_moveable" or "monster_mancubus".
 */
class Doom3EntityClass
: public IEntityClass
{
	// The name of this entity class
	std::string _name;

    // Should this entity type be treated as a light?
    bool _isLight;
    
	// Colour of this entity and flag to indicate it has been specified
	Vector3	_colour;
	bool _colourSpecified;

	// Shader versions of the colour
	ShaderPtr _fillShader;
	ShaderPtr _wireShader;

	// Does this entity have a fixed size?
	bool _fixedSize;
	
	// Map of named EntityAttribute structures. EntityAttributes are picked
	// up from the DEF file during parsing.
	typedef std::map<std::string, EntityClassAttribute> EntityAttributeMap;
	EntityAttributeMap _attributes;
	
	// The model and skin for this entity class (if it has one)
	std::string _model;
	std::string _skin;
	
	// Flag to indicate inheritance resolved. An EntityClass resolves its
	// inheritance by copying all values from the parent onto the child,
	// after recursively instructing the parent to resolve its own inheritance.
	bool _inheritanceResolved;
	
	// Name of the mod owning this class
	std::string _modName;

	// The empty attribute
	EntityClassAttribute _emptyAttribute;

	// The list of strings containing the ancestors and this eclass itself.
	InheritanceChain _inheritanceChain;

private:

	// Capture the shaders corresponding to the current colour
	void captureColour();
	
	// Release the shaders associated with the current colour
	void releaseColour();

public:

	/** 
	 * Static function to create a default entity class.
	 * 
	 * @param name
	 * The name of the entity class to create.
	 * 
	 * @param brushes
	 * Whether the entity contains brushes or not.
	 */
	static IEntityClassPtr create(const std::string& name, bool brushes);
	
    /** 
     * Default constructor.
     * 
     * @param name
     * Entity class name.
     * 
     * @param colour
     * Display colour for this entity.
     */
    Doom3EntityClass(const std::string& name, 
				 	 const Vector3& colour = Vector3(-1, -1, -1),
					 bool fixedSize = false,
				 	 const Vector3& mins = Vector3(1, 1, 1),
				 	 const Vector3& maxs = Vector3(-1, -1, -1));
    				 
    /** Destructor.
     */
	~Doom3EntityClass() {
		// Release the shaders
		releaseColour();
	}		
    
    /** Return the name of this entity class.
     */
	const std::string& getName() const {
		return _name;
	}
	
	/** Query whether this entity has a fixed size.
	 */
	bool isFixedSize() const {
        if (_fixedSize) {
            return true;
        }
        else {
            // Check for the existence of editor_mins/maxs attributes, and that
            // they do not contain only a question mark
    		return (getAttribute("editor_mins").value.size() > 1
                    && getAttribute("editor_maxs").value.size() > 1);
        }
	}
    
	/* Return the bounding AABB.
	 */
	AABB getBounds() const {
        if (isFixedSize()) {
            return AABB::createFromMinMax(
            	getAttribute("editor_mins").value, 
            	getAttribute("editor_maxs").value
            );
        }
        else {
            return AABB(); // null AABB
        }
	}

    /** Get whether this entity type is a light entity
     * 
     * @returns
     * true if this is a light, false otherwise
     */
    bool isLight() const {
        return _isLight;
    }
    
    /** Set whether this entity type is a light entity
     * 
     * @param val
     * true to set this as a light entity, false to disable
     */
    void setIsLight(bool val) {
        _isLight = val;
        if (_isLight)
        	_fixedSize = true;
    }

	/** Set the display colour for this entity.
	 * 
	 * @param colour
	 * The new colour to use.
	 */
	void setColour(const Vector3& colour) {
		// Set the specified flag
		_colourSpecified = true;
		
		// Release the current shaders, then capture the new ones
		releaseColour();
		_colour = colour;
		captureColour();
	}
     
	/** Get this entity's colour.
	 * 
	 * @returns
	 * A Vector3 containing the current colour.
	 */
	const Vector3& getColour() const {
		return _colour;
	}

	/** Return this entity's wireframe shader.
	 */
	ShaderPtr getWireShader() const {
		return _wireShader;
	}

	/** Return this entity's fill shader.
	 */
	ShaderPtr getFillShader() const {
		return _fillShader;
	}
	
	/* ATTRIBUTES */
	
	/** 
	 * Insert an EntityClassAttribute, without overwriting previous values.
	 */
	void addAttribute(const EntityClassAttribute& attribute) {
		_attributes.insert(
			EntityAttributeMap::value_type(attribute.name, attribute)
		);
	}
	
	/*
	 * Find a single attribute.
	 */
	EntityClassAttribute& getAttribute(const std::string& name);
	const EntityClassAttribute& getAttribute(const std::string& name) const;
	
	/*
	 * Return a list of all attributes matching the given name prefix.
	 */
	EntityClassAttributeList getAttributeList(const std::string& name) const;

	/** Enumerate the EntityClassAttributes.
	 */
	void forEachClassAttribute(EntityClassAttributeVisitor& visitor,
							   bool editorKeys) const;
	
	/** Set a model on this entity class.
	 * 
	 * @param
	 * The VFS model path.
	 */
	void setModelPath(const std::string& path) {
		_fixedSize = true;
		_model = path;
	}
	
	/** Return the model path
	 */
	const std::string& getModelPath() const {
		return _model;
	}
	
	/** Set the skin.
	 */
	void setSkin(const std::string& skin) {
		_skin = skin;
	}
	
	/** Get the skin.
	 */
	const std::string& getSkin() const {
		return _skin;
	}

	/**
	 * Returns the inheritance chain (including this eclass).
	 */
	virtual const InheritanceChain& getInheritanceChain();
	
	/**
	 * Resolve inheritance for this class.
	 * 
	 * @param classmap
	 * A reference to the global map of entity classes, which should be searched
	 * for the parent entity.
	 */
	typedef std::map<std::string, IEntityClassPtr> EntityClasses;
	void resolveInheritance(EntityClasses& classmap);
	
	/**
	 * Return the mod name.
	 */
	std::string getModName() const {
	    return _modName;   
	}
	
	/**
	 * Set the mod name.
	 */
	void setModName(const std::string& mn) {
	    _modName = mn;
	}
	
	// Initialises this class from the given tokens
	void parseFromTokens(parser::DefTokeniser& tokeniser);

private:
	// Rebuilds the inheritance chain (called after inheritance is resolved)
	void buildInheritanceChain();
};

/**
 * Pointer typedef.
 */
typedef boost::shared_ptr<Doom3EntityClass> Doom3EntityClassPtr;

}

#endif /*DOOM3ENTITYCLASS_H_*/
