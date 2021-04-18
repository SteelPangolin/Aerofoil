#pragma once

namespace PortabilityLayer
{
	namespace FontPresets
	{
		enum FontPreset
		{
			kSystem12,
			kSystem12Bold,

			kApplication8,
			kApplication9,
			kApplication9Bold,
			kApplication10Bold,
			kApplication12Bold,
			kApplication14,
			kApplication14Bold,
			kApplication18,
			kApplication40,

			kMono10,

			kHandwriting24,
			kHandwriting48,

			kCount,
		};
	}

	typedef FontPresets::FontPreset FontPreset_t;
}
