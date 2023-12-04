/*�t�@�C������E�����񑀍�E�R���\�[���o��*/

#include <Windows.h>
#include <urlmon.h>
#include <atlbase.h>

#include <string>

#include "file_utility.h"

#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "shlwapi.lib")


struct ComInit
{
	HRESULT m_hrComInit;
	ComInit() : m_hrComInit(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)) {}
	~ComInit() { if (SUCCEEDED(m_hrComInit)) ::CoUninitialize(); }
};

/*-----------------------  �����֐�  -----------------------*/

void WriteMessage(const char* msg);

bool WriteStringToFile(std::string data, const char* file_path);

bool GetInternetResourceSize(const char* url, ULARGE_INTEGER& size);
std::string GetFileNameFromUrl(std::string url);
std::string GetDirectoryFromUrl(std::string url);


/*�`���o��*/
void WriteMessage(const char* msg)
{
	char stamp[16];
	SYSTEMTIME tm;
	::GetLocalTime(&tm);
	sprintf_s(stamp, "%02d:%02d:%02d:%03d ", tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds);
	std::string str = stamp + std::string(msg) + "\r\n";
	printf(str.c_str());
}

/*���s�v���Z�X�̊K�w�擾*/
std::string GetFolderBasePath()
{
	char application_path[MAX_PATH]{};
	::GetModuleFileNameA(nullptr, application_path, MAX_PATH);
	std::string::size_type pos = std::string(application_path).find_last_of("\\/");
	return std::string(application_path).substr(0, pos) + "\\/";
}
/*��ƃt�H���_�쐬*/
std::string CreateWorkFolder(const char* folder_name)
{
	std::string strFolder = folder_name != nullptr ? GetFolderBasePath() + folder_name + "\\/" : GetFolderBasePath();
	::CreateDirectoryA(strFolder.c_str(), nullptr);
	return strFolder;
}
/*����q��ƃt�H���_�쐬*/
std::string CreateNestedWorkFolder(std::string strRelativePath)
{
	if (!strRelativePath.empty())
	{
		std::string strFolder = GetFolderBasePath();
		size_t nRead = 0;
		for (;;)
		{
			size_t nPos = strRelativePath.substr(nRead).find_first_of("\\/");
			if (nPos == std::string::npos)
			{
				strFolder += strRelativePath.substr(nRead) +"\\/";
				::CreateDirectoryA(strFolder.c_str(), nullptr);
				break;
			}

			strFolder += strRelativePath.substr(nRead, nPos) + "\\/";
			::CreateDirectoryA(strFolder.c_str(), nullptr);
			nRead += nPos + 3;
		}

		return strFolder;
	}

	return std::string();
}
/*����URL�����Ƀt�H���_�쐬*/
std::string CreateFolderBasedOnRelativeUrl(std::string strUrl, std::string strBaseFolder, int iDepth, bool bBeFilePath)
{
	if (!strUrl.empty())
	{
		std::string strFolder = strBaseFolder;
		size_t nRead = 0;
		int iOccurrence = 0;
		for (;;)
		{
			size_t nPos = strUrl.substr(nRead).find_first_of("/");
			if (nPos == std::string::npos)
			{
				if (bBeFilePath)
				{
					strFolder += strUrl.substr(nRead);
				}
				break;
			}
			++iOccurrence;
			if (iOccurrence > iDepth && nPos != 0)
			{
				strFolder += strUrl.substr(nRead, nPos) + "\\/";
				::CreateDirectoryA(strFolder.c_str(), nullptr);
			}
			nRead += nPos + 1;
		}

		return strFolder;
	}

	return std::string();
}

