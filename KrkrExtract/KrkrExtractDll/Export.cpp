#include <my.h>
#include <sys/types.h>


typedef int pid_t;


pid_t fork()
{
	RTL_USER_PROCESS_INFORMATION Info;
	NTSTATUS                     Status;

	Status = RtlCloneUserProcess(RTL_CLONE_PROCESS_FLAGS_INHERIT_HANDLES, NULL, NULL, NULL, &Info);

	if (Status == STATUS_SUCCESS)
	{
		return (pid_t)Info.ClientId.UniqueProcess;
	}
	else if (Status == STATUS_PROCESS_CLONED)
	{
		return 0;
	}

	return -1;
}


