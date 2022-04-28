

ifdef _WIN64

;

else

.386p
.model flat

endif

;----------------------------------------------------------------------------

.code


ifdef _WIN64
Shellcode64 PROC addr
	mov rax,[rcx]
	mov rdx,[rcx+8]
	mov r8,[rcx+16]
	mov r9,[rcx+24]
	mov rcx,rax
	mov rax,addr
	call addr
Shellcode64 ENDP
else
Shellcode32 PROC addr

	push ebp
	mov eax,[ebp]
	mov eax,[eax + 12]
	push eax
	mov eax,[eax+8]
	push eax
	mov eax,[eax+4]
	push eax
	mov eax,[eax]
	push eax
	mov eax,addr
	call eax

	pop ebp
	ret

endif