/*�t�@�C���̃������W�J*/
char* LoadExistingFile(const char* file_path)
{
	HANDLE hFile = ::CreateFileA(file_path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwSize = ::GetFileSize(hFile, nullptr);
		if (dwSize != INVALID_FILE_SIZE)
		{
			char* buffer = static_cast<char*>(malloc(static_cast<size_t>(dwSize + 1ULL)));
			if (buffer != nullptr)
			{
				DWORD dwRead = 0;
				BOOL iRet = ::ReadFile(hFile, buffer, dwSize, &dwRead, nullptr);
				if (iRet)
				{
					::CloseHandle(hFile);
					*(buffer + dwRead) = '\0';
					return buffer;
				}
				else
				{
					free(buffer);
				}
			}
		}
		::CloseHandle(hFile);
	}

	return nullptr;
}
/*NULL�I�[������̃t�@�C���o��*/
bool SaveNullTerminatedStringToFile(char* data, const char* file_path)
{
	BOOL iRet = 0;

	if (file_path != nullptr)
	{
		if (!::PathFileExistsA(file_path))
		{
			HANDLE hFile = ::CreateFileA(file_path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				::SetFilePointer(hFile, NULL, nullptr, FILE_END);

				DWORD bytesWrite = 0;
				iRet = ::WriteFile(hFile, data, static_cast<DWORD>(strlen(data)), &bytesWrite, nullptr);
				if (iRet)
				{
					WriteMessage(std::string(file_path).append(" success").c_str());
					return true;
				}
				else
				{
					WriteMessage(std::string(file_path).append(" failed").c_str());
				}
				::CloseHandle(hFile);
			}
		}
		else
		{
			WriteMessage(std::string(file_path).append(" already exists.").c_str());
		}

	}
	return iRet > 0;
}
/*�������̃t�@�C���o��*/
bool WriteStringToFile(std::string data, const char* file_path)
{
	BOOL iRet = 0;

	if (file_path != nullptr)
	{
		HANDLE hFile = ::CreateFileA(file_path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			::SetFilePointer(hFile, NULL, nullptr, FILE_END);

			DWORD bytesWrite = 0;
			iRet = ::WriteFile(hFile, data.data(), static_cast<DWORD>(data.size()), &bytesWrite, nullptr);
			::CloseHandle(hFile);
		}
	}
	return iRet > 0;
}
/*�ۑ��������b�Z�[�W�o��*/
void SaveStringToFile(std::string data, const char* file_path)
{
	std::string strFileName = GetFileNameFromUrl(file_path);
	if (!strFileName.empty())
	{
		bool bRet = WriteStringToFile(data, file_path);
		if (bRet)
		{
			WriteMessage(std::string(strFileName).append(" success.").c_str());
		}
		else
		{
			WriteMessage(std::string(strFileName).append(" failed.").c_str());
		}
	}
	else
	{
		WriteMessage(std::string("File path invalid; argument: ").append(file_path).c_str());
	}
}
/*�t�@�C���L���m�F*/
bool DoesFilePathExist(const char* file_path)
{
	BOOL iRet = ::PathFileExistsA(file_path);
	if (iRet)
	{
		std::string strFileName = GetFileNameFromUrl(file_path);
		WriteMessage(std::string(strFileName).append(" already exists.").c_str());
	}

	return iRet == TRUE;
}

