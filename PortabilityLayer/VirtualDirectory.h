#pragma once

namespace PortabilityLayer
{
	namespace VirtualDirectories
	{
		enum VirtualDirectory
		{
			kUnspecified = 0,

			kApplicationData = 1,
			kGameData,
			kUserData,
			kUserSaves,
			kPrefs,
			kPreinstalledFonts,
			kFonts,
			kCursors,
			kHighScores,
			kLogs,

			kSourceExport,
		};
	}

	typedef VirtualDirectories::VirtualDirectory VirtualDirectory_t;
}
