
#include "functions.h"
#include "hookApi.h"
#include "dllmain.h"



HOOK_TRAMPS g_trump[MAX_TRAMP_SIZE] =	{ 0 };


HOOK_TRAMPS * searchTrump(const WCHAR* funcname) {

	for (int i = 0;i < TRAMP_BAR_LIMIT;i ++)
	{
		if (g_trump[i].valid)
		{
			if (lstrcmpiW(funcname, g_trump[i].apiName) == 0)
			{
				return &g_trump[i];
			}
		}
	}

	return 0;
}



int deleteTrump(const WCHAR* funcname) {
	for (int i = 0; i < TRAMP_BAR_LIMIT; i++)
	{
		if (g_trump[i].valid)
		{
			if (lstrcmpiW(funcname, g_trump[i].apiName) == 0)
			{
				g_trump[i].valid = FALSE;
				return TRUE;
			}
		}
	}
	return 0;
}



HOOK_TRAMPS * addTrump(const WCHAR* funcname) {
	int result = 0;
	for (int i = 0; i < MAX_TRAMP_SIZE; i++)
	{
		if (g_trump[i].valid == FALSE)
		{
			g_trump[i].valid = TRUE;
			lstrcpyW(g_trump[i].apiName, funcname);
			DWORD oldprotect = 0;
			result = VirtualProtect(&g_trump[i], sizeof(HOOK_TRAMPS), PAGE_EXECUTE_READWRITE, &oldprotect);
			return &g_trump[i];
		}
	}
	return 0;
}

#ifdef _WIN64
#include "hde/hde64.h"

#define ADDRESS64_HIGI_MASK				0xffffffff00000000L

#define ADDRESS64_LOW_MASK				0xffffffffL

#define AMD64_INLINE_HOOK_STUB_SIZE		14

