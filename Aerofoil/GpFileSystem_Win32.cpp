#include "GpFileSystem_Win32.h"

#include "GpApplicationName.h"
#include "GpFileStream_Win32.h"
#include "GpWindows.h"
#include "IGpDirectoryCursor.h"

#include <string>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <commdlg.h>

#include <assert.h>

extern GpWindowsGlobals g_gpWindowsGlobals;

class GpDirectoryCursor_Win32 final : public IGpDirectoryCursor
{
public:
	static GpDirectoryCursor_Win32 *Create(const HANDLE &handle, const WIN32_FIND_DATAW &findData);

	bool GetNext(const char *&outFileName) override;
	void Destroy() override;

private:
	GpDirectoryCursor_Win32(const HANDLE &handle, const WIN32_FIND_DATAW &findData);
	~GpDirectoryCursor_Win32();

	HANDLE m_handle;
	WIN32_FIND_DATAW m_findData;
	char m_chars[MAX_PATH + 1];
	bool m_haveNext;
};

GpDirectoryCursor_Win32 *GpDirectoryCursor_Win32::Create(const HANDLE &handle, const WIN32_FIND_DATAW &findData)
{
	void *storage = malloc(sizeof(GpDirectoryCursor_Win32));
	if (!storage)
		return nullptr;

	return new (storage) GpDirectoryCursor_Win32(handle, findData);
}

bool GpDirectoryCursor_Win32::GetNext(const char *&outFileName)
{
	while (m_haveNext)
	{
		bool haveResult = false;

		bool hasInvalidChars = false;
		for (const wchar_t *fnameScan = m_findData.cFileName; *fnameScan; fnameScan++)
		{
			const int32_t asInt = static_cast<int32_t>(*fnameScan);
			if (asInt < 1 || asInt >= 128)
			{
				hasInvalidChars = true;
				break;
			}
		}

		if (!hasInvalidChars && wcscmp(m_findData.cFileName, L".") && wcscmp(m_findData.cFileName, L".."))
		{
			const size_t len = wcslen(m_findData.cFileName);

			haveResult = true;

			for (size_t i = 0; i <= len; i++)
				m_chars[i] = static_cast<char>(m_findData.cFileName[i]);
		}

		m_haveNext = (FindNextFileW(m_handle, &m_findData) != FALSE);

		if (haveResult)
		{
			outFileName = m_chars;
			return true;
		}
	}

	return false;
}

void GpDirectoryCursor_Win32::Destroy()
{
	this->~GpDirectoryCursor_Win32();
	free(this);
}

GpDirectoryCursor_Win32::GpDirectoryCursor_Win32(const HANDLE &handle, const WIN32_FIND_DATAW &findData)
	: m_handle(handle)
	, m_findData(findData)
	, m_haveNext(true)
{
}

GpDirectoryCursor_Win32::~GpDirectoryCursor_Win32()
{
	FindClose(m_handle);
}

GpFileSystem_Win32::GpFileSystem_Win32()
{
	// GP TODO: This shouldn't be static init since it allocates memory
	m_executablePath[0] = 0;

	PWSTR docsPath;
	if (!FAILED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, &docsPath)))
	{
		try
		{
			m_prefsDir = docsPath;
		}
		catch(...)
		{
			CoTaskMemFree(docsPath);
			throw;
		}

		m_prefsDir.append(L"\\" GP_APPLICATION_NAME_W);

		m_userHousesDir = m_prefsDir + L"\\Houses";
		m_userSavesDir = m_prefsDir + L"\\SavedGames";
		m_scoresDir = m_prefsDir + L"\\Scores";
		m_logsDir = m_prefsDir + L"\\Logs";
		m_fontCacheDir = m_prefsDir + L"\\FontCache";

		CreateDirectoryW(m_prefsDir.c_str(), nullptr);
		CreateDirectoryW(m_scoresDir.c_str(), nullptr);
		CreateDirectoryW(m_userHousesDir.c_str(), nullptr);
		CreateDirectoryW(m_userSavesDir.c_str(), nullptr);
		CreateDirectoryW(m_logsDir.c_str(), nullptr);
		CreateDirectoryW(m_fontCacheDir.c_str(), nullptr);

		m_prefsDir.append(L"\\");
		m_scoresDir.append(L"\\");
		m_userHousesDir.append(L"\\");
		m_userSavesDir.append(L"\\");
		m_logsDir.append(L"\\");
		m_fontCacheDir.append(L"\\");
		m_resourcesDir.append(L"\\");
	}

	DWORD modulePathSize = GetModuleFileNameW(nullptr, m_executablePath, MAX_PATH);
	if (modulePathSize == MAX_PATH || modulePathSize == 0)
		m_executablePath[0] = 0;

	size_t currentPathLength = wcslen(m_executablePath);

	for (;;)
	{
		while (currentPathLength > 0 && m_executablePath[currentPathLength - 1] != '\\')
			currentPathLength--;

		m_executablePath[currentPathLength] = 0;

		if (currentPathLength + 11 > MAX_PATH)
		{
			// "Resources" append is a longer path than the executable
			continue;
		}

		if (wcscat_s(m_executablePath, L"Packaged"))
		{
			currentPathLength = 0;
			break;
		}

		if (PathFileExistsW(m_executablePath) && PathIsDirectoryW(m_executablePath))
		{
			m_executablePath[currentPathLength] = 0;
			break;
		}
		else
			currentPathLength--;
	}

	if (currentPathLength > 0)
	{
		m_packagedDir = std::wstring(m_executablePath) + L"Packaged\\";
		m_housesDir = std::wstring(m_executablePath) + L"Packaged\\Houses\\";
		m_resourcesDir = std::wstring(m_executablePath) + L"Resources\\";
	}
}

