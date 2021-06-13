

#include "syscall.h"


int
main()
{
	int pid, exitCode;
	pid = Exec("matmult");
	exitCode = Join(pid);

	Exit(exitCode);
}
