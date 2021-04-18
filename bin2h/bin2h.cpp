#include <direct.h>
#include <Windows.h>

#include "WindowsUnicodeToolShim.h"

#include <string>
#include <vector>

int toolMain(int argc, const char **argv)
{
	if (argc != 3)
	{
		fprintf(stderr, "Usage: bin2h <dir> <output>");
		return -1;
	}

	std::string dir = argv[1];
	std::string filter = dir + "\\*";

	WIN32_FIND_DATAA findData;
	HANDLE findDataH = FindFirstFileA(filter.c_str(), &findData);

	if (findDataH == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to open directory scan");
		return -1;
	}


	FILE *outF = fopen_utf8(argv[2], "wb");
	if (!outF)
	{
		fprintf(stderr, "Failed to open output file");
		return -1;
	}

	std::vector<std::string> fileNames;
	int numFiles = 0;

	for (;;)
	{
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			fileNames.push_back(std::string(findData.cFileName));

			std::string filePath = dir + "\\" + findData.cFileName;

			FILE *inF = fopen_utf8(filePath.c_str(), "rb");
			if (!inF)
			{
				fprintf(stderr, "Failed to open input file");
				return -1;
			}

			fprintf(outF, "const unsigned char g_fileContents%i[] = {\n", numFiles);

			fseek(inF, 0, SEEK_END);
			long fileSize = ftell(inF);
			fseek(inF, 0, SEEK_SET);

			for (long i = 0; i < fileSize; i++)
			{
				if (i % 20 == 0)
					fputs("\n\t\t", outF);

				int c = fgetc(inF);
				fprintf(outF, "% 4i,", (c & 0xff));
			}
			fprintf(outF, "\n};\n\n");

			numFiles++;

			fclose(inF);
		}

		if (!FindNextFileA(findDataH, &findData))
			break;
	}

	FindClose(findDataH);

	fprintf(outF, "FileCatalogEntry g_catalogEntries[] = {\n");
	for (int i = 0; i < numFiles; i++)
		fprintf(outF, "\t{ \"%s\", g_fileContents%i, sizeof(g_fileContents%i) },\n", fileNames[i].c_str(), i, i);
	fprintf(outF, "};\n");
	fprintf(outF, "FileCatalog g_catalog = { g_catalogEntries, sizeof(g_catalogEntries) / sizeof(g_catalogEntries[0]), sizeof(g_catalogEntries), sizeof(g_catalogEntries[0]) };\n");
	fprintf(outF, "const FileCatalog &GetCatalog() { return g_catalog; }\n");

	fclose(outF);

	return 0;
}
