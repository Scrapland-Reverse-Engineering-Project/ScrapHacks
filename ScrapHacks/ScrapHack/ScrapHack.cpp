#include "stdafx.h"
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include "Scrapland.h"

#define DLL_EXPORT extern "C" __declspec(dllexport)

struct Module;
using namespace std;

map<string, Module> Py;

bool initialized = false;
bool running = true;
HMODULE mod = 0;

string GetLastErrorAsString()
{
	DWORD errorMessageID = GetLastError();
	if (errorMessageID == 0)
		return "No error";
	LPSTR messageBuffer = NULL;
	size_t m_size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								   NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
	string message(messageBuffer, m_size);
	LocalFree(messageBuffer);
	if (!message.empty() && message[message.length() - 1] == '\n')
	{
		message.erase(message.length() - 1);
	}
	return message;
}

void SetupStreams()
{
	FILE *fIn;
	FILE *fOut;
	freopen_s(&fIn, "conin$", "r", stdin);
	freopen_s(&fOut, "conout$", "w", stdout);
	freopen_s(&fOut, "conout$", "w", stderr);
	ios::sync_with_stdio();
	std::wcout.clear();
	std::cout.clear();
	std::wcerr.clear();
	std::cerr.clear();
	std::wcin.clear();
	std::cin.clear();
}

void SetupConsole()
{
	if (!AllocConsole())
	{
		FreeConsole();
		AllocConsole();
	}
	AttachConsole(GetCurrentProcessId());
	SetupStreams();
}

void SetupConsole(const char *title)
{
	SetupConsole();
	SetConsoleTitleA(title);
}

void FreeConsole(bool wait)
{
	if (wait)
	{
		cout << "[?] Press Enter to Exit";
		cin.ignore();
	}
	FreeConsole();
}

bool in_foreground = false;
BOOL CALLBACK EnumWindowsProcMy(HWND hwnd, LPARAM lParam)
{
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	if (lpdwProcessId == lParam)
	{
		in_foreground = (hwnd == GetForegroundWindow()) || (hwnd == GetActiveWindow());
		return FALSE;
	}
	return TRUE;
}

bool key_down(int keycode, int delay = 100)
{
	in_foreground = false;
	EnumWindows(EnumWindowsProcMy, GetCurrentProcessId());
	if (in_foreground)
	{
		if (GetAsyncKeyState(keycode))
		{
			Sleep(delay);
			return true;
		}
	}
	return false;
}

bool key_down_norepeat(int keycode, int delay = 100)
{
	in_foreground = false;
	EnumWindows(EnumWindowsProcMy, GetCurrentProcessId());
	if (in_foreground)
	{
		if (GetAsyncKeyState(keycode))
		{
			while (GetAsyncKeyState(keycode))
			{
				Sleep(delay);
			}
			return true;
		}
	}
	return false;
}

struct PyMethodDef
{
	char *ml_name;
	void *ml_meth;
	int ml_flags;
	char *ml_doc;
};

struct PyMod
{
	char *name;
	void *init_func;
};

struct Module
{
	PyMod *mod;
	map<string, PyMethodDef> methods;
};

void hexdump(void *addr, size_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		unsigned int val = (unsigned int)((unsigned char *)addr)[i];
		cout << setfill('0') << setw(2) << std::hex << val << " ";
		if (((i + 1) % 16) == 0)
		{
			cout << endl;
		}
	}
	cout << endl;
}

PyMethodDef *find_method_table(size_t base, size_t size)
{
	uint8_t *ptr = reinterpret_cast<uint8_t *>(base);
	for (size_t offset = 0; offset < size; ++offset)
	{
		if ((uint16_t)ptr[offset] == 0x68)
		{
			uint32_t mod_addr = reinterpret_cast<uint32_t *>(base + offset + 1)[0];
			if ((mod_addr & 0xf00000) == 0x700000)
			{
				if (strlen(reinterpret_cast<char *>(mod_addr)) == 3)
				{
					return reinterpret_cast<PyMethodDef *>(mod_addr);
				}
			}
		}
	}
	return reinterpret_cast<PyMethodDef *>(0);
}

