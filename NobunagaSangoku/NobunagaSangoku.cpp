// NobunagaSangoku.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <string>
#include <vector>
#include <algorithm>

#include "file_utility.h"
#include "win_http_session.h"

/*HTTPヘッダ生成*/
std::wstring CreateRequestHeader()
{
	std::wstring header;

	header += L"Referer: https://web-r-production.nobunagasangoku.com/\r\n";
	header += L"Sec-Ch-Ua: \"Not.A/Brand\";v=\"8\", \"Chromium\";v=\"117\", \"Microsoft Edge\";v=\"117\"\r\n";
	header += L"Sec-Ch-Ua-Mobile: ?0\r\n";
	header += L"Sec-Ch-Ua-Platform: \"Windows\"\r\n";
	header += L"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36 Edg/117.0.2045.41\r\n";

	return header;
}
/*HTTP GET要求*/
bool RequestHttpGet(std::wstring wstrUrl, std::wstring wstrHeader, std::string strFilePath)
{
	bool bRet = false;

	CWinHttpSession* pSession = new CWinHttpSession();
	if (pSession != nullptr)
	{
		bRet = pSession->Open();
		if (bRet)
		{
			ResponseData r;

			bRet = pSession->RequestGet(wstrUrl.c_str(), wstrHeader.c_str(), r);
			if (bRet)
			{
				if (wcsstr(r.header.c_str(), L"HTTP/1.1 200 OK"))
				{
					SaveStringToFile(r.content, strFilePath.c_str());
				}
				else
				{
					printf("Header content:\r\n%S", r.header.c_str());
					printf("Response data:\r\n%s", r.content.c_str());
					bRet = false;
				}
			}
			else
			{
				printf("WinHttp Failed; url: %S, function: %s, code: %ld\r\n", wstrUrl.c_str(), r.error.c_str(), r.ulErrorCode);
			}

		}
		delete pSession;
	}

	return bRet;
}

/*電子網資源要求*/
bool RequestResource(const wchar_t* pwzUrl, const char* pzFolder, const char* pzFileName)
{
	if (pwzUrl == nullptr || pzFolder == nullptr)return false;

	std::wstring wstrUrl = pwzUrl;
	std::wstring wstrHeader = CreateRequestHeader();
	std::string strFile = pzFolder;
	pzFileName == nullptr ? strFile.append(TruncateFileName(NarrowUtf8(wstrUrl).c_str())) : strFile.append(pzFileName);

	return DoesFilePathExist(strFile.c_str()) ? true : RequestHttpGet(wstrUrl, wstrHeader, strFile);
}
/*相対経路を基に要求*/
bool RequestThroughRelativePath(const wchar_t* pwzRelativePath, const char* pzFolder, const char* pzFileName)
{
	if (pwzRelativePath == nullptr)return false;

	std::wstring wstrUrl = L"https://cdn-production.nobunagasangoku.com";
	wstrUrl.append(pwzRelativePath);

	std::string strFolder = pzFolder == nullptr ? CreateFolderBasedOnRelativeUrl(NarrowUtf8(pwzRelativePath), GetFolderBasePath(), 0, false) : pzFolder;

	return RequestResource(wstrUrl.c_str(), strFolder.c_str(), pzFileName);
}

/*資源経路探索*/
void SearchResourceFilePath(char* src, const char* name, std::vector<std::string>& paths)
{
	char* p = nullptr;
	char* pp = src;

	if (src == nullptr || name == nullptr)return;

	size_t nameLen = strlen(name);
	char* key = static_cast<char*>(malloc(nameLen + 2LL));
	if (key == nullptr)return;
	*key = '"';
	memcpy(key + 1, name, nameLen);
	*(key + nameLen + 1) = '\0';

	for (;;)
	{
		p = strstr(pp, key);
		if (p == nullptr)break;
		++p;

		pp = strchr(p, '"');
		if (pp == nullptr)break;

		size_t len = pp - p;
		char* buffer = static_cast<char*>(malloc(len + 1LL));
		if (buffer == nullptr)break;

		memcpy(buffer, p, len);
		*(buffer + len) = '\0';
		paths.push_back(buffer);
		free(buffer);
	}

	free(key);

}
/*寝室台本ファイル名拾い上げ*/
void PickupScenarioFileNames(std::vector<std::string> paths, std::vector<std::string>& scenario_names)
{
	for (std::string path : paths)
	{
		size_t nPos = path.find("cg_");
		size_t nPos2 = path.rfind("_");
		if (nPos != std::string::npos && nPos2 != std::string::npos)
		{
			std::string strScenarioName = path.substr(nPos + 3, nPos2 - nPos - 3);
			std::vector<std::string>::iterator itr = std::find(scenario_names.begin(), scenario_names.end(), strScenarioName);
			if (itr == scenario_names.cend())scenario_names.push_back(strScenarioName);
		}
	}
}
/*資源一覧確認*/
void CheckResourceFile()
{
	std::string strFile = GetFolderBasePath() + "\\/client\\/res\\/resource.json";

	char* buffer = LoadExistingFile(strFile.c_str());
	if (buffer != nullptr)
	{
		std::vector<std::string> paths;
		SearchResourceFilePath(buffer, "res/", paths);
		for (std::string path : paths)
		{
			std::wstring wstrPath = L"/client/" + WidenUtf8(path);
			bool bRet = RequestThroughRelativePath(wstrPath.c_str(), nullptr, nullptr);
			if (!bRet)break;
		}

		std::vector<std::string> scenario_names;
		PickupScenarioFileNames(paths, scenario_names);
		for (std::string strScenarioName : scenario_names)
		{
			std::wstring wstrPath = L"/scenario/h/" + WidenUtf8(strScenarioName);
			wstrPath.append(L".scr");
			std::string strFileName = strScenarioName + ".txt";
			bool bRet = RequestThroughRelativePath(wstrPath.c_str(), nullptr, strFileName.c_str());
			if (!bRet)break;
		}

		free(buffer);
	}
}

int main()
{
	RequestThroughRelativePath(L"/client/res/resource.json", nullptr, nullptr);
	//RequestResource(L"https://web-r-production.nobunagasangoku.com/sangoku/client/game.min.js", CreateFolderBasedOnRelativeUrl("/sangoku/client/game.min.js", GetFolderBasePath(), 0, false).c_str(), nullptr);
	CheckResourceFile();
}