int inlinehook64(BYTE* newfun, BYTE* hookaddr, PROC* keepaddr, const WCHAR* funcname) {
	int result = 0;

	HOOK_TRAMPS* trump = addTrump(funcname);
	if (trump <= 0)
	{
		log(L"inlinehook64 addTrump function:%ws error\r\n", funcname);
		return FALSE;
	}

	//DebugBreak();

	ULONGLONG offset = 0;

	int codelen = 0;

	BYTE* oldcode = hookaddr;

	BYTE* oldfun = oldcode;

	int total = 0;

	ULONGLONG minimum = AMD64_INLINE_HOOK_STUB_SIZE;

	hde64s asm64 = { 0 };

	while (codelen < minimum)
	{
		int instructionlen = hde64_disasm(oldfun, &asm64);
		if (instructionlen <= 0)
		{
			log(L"inlinehook64 function:%ws address:%p hde64_disasm error\r\n", funcname, oldfun);
			deleteTrump(funcname);
			return FALSE;
		}

		if (total == 0)
		{
			if ( (*oldfun == 0xff && *(oldfun + 1) == 0x25) || (*oldfun == 0xff && *(oldfun + 1) == 0x15) )
			{
				offset = *(DWORD*)(oldfun + 2);
				offset = ((offset + 6 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK) + ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);
				offset = *(ULONGLONG*)offset;
				oldfun = (BYTE*)(offset);
				oldcode = oldfun;
				codelen = 0;

				continue;
			}
			else if ( (*oldfun == 0x48 && *(oldfun + 1) == 0xff && *(oldfun + 2) == 0x25)||
				(*oldfun == 0x48 && *(oldfun + 1) == 0xff && *(oldfun + 2) == 0x15) )
			{
				offset = *(DWORD*)(oldfun + 3);
				offset = ((offset + 7 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK) + ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);
				offset = *(ULONGLONG*)offset;
				oldfun = (BYTE*)(offset);
				oldcode = oldfun;
				codelen = 0;
				continue;
			}
			else if (*oldfun == 0xeb)
			{
				offset = *(oldfun + 1);
				oldfun = (BYTE*)((offset + 2 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK) + ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);
				oldcode = oldfun;
				codelen = 0;
				continue;
			}
			else if (*oldfun == 0xe9 || *oldfun == 0xe8)		//if hooked by others ,how to find lost 5 bytes?
			{
				offset = *(DWORD*)(oldfun + 1);
				oldfun = (BYTE*)(((ULONGLONG)oldfun + 5 + offset) & ADDRESS64_LOW_MASK) + ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);

				oldcode = oldfun;
				codelen = 0;
				continue;
			}

			else if (*oldfun == 0x83 && *(oldfun + 1) == 0x3d)
				//0000000180026130 83 3D 35 C8 08 00 05                    cmp     cs:?g_systemCallFilterId@@3KA, 5
				//0000000180026137 74 0C                                   jz      short loc_180026145
			{
				memcpy(trump->code + codelen, oldfun, instructionlen);

				offset = *(DWORD*)(oldfun + 2);

				ULONGLONG offsetlow = ((offset + 7 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK);
				ULONGLONG offsethigh = ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);

				ULONGLONG delta = (offsetlow - ((ULONGLONG)trump->code + codelen + instructionlen)) & ADDRESS64_LOW_MASK;
				*(DWORD*)(trump->code + codelen + 2) = (DWORD)delta;
			}
			else {
				memcpy(trump->code + codelen, oldfun, instructionlen);
			}
		}
		else {

			if ((*oldfun == 0xff && *(oldfun + 1) == 0x25) || (*oldfun == 0xff && *(oldfun + 1) == 0x15))
			{
				memcpy(trump->code + codelen, oldfun, instructionlen);

				offset = *(DWORD*)(oldfun + 2);

				ULONGLONG offsetlow =( (offset + 6 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK);
				ULONGLONG offsethigh = ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);

				ULONGLONG delta = (offsetlow - ((ULONGLONG)trump->code + codelen + instructionlen)) & ADDRESS64_LOW_MASK;
				*(DWORD*)(trump->code + codelen + 2) = (DWORD)delta;
				
			}
			else if (*oldfun == 0x83 && *(oldfun + 1) == 0x3d)
				//GetDC in user32.dll in win10 and win11 x64
				//0000000180026130 83 3D 35 C8 08 00 05                    cmp     cs:?g_systemCallFilterId@@3KA, 5
				//0000000180026137 74 0C                                   jz      short loc_180026145
			{
				memcpy(trump->code + codelen, oldfun, instructionlen);

				offset = *(DWORD*)(oldfun + 2);

				ULONGLONG offsetlow = ((offset + 7 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK);
				ULONGLONG offsethigh = ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);

				ULONGLONG delta = (offsetlow - ((ULONGLONG)trump->code + codelen + instructionlen)) & ADDRESS64_LOW_MASK;
				*(DWORD*)(trump->code + codelen + 2) = (DWORD)delta;
			}
			else if (*oldfun == 0x48 && *(oldfun + 1) == 0x8b && *(oldfun + 2) == 0x05)
				//OpenPrinterW in win10 and win11 x64,winspool.drv
				//0000000180003294 48 8B 05 E5 30 07 00		mov     rax, cs:qword_180076380
				//000000018000329B 
			{
				memcpy(trump->code + codelen, oldfun, instructionlen);

				offset = *(DWORD*)(oldfun + 3);

				ULONGLONG offsetlow = ((offset + 7 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK);
				ULONGLONG offsethigh = ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);

				ULONGLONG delta = (offsetlow - ((ULONGLONG)trump->code + codelen + instructionlen)) & ADDRESS64_LOW_MASK;
				*(DWORD*)(trump->code + codelen + 3) = (DWORD)delta;
			}
			else if ((*oldfun == 0x48 && *(oldfun + 1) == 0xff && *(oldfun + 2) == 0x25)||
				(*oldfun == 0x48 && *(oldfun + 1) == 0xff && *(oldfun + 2) == 0x15))
			{
				memcpy(trump->code + codelen, oldfun, instructionlen);

				offset = *(DWORD*)(oldfun + 3);
				ULONGLONG offsetlow = ((offset + 7 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK);
				ULONGLONG offsethigh = ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);

				ULONGLONG delta = (offsetlow - ((ULONGLONG)trump->code + codelen + instructionlen)) & ADDRESS64_LOW_MASK;
				*(DWORD*)(trump->code + codelen + 3) = (DWORD)delta;
			}
			else if (*oldfun == 0xe9 || *oldfun == 0xe8 || *oldfun == 0xea)		//if hooked by others ,how to find lost 5 bytes?
			{
				memcpy(trump->code + codelen, oldfun, instructionlen);

				offset = *(DWORD*)(oldfun + 1);
				ULONGLONG offsetlow = ((offset + 5 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK);
				ULONGLONG offsethigh = ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);

				ULONGLONG delta = (offsetlow - ((ULONGLONG)trump->code + codelen + instructionlen)) & ADDRESS64_LOW_MASK;
				*(DWORD*)(trump->code + codelen + 1) = (DWORD)delta;
			}
			else if (*oldfun == 0xeb)
			{
				offset = *(oldfun + 1);
				byte* next_instruction = (BYTE*)((offset + 2 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK) + ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);
				minimum =  next_instruction - oldcode + 1;
				if (minimum > MAX_TRAMP_SIZE - AMD64_INLINE_HOOK_STUB_SIZE || minimum <= 0)
				{
					log(L"inlinehook64 function:%ws jump to target address:%p out of range on base:%p\r\n", funcname, oldfun, oldcode);
					deleteTrump(funcname);
					return FALSE;
				}
				memcpy(trump->code + codelen, oldfun, instructionlen);
			}
			else if (*oldfun == 0x0f && (*(oldfun + 1) >= 0x80 && *(oldfun + 1) <= 0x8f))
			{
				log(L"inlinehook64 function:%ws address:%p found irregular jump instruction: %02x %02x\r\n", funcname, oldfun, *oldfun, *(oldfun + 1));

				memcpy(trump->code + codelen, oldfun, instructionlen);

				offset = *(WORD*)(oldfun + 2);
				byte* next_instruction = (BYTE*)((offset + 4 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK) + ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);
				
				minimum = next_instruction - oldcode + 1;
				if (minimum > MAX_TRAMP_SIZE - AMD64_INLINE_HOOK_STUB_SIZE || minimum <= 0)
				{
					log(L"inlinehook64 function:%ws jump to target address:%p out of range on base:%p\r\n", funcname, oldfun, oldcode);
					deleteTrump(funcname);
					return FALSE;
				}
				
				//oldfun = oldfun + 4;
			}
			else if (*oldfun >= 0x70 && *oldfun <= 0x7f)		//jump with flag range in 128 bytes
			{
				memcpy(trump->code + codelen, oldfun, instructionlen);

				offset = *(oldfun + 1);

				byte* next_instruction = oldfun + 2 + offset ;

				minimum = next_instruction - oldcode + 1;
				if (minimum > MAX_TRAMP_SIZE - AMD64_INLINE_HOOK_STUB_SIZE || minimum <= 0)
				{
					log(L"inlinehook64 function:%ws jump to target address:%p out of range on base:%p\r\n", funcname, oldfun, oldcode);
					deleteTrump(funcname);
					return FALSE;
				}
				//oldfun += 2;
			}
// 			else if (*oldfun == 0xc3 )
// 			{
// 
// 			}
			else {
				memcpy(trump->code + codelen, oldfun, instructionlen);
			}
		}

		total++;
		codelen += instructionlen;
		oldfun += instructionlen;
	}

	if (oldcode == newfun)
	{
		log(L"inlinehook64 function:%ws jump address:%p already exist\r\n", funcname, oldfun);
		deleteTrump(funcname);
		return FALSE;
	}

	DWORD oldprotect = 0;
	result = VirtualProtect(oldcode, codelen, PAGE_EXECUTE_READWRITE, &oldprotect);
	if (result == 0)
	{
		log(L"inlinehook64 VirtualProtect function:%ws address:%p error\r\n", funcname, oldcode);
		deleteTrump(funcname);
		return FALSE;
	}

	memcpy(trump->oldcode, oldcode, codelen);

	oldcode[0] = 0xff;
	oldcode[1] = 0x25;
	*(DWORD*)(oldcode + 2) = 0;
	*(ULONGLONG*)(oldcode + 6) = (ULONGLONG)newfun;

	trump->code[codelen] = 0xff;
	trump->code[codelen + 1] = 0x25;
	*(DWORD*)(trump->code + codelen + 2) = 0;
	*(ULONGLONG*)(trump->code + codelen + 6) = (ULONGLONG)(oldcode + codelen);

	*keepaddr = (FARPROC) & (trump->code);

	DWORD dummyprotect = 0;
	result = VirtualProtect(oldcode, codelen, oldprotect, &dummyprotect);

	trump->replace.oldaddr = oldcode;
	trump->replace.len = codelen;


	log(L"inlinehook64 function:%ws, hook size:%d ,tramp address:%p, instruction address:%p,new func address:%p,keep address:%p",
		funcname, codelen, trump->code,oldcode,newfun,*keepaddr);
	WCHAR szout[1024];
	WCHAR* loginfo = szout;
	int len = 0;

	for (int i = 0; i < 14; i++)
	{
		len = wsprintfW(loginfo, L" %02X ", *(oldcode + i));
		loginfo = loginfo + len;
	}
	*loginfo = 0;
	log(szout);

	loginfo = szout;
	for (int i = 0; i < codelen + 14; i++)
	{
		len = wsprintfW(loginfo, L" %02X ", *(trump->code + i));
		loginfo = loginfo + len;
	}
	*loginfo = 0;
	log(szout);

	return result;
}
#else
#include "hde/hde32.h"

