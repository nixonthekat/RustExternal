#include <ntddk.h>
#include <windef.h>
#include <ntstrsafe.h>

#define IOCTL_READ_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_PROCESS_BASE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_MODULE_BASE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _KERNEL_READ_REQUEST {
    ULONG ProcessId;
    ULONG_PTR Address;
    PVOID Buffer;
    ULONG Size;
    BOOLEAN UsePhysical;
} KERNEL_READ_REQUEST, *PKERNEL_READ_REQUEST;

typedef struct _KERNEL_WRITE_REQUEST {
    ULONG ProcessId;
    ULONG_PTR Address;
    PVOID Buffer;
    ULONG Size;
} KERNEL_WRITE_REQUEST, *PKERNEL_WRITE_REQUEST;

typedef struct _KERNEL_PROCESS_REQUEST {
    ULONG ProcessId;
    ULONG_PTR ProcessBase;
    ULONG_PTR ModuleBase;
    WCHAR ModuleName[260];
} KERNEL_PROCESS_REQUEST, *PKERNEL_PROCESS_REQUEST;

// Function prototypes
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// Kernel utility functions
PEPROCESS GetProcessByPid(HANDLE ProcessId);
ULONG_PTR GetProcessImageBase(PEPROCESS Process);
ULONG_PTR GetModuleBase(PEPROCESS Process, LPCWSTR ModuleName);
NTSTATUS KernelReadVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size);
NTSTATUS KernelWriteVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size);

extern "C" {
    NTKERNELAPI NTSTATUS NTAPI MmCopyVirtualMemory(
        PEPROCESS SourceProcess,
        PVOID SourceAddress,
        PEPROCESS TargetProcess,
        PVOID TargetAddress,
        SIZE_T BufferSize,
        KPROCESSOR_MODE PreviousMode,
        PSIZE_T ReturnSize
    );

    NTKERNELAPI PPEB NTAPI PsGetProcessPeb(PEPROCESS Process);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\KernelDriver");
    UNICODE_STRING symbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\KernelDriver");
    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS status;

    // Create device
    status = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &deviceObject
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create symbolic link
    status = IoCreateSymbolicLink(&symbolicLink, &deviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        return status;
    }

    // Set up dispatch routines
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    deviceObject->Flags |= DO_DIRECT_IO;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DbgPrint("Kernel Driver loaded successfully\n");
    return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING symbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\KernelDriver");
    
    IoDeleteSymbolicLink(&symbolicLink);
    IoDeleteDevice(DriverObject->DeviceObject);
    
    DbgPrint("Kernel Driver unloaded\n");
}

NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG controlCode = stack->Parameters.DeviceIoControl.IoControlCode;
    PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG inBufferLength = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outBufferLength = stack->Parameters.DeviceIoControl.OutputBufferLength;
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG bytesReturned = 0;

    switch (controlCode) {
        case IOCTL_READ_MEMORY: {
            if (inBufferLength >= sizeof(KERNEL_READ_REQUEST)) {
                PKERNEL_READ_REQUEST request = (PKERNEL_READ_REQUEST)buffer;
                PEPROCESS targetProcess = GetProcessByPid((HANDLE)request->ProcessId);
                
                if (targetProcess) {
                    if (request->UsePhysical) {
                        SIZE_T returnSize;
                        status = MmCopyVirtualMemory(
                            targetProcess,
                            (PVOID)request->Address,
                            PsGetCurrentProcess(),
                            request->Buffer,
                            request->Size,
                            KernelMode,
                            &returnSize
                        );
                        bytesReturned = (ULONG)returnSize;
                    } else {
                        status = KernelReadVirtualMemory(
                            targetProcess,
                            (PVOID)request->Address,
                            request->Buffer,
                            request->Size
                        );
                        if (NT_SUCCESS(status)) {
                            bytesReturned = request->Size;
                        }
                    }
                    ObDereferenceObject(targetProcess);
                } else {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            break;
        }

        case IOCTL_WRITE_MEMORY: {
            if (inBufferLength >= sizeof(KERNEL_WRITE_REQUEST)) {
                PKERNEL_WRITE_REQUEST request = (PKERNEL_WRITE_REQUEST)buffer;
                PEPROCESS targetProcess = GetProcessByPid((HANDLE)request->ProcessId);
                
                if (targetProcess) {
                    status = KernelWriteVirtualMemory(
                        targetProcess,
                        request->Buffer,
                        (PVOID)request->Address,
                        request->Size
                    );
                    if (NT_SUCCESS(status)) {
                        bytesReturned = request->Size;
                    }
                    ObDereferenceObject(targetProcess);
                } else {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            break;
        }

        case IOCTL_GET_PROCESS_BASE: {
            if (inBufferLength >= sizeof(KERNEL_PROCESS_REQUEST) && 
                outBufferLength >= sizeof(KERNEL_PROCESS_REQUEST)) {
                PKERNEL_PROCESS_REQUEST request = (PKERNEL_PROCESS_REQUEST)buffer;
                PEPROCESS targetProcess = GetProcessByPid((HANDLE)request->ProcessId);
                
                if (targetProcess) {
                    request->ProcessBase = GetProcessImageBase(targetProcess);
                    status = STATUS_SUCCESS;
                    bytesReturned = sizeof(KERNEL_PROCESS_REQUEST);
                    ObDereferenceObject(targetProcess);
                } else {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            break;
        }

        case IOCTL_GET_MODULE_BASE: {
            if (inBufferLength >= sizeof(KERNEL_PROCESS_REQUEST) && 
                outBufferLength >= sizeof(KERNEL_PROCESS_REQUEST)) {
                PKERNEL_PROCESS_REQUEST request = (PKERNEL_PROCESS_REQUEST)buffer;
                PEPROCESS targetProcess = GetProcessByPid((HANDLE)request->ProcessId);
                
                if (targetProcess) {
                    request->ModuleBase = GetModuleBase(targetProcess, request->ModuleName);
                    status = STATUS_SUCCESS;
                    bytesReturned = sizeof(KERNEL_PROCESS_REQUEST);
                    ObDereferenceObject(targetProcess);
                } else {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            break;
        }

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

PEPROCESS GetProcessByPid(HANDLE ProcessId) {
    PEPROCESS process = NULL;
    NTSTATUS status = PsLookupProcessByProcessId(ProcessId, &process);
    
    if (NT_SUCCESS(status)) {
        return process;
    }
    
    return NULL;
}

ULONG_PTR GetProcessImageBase(PEPROCESS Process) {
    PPEB peb = PsGetProcessPeb(Process);
    if (!peb) {
        return 0;
    }
    
    return (ULONG_PTR)peb->ImageBaseAddress;
}

ULONG_PTR GetModuleBase(PEPROCESS Process, LPCWSTR ModuleName) {
    PPEB peb = PsGetProcessPeb(Process);
    if (!peb) {
        return 0;
    }

    __try {
        PPEB_LDR_DATA ldr = peb->Ldr;
        if (!ldr) {
            return 0;
        }

        for (PLIST_ENTRY listEntry = ldr->InLoadOrderModuleList.Flink;
             listEntry != &ldr->InLoadOrderModuleList;
             listEntry = listEntry->Flink) {
            
            PLDR_DATA_TABLE_ENTRY moduleEntry = CONTAINING_RECORD(listEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
            
            if (moduleEntry->BaseDllName.Buffer && moduleEntry->BaseDllName.Length > 0) {
                if (_wcsicmp(moduleEntry->BaseDllName.Buffer, ModuleName) == 0) {
                    return (ULONG_PTR)moduleEntry->DllBase;
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }

    return 0;
}

NTSTATUS KernelReadVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size) {
    SIZE_T bytesRead = 0;
    
    return MmCopyVirtualMemory(
        Process,
        SourceAddress,
        PsGetCurrentProcess(),
        TargetAddress,
        Size,
        KernelMode,
        &bytesRead
    );
}

NTSTATUS KernelWriteVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size) {
    SIZE_T bytesWritten = 0;
    
    return MmCopyVirtualMemory(
        PsGetCurrentProcess(),
        SourceAddress,
        Process,
        TargetAddress,
        Size,
        KernelMode,
        &bytesWritten
    );
} 