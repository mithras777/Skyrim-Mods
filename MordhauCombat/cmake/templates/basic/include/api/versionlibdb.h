#pragma once

#include <map>
#include <fstream>
#include <cstdint>
#include <stdio.h>
#include <Windows.h>

#pragma comment(lib, "version.lib")

class VersionDb
{
public:
	VersionDb() { Clear(); }
	~VersionDb() { }

private:
	std::map<unsigned long long, unsigned long long> _data;
	std::map<unsigned long long, unsigned long long> _rdata;
	int _ver[4];
	std::string _verStr;
	std::string _moduleName;
	unsigned long long _base;

	template <typename T>
	static T read(std::ifstream& file)
	{
		T v{};
		file.read((char*)&v, sizeof(T));
		return v;
	}

	static void* ToPointer(unsigned long long v)
	{
		return (void*)v;
	}

	static unsigned long long FromPointer(void* ptr)
	{
		return (unsigned long long)ptr;
	}
	
	static bool ParseVersionFromString(const char* ptr, int& major, int& minor, int& revision, int& build)
	{
		return sscanf_s(ptr, "%d.%d.%d.%d", &major, &minor, &revision, &build) == 4 && ((major != 1 && major != 0) || minor != 0 || revision != 0 || build != 0);
	}

public:

	const std::string& GetModuleName() const { return _moduleName; }
	const std::string& GetLoadedVersionString() const { return _verStr; }

	const std::map<unsigned long long, unsigned long long>& GetOffsetMap() const
	{
		return _data;
	}

	void* FindAddressById(unsigned long long id) const
	{
		unsigned long long b = _base;
		if (b == 0)
			return NULL;

		unsigned long long offset = 0;
		if (!FindOffsetById(id, offset))
			return NULL;

		return ToPointer(b + offset);
	}

	bool FindOffsetById(unsigned long long id, unsigned long long& result) const
	{
		auto itr = _data.find(id);
		if (itr != _data.end())
		{
			result = itr->second;
			return true;
		}
		return false;
	}

	bool FindIdByAddress(void* ptr, unsigned long long& result) const
	{
		unsigned long long b = _base;
		if (b == 0)
			return false;

		unsigned long long addr = FromPointer(ptr);
		return FindIdByOffset(addr - b, result);
	}

	bool FindIdByOffset(unsigned long long offset, unsigned long long& result) const
	{
		auto itr = _rdata.find(offset);
		if (itr == _rdata.end())
			return false;

		result = itr->second;
		return true;
	}

	bool GetExecutableVersion(int& major, int& minor, int& revision, int& build) const
	{
		TCHAR szVersionFile[MAX_PATH];
		std::uint32_t module = GetModuleFileName(NULL, szVersionFile, MAX_PATH);
		if (!module)
			return false;

		DWORD verHandle = 0;
		DWORD verSize = GetFileVersionInfoSize(szVersionFile, nullptr);

		if (verSize != NULL)
		{
			LPSTR verData = new char[verSize];

			if (GetFileVersionInfo(szVersionFile, verHandle, verSize, verData))
			{
				{
					char * vstr = NULL;
					UINT vlen = 0;
					if (VerQueryValueA(verData, "\\StringFileInfo\\040904B0\\ProductVersion", (LPVOID*)&vstr, &vlen) && vlen && vstr && *vstr)
					{
						if (ParseVersionFromString(vstr, major, minor, revision, build))
						{
							delete[] verData;
							return true;
						}
					}
				}

				{
					char * vstr = NULL;
					UINT vlen = 0;
					if (VerQueryValueA(verData, "\\StringFileInfo\\040904B0\\FileVersion", (LPVOID*)&vstr, &vlen) && vlen && vstr && *vstr)
					{
						if (ParseVersionFromString(vstr, major, minor, revision, build))
						{
							delete[] verData;
							return true;
						}
					}
				}
			}

			delete[] verData;
		}

		return false;
	}

	void GetLoadedVersion(int& major, int& minor, int& revision, int& build) const
	{
		major = _ver[0];
		minor = _ver[1];
		revision = _ver[2];
		build = _ver[3];
	}

	void Clear()
	{
		_data.clear();
		_rdata.clear();
		for (int i = 0; i < 4; i++) _ver[i] = 0;
		_moduleName = std::string();
		_base = 0;
	}

	bool Load()
	{
		int major, minor, revision, build;
		
		if (!GetExecutableVersion(major, minor, revision, build))
			return false;

		return Load(major, minor, revision, build);
	}