/*�d�q�Ԏ����̑傫���擾*/
bool GetInternetResourceSize(const char* url, ULARGE_INTEGER& size)
{
	ComInit init;
	CComPtr<IStream> pStream;
	HRESULT hr = ::URLOpenBlockingStreamA(nullptr, url, &pStream, 0, nullptr);

	if (hr == S_OK)
	{
		STATSTG stat;
		hr = pStream->Stat(&stat, STATFLAG_DEFAULT);
		if (hr == S_OK)
		{
			size = stat.cbSize;
			return true;
		}
	}
	return false;
}
/*URL����t�@�C�����擾*/
std::string GetFileNameFromUrl(std::string url)
{
	size_t pos = url.find_last_of("/");
	if (pos != std::string::npos)
	{
		return url.substr(pos + 1);
	}

	return std::string();
}
/*URL����K�w�擾*/
std::string GetDirectoryFromUrl(std::string url)
{
	size_t pos = url.find_last_of("/");
	if (pos != std::string::npos)
	{
		return url.substr(0, pos + 1);
	}

	return std::string();
}
/*�d�q�Ԏ����̕ۑ�*/
bool SaveInternetResourceToFile(const char* url, const char* folder, const char* file_name, unsigned long nMinFileSize, bool bFolderCreation)
{
	std::string strFileName = file_name == nullptr ? GetFileNameFromUrl(url) : strchr(file_name, '/') == nullptr ? file_name : GetFileNameFromUrl(file_name);
	if (!strFileName.empty())
	{
		std::string strFilePath = folder + strFileName;

		if (!::PathFileExistsA(strFilePath.c_str()))
		{
			ULARGE_INTEGER size{};
			bool bRet = GetInternetResourceSize(url, size);
			if (bRet)
			{
				if (size.LowPart > nMinFileSize)
				{
					if (bFolderCreation)
					{
						::CreateDirectoryA(folder, nullptr);
					}

					HRESULT hr = ::URLDownloadToFileA(nullptr, url, strFilePath.c_str(), 0, nullptr);
					if (hr == S_OK)
					{
						WriteMessage(std::string(url).append(" success").c_str());
						return true;
					}
					else
					{
						WriteMessage(std::string(url).append(" failed").c_str());
					}
				}

			}
		}
		else
		{
			WriteMessage(std::string(strFileName).append(" already exists.").c_str());
			return true;
		}
	}
	else
	{
		WriteMessage("File path invalid.");
	}

	return false;
}
/*�d�q�Ԏ����̃������W�J*/
bool LoadInternetResourceToBuffer(const char* url, char** dst)
{
	if (url == nullptr || *dst != nullptr)return false;

	ComInit init;
	CComPtr<IStream> pStream;
	HRESULT hr = ::URLOpenBlockingStreamA(nullptr, url, &pStream, 0, nullptr);

	if (hr == S_OK)
	{
		STATSTG stat;
		hr = pStream->Stat(&stat, STATFLAG_DEFAULT);
		if (hr == S_OK)
		{
			char* buffer = static_cast<char*>(malloc(stat.cbSize.LowPart + 1LL));
			if (buffer != nullptr)
			{
				DWORD dwReadBytes = 0;
				DWORD dwSize = 0;
				for (;;)
				{
					hr = pStream->Read(buffer + dwSize, stat.cbSize.LowPart - dwSize, &dwReadBytes);
					if (FAILED(hr))break;
					dwSize += dwReadBytes;
					if (dwSize >= stat.cbSize.LowPart)break;
				}
				*(buffer + dwSize) = '\0';

				*dst = buffer;

				return true;
			}
		}
	}

	return false;
}

/*JSON�����l�̒��o*/
bool ExtractJsonObject(char* src, const char* name, char** dst)
{
	char* p = nullptr;
	char* pp = src;
	char* q = nullptr;
	char* qq = nullptr;
	size_t len = 0;
	int iCount = 0;

	p = strstr(pp, name);
	if (p == nullptr)return false;

	pp = strchr(p, ':');
	if (pp == nullptr)false;
	++pp;

	for (;;)
	{
		q = strchr(pp, '}');
		if (q == nullptr)return false;

		qq = strchr(pp, '{');
		if (qq == nullptr)break;

		if (q < qq)
		{
			--iCount;
			pp = q + 1;
		}
		else
		{
			++iCount;
			pp = qq + 1;
		}

		if (iCount == 0)break;
	}

	len = q - p + 1;
	char* buffer = static_cast<char*>(malloc(len + 1));
	if (buffer == nullptr)return false;
	memcpy(buffer, p, len);
	*(buffer + len) = '\0';
	*dst = buffer;

	return true;

}
/*JSON�z��̒��o*/
bool ExtractJsonArray(char* src, const char* name, char** dst)
{
	char* p = nullptr;
	char* pp = src;
	char* q = nullptr;
	char* qq = nullptr;
	size_t len = 0;
	int iCount = 0;

	p = strstr(pp, name);
	if (p == nullptr)return false;

	pp = strchr(p, ':');
	if (pp == nullptr)false;
	++pp;

	for (;;)
	{
		q = strchr(pp, ']');
		if (q == nullptr)return false;

		qq = strchr(pp, '[');
		if (qq == nullptr)break;

		if (q < qq)
		{
			--iCount;
			pp = q + 1;
		}
		else
		{
			++iCount;
			pp = qq + 1;
		}

		if (iCount == 0)break;
	}

	len = q - p + 1;
	char* buffer = static_cast<char*>(malloc(len + 1));
	if (buffer == nullptr)return false;
	memcpy(buffer, p, len);
	*(buffer + len) = '\0';
	*dst = buffer;

	return true;
}
/*JSON��؂�ʒu�T��*/
char* FindJsonDataEnd(char* src)
{
	const char ref[] = " ,]}\"";

	for (char* pp = src; pp != nullptr; ++pp)
	{
		for (size_t i = 0; i < sizeof(ref); ++i)
		{
			if (*pp == ref[i])
			{
				return pp;
			}
		}
	}

	return nullptr;
}
/*JSON�v�f�̒l���擾*/
bool GetJsonElementValue(char* src, const char* name, char* dst, size_t dst_size)
{
	char* p = nullptr;
	char* pp = src;
	size_t len = 0;

	p = strstr(pp, name);
	if (p == nullptr)return false;
	p += strlen(name);

	pp = strchr(p, ':');
	if (pp == nullptr)return false;
	++pp;

	p = FindJsonDataEnd(pp);
	if (p == nullptr)return false;
	if (*p == '"')
	{
		pp = p + 1;
		p = strchr(pp, '"');
		if (p == nullptr)return false;
	}

	len = p - pp;
	if (len > dst_size)return false;
	memcpy(dst, pp, len);
	*(dst + len) = '\0';

	return true;
}

