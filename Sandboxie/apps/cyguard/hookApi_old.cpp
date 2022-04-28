
#include "functions.h"
#include "hookApi.h"
#include "dllmain.h"

HOOK_TRAMPS*g_trump = 0;

HOOK_TRAMPS* searchTrump(const WCHAR* funcname) {
	HOOK_TRAMPS* trump = g_trump;
	do
	{
		if (trump && trump->apiName)
		{
			if (lstrcmpiW(funcname, trump->apiName) == 0)
			{
				return trump;
			}
			trump = trump->next;
		}
		else {
			break;
		}
	} while (trump != g_trump);

	return 0;
}

int deleteTrump(const WCHAR* funcname) {
	HOOK_TRAMPS* trump = searchTrump(funcname);
	if (trump)
	{
		HOOK_TRAMPS* next = g_trump->next;
		HOOK_TRAMPS* prev = g_trump->prev;
		next->prev = prev;
		prev->next = next;
		delete trump;
		if (trump == g_trump)
		{
			g_trump = 0;
		}
		return TRUE;
	}
	return 0;
}





HOOK_TRAMPS* addTrump(const WCHAR * funcname) {
	int result = 0;

	if (g_trump == 0)
	{
		g_trump = new HOOK_TRAMPS;
		memset(g_trump, 0, sizeof(HOOK_TRAMPS));
		g_trump->prev = g_trump;
		g_trump->next = g_trump;
		lstrcpyW(g_trump->apiName, funcname);
		DWORD oldprotect = 0;
		result = VirtualProtect(g_trump, sizeof(HOOK_TRAMPS), PAGE_EXECUTE_READWRITE, &oldprotect);
		return g_trump;
	}
	else {
		HOOK_TRAMPS* trump = searchTrump(funcname);
		if (trump == FALSE)
		{
			trump = new HOOK_TRAMPS;
			memset(trump, 0, sizeof(HOOK_TRAMPS));
			lstrcpyW(trump->apiName, funcname);

			trump->prev = g_trump;
			trump->next = g_trump->next;

			HOOK_TRAMPS* next = g_trump->next;
			//HOOK_TRAMPS* prev = g_trump->prev;
			next->prev = trump;
			g_trump->next = trump;

			DWORD oldprotect = 0;
			result = VirtualProtect(trump, sizeof(HOOK_TRAMPS), PAGE_EXECUTE_READWRITE, &oldprotect);
			return trump;
		}
	}
	return 0;
}

#ifdef _WIN64
#include "hde/hde64.h"

#define ADDRESS64_HIGI_MASK		0xffffffff00000000L

#define ADDRESS64_LOW_MASK		0xffffffffL

