#pragma once

namespace GpFileSystem_Web_Resources
{
	struct FileCatalogEntry
	{
		const char *m_fileName;
		const unsigned char *m_data;
		const unsigned int m_size;
	};
	
	struct FileCatalog
	{
		const FileCatalogEntry *m_entries;
		const unsigned int m_numEntries;
		const unsigned int m_size;
		const unsigned int m_size0;
	};
	
	namespace ApplicationData
	{
		const FileCatalog &GetCatalog();
	}
	namespace GameData
	{
		const FileCatalog &GetCatalog();
	}
}