std::wstring WidenUtf8(std::string str)
{
	if (!str.empty())
	{
		int len = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
		if (len > 0)
		{
			std::wstring wstr(len, 0);
			::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), &wstr[0], len);
			return wstr;
		}
	}

	return std::wstring();
}

std::string NarrowUtf8(std::wstring wstr)
{
	if (!wstr.empty())
	{
		int len = ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), nullptr, 0, nullptr, nullptr);
		if (len > 0)
		{
			std::string str(len, 0);
			::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), &str[0], len, nullptr, nullptr);
			return str;
		}
	}
	return std::string();
}

/*�����u��*/
char* ReplaceString(char* src, const char* pzOld, const char* pzNew)
{
	if (src == nullptr)return nullptr;
	size_t nSrcLen = strlen(src);

	size_t nOldLen = strlen(pzOld);
	if (nOldLen == 0)return nullptr;

	size_t nNewLen = strlen(pzNew);

	char* p = nullptr;
	char* pp = src;
	int iCount = 0;

	for (;;)
	{
		p = strstr(pp, pzOld);
		if (p == nullptr)break;

		pp = p + nOldLen;
		++iCount;
	}

	size_t nSize = nSrcLen + iCount * (nNewLen - nOldLen) + 1;
	char* pResult = static_cast<char*>(malloc(nSize));
	if (pResult == nullptr)return nullptr;

	size_t pos = 0;
	size_t len = 0;
	pp = src;
	for (;;)
	{
		p = strstr(pp, pzOld);
		if (p == nullptr)
		{
			len = nSrcLen - (pp - src);
			memcpy(pResult + pos, pp, len);
			pos += len;
			break;
		}

		len = p - pp;
		memcpy(pResult + pos, pp, len);
		pos += len;
		memcpy(pResult + pos, pzNew, nNewLen);
		pos += nNewLen;
		pp = p + nOldLen;
	}

	*(pResult + pos) = '\0';
	return pResult;
}

/*�o�H����t�@�C�����擾*/
std::string GetFileNameFromFilePath(const char* path)
{
	if (path != nullptr)
	{
		std::string strPath = path;
		size_t pos = strPath.find_last_of("\\/");
		if (pos != std::string::npos)
		{
			return strPath.substr(pos + 1);
		}
	}
	return std::string();
}
/*�t�@�C��������g���q�擾*/
std::string GetExtensionFromFileName(const char* file_name)
{
	if (file_name != nullptr)
	{
		std::string strFile = file_name;
		size_t pos = strFile.find_last_of(".");
		if (pos != std::string::npos)
		{
			return strFile.substr(pos);
		}
	}
	return std::string();
}
/*�t�@�C�������������̍폜*/
std::string TruncateFileName(const char* file_name)
{
	if (file_name != nullptr)
	{
		std::string str = GetFileNameFromUrl(std::string(file_name));
		return str.empty() ? std::string(file_name) : str;
	}
	return std::string();
}
/*�g���q���O*/
std::string RemoveFileExtension(const char* file_name)
{
	if (file_name != nullptr)
	{
		std::string strFile = file_name;
		size_t pos = strFile.find_last_of(".");
		if (pos != std::string::npos)
		{
			return strFile.substr(0, pos);
		}
	}
	return std::string();
}