map<string, Module> get_modules(size_t base)
{
	map<string, Module> Py;
	PyMod *modules = reinterpret_cast<PyMod *>(base);
	for (int i = 0; modules[i].init_func != NULL; i++)
	{
		Module mod;
		mod.mod = &modules[i];
		PyMethodDef *method_table = find_method_table((size_t)modules[i].init_func, 64);
		for (int j = 0; method_table != NULL && method_table[j].ml_name != NULL; j++)
		{
			mod.methods[method_table[j].ml_name] = method_table[j];
		}
		Py[mod.mod->name] = mod;
	}
	return Py;
}

void *get_py(const char *mod, const char *meth)
{
	try
	{
		return Py.at(mod).methods.at(meth).ml_meth;
	}
	catch (out_of_range)
	{
		return NULL;
	}
}

void inject(const char *mod, const char *meth, void *detour)
{
	try
	{
		void *orig = get_py(mod, meth);
		Py.at(mod).methods.at(meth).ml_meth = detour;
		cout << mod << "." << meth << ": " << orig << " -> " << detour << endl;
	}
	catch (out_of_range)
	{
		cout << mod << "." << meth << " not found!" << endl;
	}
}

uint32_t ptr(uint32_t addr, vector<uint32_t> offsets)
{
	cout << "[" << (void *)addr << "]";
	for (uint32_t offset : offsets)
	{
		addr = reinterpret_cast<uint32_t *>(addr)[0];
		cout << " -> [" << (void *)addr << " + " << offset << "]";
		addr += offset;
	};
	cout << " -> " << (void *)addr;
	cout << endl;
	return addr;
}

void MainLoop(HMODULE mod)
{
	Sleep(100);
	cout << "[*] Starting main Loop" << endl;
	cout << endl;
	cout << "[F7 ] Set Money to 0x7fffffff" << endl;
	cout << "[F8 ] Dump python modules" << endl;
	cout << "[F10] Enable python tracing" << endl;
	cout << "[F11] Unload ScrapHacks" << endl;
	cout << "[ F ] \"Handbrake\" (*Will* crash the game after some time!)" << endl;

	while (running)
	{
		Sleep(100);
		if (key_down_norepeat(VK_F10))
		{
			scrap_exec("dbg.settrace()");
		}
		while (key_down('F'))
		{
			scrap_exec("dbg.brake()");
		}
		if (key_down_norepeat(VK_F6))
		{
			/*
			int32_t* alarm = reinterpret_cast<int32_t*>(ptr(WORLD, { 0x1C6C }));
			int16_t* alarm = reinterpret_cast<int16_t*>(ptr(WORLD, { 0x1C6C }));
			*/
		}
		if (key_down_norepeat(VK_F7))
		{
			/*==========================
			mov     ecx, [7FE944h]
			mov     edx, [ecx + 2090h]
			==========================*/
			int32_t *money = reinterpret_cast<int32_t *>(ptr(WORLD, {0x2090}));
			cout << "Money: " << money[0] << endl;
			money[0] = 0x7fffffff;
		}
		if (key_down_norepeat(VK_F8))
		{
			for (auto mod : Py)
			{
				for (auto meth : mod.second.methods)
				{
					if (meth.second.ml_doc != NULL)
					{
						cout << mod.first << "." << meth.first << " @ " << meth.second.ml_meth << " [" << &(meth.second) << "]" << endl;
					}
				}
			}
		}
		if (key_down_norepeat(VK_F11))
		{
			break;
		}
	}
	SetConsoleCtrlHandler(NULL, false);
	cout << "[+] ScrapHacks unloaded, you can now close the console!" << endl;
	;
	FreeConsole();
	FreeLibraryAndExitThread(mod, 0);
}

void InitConsole()
{
	char me[1024];
	GetModuleFileName(mod, me, 1024);
	SetupConsole(me);
}

void DllInit(HMODULE _mod)
{
	initialized = true;
	mod = _mod;
	char mfn[1024];
	InitConsole();
	GetModuleFileName(0, mfn, 1024);
	cout << "[+] ScrapHacks v0.1 Loaded in " << mfn << endl;
	Py = get_modules(PY_MODS);
	cout << "[*] Importing python dbg module" << endl;
	scrap_exec("import dbg");
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MainLoop, mod, 0, 0);
	cout << "[*] Starting message pump" << endl;
	;
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return;
}