int inlinehook64(BYTE* newfun, BYTE* hookaddr, PROC* keepaddr,const WCHAR * funcname) {
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

	hde64s asm64 = { 0 };

	while (1)
	{
		if (codelen >= 14)
		{
			break;
		}
		else if ( (*oldfun == 0xff && *(oldfun + 1) == 0x25) /*|| (*oldfun == 0xff && *(oldfun + 1) == 0x15)*/ )
		{
			offset = *(DWORD*)(oldfun + 2);
			offset = ( (offset + 6 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK) + ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);
			offset = *(ULONGLONG*)offset;
			oldfun = (BYTE*)(offset);
			oldcode = oldfun;
			codelen = 0;
		}
		else if ( (*oldfun == 0x48 && *(oldfun + 1) == 0xff && *(oldfun + 2) == 0x25)
			/*|| (*oldfun == 0x48 && *(oldfun + 1) == 0xff && *(oldfun + 2) == 0x15)*/ )
		{
			offset = *(DWORD*)(oldfun + 3);
			offset = ((offset + 7 + (ULONGLONG)oldfun)& ADDRESS64_LOW_MASK) + ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);
			offset = *(ULONGLONG*)offset;
			oldfun = (BYTE*)(offset);
			oldcode = oldfun;
			codelen = 0;
		}
		else if (*oldfun == 0xeb)
		{
			offset = *(oldfun + 1);
			oldfun = (BYTE*)((offset + 2 + (ULONGLONG)oldfun)&ADDRESS64_LOW_MASK) + ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);
			oldcode = oldfun;
			codelen = 0;
		}
		else if (*oldfun == 0xe9 || *oldfun == 0xe8)		//if hooked by others ,how to find lost 5 bytes?
		{
			offset = *(DWORD*)(oldfun + 1);
			oldfun = (BYTE*)(((ULONGLONG)oldfun + 5 + offset) & ADDRESS64_LOW_MASK) + ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);
			if (oldfun == newfun)
			{
				log(L"inlinehook64 function:%ws jump address:%p already exist\r\n",funcname, oldfun);
				deleteTrump(funcname);
				return FALSE;
			}
			oldcode = oldfun;
			codelen = 0;
		}
		else if (*oldfun == 0xea)		
		{
			//48 bits jump
		}
		else if ( *oldfun == 0x0f && ( *(oldfun + 1) >= 0x80 && *(oldfun + 1) <= 0x8f) )
		{
			log(L"inlinehook64 function:%ws address:%p found irregular jump instruction: %02x %02x\r\n", funcname, oldfun, *oldfun, *(oldfun + 1));

			offset = *(WORD*)(oldfun + 2);
			oldfun = (BYTE*)((offset + 4 + (ULONGLONG)oldfun) & ADDRESS64_LOW_MASK) + ((ULONGLONG)oldfun & ADDRESS64_HIGI_MASK);
			oldcode = oldfun;
			codelen = 0;
		}
		else if (*oldfun >= 0x70 && *oldfun <= 0x7f)		//jump with flag range in 128 bytes
		{
// 			offset = *(oldfun + 1);
// 			oldfun += (offset + 2);
// 			codelen = oldfun - oldcode;
// 			if (codelen < MAX_TRAMP_SIZE - 14)
// 			{
// 				goto __parseInstruction64;
// 			}
// 			else {
// 				log(L"inlinehook64 function:%ws jump to target address:%p out of range on base:%p\r\n", funcname, oldfun, oldcode);
// 				deleteTrump(funcname);
// 				return FALSE;
// 			}
		}
		else if (*oldfun == 0xc3 && codelen < 14)
		{

		}
		else {
			//dummy instructions
		}

		__parseInstruction64:
		int instructionlen = hde64_disasm(oldfun, &asm64);
		if (instructionlen > 0)
		{
			codelen += instructionlen;
			oldfun += instructionlen;
		}
		else {
			log(L"inlinehook64 function:%ws address:%p hde64_disasm error\r\n", funcname, oldfun);
			deleteTrump(funcname);
			return FALSE;
		}
	}

	DWORD oldprotect = 0;
	result = VirtualProtect(oldcode, codelen, PAGE_EXECUTE_READWRITE, &oldprotect);
	if (result == 0)
	{
		log(L"inlinehook64 VirtualProtect function:%ws address:%p error\r\n", funcname, oldcode);
		deleteTrump(funcname);
		return FALSE;
	}

	memcpy(trump->code, oldcode, codelen);

	oldcode[0] = 0xff;
	oldcode[1] = 0x25;
	*(DWORD*)(oldcode + 2) = 0;
	*(ULONGLONG*)(oldcode + 6) = (ULONGLONG)newfun;

	trump->code[codelen] = 0xff;
	trump->code[codelen + 1] = 0x25;
	*(DWORD*)(trump->code + codelen + 2) = 0;
	*(ULONGLONG*)(trump->code + codelen + 6) = (ULONGLONG)(oldcode + codelen);

	*keepaddr = (FARPROC)&(trump->code);

	DWORD dummyprotect = 0;
	result = VirtualProtect(oldcode, codelen, oldprotect, &dummyprotect);

	trump->replace.oldaddr = oldcode;
	trump->replace.len = codelen;



	log(L"inlinehook64 replace function:%ws len:%d address:%p instructions:", funcname,codelen, trump->code);
	WCHAR szout[1024];
	WCHAR* loginfo = szout;
	int loginfolen = 0;
	for (int i = 0; i < codelen; i++)
	{
		int len = wsprintfW(loginfo, L" %02x ", *(trump->code + i) );
		loginfo = loginfo + len;
	}
	*loginfo = 0;
	log(szout);

	return result;
}
#else
#include "hde/hde32.h"
int inlinehook32(BYTE * newfun,BYTE * hookaddr,PROC * keepaddr, const WCHAR* funcname) {
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
	while (1)
	{
		if (codelen >= 5)
		{
			break;
		}
// 		else if (*oldfun == 0x8b)
// 		{
// 			oldfun += 2;
// 			codelen += 2;
// 		}
// 		else if (*oldfun == 0x55)
// 		{
// 			oldfun += 1;
// 			codelen += 1;
// 		}
		else if ( (*oldfun == 0xff && *(oldfun + 1) == 0x25) /*|| (*oldfun == 0xff && *(oldfun + 1) == 0x15)*/ )
		{
			//FF 25 D4 0F B1 76
			//76b10fd4:76 1e a4 70
			//70a41e76:CreateFileA
			offset = *(DWORD*)(oldfun + 2);
			offset = *(DWORD*)offset;
			oldfun = (BYTE*)offset;
			oldcode = oldfun;
			codelen = 0;
		}
		else if (*oldfun == 0xeb)
		{
			offset = *(oldfun + 1);
			oldfun += offset + 2;
			oldcode = oldfun;
			codelen = 0;
		}else if (*oldfun == 0xe9 || *oldfun == 0xe8)		//if hooked by others ,how to find lost 5 bytes?
		{
			offset = *(DWORD*)(oldfun + 1);
			oldfun += offset + 5;
			if (oldfun == newfun)
			{
				log(L"inlinehook32 function:%ws jump address:%p already exist\r\n", funcname, oldfun);
				deleteTrump(funcname);
				return FALSE;
			}
			oldcode = oldfun;
			codelen = 0;
		}
		else if (*oldfun == 0xea)
		{
			//48 bits jump
		}
		else if (*oldfun == 0x0f && (*(oldfun + 1) >= 0x80 && *(oldfun + 1) <= 0x8f))
		{
			log(L"inlinehook32 function:%ws address:%p found irregular jump instruction: %02x %02x\r\n", funcname, oldfun, *oldfun, *(oldfun + 1));

			offset = *(WORD*)(oldfun + 2);
			oldfun = (BYTE*)(offset + 4 + oldfun);
			oldcode = oldfun;
			codelen = 0;
		}
// 		else if (*oldfun >= 0x70 && *oldfun <= 0x7f)		//jump with flag range in 128 bytes
// 		{
// 			offset = *(oldfun + 1);
// 			oldfun += (offset + 2);
// 			codelen = oldfun - oldcode;
// 			if (oldfun - oldcode < MAX_TRUMP_SIZE - 14)
// 			{
// 				goto __parseInstruction32;
// 			}
// 			else {
// 				log(L"inlinehook32 function:%ws jump to target address:%p out of range on base:%p\r\n", funcname, oldfun, oldcode);
// 				deleteTrump(funcname);
// 				return FALSE;
// 			}
// 			//oldcode = oldfun;
// 		}
		else if (*oldfun == 0xc3 && codelen < 5)
		{

		}
		else {
			//dummy
		}

		__parseInstruction32:
		int instructionlen = hde32_disasm(oldfun, &asm32);
		if (instructionlen > 0)
		{
			codelen += instructionlen;
			oldfun += instructionlen;

// 			log(L"inlinehook32 function:%ws address:%p instructions:", funcname, oldfun - instructionlen);
// 			for (int i = 0; i < instructionlen; i++)
// 			{
// 				log(L" %02x ", *(oldfun - instructionlen + i));
// 			}
// 			log(L"\r\n");
		}
		else {
			log(L"inlinehook32 function:%ws address:%p hde64_disasm error\r\n", funcname, oldfun);
			deleteTrump(funcname);
			return FALSE;
		}
	}

	DWORD oldprotect = 0;
	result = VirtualProtect(oldcode, codelen, PAGE_EXECUTE_READWRITE, &oldprotect);
	if (result == 0)
	{
		log(L"inlinehook32 VirtualProtect function:%ws address:%p error\r\n", funcname, oldcode);
		deleteTrump(funcname);
		return FALSE;
	}

	memcpy(trump->code, oldcode, codelen);

	oldcode[0] = 0xe9;

	*(DWORD*)(oldcode + 1) = newfun - (oldcode + 5);

	trump->code[codelen] = 0xe9;

	*(DWORD*)(trump->code + codelen + 1) = oldcode + codelen - (trump->code + codelen + 5);

	*keepaddr = (FARPROC)&(trump->code);

	DWORD dummyprotect = 0;
	result = VirtualProtect(oldcode, codelen, oldprotect, &dummyprotect);

	trump->replace.oldaddr = oldcode;
	trump->replace.len = codelen;



	log(L"inlinehook32 replace function:%ws len:%d address:%p instructions:", funcname, codelen, trump->code);
	WCHAR szout[1024];
	WCHAR* loginfo = szout;
	int loginfolen = 0;
	for (int i = 0; i < codelen; i++)
	{
		int len = wsprintfW(loginfo, L" %02x ", *(trump->code + i));
		loginfo = loginfo + len;
	}
	*loginfo = 0;
	log(szout);

	return result;
}