bool GpFileSystem_Win32::FileExists(PortabilityLayer::VirtualDirectory_t virtualDirectory, const char *path)
{
	wchar_t winPath[MAX_PATH + 1];

	if (!ResolvePath(virtualDirectory, &path, 1, winPath))
		return false;

	return PathFileExistsW(winPath) != 0;
}

bool GpFileSystem_Win32::FileLocked(PortabilityLayer::VirtualDirectory_t virtualDirectory, const char *path, bool &exists)
{
	wchar_t winPath[MAX_PATH + 1];

	if (!ResolvePath(virtualDirectory, &path, 1, winPath))
	{
		exists = false;
		return false;
	}

	DWORD attribs = GetFileAttributesW(winPath);
	if (attribs == INVALID_FILE_ATTRIBUTES)
	{
		exists = false;
		return false;
	}

	exists = true;
	return (attribs & FILE_ATTRIBUTE_READONLY) != 0;
}

GpIOStream *GpFileSystem_Win32::OpenFileNested(PortabilityLayer::VirtualDirectory_t virtualDirectory, char const* const* paths, size_t numPaths, bool writeAccess, GpFileCreationDisposition_t createDisposition)
{
	wchar_t winPath[MAX_PATH + 1];

	if (!ResolvePath(virtualDirectory, paths, numPaths, winPath))
		return false;

	const DWORD desiredAccess = writeAccess ? (GENERIC_WRITE | GENERIC_READ) : GENERIC_READ;
	DWORD winCreationDisposition = 0;

	switch (createDisposition)
	{
	case GpFileCreationDispositions::kCreateOrOverwrite:
		winCreationDisposition = CREATE_ALWAYS;
		break;
	case GpFileCreationDispositions::kCreateNew:
		winCreationDisposition = CREATE_NEW;
		break;
	case GpFileCreationDispositions::kCreateOrOpen:
		winCreationDisposition = OPEN_ALWAYS;
		break;
	case GpFileCreationDispositions::kOpenExisting:
		winCreationDisposition = OPEN_EXISTING;
		break;
	case GpFileCreationDispositions::kOverwriteExisting:
		winCreationDisposition = TRUNCATE_EXISTING;
		break;
	default:
		return false;
	}

	HANDLE h = CreateFileW(winPath, desiredAccess, FILE_SHARE_READ, nullptr, winCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h == INVALID_HANDLE_VALUE)
		return false;

	return new GpFileStream_Win32(h, true, writeAccess, true);
}

bool GpFileSystem_Win32::DeleteFile(PortabilityLayer::VirtualDirectory_t virtualDirectory, const char *path, bool &existed)
{
	wchar_t winPath[MAX_PATH + 1];

	if (!ResolvePath(virtualDirectory, &path, 1, winPath))
		return false;

	if (DeleteFileW(winPath))
	{
		existed = true;
		return true;
	}

	DWORD err = GetLastError();
	if (err == ERROR_FILE_NOT_FOUND)
		existed = false;
	else
		existed = true;

	return false;
}

IGpDirectoryCursor *GpFileSystem_Win32::ScanDirectoryNested(PortabilityLayer::VirtualDirectory_t virtualDirectory, char const* const* paths, size_t numPaths)
{
	wchar_t winPath[MAX_PATH + 2];

	const char **expandedPaths = static_cast<const char**>(malloc(sizeof(const char*) * (numPaths + 1)));
	if (!expandedPaths)
		return nullptr;

	for (size_t i = 0; i < numPaths; i++)
		expandedPaths[i] = paths[i];
	expandedPaths[numPaths] = "*";

	const bool isPathResolved = ResolvePath(virtualDirectory, expandedPaths, numPaths + 1, winPath);
	free(expandedPaths);

	if (!isPathResolved)
		return nullptr;

	WIN32_FIND_DATAW findData;
	HANDLE ff = FindFirstFileW(winPath, &findData);

	if (ff == INVALID_HANDLE_VALUE)
		return nullptr;

	return GpDirectoryCursor_Win32::Create(ff, findData);
}