#define IA32_INLINE_HOOK_STUB_SIZE		5

int inlinehook32(BYTE* newfun, BYTE* hookaddr, PROC* keepaddr, const WCHAR* funcname) {
	int result = 0;

	HOOK_TRAMPS* trump = addTrump(funcname);
	if (trump <= 0)
	{
		log(L"inlinehook32 addTrump function:%ws error\r\n", funcname);
		return FALSE;
	}

	DWORD offset = 0;

	int codelen = 0;

	hde32s asm32 = { 0 };

	BYTE* oldcode = hookaddr;

	BYTE* oldfun = oldcode;

	int total = 0;

	int minimum = IA32_INLINE_HOOK_STUB_SIZE;

	while (codelen < minimum)
	{
		int instructionlen = hde32_disasm(oldfun, &asm32);
		if (instructionlen <= 0)
		{
			log(L"inlinehook32 function:%ws address:%p hde64_disasm error\r\n", funcname, oldfun);
			deleteTrump(funcname);
			return FALSE;
		}

		if (total == 0)
		{
			if ((*oldfun == 0xff && *(oldfun + 1) == 0x25) || (*oldfun == 0xff && *(oldfun + 1) == 0x15))
			{
				//FF 25 D4 0F B1 76
				//76b10fd4:76 1e a4 70
				//70a41e76:CreateFileA
				offset = *(DWORD*)(oldfun + 2);
				offset = *(DWORD*)offset;
				oldfun = (BYTE*)offset;

				oldcode = oldfun;
				codelen = 0;
				continue;
			}
			else if (*oldfun == 0xe9 || *oldfun == 0xe8)		//if hooked by others ,how to find lost 5 bytes?
			{
				offset = *(DWORD*)(oldfun + 1);
				oldfun += offset + 5;

				oldcode = oldfun;
				codelen = 0;
				continue;
			}
			else if (*oldfun == 0xeb)
			{
				offset = *(oldfun + 1);
				oldfun += offset + 2;

				oldcode = oldfun;
				codelen = 0;
				continue;
			}
			else {
				memcpy(trump->code + codelen, oldfun, instructionlen);
			}
		}
		else {
			if ((*oldfun == 0xff && *(oldfun + 1) == 0x25) || (*oldfun == 0xff && *(oldfun + 1) == 0x15) )
			{
				memcpy(trump->code + codelen, oldfun, instructionlen);

				//FF 25 D4 0F B1 76
				//76b10fd4:76 1e a4 70
				//70a41e76:CreateFileA
			}
			else if (*oldfun == 0xe9 || *oldfun == 0xe8 || *oldfun == 0xea)		//if hooked by others ,how to find lost 5 bytes?
			{
				memcpy(trump->code + codelen, oldfun, instructionlen);

				offset = *(DWORD*)(oldfun + 1);

				offset = (DWORD)oldfun + offset + 5;

				DWORD delta = offset - ((DWORD)trump->code + codelen + instructionlen);

				*(DWORD*)(trump->code + codelen + 1) = delta ;
			}

			else if (*oldfun == 0xeb)
			{
				memcpy(trump->code + codelen, oldfun, instructionlen);

				offset = *(oldfun + 1);
				byte* next_instruction = oldfun + offset + 2;

				minimum = next_instruction - oldcode + 1;
				if (minimum > MAX_TRAMP_SIZE - IA32_INLINE_HOOK_STUB_SIZE || minimum <= 0)
				{
					log(L"inlinehook32 function:%ws jump to target address:%p out of range on base:%p\r\n", funcname, oldfun, oldcode);
					deleteTrump(funcname);
					return FALSE;
				}
			}

			else if (*oldfun == 0x0f && (*(oldfun + 1) >= 0x80 && *(oldfun + 1) <= 0x8f))
			{
				log(L"inlinehook32 function:%ws address:%p found irregular jump instruction: %02x %02x\r\n", funcname, oldfun, *oldfun, *(oldfun + 1));

				memcpy(trump->code + codelen, oldfun, instructionlen);
				offset = *(WORD*)(oldfun + 2);
				byte* next_instruction = (BYTE*)(offset + 4 + oldfun);

				minimum = next_instruction - oldcode + 1;
				if (minimum > MAX_TRAMP_SIZE - IA32_INLINE_HOOK_STUB_SIZE || minimum <= 0)
				{
					log(L"inlinehook32 function:%ws jump to target address:%p out of range on base:%p\r\n", funcname, oldfun, oldcode);
					deleteTrump(funcname);
					return FALSE;
				}
				//oldfun += 4;
			}
			else if (*oldfun >= 0x70 && *oldfun <= 0x7f)		//jump with flag range in 128 bytes
			{
				memcpy(trump->code + codelen, oldfun, instructionlen);

				offset = *(oldfun + 1);

				byte* next_instruction = oldfun + 2 + offset;

				minimum = next_instruction - oldcode + 1;
				if (minimum > MAX_TRAMP_SIZE - IA32_INLINE_HOOK_STUB_SIZE || minimum <= 0)
				{
					log(L"inlinehook32 function:%ws jump to target address:%p out of range on base:%p\r\n", funcname, oldfun, oldcode);
					deleteTrump(funcname);
					return FALSE;
				}
				//oldfun += 2;
			}
			else {
				memcpy(trump->code + codelen, oldfun, instructionlen);
			}
		}
		codelen += instructionlen;
		oldfun += instructionlen;
		total++;
	}

	if (oldcode == newfun)
	{
		log(L"inlinehook32 function:%ws jump address:%p already exist\r\n", funcname, oldfun);
		deleteTrump(funcname);
		return FALSE;
	}

	DWORD oldprotect = 0;
	result = VirtualProtect(oldcode, codelen, PAGE_EXECUTE_READWRITE, &oldprotect);
	if (result == 0)
	{
		log(L"inlinehook32 VirtualProtect function:%ws address:%p error\r\n", funcname, oldcode);
		deleteTrump(funcname);
		return FALSE;
	}

	memcpy(trump->oldcode, oldcode, codelen);

	oldcode[0] = 0xe9;

	*(DWORD*)(oldcode + 1) = newfun - (oldcode + 5);

	trump->code[codelen] = 0xe9;

	*(DWORD*)(trump->code + codelen + 1) = oldcode + codelen - (trump->code + codelen + 5);

	*keepaddr = (FARPROC) & (trump->code);

	DWORD dummyprotect = 0;
	result = VirtualProtect(oldcode, codelen, oldprotect, &dummyprotect);

	trump->replace.oldaddr = oldcode;
	trump->replace.len = codelen;


	log(L"inlinehook32 function:%ws,hook size:%d,tramp address:%p,instruction address:%p,new func address:%p,keep address:%p",
		funcname, codelen, trump->code,oldcode,newfun,keepaddr);
	WCHAR szout[1024];
	WCHAR* loginfo = szout;
	int len = 0;

	for (int i = 0; i < 5; i++)
	{
		len = wsprintfW(loginfo, L" %02X ", *(oldcode + i));
		loginfo = loginfo + len;
	}
	*loginfo = 0;
	log(szout);

	loginfo = szout;
	for (int i = 0; i < codelen + 5; i++)
	{
		len = wsprintfW(loginfo, L" %02X ", *(trump->code + i));
		loginfo = loginfo + len;
	}
	*loginfo = 0;
	log(szout);

	return result;
}

