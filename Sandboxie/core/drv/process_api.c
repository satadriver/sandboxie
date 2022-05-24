/*
 * Copyright 2004-2020 Sandboxie Holdings, LLC 
 * Copyright 2020 David Xanatos, xanasoft.com
 *
 * This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

//---------------------------------------------------------------------------
// Process Management:  API for User Mode
//---------------------------------------------------------------------------


#include "process.h"
#include "api.h"
#include "util.h"
#include "conf.h"
#include "token.h"
#include "file.h"
#include "key.h"
#include "ipc.h"
#include "thread.h"
#include "session.h"
#include "common/pattern.h"
#include "common/my_version.h"


//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------


static NTSTATUS Process_Api_CopyBoxPathsToUser(
    BOX *box, ULONG *file_path_len, ULONG *key_path_len, ULONG *ipc_path_len,
    UNICODE_STRING64 *file_path,
    UNICODE_STRING64 *key_path,
    UNICODE_STRING64 *ipc_path);


//---------------------------------------------------------------------------
// Process_Api_Start
//---------------------------------------------------------------------------


_FX NTSTATUS Process_Api_Start(PROCESS *proc, ULONG64 *parms)
{
    LONG_PTR user_box_parm;
    HANDLE user_pid_parm;
    BOX *box = NULL;
    PEPROCESS ProcessObject = NULL;
    NTSTATUS status;

    //
    // already sandboxed?
    //

    if (proc || (PsGetCurrentProcessId() != Api_ServiceProcessId))
        return STATUS_NOT_IMPLEMENTED;

    //
    // if not ready, don't even try
    //

    if (! Process_ReadyToSandbox)
        return STATUS_SERVER_DISABLED;

    //
    // first parameter is box name or box model pid
    //

    user_box_parm = (LONG_PTR)parms[1];

    if (user_box_parm < 0) {

        //
        // if paramter is negative, it specifies the pid number for a
        // process, from which we copy the box information, including
        // SID and session
        //

        PROCESS *proc2;
        KIRQL irql;

        proc2 = Process_Find((HANDLE)(-user_box_parm), &irql);
        if (proc2)
            box = Box_Clone(Driver_Pool, proc2->box);

        ExReleaseResourceLite(Process_ListLock);
        KeLowerIrql(irql);

        if (! proc2)
            return STATUS_INVALID_CID;

        if (! box)
            return STATUS_INSUFFICIENT_RESOURCES;

    } else {

        //
        // otherwise parameter specifies the box name to use, and the
        // thread impersonation token specifies SID and session
        //

        WCHAR boxname[34];

        void *TokenObject;
        BOOLEAN CopyOnOpen;
        BOOLEAN EffectiveOnly;
        SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;

        UNICODE_STRING SidString;
        ULONG SessionId;

        if (! Api_CopyBoxNameFromUser(boxname, (WCHAR *)user_box_parm))
            return STATUS_INVALID_PARAMETER;

        TokenObject = PsReferenceImpersonationToken(PsGetCurrentThread(),
                        &CopyOnOpen, &EffectiveOnly, &ImpersonationLevel);

        if (! TokenObject)
            return STATUS_NO_IMPERSONATION_TOKEN;

        status = SeQuerySessionIdToken(TokenObject, &SessionId);

        if (NT_SUCCESS(status)) {

            status = Token_QuerySidString(TokenObject, &SidString);
        }

        PsDereferenceImpersonationToken(TokenObject);

        if (! NT_SUCCESS(status))
            return status;

        box = Box_CreateEx(
                Driver_Pool, boxname, SidString.Buffer, SessionId, TRUE);

        RtlFreeUnicodeString(&SidString);

        if (! box)
            return STATUS_INSUFFICIENT_RESOURCES;

        if (! Conf_IsBoxEnabled(boxname, box->sid, box->session_id)) {
            Box_Free(box);
            return STATUS_ACCOUNT_RESTRICTION;
        }
    }

    //
    // second parameter is the process id for the new process
    //

    user_pid_parm = (HANDLE)parms[2];

    if (! user_pid_parm)
        status = STATUS_INVALID_CID;
    else
        status = PsLookupProcessByProcessId(user_pid_parm, &ProcessObject);

    if (NT_SUCCESS(status)) {

        if (PsGetProcessSessionId(ProcessObject) != box->session_id) {

            status = STATUS_LOGON_SESSION_COLLISION;

        } else {

            if (!Process_NotifyProcess_Create(
                                user_pid_parm, Api_ServiceProcessId, Api_ServiceProcessId, box)) {

                status = STATUS_INTERNAL_ERROR;
            }

            box = NULL;         // freed by Process_NotifyProcess_Create
        }

        ObDereferenceObject(ProcessObject);
    }

    if (box)
        Box_Free(box);

    return status;
}


//---------------------------------------------------------------------------
// Process_Api_Query
//---------------------------------------------------------------------------


_FX NTSTATUS Process_Api_Query(PROCESS *proc, ULONG64 *parms)
{
    API_QUERY_PROCESS_ARGS *args = (API_QUERY_PROCESS_ARGS *)parms;
    NTSTATUS status;
    HANDLE ProcessId;
    ULONG *num32;
    ULONG64 *num64;
    KIRQL irql;

    //
    // this is the first SbieApi call by SbieDll
    //

    if (proc)
        proc->sbiedll_loaded = TRUE;

    //
    // if a ProcessId was specified, then locate and lock the matching
    // process. ProcessId must be specified if the caller is not sandboxed
    //

    ProcessId = args->process_id.val;
    if (proc) {
        if (ProcessId == proc->pid || IS_ARG_CURRENT_PROCESS(ProcessId))
            ProcessId = 0;  // don't have to search for the current pid
    } else {
        if ((! ProcessId) || IS_ARG_CURRENT_PROCESS(ProcessId))
            return STATUS_INVALID_CID;
    }
    if (ProcessId) {

        proc = Process_Find(ProcessId, &irql);
        if (! proc) {
            ExReleaseResourceLite(Process_ListLock);
            KeLowerIrql(irql);
            return STATUS_INVALID_CID;
        }
    }

    //
    // the rest of the code now has to be protected, because we may
    // have a lock on the Process_List, that we must release
    //

    status = STATUS_SUCCESS;

    __try {

        // boxname unicode string can be specified in parameter 2

        Api_CopyStringToUser(
            (UNICODE_STRING64 *)parms[2],
            proc->box->name, proc->box->name_len);

        // image name unicode string can be specified in parameter 3

        Api_CopyStringToUser(
            (UNICODE_STRING64 *)parms[3],
            proc->image_name, proc->image_name_len);

        // sid unicode string can be specified in parameter 4

        Api_CopyStringToUser(
            (UNICODE_STRING64 *)parms[4],
            proc->box->sid, proc->box->sid_len);

        // session_id number can be specified in parameter 5

        num32 = (ULONG *)parms[5];
        if (num32) {
            ProbeForWrite(num32, sizeof(ULONG), sizeof(ULONG));
            *num32 = proc->box->session_id;
        }

        // create_time timestamp can be specified in parameter 6

        num64 = (ULONG64 *)parms[6];
        if (num64) {
            ProbeForWrite(num64, sizeof(ULONG64), sizeof(ULONG));
            *num64 = proc->create_time;
        }

    //
    // release the lock, if taken
    //

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    if (ProcessId) {

        ExReleaseResourceLite(Process_ListLock);
        KeLowerIrql(irql);
    }

    return status;
}


//---------------------------------------------------------------------------
// Process_Api_QueryInfo
//---------------------------------------------------------------------------


_FX NTSTATUS Process_Api_QueryInfo(PROCESS *proc, ULONG64 *parms)
{
    API_QUERY_PROCESS_INFO_ARGS *args = (API_QUERY_PROCESS_INFO_ARGS *)parms;
    NTSTATUS status;
    HANDLE ProcessId;
    KIRQL irql;
	BOOLEAN is_caller_sandboxed = FALSE;

    //
    // if a ProcessId was specified, then locate and lock the matching
    // process. ProcessId must be specified if the caller is not sandboxed
    //

    ProcessId = args->process_id.val;
    if (proc) {
		is_caller_sandboxed = TRUE;
        if (ProcessId == proc->pid || IS_ARG_CURRENT_PROCESS(ProcessId))
            ProcessId = 0;  // don't have to search for the current pid
    } else {
        if ((! ProcessId) || IS_ARG_CURRENT_PROCESS(ProcessId))
            return STATUS_INVALID_CID;
    }
    if (ProcessId) {

        proc = Process_Find(ProcessId, &irql);
        if (! proc) {
            ExReleaseResourceLite(Process_ListLock);
            KeLowerIrql(irql);
            return STATUS_INVALID_CID;
        }
    }

    // the rest of the code now has to be protected, because we may
    // have a lock on the Process_List, that we must release

    status = STATUS_SUCCESS;

    __try {

        ULONG64 *data = args->info_data.val;
        ProbeForWrite(data, sizeof(ULONG64), sizeof(ULONG64));

        if (args->info_type.val == 0) {

            ULONG64 flags = 0;

            if (!proc->bHostInject)
            {
                flags = SBIE_FLAG_VALID_PROCESS;

                if (proc->forced_process)
                    flags |= SBIE_FLAG_FORCED_PROCESS;
                if (proc->is_start_exe)
                    flags |= SBIE_FLAG_PROCESS_IS_START_EXE;
                if (proc->parent_was_start_exe)
                    flags |= SBIE_FLAG_PARENT_WAS_START_EXE;
                if (proc->drop_rights)
                    flags |= SBIE_FLAG_DROP_RIGHTS;
                if (proc->rights_dropped)
                    flags |= SBIE_FLAG_RIGHTS_DROPPED;
                if (proc->untouchable)
                    flags |= SBIE_FLAG_PROTECTED_PROCESS;
                if (proc->image_sbie)
                    flags |= SBIE_FLAG_IMAGE_FROM_SBIE_DIR;
                if (proc->image_from_box)
                    flags |= SBIE_FLAG_IMAGE_FROM_SANDBOX;
                if (proc->in_pca_job)
                    flags |= SBIE_FLAG_PROCESS_IN_PCA_JOB;

                if (proc->create_console_flag == 'S')
                    flags |= SBIE_FLAG_CREATE_CONSOLE_SHOW;
                else if (proc->create_console_flag == 'H')
                    flags |= SBIE_FLAG_CREATE_CONSOLE_HIDE;

                if (proc->open_all_win_classes)
                    flags |= SBIE_FLAG_OPEN_ALL_WIN_CLASS;
                extern ULONG Syscall_MaxIndex32;
                if (Syscall_MaxIndex32 != 0)
                    flags |= SBIE_FLAG_WIN32K_HOOKABLE;

                if (proc->use_rule_specificity)
                    flags |= SBIE_FLAG_RULE_SPECIFICITY;
                if (proc->use_privacy_mode)
                    flags |= SBIE_FLAG_PRIVACY_MODE;
                if (proc->bAppCompartment)
                    flags |= SBIE_FLAG_APP_COMPARTMENT;
            }
            else
            {
                flags = SBIE_FLAG_HOST_INJECT_PROCESS;
            }

            *data = flags;

        } else if (args->info_type.val == 'pril') {

            *data = proc->integrity_level;

        } else if (args->info_type.val == 'nt32') {

            *data = proc->ntdll32_base;

        } else if (args->info_type.val == 'ptok') { // primary token

			if(is_caller_sandboxed || !Session_CheckAdminAccess(TRUE))
				status = STATUS_ACCESS_DENIED;
			else
			{
				void *PrimaryTokenObject = proc->primary_token;
				if (PrimaryTokenObject)
				{
					ObReferenceObject(PrimaryTokenObject);

                    //ACCESS_MASK access = (PsGetCurrentProcessId() != Api_ServiceProcessId) ? TOKEN_ALL_ACCESS : (TOKEN_QUERY | TOKEN_DUPLICATE);

					HANDLE MyTokenHandle;
					status = ObOpenObjectByPointer(PrimaryTokenObject, 0, NULL, TOKEN_QUERY | TOKEN_DUPLICATE, *SeTokenObjectType, UserMode, &MyTokenHandle);

					ObDereferenceObject(PrimaryTokenObject);

					*data = (ULONG64)MyTokenHandle;
				}
				else
					status = STATUS_NOT_FOUND;
			}

		} else if (args->info_type.val == 'itok' || args->info_type.val == 'ttok') { // impersonation token / test thread token

			if(is_caller_sandboxed || (args->info_type.val == 'itok' && !Session_CheckAdminAccess(TRUE)))
				status = STATUS_ACCESS_DENIED;
			else
			{
                HANDLE tid = (HANDLE)(args->ext_data.val);

                THREAD *thrd = Thread_GetByThreadId(proc, tid);
				if (thrd)
				{
                    if (args->info_type.val == 'ttok')
                    {
                        *data = thrd->token_object ? TRUE : FALSE;
                    }
                    else
                    {
                        KIRQL irql2;
                        void* ImpersonationTokenObject;

                        KeRaiseIrql(APC_LEVEL, &irql2);
                        ExAcquireResourceExclusiveLite(proc->threads_lock, TRUE);

                        ImpersonationTokenObject = thrd->token_object;

                        if (ImpersonationTokenObject) {
                            ObReferenceObject(ImpersonationTokenObject);
                        }

                        ExReleaseResourceLite(proc->threads_lock);
                        KeLowerIrql(irql2);

                        if (ImpersonationTokenObject)
                        {
                            HANDLE MyTokenHandle;
                            status = ObOpenObjectByPointer(ImpersonationTokenObject, 0, NULL, TOKEN_QUERY | TOKEN_DUPLICATE, *SeTokenObjectType, UserMode, &MyTokenHandle);

                            ObDereferenceObject(ImpersonationTokenObject);

                            *data = (ULONG64)MyTokenHandle;
                        }
                        else
                            status = STATUS_NO_IMPERSONATION_TOKEN;
                    }
				}
				else
					status = STATUS_NOT_FOUND;
			}

		} else if (args->info_type.val == 'ippt') { // is primary process token

            HANDLE handle = (HANDLE)(args->ext_data.val);

            OBJECT_TYPE* object;
            status = ObReferenceObjectByHandle(handle, 0, NULL, UserMode, &object, NULL);
            if (NT_SUCCESS(status))
            {
                *data = (object == proc->primary_token);

                ObDereferenceObject(object);
            }

        } else if (args->info_type.val == 'spit') { // set process image type

            if (ProcessId != 0)
                status = STATUS_ACCESS_DENIED;
            
            if(proc->detected_image_type == -1)
                proc->detected_image_type = (ULONG)(args->ext_data.val);

            *data = 0;

        } else if (args->info_type.val == 'gpit') { // get process image type
            
            *data = proc->detected_image_type;

        } else
            status = STATUS_INVALID_INFO_CLASS;

    //
    // release the lock, if taken
    //

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    if (ProcessId) {

        ExReleaseResourceLite(Process_ListLock);
        KeLowerIrql(irql);
    }

    return status;
}


//---------------------------------------------------------------------------
// Process_Api_QueryBoxPath
//---------------------------------------------------------------------------


_FX NTSTATUS Process_Api_QueryBoxPath(PROCESS *proc, ULONG64 *parms)
{
    API_QUERY_BOX_PATH_ARGS *args = (API_QUERY_BOX_PATH_ARGS *)parms;
    BOX *box;
    BOOLEAN free_box;
    NTSTATUS status;

    //
    // if this API is invoked by a sandboxed process, use its box.
    // otherwise, construct a box according to specified box name
    //

    if (proc) {

        box = proc->box;
        free_box = FALSE;

    } else {

        WCHAR boxname[34];
        BOOLEAN ok = Api_CopyBoxNameFromUser(
            boxname, (WCHAR *)args->box_name.val);
        if (! ok)
            return STATUS_INVALID_PARAMETER;

        box = Box_Create(Driver_Pool, boxname, TRUE);
        if (! box)
            return STATUS_UNSUCCESSFUL;
        free_box = TRUE;
    }

    status = Process_Api_CopyBoxPathsToUser(
                box,
                args->file_path_len.val,
                args->key_path_len.val,
                args->ipc_path_len.val,
                args->file_path.val,
                args->key_path.val,
                args->ipc_path.val);

    if (free_box)
        Box_Free(box);

    return status;
}


//---------------------------------------------------------------------------
// Process_Api_QueryProcessPath
//---------------------------------------------------------------------------


_FX NTSTATUS Process_Api_QueryProcessPath(PROCESS *proc, ULONG64 *parms)
{
    API_QUERY_PROCESS_PATH_ARGS *args = (API_QUERY_PROCESS_PATH_ARGS *)parms;
    HANDLE ProcessId;
    NTSTATUS status;
    KIRQL irql;

    //
    // if a ProcessId was specified, then locate and lock the matching
    // process. ProcessId must be specified if the caller is not sandboxed
    //

    ProcessId = args->process_id.val;
    if (proc) {
        if (ProcessId == proc->pid || IS_ARG_CURRENT_PROCESS(ProcessId))
            ProcessId = 0;  // don't have to search for the current pid
    } else {
        if ((! ProcessId) || IS_ARG_CURRENT_PROCESS(ProcessId))
            return STATUS_INVALID_CID;
    }
    if (ProcessId) {

        proc = Process_Find(ProcessId, &irql);
        if ((! proc) || proc->terminated) {
            ExReleaseResourceLite(Process_ListLock);
            KeLowerIrql(irql);
            return STATUS_INVALID_CID;
        }
    }

    status = Process_Api_CopyBoxPathsToUser(
                proc->box,
                args->file_path_len.val,
                args->key_path_len.val,
                args->ipc_path_len.val,
                args->file_path.val,
                args->key_path.val,
                args->ipc_path.val);

    if (ProcessId) {

        ExReleaseResourceLite(Process_ListLock);
        KeLowerIrql(irql);
    }

    return status;
}


//---------------------------------------------------------------------------
// Process_Api_CopyBoxPathsToUser
//---------------------------------------------------------------------------


_FX NTSTATUS Process_Api_CopyBoxPathsToUser(
    BOX *box, ULONG *file_path_len, ULONG *key_path_len, ULONG *ipc_path_len,
    UNICODE_STRING64 *file_path,
    UNICODE_STRING64 *key_path,
    UNICODE_STRING64 *ipc_path)
{
    NTSTATUS status;

    __try {

    //
    // store the result lengths, if parameters were given
    //

    if (file_path_len) {
        ProbeForWrite(file_path_len, sizeof(ULONG), sizeof(ULONG));
        *file_path_len = box->file_path_len;
    }

    if (key_path_len) {
        ProbeForWrite(key_path_len, sizeof(ULONG), sizeof(ULONG));
        *key_path_len = box->key_path_len;
    }

    if (ipc_path_len) {
        ProbeForWrite(ipc_path_len, sizeof(ULONG), sizeof(ULONG));
        *ipc_path_len = box->ipc_path_len;
    }

    //
    // copy the strings, if specified
    //

    Api_CopyStringToUser(file_path, box->file_path, box->file_path_len);
    Api_CopyStringToUser(key_path, box->key_path, box->key_path_len);
    Api_CopyStringToUser(ipc_path, box->ipc_path, box->ipc_path_len);

    status = STATUS_SUCCESS;

    //
    // finish
    //

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    return status;
}


//---------------------------------------------------------------------------
// Process_Api_QueryPathList
//---------------------------------------------------------------------------


_FX NTSTATUS Process_Api_QueryPathList(PROCESS *proc, ULONG64 *parms)
{
    API_QUERY_PATH_LIST_ARGS *args = (API_QUERY_PATH_LIST_ARGS *)parms;
    PERESOURCE lock;
    LIST *list;
    WCHAR *path;
    PATTERN *pat;
    NTSTATUS status;
    ULONG path_len;
    KIRQL irql;
    BOOLEAN process_list_locked;
    BOOLEAN prepend_level;

    //
    // caller can either be a sandboxed process asking its own path list,
    // or the Sandboxie Service asking the path list of a sandboxed process
    //

    if (proc) {

        process_list_locked = FALSE;

    } else {

        //if (! MyIsCurrentProcessRunningAsLocalSystem())
        //    return STATUS_NOT_IMPLEMENTED;

        proc = Process_Find(args->process_id.val, &irql);

        if (! proc) {

            ExReleaseResourceLite(Process_ListLock);
            KeLowerIrql(irql);
            return STATUS_INVALID_CID;
        }

        process_list_locked = TRUE;
    }

    //
    // select path list based on the parameter given
    //

#ifdef USE_MATCH_PATH_EX
    if (args->path_code.val == 'fn') {
        list   = &proc->normal_file_paths;
        lock   =  proc->file_lock;
    } else 
#endif
    if (args->path_code.val == 'fo') {
        list   = &proc->open_file_paths;
        lock   =  proc->file_lock;
    } else if (args->path_code.val == 'fc') {
        list   = &proc->closed_file_paths;
        lock   =  proc->file_lock;
    } else if (args->path_code.val == 'fr') {
        list   = &proc->read_file_paths;
        lock   =  proc->file_lock;
    } else if (args->path_code.val == 'fw') {
        list   = &proc->write_file_paths;
        lock   =  proc->file_lock;

#ifdef USE_MATCH_PATH_EX
    } else  if (args->path_code.val == 'kn') {
        list   = &proc->normal_key_paths;
        lock   =  proc->key_lock;
#endif
    } else if (args->path_code.val == 'ko') {
        list   = &proc->open_key_paths;
        lock   =  proc->key_lock;
    } else if (args->path_code.val == 'kc') {
        list   = &proc->closed_key_paths;
        lock   =  proc->key_lock;
    } else if (args->path_code.val == 'kr') {
        list   = &proc->read_key_paths;
        lock   =  proc->key_lock;
    } else if (args->path_code.val == 'kw') {
        list   = &proc->write_key_paths;
        lock   =  proc->key_lock;

#ifdef USE_MATCH_PATH_EX
    } else  if (args->path_code.val == 'in') {
        list   = &proc->normal_ipc_paths;
        lock   =  proc->ipc_lock;
#endif
    } else if (args->path_code.val == 'io') {
        list   = &proc->open_ipc_paths;
        lock   =  proc->ipc_lock;
    } else if (args->path_code.val == 'ic') {
        list   = &proc->closed_ipc_paths;
        lock   =  proc->ipc_lock;

    } else if (args->path_code.val == 'wo') {
        list   = &proc->open_win_classes;
        lock   =  proc->gui_lock;

    } else {

        if (process_list_locked) {
            ExReleaseResourceLite(Process_ListLock);
            KeLowerIrql(irql);
        }

        return STATUS_INVALID_PARAMETER;
    }

    //
    // take a lock on the specified path list.  note that if we called
    // Process_Find above, then IRQL was already raised
    //

    if (! process_list_locked)
        KeRaiseIrql(APC_LEVEL, &irql);

    ExAcquireResourceSharedLite(lock, TRUE);

    prepend_level = args->prepend_level.val;

    //
    // path format: ([level 4])[wchar 2*n][0x0000]
    // level is optional
    //

    //
    // count the length of the desired path list
    //

    path_len = 0;

    pat = List_Head(list);
    while (pat) {
        if (prepend_level) path_len += sizeof(ULONG);
        path_len += (wcslen(Pattern_Source(pat)) + 1) * sizeof(WCHAR);
        pat = List_Next(pat);
    }

    if (prepend_level) path_len += sizeof(ULONG);
    path_len += sizeof(WCHAR);

    //
    // copy data to caller
    //

    __try {
        
        if(args->path_str.val) {

            //
            // if a output buffer was specified store the paths into it
            //

            if (args->path_len.val && *args->path_len.val < path_len) {

                status = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }
            
            path = args->path_str.val;
            ProbeForWrite(path, path_len, sizeof(WCHAR));

            pat = List_Head(list);
            while (pat) {
                if (prepend_level) {
                    *((ULONG*)path) = Pattern_Level(pat);
                    path += sizeof(ULONG)/sizeof(WCHAR);
                }
                const WCHAR *pat_src = Pattern_Source(pat);
                ULONG pat_len = wcslen(pat_src) + 1;
                wmemcpy(path, pat_src, pat_len);
                path += pat_len;
                pat = List_Next(pat);
            }

            if (prepend_level){
                *((ULONG*)path) = -1;
                path += sizeof(ULONG)/sizeof(WCHAR);
            }
            *path = L'\0';
        }

        if (args->path_len.val) {

            //
            // if the length parameter was specified, store length
            //

            ProbeForWrite(args->path_len.val, sizeof(ULONG), sizeof(ULONG));
            *args->path_len.val = path_len;

        } 

        status = STATUS_SUCCESS;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    //
    // release locks and finish
    //

    ExReleaseResourceLite(lock);

    if (process_list_locked)
        ExReleaseResourceLite(Process_ListLock);

    KeLowerIrql(irql);

    return status;
}


//---------------------------------------------------------------------------
// Process_Enumerate
//---------------------------------------------------------------------------


_FX NTSTATUS Process_Enumerate(
    const WCHAR *boxname, BOOLEAN all_sessions, ULONG session_id,
    ULONG *pids, ULONG *count)
{
    NTSTATUS status;
    PROCESS *proc1;
    ULONG num;
    KIRQL irql;

    if (count == NULL)
        return STATUS_INVALID_PARAMETER;

    //
    // return only processes of the caller user in their logon session
    //

    if ((! all_sessions) && (session_id == -1)) {

        status = MyGetSessionId(&session_id);
        if (! NT_SUCCESS(status))
            return status;
    }

    KeRaiseIrql(APC_LEVEL, &irql);
    ExAcquireResourceSharedLite(Process_ListLock, TRUE);

    __try {

        num = 0;

#ifdef USE_PROCESS_MAP

        //
        // quick shortcut for global count retrival
        //

        if (pids == NULL && (! boxname[0]) && all_sessions) { // no pids, all boxes, all sessions

            num = Process_Map.nnodes;
            goto done;
        }

	    map_iter_t iter = map_iter();
	    while (map_next(&Process_Map, &iter)) {
            proc1 = iter.value;
#else
        proc1 = List_Head(&Process_List);
        while (proc1) {
#endif
            BOX *box1 = proc1->box;
            if (box1 && !proc1->bHostInject) {
                BOOLEAN same_box =
                    (! boxname[0]) || (_wcsicmp(box1->name, boxname) == 0);
                BOOLEAN same_session =
                    (all_sessions || box1->session_id == session_id);
                if (same_box && same_session) {
                    if (pids) {
						if(num >= *count)
							break;
                        pids[num] = (ULONG)(ULONG_PTR)proc1->pid;
                    }
                    ++num;
                }
            }

#ifndef USE_PROCESS_MAP
            proc1 = (PROCESS *)List_Next(proc1);
#endif
        }

#ifdef USE_PROCESS_MAP
        done:
#endif
        *count = num;

        status = STATUS_SUCCESS;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    ExReleaseResourceLite(Process_ListLock);
    KeLowerIrql(irql);

    return status;
}


//---------------------------------------------------------------------------
// Process_Api_Enum
//---------------------------------------------------------------------------


_FX NTSTATUS Process_Api_Enum(PROCESS *proc, ULONG64 *parms)
{
    NTSTATUS status;
    ULONG count;
    ULONG *user_pids;                   // user mode ULONG [512]
    WCHAR *user_boxname;                // user mode WCHAR [34]
    BOOLEAN all_sessions;
    ULONG session_id;
    WCHAR boxname[48];
    ULONG *user_count;

    // get boxname from second parameter

    memzero(boxname, sizeof(boxname));
    if (proc)
        wcscpy(boxname, proc->box->name);
    user_boxname = (WCHAR *)parms[2];
    if ((! boxname[0]) && user_boxname) {
        ProbeForRead(user_boxname, sizeof(WCHAR) * 32, sizeof(UCHAR));
        if (user_boxname[0])
            wcsncpy(boxname, user_boxname, 32);
    }

    // get "all users/current user only" flag from third parameter

    all_sessions = FALSE;
    if (parms[3])
        all_sessions = TRUE;

    session_id = (ULONG)parms[4];

    // get user pid buffer from first parameter

    user_count = (ULONG *)parms[5];
    user_pids = (ULONG *)parms[1];
    
    if (user_count) {
        ProbeForRead(user_count, sizeof(ULONG), sizeof(ULONG));
        count = user_pids ? *user_count : 0;
    }
    else // legacy case
    {
        if (!user_pids)
            return STATUS_INVALID_PARAMETER;
        count = API_MAX_PIDS - 1;
        user_count = user_pids;
        user_pids += 1;
    }

    ProbeForWrite(user_count, sizeof(ULONG), sizeof(ULONG));
    if (user_pids) {
        ProbeForWrite(user_pids, sizeof(ULONG) * count, sizeof(ULONG));
    }

    status = Process_Enumerate(boxname, all_sessions, session_id,
                               user_pids, &count);
    if (! NT_SUCCESS(status))
        return status;

    *user_count = count;

    return status;
}




PROCESS_MONITOR_LIST* g_process_monitor_list = 0;
WATERMARK_BOX_LIST* g_watermark_box_list = 0;
SCREENSHOT_BOX_LIST* g_screenshot_box_list = 0;
FILEEXPORT_BOX_LIST* g_fileexport_box_list = 0;
PRINTER_BOX_LIST* g_printer_box_list = 0;



NTSTATUS Process_Api_ResetList(PROCESS_MONITOR_LIST * proclist) {
	PROCESS_MONITOR_LIST* list = proclist;
	do
	{
		PROCESS_MONITOR_LIST* next = list->next;

		ExFreePoolWithTag((PVOID)list, 'kcuf');

		list = next;

	} while (list && list != proclist);

	return STATUS_SUCCESS;
}

NTSTATUS Process_Api_ResetAll(PROCESS* proc, ULONG64* parms) {

	Process_Api_ResetList(g_process_monitor_list);
	g_process_monitor_list = 0;

	Process_Api_ResetList((PROCESS_MONITOR_LIST *)g_watermark_box_list);
	g_watermark_box_list = 0;

	Process_Api_ResetList((PROCESS_MONITOR_LIST *)g_screenshot_box_list);
	g_screenshot_box_list = 0;

	Process_Api_ResetList((PROCESS_MONITOR_LIST *)g_fileexport_box_list);
	g_fileexport_box_list = 0;

	Process_Api_ResetList((PROCESS_MONITOR_LIST *)g_printer_box_list);
	g_printer_box_list = 0;

	return STATUS_SUCCESS;
}







PROCESS_MONITOR_LIST* getProhibitProcess(const WCHAR* str) {
	WCHAR* name = wcsrchr(str, '\\');
	if (name)
	{
		name++;
	}
	else {
		name = (WCHAR*)str;
	}

	PROCESS_MONITOR_LIST* list = g_process_monitor_list;
	do 
	{
		if (_wcsicmp(name, list->processname) == 0)
		{
			return list;
		}
		list = list->next;

	} while (list && list != g_process_monitor_list);

	return 0;
}


NTSTATUS Process_Api_SetProcessMonitor(PROCESS* proc, ULONG64* parms) {
// 	WCHAR* processname = (WCHAR*)parms[2];
// 	WCHAR * boxname = (WCHAR*)parms[1];
	API_SET_PROCESS_MONITOR_ARGS* args = (API_SET_PROCESS_MONITOR_ARGS*)parms;
	WCHAR* processname = (WCHAR*)args->processname.val;
	WCHAR * boxname = (WCHAR*)args->boxname.val;

	if (g_process_monitor_list == 0)
	{
		g_process_monitor_list = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_MONITOR_LIST), 'kcuf');
		if (g_process_monitor_list)
		{
			g_process_monitor_list->next = 0;
			g_process_monitor_list->prev = 0;
			wcsncpy(g_process_monitor_list->processname, processname, sizeof(g_process_monitor_list->processname)/2);
			wcsncpy(g_process_monitor_list->boxname, boxname, sizeof(g_process_monitor_list->boxname)/2);
		}
	}
	else {
		PROCESS_MONITOR_LIST* list = getProhibitProcess(processname);
		if (list == 0)
		{
			list = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_MONITOR_LIST), 'kcuf');
			if (list)
			{
				wcsncpy(list->processname, processname, sizeof(list->processname)/2);
				wcsncpy(list->boxname, boxname, sizeof(list->boxname)/2);

				PROCESS_MONITOR_LIST* next = g_process_monitor_list->next;
				list->next = next;
				list->prev = g_process_monitor_list;
				g_process_monitor_list->next = list;
				if (next)
				{
					next->prev = list;
				}
			}
		}
	}

	return TRUE;
}



PRINTER_BOX_LIST* getBoxInList(PRINTER_BOX_LIST * hdrlist,const WCHAR* boxname) {

	PRINTER_BOX_LIST * list = hdrlist;
	if (list == 0)
	{
		return 0;
	}

	do
	{
		if (_wcsicmp(boxname, list->boxname) == 0)
		{
			return list;
		}

		list = list->next;

	} while (list && list != hdrlist);

	return 0;
}


NTSTATUS Process_Api_SetBoxListItem(PRINTER_BOX_LIST ** list,WCHAR * boxname, int enable) {

	if (*list == 0)
	{
		(*list) = ExAllocatePoolWithTag(NonPagedPool, sizeof(PRINTER_BOX_LIST), 'kcuf');
		if (*list)
		{
			(*list)->next = 0;
			(*list)->prev = 0;
			(*list)->enable = enable;
			wcsncpy((*list)->boxname, boxname, sizeof((*list)->boxname));
		}
	}
	else {
		PRINTER_BOX_LIST * newlist = getBoxInList(*list,boxname);
		if (newlist == 0)
		{
			newlist = ExAllocatePoolWithTag(NonPagedPool, sizeof(PRINTER_BOX_LIST), 'kcuf');
			if (newlist)
			{
				wcsncpy(newlist->boxname, boxname, 64);

				PRINTER_BOX_LIST* next = (*list)->next;
				newlist->next = next;
				newlist->prev = (*list);
				(*list)->next = newlist;
				if (next)
				{
					next->prev = newlist;
				}
			}
		}
	}

	return STATUS_SUCCESS;
}





NTSTATUS Process_Api_QueryFileExport(PROCESS* proc, ULONG64* parms) {
	//__debugbreak();
	API_SET_FILEEXPORT_ARGS* args = (API_SET_FILEEXPORT_ARGS*)parms;

	PRINTER_BOX_LIST * list = getBoxInList((PRINTER_BOX_LIST *)g_fileexport_box_list, proc->box->name);
	if (list)
	{
		*(DWORD*)args->enable.val = list->enable;
	}
	else {
		*(DWORD*)args->enable.val = FALSE;
	}

	//DbgPrint("Process_Api_QueryPrinter is:%x\r\n", g_watermarkEnable);
	return STATUS_SUCCESS;
}

NTSTATUS Process_Api_SetFileExport(PROCESS* proc, ULONG64* parms) {
	//__debugbreak();
	API_SET_FILEEXPORT_ARGS* args = (API_SET_FILEEXPORT_ARGS*)parms;
	DWORD enable = (DWORD)args->enable.val;

	WCHAR * boxname = args->boxname.val;

	Process_Api_SetBoxListItem((PRINTER_BOX_LIST **)&g_fileexport_box_list, boxname, enable);

	//DbgPrint("Process_Api_SetPrinter is:%x\r\n", g_watermarkEnable);
	return STATUS_SUCCESS;
}


NTSTATUS Process_Api_QueryPrinter(PROCESS* proc, ULONG64* parms) {
	//__debugbreak();
	API_SET_PRINTER_ARGS* args = (API_SET_PRINTER_ARGS*)parms;

	PRINTER_BOX_LIST * list = getBoxInList(g_printer_box_list, proc->box->name);
	if (list)
	{
		*(DWORD*)args->enable.val = list->enable;
	}
	else {
		*(DWORD*)args->enable.val = FALSE;
	}

	//DbgPrint("Process_Api_QueryPrinter is:%x\r\n", g_watermarkEnable);
	return STATUS_SUCCESS;
}

NTSTATUS Process_Api_SetPrinter(PROCESS* proc, ULONG64* parms) {
	//__debugbreak();
	API_SET_PRINTER_ARGS* args = (API_SET_PRINTER_ARGS*)parms;
	DWORD enable = (DWORD)args->enable.val;

	WCHAR * boxname = args->boxname.val;
	Process_Api_SetBoxListItem(&g_printer_box_list, boxname, enable);

	//DbgPrint("Process_Api_SetPrinter is:%x\r\n", g_watermarkEnable);
	return STATUS_SUCCESS;
}


NTSTATUS Process_Api_QueryWatermark(PROCESS* proc, ULONG64* parms) {
	//__debugbreak();
	API_SET_WATERMARK_ARGS* args = (API_SET_WATERMARK_ARGS*)parms;
	PRINTER_BOX_LIST * list = getBoxInList((PRINTER_BOX_LIST *)g_watermark_box_list, proc->box->name);
	if (list)
	{
		*(DWORD*)args->enable.val = list->enable;
	}
	else {
		*(DWORD*)args->enable.val = FALSE;
	}

	//DbgPrint("Process_Api_QueryWatermark is:%x\r\n", g_watermarkEnable);
	return STATUS_SUCCESS;
}

NTSTATUS Process_Api_SetWatermark(PROCESS* proc, ULONG64* parms) {
	//__debugbreak();
	API_SET_WATERMARK_ARGS* args = (API_SET_WATERMARK_ARGS*)parms;
	DWORD enable = (DWORD)args->enable.val;

	WCHAR * boxname = args->boxname.val;
	Process_Api_SetBoxListItem((PRINTER_BOX_LIST **)&g_watermark_box_list, boxname, enable);

	//DbgPrint("Process_Api_SetWatermark is:%x\r\n", g_watermarkEnable);
	return STATUS_SUCCESS;
}


NTSTATUS Process_Api_QueryScreenshot(PROCESS* proc, ULONG64* parms) {
	//__debugbreak();
	API_SET_SCREENSHOT_ARGS* args = (API_SET_SCREENSHOT_ARGS*)parms;
	PRINTER_BOX_LIST * list = getBoxInList((PRINTER_BOX_LIST *)g_screenshot_box_list, proc->box->name);
	if (list)
	{
		*(DWORD*)args->enable.val = list->enable;
	}
	else {
		*(DWORD*)args->enable.val = FALSE;
	}

	//DbgPrint("Process_Api_QueryScreenshot is:%x\r\n", g_screenshotEnable);
	return STATUS_SUCCESS;
}

NTSTATUS Process_Api_SetScreenshot(PROCESS* proc, ULONG64* parms) {
	//__debugbreak();
	API_SET_SCREENSHOT_ARGS* args = (API_SET_SCREENSHOT_ARGS*)parms;
	DWORD enable = (DWORD)args->enable.val;

	WCHAR * boxname = args->boxname.val;
	Process_Api_SetBoxListItem((PRINTER_BOX_LIST **)&g_screenshot_box_list, boxname, enable);

	//DbgPrint("Process_Api_SetScreenshot is:%x\r\n", g_screenshotEnable);
	return STATUS_SUCCESS;
}



#define VERACRYPT_VOLUME_DEVICE			L"\\Device\\VeraCryptVolume"

#define HARDDISK_VOLUME_DEVICE			L"\\Device\\HarddiskVolume"

int createPathRecursive(const WCHAR* dstpath) {

	//__debugbreak();

	OBJECT_ATTRIBUTES next_oa;
	UNICODE_STRING next_name;
	InitializeObjectAttributes(&next_oa, &next_name, OBJ_CASE_INSENSITIVE, NULL, 0);

	NTSTATUS status;

	IO_STATUS_BLOCK iosb;

	HANDLE hdir;

	WCHAR path[1024];

	WCHAR dstfn[1024];

	wcscpy(dstfn, dstpath);
	WCHAR* p = wcsrchr(dstfn, L'\\');
	if (p)
	{
		*p = 0;
	}

	WCHAR* hdr = wcsstr(dstfn, HARDDISK_VOLUME_DEVICE);
	if (hdr)
	{
		hdr = hdr + wcslen(HARDDISK_VOLUME_DEVICE) + 2;
	}
	else {
		hdr = wcsstr(dstfn, VERACRYPT_VOLUME_DEVICE);
		if (hdr)
		{
			hdr = hdr + wcslen(VERACRYPT_VOLUME_DEVICE) + 2;
		}
		else {
			return FALSE;
		}
	}

	int endflag = 0;

	while (1)
	{
		WCHAR* pos = wcschr(hdr, '\\');
		if (pos)
		{
			pos++;

			int len = (int)(pos - dstfn);
			wmemcpy(path, dstfn, len);
			path[len] = 0;

			hdr = pos;
		}
		else {
			wcscpy(path, dstfn);
			endflag = TRUE;
		}

		RtlInitUnicodeString(&next_name, path);
		status = NtCreateFile(&hdir, FILE_GENERIC_READ, &next_oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
			FILE_OPEN, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		if (!NT_SUCCESS(status))
		{
			//STATUS_OBJECT_PATH_NOT_FOUND 0xc000003a
			status = NtCreateFile(&hdir, FILE_GENERIC_WRITE, &next_oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
				FILE_OPEN_IF, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
			if (!NT_SUCCESS(status))
			{
				DbgPrint( "createPathRecursive create directory:%ws error code:%x",path, status);

				return FALSE;
			}
		}
		else {
			NtClose(hdir);
		}

		if (endflag)
		{
			return TRUE;
		}
	}

	return FALSE;
}


NTSTATUS Veracrypt_Api_CopyFile(PROCESS* proc, ULONG64* parms) {

	API_VERACRYPT_COPYFILE_ARGS* args = (API_VERACRYPT_COPYFILE_ARGS*)parms;

	WCHAR* srcfn = (WCHAR*)(args->src.val64);
	WCHAR* dstfn = (WCHAR*)(args->dst.val64);

	NTSTATUS status;
	HANDLE hfsrc, hfdst;
	OBJECT_ATTRIBUTES objattrs = { 0 };
	UNICODE_STRING objname = { 0 };
	IO_STATUS_BLOCK IoStatusBlock;

	char buffer[PAGE_SIZE];

	unsigned __int64 filesize;

	FILE_NETWORK_OPEN_INFORMATION fnetwork_openinfo;

	InitializeObjectAttributes(&objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, 0);

	RtlInitUnicodeString(&objname, dstfn);
	status = NtCreateFile(&hfdst, FILE_GENERIC_WRITE, &objattrs, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Veracrypt_Api_CopyFile  NtCreateFile:%ws error code:%x", dstfn,status);

		status = createPathRecursive(dstfn);

		RtlInitUnicodeString(&objname, dstfn);
		status = NtCreateFile(&hfdst, FILE_GENERIC_WRITE, &objattrs, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("Veracrypt_Api_CopyFile  NtCreateFile:%ws second error code:%x", dstfn,status);
			return status;
		}	
	}

	RtlInitUnicodeString(&objname, srcfn);
	status = NtCreateFile(&hfsrc, FILE_GENERIC_READ | FILE_READ_ATTRIBUTES, &objattrs, &IoStatusBlock,
		NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (!NT_SUCCESS(status))
	{
		NtClose(hfdst);
		DbgPrint( "copyfile  __sys_NtCreateFile src file error,result:%x",  status);
		return status;
	}

	status = NtQueryInformationFile(hfsrc, &IoStatusBlock, &fnetwork_openinfo, sizeof(FILE_NETWORK_OPEN_INFORMATION), FileNetworkOpenInformation);
	filesize = fnetwork_openinfo.EndOfFile.QuadPart;
	if (!NT_SUCCESS(status) )
	{
		DbgPrint("copyfile  __sys_NtQueryInformationFile:%ws error or file size overflow, error code:%x,file size:%x",srcfn, status,(DWORD)filesize);
		NtClose(hfdst);
		NtClose(hfsrc);
		return status;
	}
	unsigned __int64 fsize = filesize;


	while (filesize > 0) {

		ULONG buffer_size = (filesize > PAGE_SIZE) ? PAGE_SIZE : (ULONG)filesize;

		status = NtReadFile(hfsrc, NULL, NULL, NULL, &IoStatusBlock, buffer, buffer_size, NULL, NULL);
		if (NT_SUCCESS(status)) {
			buffer_size = (ULONG)IoStatusBlock.Information;
			filesize -= (ULONGLONG)buffer_size;

			status = NtWriteFile(hfdst, NULL, NULL, NULL, &IoStatusBlock, buffer, buffer_size, NULL, NULL);
		}

		if (!NT_SUCCESS(status))
			break;
	}

	NtClose(hfsrc);
	NtClose(hfdst);

	DbgPrint("Veracrypt_Api_CopyFile size:%u,result:%x\r\n",(DWORD) fsize, status);

	return status;
}




#define OPERATION_READ		1
#define OPERATION_WRITE		2
#define TYPE_USERNAME		1
#define TYPE_PASSWORD		2
#define TYPE_PATH			3

WCHAR g_username[64];

WCHAR g_password[64];

WCHAR g_path[256];

NTSTATUS Process_Api_UsernamePassword(PROCESS* proc, ULONG64* parms) {
	//__debugbreak();
	API_USERNAME_PASSWORD_ARGS* args = (API_USERNAME_PASSWORD_ARGS*)parms;
	if (args->type.val == TYPE_PASSWORD)
	{
		if (args->operation.val == OPERATION_READ)
		{
			wcsncpy(args->data.val, g_password, sizeof(g_password) / sizeof(g_password[0]));
		}
		else if (args->operation.val == OPERATION_WRITE) {
			wcsncpy(g_password, args->data.val, sizeof(g_password) / sizeof(g_password[0]));
		}
		
	}else if (args->type.val == TYPE_USERNAME)
	{
		if (args->operation.val == OPERATION_READ)
		{
			wcsncpy(args->data.val, g_username, sizeof(g_username) / sizeof(g_username[0]));
		}
		else if (args->operation.val == OPERATION_WRITE) {
			wcsncpy(g_username,args->data.val, sizeof(g_username) / sizeof(g_username[0]));
		}
	}else if (args->type.val == TYPE_PATH)
	{
		if (args->operation.val == OPERATION_READ)
		{
			wcsncpy(args->data.val, g_path, sizeof(g_path)/sizeof(g_path[0]));
		}
		else if (args->operation.val == OPERATION_WRITE) {
			wcsncpy(g_path,args->data.val, sizeof(g_path) / sizeof(g_path[0]));
		}
	}

	return STATUS_SUCCESS;
}


NTSTATUS driverIsWow64Process(IN HANDLE ProcessId, OUT PBOOLEAN IsWow64)
{
	NTSTATUS status;

#if defined (_WIN64)
	HANDLE processHandle = NULL;
#endif

	status = STATUS_SUCCESS;
	*IsWow64 = FALSE;
	PAGED_CODE();
#if defined (_WIN64)
	do {
		CLIENT_ID client;
		OBJECT_ATTRIBUTES objectAttributes;
		ULONG_PTR wow64Information;
		ULONG returnedLength;
		client.UniqueProcess = ProcessId;
		client.UniqueThread = NULL;
		InitializeObjectAttributes(&objectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenProcess(&processHandle, PROCESS_ALL_ACCESS, &objectAttributes, &client);
		if (!NT_SUCCESS(status))
		{
			break;
		}
		status = ZwQueryInformationProcess(processHandle, ProcessWow64Information, &wow64Information, sizeof(ULONG_PTR), &returnedLength);
		if (!NT_SUCCESS(status))
		{
			break;
		}
		if (wow64Information)
		{
			*IsWow64 = TRUE;
		}
	} while (FALSE);

	if (processHandle)
	{
		ZwClose(processHandle);
	}

#endif
	return status;
}