	bool Load(int major, int minor, int revision, int build)
	{
		Clear();

		char fileName[256];
		_snprintf_s(fileName, 256, "Data\\SKSE\\Plugins\\versionlib-%d-%d-%d-%d.bin", major, minor, revision, build);

		std::ifstream file(fileName, std::ios::binary);
		if (!file.good())
			return false;

		int format = read<int>(file);

		if (format == 5) {
			std::uint32_t version[4]{};
			file.read(reinterpret_cast<char*>(version), sizeof(version));

			char moduleName[64]{};
			file.read(moduleName, sizeof(moduleName));
			moduleName[sizeof(moduleName) - 1] = '\0';

			const auto ptrSize = read<std::int32_t>(file);
			const auto dataFormat = read<std::int32_t>(file);
			const auto addrCount = read<std::int32_t>(file);
			(void)dataFormat;  // Reserved by format 5; offsets are stored as dense uint32 values.

			const auto dataStart = file.tellg();
			if (!file || ptrSize != 8 || addrCount < 0 || dataStart == std::streampos(-1)) {
				return false;
			}

			file.seekg(0, std::ios::end);
			const auto fileEnd = file.tellg();
			const auto requiredBytes = static_cast<std::uint64_t>(addrCount) * sizeof(std::uint32_t);
			if (!file || fileEnd < dataStart ||
				static_cast<std::uint64_t>(fileEnd - dataStart) < requiredBytes) {
				return false;
			}
			file.seekg(dataStart);
			if (!file) {
				return false;
			}

			const int requestedVersion[4]{ major, minor, revision, build };
			for (int i = 0; i < 4; ++i) {
				_ver[i] = static_cast<int>(version[i]);
				if (_ver[i] != requestedVersion[i]) {
					Clear();
					return false;
				}
			}

			{
				char verName[64];
				_snprintf_s(verName, 64, "%d.%d.%d.%d", _ver[0], _ver[1], _ver[2], _ver[3]);
				_verStr = verName;
			}

			_moduleName = moduleName;
			_base = reinterpret_cast<unsigned long long>(
				GetModuleHandleA(_moduleName.empty() ? nullptr : _moduleName.c_str()));

			for (int id = 0; id < addrCount; ++id) {
				const auto offset = read<std::uint32_t>(file);
				if (!file) {
					Clear();
					return false;
				}
				if (offset != 0) {
					_data[static_cast<unsigned long long>(id)] = offset;
					_rdata[offset] = static_cast<unsigned long long>(id);
				}
			}

			return true;
		}

		if (format != 1 && format != 2)
			return false;

		for (int i = 0; i < 4; i++)
			_ver[i] = read<int>(file);

		{
			char verName[64];
			_snprintf_s(verName, 64, "%d.%d.%d.%d", _ver[0], _ver[1], _ver[2], _ver[3]);
			_verStr = verName;
		}

		int tnLen = read<int>(file);

		if (tnLen < 0 || tnLen >= 0x10000)
			return false;

		if(tnLen > 0)
		{
			char* tnbuf = (char*)malloc(tnLen + 1);
			file.read(tnbuf, tnLen);
			tnbuf[tnLen] = '\0';
			_moduleName = tnbuf;
			free(tnbuf);
		}

		{
			HMODULE handle = GetModuleHandleA(_moduleName.empty() ? NULL : _moduleName.c_str());
			_base = (unsigned long long)handle;
		}

		int ptrSize = read<int>(file);

		int addrCount = read<int>(file);

		unsigned char type, low, high;
		unsigned char b1, b2;
		unsigned short w1, w2;
		unsigned int d1, d2;
		unsigned long long q1, q2;
		unsigned long long pvid = 0;
		unsigned long long poffset = 0;
		unsigned long long tpoffset;
		for (int i = 0; i < addrCount; i++)
		{
			type = read<unsigned char>(file);
			low = type & 0xF;
			high = type >> 4;

			switch (low)
			{
			case 0: q1 = read<unsigned long long>(file); break;
			case 1: q1 = pvid + 1; break;
			case 2: b1 = read<unsigned char>(file); q1 = pvid + b1; break;
			case 3: b1 = read<unsigned char>(file); q1 = pvid - b1; break;
			case 4: w1 = read<unsigned short>(file); q1 = pvid + w1; break;
			case 5: w1 = read<unsigned short>(file); q1 = pvid - w1; break;
			case 6: w1 = read<unsigned short>(file); q1 = w1; break;
			case 7: d1 = read<unsigned int>(file); q1 = d1; break;
			default:
			{
				Clear();
				return false;
			}
			}

			tpoffset = (high & 8) != 0 ? (poffset / (unsigned long long)ptrSize) : poffset;

			switch (high & 7)
			{
			case 0: q2 = read<unsigned long long>(file); break;
			case 1: q2 = tpoffset + 1; break;
			case 2: b2 = read<unsigned char>(file); q2 = tpoffset + b2; break;
			case 3: b2 = read<unsigned char>(file); q2 = tpoffset - b2; break;
			case 4: w2 = read<unsigned short>(file); q2 = tpoffset + w2; break;
			case 5: w2 = read<unsigned short>(file); q2 = tpoffset - w2; break;
			case 6: w2 = read<unsigned short>(file); q2 = w2; break;
			case 7: d2 = read<unsigned int>(file); q2 = d2; break;
			default: throw std::exception();
			}

			if ((high & 8) != 0)
				q2 *= (unsigned long long)ptrSize;

			_data[q1] = q2;
			_rdata[q2] = q1;

			poffset = q2;
			pvid = q1;
		}

		return true;
	}

	bool Dump(const std::string& path)
	{
		std::ofstream f = std::ofstream(path.c_str());
		if (!f.good())
			return false;

		for (auto itr = _data.begin(); itr != _data.end(); itr++)
		{
			f << std::dec;
			f << itr->first;
			f << '\t';
			f << std::hex;
			f << itr->second;
			f << '\n';
		}

		return true;
	}
};