#endif

int hook(CONST WCHAR * modulename,const WCHAR* wstrfuncname, BYTE* newfuncaddr,PROC * keepaddr) {
	int result = 0;
	HMODULE h = GetModuleHandleW(modulename);
	if (h)
	{
		CHAR funcname[MAX_PATH];
		result = WideCharToMultiByte(CP_ACP, 0, wstrfuncname, -1, funcname, MAX_PATH,0,0);
		if (result)
		{
			LPBYTE  oldfunc = (LPBYTE)GetProcAddress(h, funcname);
			if (oldfunc)
			{
#ifdef _WIN64

				result = inlinehook64(newfuncaddr, oldfunc, keepaddr,wstrfuncname);
#else
				result = inlinehook32(newfuncaddr, oldfunc, (FARPROC*)keepaddr,  wstrfuncname);
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
		log( L"module %ws not found",  modulename);
	}
	return FALSE;
}


int unhook(CONST WCHAR* modulename, const WCHAR* wstrfuncname) {
	int result = 0;

	HOOK_TRAMPS* trump = searchTrump(wstrfuncname);
	if (trump)
	{
		HOOK_TRAMPS* prev = trump->prev;
		HOOK_TRAMPS* next = trump->next;

		prev->next = next;
		next->prev = prev;

		unsigned char* oldcode = trump->replace.oldaddr;
		int oldcodelen = trump->replace.len;
		DWORD oldprotect = 0;
		result = VirtualProtect(oldcode, oldcodelen, PAGE_EXECUTE_READWRITE, &oldprotect);
		memcpy(oldcode, trump->code, oldcodelen);
		DWORD dummyprotect = 0;
		result = VirtualProtect(oldcode, oldcodelen, oldprotect, &dummyprotect);

		if (trump == g_trump)
		{
			g_trump = 0;
		}
		delete trump;

		log(L"delete hooked function:%ws ok",wstrfuncname);
		return TRUE;
	}
	return FALSE;
}


int  unhookall() {
	int result = 0;
	int cnt = 0;
	HOOK_TRAMPS* trump = g_trump;
	do
	{
		if (trump && trump->apiName)
		{
			HOOK_TRAMPS* next = trump->next;

			unsigned char* oldcode = trump->replace.oldaddr;
			int oldcodelen = trump->replace.len;
			DWORD oldprotect = 0;
			result = VirtualProtect(oldcode, oldcodelen, PAGE_EXECUTE_READWRITE, &oldprotect);
			memcpy(oldcode, trump->code, oldcodelen);
			DWORD dummyprotect = 0;
			result = VirtualProtect(oldcode, oldcodelen, oldprotect, &dummyprotect);

			log(L"delete hooked function:%ws ok", trump->apiName);

			cnt++;
			delete trump;
			trump = next;
		}
		else {
			break;
		}
	} while (trump != g_trump);

	g_trump = 0;

	return cnt;
}