#endif

int hook(CONST WCHAR* modulename, const WCHAR* wstrfuncname, BYTE* newfuncaddr, PROC* keepaddr) {
	int result = 0;
	HMODULE h = GetModuleHandleW(modulename);
	if (h)
	{
		CHAR funcname[MAX_PATH];
		result = WideCharToMultiByte(CP_ACP, 0, wstrfuncname, -1, funcname, MAX_PATH, 0, 0);
		if (result)
		{
			LPBYTE  oldfunc = (LPBYTE)GetProcAddress(h, funcname);
			if (oldfunc)
			{
#ifdef _WIN64

				result = inlinehook64(newfuncaddr, oldfunc, keepaddr, wstrfuncname);
#else
				result = inlinehook32(newfuncaddr, oldfunc, (FARPROC*)keepaddr, wstrfuncname);
#endif
				if (result)
				{
					log(L"hook %ws %ws ok\r\n", modulename, wstrfuncname);
				}
				return result;
			}
			else {
				log(L"function %ws not found in %ws", wstrfuncname, modulename);
			}
		}
		else {
			log(L"WideCharToMultiByte function %ws error", wstrfuncname);
		}
	}
	else {
		log(L"module %ws not found", modulename);
	}
	return FALSE;
}


int unhook(CONST WCHAR* modulename, const WCHAR* wstrfuncname) {
	int result = 0;

	for (int i = 0; i < TRAMP_BAR_LIMIT; i++)
	{
		if (g_trump[i].valid)
		{
			if (lstrcmpiW(wstrfuncname, g_trump[i].apiName) == 0)
			{
				unsigned char* oldcode = g_trump[i].replace.oldaddr;
				int oldcodelen = g_trump[i].replace.len;
				DWORD oldprotect = 0;
				result = VirtualProtect(oldcode, oldcodelen, PAGE_EXECUTE_READWRITE, &oldprotect);
				memcpy(oldcode, g_trump[i].oldcode, oldcodelen);
				DWORD dummyprotect = 0;
				result = VirtualProtect(oldcode, oldcodelen, oldprotect, &dummyprotect);

				g_trump[i].valid = FALSE;

				log(L"delete hooked function:%ws ok", wstrfuncname);
				return TRUE;
			}
		}
	}

	return FALSE;
}


