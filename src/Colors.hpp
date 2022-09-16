#ifndef AVI_COLORS
#define AVI_COLORS

#include <glm/glm.hpp>

namespace AVis {
	typedef glm::vec4 Color;
	static Color fromHex(int hex) {
		return Color((hex >> 16) & 0xFF , (hex >> 8) & 0xFF, (hex) & 0xFF , 225) / 225.0f;
	}
namespace Colors {
	const Color White = { 1.0f, 1.0f, 1.0f, 1.0f };
	const Color Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	const Color Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	const Color Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	const Color Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	const Color Purple = { 0.5f, 0.0f, 0.5f, 1.0f };

	const Color Pink = { 1.0f, 0.3f, 0.7f, 1.0f };


	const Color Aqua = { 0.0f, 1.0f, 1.0f, 1.0f };
	const Color Teal = { 0.0f, 0.5f, 0.5f, 1.0f };

	const Color NeonIndigo = fromHex(0x440BD4);
	const Color NeonPink = fromHex(0xE92EFB);
	const Color NeonBrightRed = fromHex(0xFF2079);
	const Color NeonBlue = fromHex(0x04005E);


};
};
#endif // !AVI_COLORS