bool GpFileSystem_Win32::ValidateFilePathUnicodeChar(uint32_t c) const
{
	if (c >= '0' && c <= '9')
		return true;

	if (c == '_' || c == '\'')
		return true;

	if (c == ' ')
		return true;

	if (c >= 'a' && c <= 'z')
		return true;

	if (c >= 'A' && c <= 'Z')
		return true;

	return false;
}

void GpFileSystem_Win32::SetDelayCallback(GpFileSystem_Win32::DelayCallback_t delayCallback)
{
}

bool GpFileSystem_Win32::ValidateFilePath(const char *str, size_t length) const
{
	for (size_t i = 0; i < length; i++)
	{
		const char c = str[i];
		if (c >= '0' && c <= '9')
			continue;

		if (c == '_' || c == '.' || c == '\'' || c == '!')
			continue;

		if (c == ' ' && i != 0 && i != length - 1)
			continue;

		if (c >= 'a' && c <= 'z')
			continue;

		if (c >= 'A' && c <= 'Z')
			continue;

		return false;
	}

	const char *bannedNames[] =
	{
		"CON",
		"PRN",
		"AUX",
		"NUL",
		"COM1",
		"COM2",
		"COM3",
		"COM4",
		"COM5",
		"COM6",
		"COM7",
		"COM8",
		"COM9",
		"LPT1",
		"LPT2",
		"LPT3",
		"LPT4",
		"LPT5",
		"LPT6",
		"LPT7",
		"LPT8",
		"LPT9"
	};

	size_t nameLengthWithoutExt = length;
	for (size_t i = 0; i < length; i++)
	{
		if (str[i] == '.')
		{
			nameLengthWithoutExt = i;
			break;
		}
	}

	const size_t numBannedNames = sizeof(bannedNames) / sizeof(bannedNames[0]);

	for (size_t i = 0; i < numBannedNames; i++)
	{
		const char *bannedName = bannedNames[i];
		const size_t banLength = strlen(bannedName);

		if (banLength == nameLengthWithoutExt)
		{
			bool isBanned = true;

			for (size_t j = 0; j < banLength; j++)
			{
				char checkCH = str[j];
				if (checkCH >= 'a' && checkCH <= 'z')
					checkCH += ('A' - 'a');

				if (bannedName[j] != checkCH)
				{
					isBanned = false;
					break;
				}
			}

			if (isBanned)
				return false;
		}
	}

	return true;
}

const wchar_t *GpFileSystem_Win32::GetBasePath() const
{
	return m_executablePath;
}

GpFileSystem_Win32 *GpFileSystem_Win32::GetInstance()
{
	return &ms_instance;
}

bool GpFileSystem_Win32::ResolvePath(PortabilityLayer::VirtualDirectory_t virtualDirectory, char const* const* paths, size_t numPaths, wchar_t *outPath)
{
	const wchar_t *baseDir = nullptr;

	switch (virtualDirectory)
	{
	case PortabilityLayer::VirtualDirectories::kApplicationData:
		baseDir = m_packagedDir.c_str();
		break;
	case PortabilityLayer::VirtualDirectories::kGameData:
		baseDir = m_housesDir.c_str();
		break;
	case PortabilityLayer::VirtualDirectories::kUserData:
		baseDir = m_userHousesDir.c_str();
		break;
	case PortabilityLayer::VirtualDirectories::kUserSaves:
		baseDir = m_userSavesDir.c_str();
		break;
	case PortabilityLayer::VirtualDirectories::kPrefs:
		baseDir = m_prefsDir.c_str();
		break;
	case PortabilityLayer::VirtualDirectories::kFonts:
		baseDir = m_resourcesDir.c_str();
		break;
	case PortabilityLayer::VirtualDirectories::kHighScores:
		baseDir = m_scoresDir.c_str();
		break;
	case PortabilityLayer::VirtualDirectories::kLogs:
		baseDir = m_logsDir.c_str();
		break;
	default:
		return false;
	}

	if (baseDir == nullptr)
		return false;

	const size_t baseDirLen = wcslen(baseDir);
	memcpy(outPath, baseDir, sizeof(wchar_t) * baseDirLen);
	outPath[baseDirLen] = static_cast<wchar_t>(0);

	for (size_t i = 0; i < numPaths; i++)
	{
		size_t outDirLen = wcslen(outPath);

		if (i != 0)
		{
			if (baseDirLen >= MAX_PATH || MAX_PATH - baseDirLen < 1)
				return false;

			outPath[outDirLen++] = '\\';
		}

		const char *path = paths[i];
		const size_t pathLen = strlen(path);

		if (baseDirLen >= MAX_PATH || MAX_PATH - baseDirLen < pathLen)
			return false;

		for (size_t j = 0; j < pathLen; j++)
		{
			char c = path[j];
			if (c == '/')
				c = '\\';

			outPath[outDirLen + j] = static_cast<wchar_t>(c);
		}

		outPath[outDirLen + pathLen] = static_cast<wchar_t>(0);
	}

	return true;
}

GpFileSystem_Win32 GpFileSystem_Win32::ms_instance;
