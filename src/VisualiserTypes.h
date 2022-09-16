#ifndef AVIS_VISUALISER_TYPES
#define AVIS_VISUALISER_TYPES
// Types representing all Objects in the visualiser that all parts of the application
// can understand and access

#include <glm/glm.hpp>
#include "Colors.hpp"


namespace AVis {

// point of visualiser that goes up and down in response to sound
struct Peak {
	static const uint32_t MaxNumPeaks = 32;
	static const float MaxHeight;
	static const float MinHeight;
	static const float MaxGridPos;
	static const float MinGridPos;

	// Formulas used too calculate peaks. Need to manually sync With shader
	enum Type { Quadratic, Sphere, NTypes };// , InvQuad, InvAbs };
	static const char* TypeNames[];
	// The epicenter of a rising point in the 2D plane
	//What formula should be used to calculate its curve
	Type type;
	// It's position on the plane
	glm::vec2 point = { 0.0f, 0.0f };

	// How pronounced it is
	float maxHeight = 1.0;
	// how much of its max hieght it fills [0,1]
	float energy = 0.0f;

	// The light waves in reflects 
	glm::vec4 color = Colors::Blue;

};



}


#endif // !AVIS_VISUALISER_TYPES