int  unhookall() {
	int result = 0;

	for (int i = 0; i < TRAMP_BAR_LIMIT; i++)
	{
		if (g_trump[i].valid != 0 && g_trump[i].valid != 1) {
			log(L"funciton :%d error", i);
		}
		if (g_trump[i].valid == TRUE /*&& g_trump[i].apiName[0]*/)
		{
			unsigned char* oldcode = g_trump[i].replace.oldaddr;

			int oldcodelen = g_trump[i].replace.len;
			result = IsBadReadPtr(oldcode, oldcodelen);
			if (result == 0)
			{
				DWORD oldprotect = 0;
				result = VirtualProtect(oldcode, oldcodelen, PAGE_EXECUTE_READWRITE, &oldprotect);

				memcpy(oldcode, g_trump[i].oldcode, oldcodelen);
				DWORD dummyprotect = 0;
				result = VirtualProtect(oldcode, oldcodelen, oldprotect, &dummyprotect);
				log(L"delete hooked function:%ws ok", g_trump[i].apiName);
			}
			else {
				log(L"funciton:%ws address:%p can not be write,maybe the dll has been unload?", g_trump[i].apiName,oldcode);
			}

			g_trump[i].valid = FALSE;	
		}
		else {
			
		}
	}

	return TRUE;
}

 


PUCHAR allocTrampAddress(PUCHAR  module) {
	IMAGE_NT_HEADERS64* hdr = (IMAGE_NT_HEADERS64*)module;
	PUCHAR* address = (PUCHAR * )hdr->OptionalHeader.ImageBase + hdr->OptionalHeader.SizeOfImage;

	PUCHAR* alloc = (PUCHAR*)VirtualAlloc(address, 0x4000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (alloc)
	{
	}
	return 0;
}
